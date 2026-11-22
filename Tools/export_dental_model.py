"""One-time export; the desktop application never invokes Python."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
import torch
from dynamic_network_architectures.architectures.unet import PlainConvUNet


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    plans = json.loads((args.model / "plans.json").read_text())
    config = plans["configurations"]["3d_fullres"]
    assert plans["dataset_name"] == "Dataset111_453CT"
    assert config["UNet_class_name"] == "PlainConvUNet"
    stages = len(config["conv_kernel_sizes"])
    network = PlainConvUNet(
        input_channels=1, n_stages=stages,
        features_per_stage=[min(config["UNet_base_num_features"] * 2**i,
                                config["unet_max_num_features"]) for i in range(stages)],
        conv_op=torch.nn.Conv3d, kernel_sizes=config["conv_kernel_sizes"],
        strides=config["pool_op_kernel_sizes"],
        n_conv_per_stage=config["n_conv_per_stage_encoder"], num_classes=6,
        n_conv_per_stage_decoder=config["n_conv_per_stage_decoder"], conv_bias=True,
        norm_op=torch.nn.InstanceNorm3d, norm_op_kwargs={"eps": 1e-5, "affine": True},
        dropout_op=None, dropout_op_kwargs=None,
        nonlin=torch.nn.LeakyReLU, nonlin_kwargs={"inplace": True}, deep_supervision=False)
    checkpoint_path = args.model / "fold_0" / "checkpoint_final.pth"
    checkpoint_hash = hashlib.sha256(checkpoint_path.read_bytes()).hexdigest()
    if checkpoint_hash != "a3a91ae0f8d7ab403d8662e47d2f0bdf7fc51e7783b46632841274dff8266c32":
        raise ValueError("Checkpoint SHA-256 does not match the supported official model")
    # Verify the known checkpoint before deserializing its pickle payload.
    checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=False)
    network.load_state_dict(checkpoint["network_weights"], strict=True)
    network.eval()
    torch.set_num_threads(4)
    torch.manual_seed(2026)
    patch = torch.randn(1, 1, *config["patch_size"])
    args.output.mkdir(parents=True, exist_ok=True)
    destination = args.output / "dental.onnx"
    with torch.inference_mode():
        torch.onnx.export(network, patch, str(destination), input_names=["image"],
                          output_names=["logits"], opset_version=17, dynamo=False)
        expected = network(patch).numpy()
    import onnxruntime as ort
    options = ort.SessionOptions()
    options.intra_op_num_threads = 4
    session = ort.InferenceSession(str(destination), options, providers=["CPUExecutionProvider"])
    actual = session.run(None, {"image": patch.numpy()})[0]
    np.testing.assert_allclose(actual, expected, atol=2e-3, rtol=2e-3)
    from scipy.ndimage import gaussian_filter
    gaussian = np.zeros(config["patch_size"], dtype=np.float64)
    gaussian[tuple(n // 2 for n in config["patch_size"])] = 1
    gaussian = gaussian_filter(gaussian, [n / 8 for n in config["patch_size"]], mode="constant")
    gaussian = (gaussian / gaussian.max() * 10).astype(np.float16)
    gaussian[gaussian == 0] = gaussian[gaussian != 0].min()
    gaussian.astype("<f4").tofile(args.output / "gaussian.bin")
    report = {"checkpoint_sha256": checkpoint_hash,
              "max_absolute_error": float(np.max(np.abs(actual - expected))),
              "patch_size": config["patch_size"], "torch": torch.__version__,
              "onnxruntime": ort.__version__}
    (args.output / "export-validation.json").write_text(json.dumps(report, indent=2))
    (args.output / "plans.json").write_text(json.dumps(plans, indent=2))
    (args.output / "dataset.json").write_bytes((args.model / "dataset.json").read_bytes())
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

