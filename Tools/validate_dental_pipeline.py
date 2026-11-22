"""Offline end-to-end comparison on a deterministic small synthetic CT."""
import argparse
import subprocess
import json
import shutil
from pathlib import Path
import numpy as np
import SimpleITK as sitk
import torch
from nnunetv2.inference.predict_from_raw_data import nnUNetPredictor


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("exported", type=Path)
    parser.add_argument("work", type=Path)
    parser.add_argument("--phantom-network", action="store_true")
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    torch.set_num_threads(4)
    z, y, x = np.indices((24, 44 if args.phantom_network else 28, 32))
    data = (-600 + 2200 * np.exp(-((x-16)**2+(y-14)**2+(z-12)**2)/90)
            + 140*np.sin(x*.8)*np.cos(z*.3)).astype(np.float32)
    data[:2] = 0
    data[:, :3] = 0
    data[:, :, :2] = 0
    image = sitk.GetImageFromArray(data)
    image.SetSpacing((.7, 1.6, .4))
    image.SetOrigin((12., -23., 45.))
    image.SetDirection((0., -1., 0., 1., 0., 0., 0., 0., 1.))
    network = None
    if args.phantom_network:
        # A diagnostic network exercises all labels, overlapping tiles and
        # boundaries; it is never deployed as a dental model.
        data = (-110 + (x / 31) * 3177 + 30 * np.sin(y*.5)).astype(np.float32)
        data[:2] = 0; data[:, :3] = 0; data[:, :, :2] = 0
        image = sitk.GetImageFromArray(data)
        image.SetSpacing((.7, 1.6, .4))
        image.SetOrigin((12., -23., 45.))
        image.SetDirection((0., -1., 0., 1., 0., 0., 0., 0., 1.))
        directory = args.work / "diagnostic-model"
        directory.mkdir(exist_ok=True)
        for name in ("plans.json", "dataset.json", "gaussian.bin"):
            shutil.copyfile(args.exported / name, directory / name)
        network = torch.nn.Conv3d(1, 6, 1)
        centers = torch.tensor([-2.4, -1.3, -.2, .8, 1.8, 2.8])
        with torch.no_grad():
            network.weight.copy_((2 * centers).reshape(6, 1, 1, 1, 1))
            network.bias.copy_(-centers.square())
        network.eval()
        torch.onnx.export(network, torch.zeros(1, 1, 128, 160, 112), str(directory / "dental.onnx"),
                          input_names=["image"], output_names=["logits"], opset_version=17, dynamo=False)
        args.exported = directory
    source = args.work / "synthetic.nii.gz"
    output = args.work / "native.nii.gz"
    sitk.WriteImage(image, str(source))
    subprocess.run([str(args.executable.resolve()), str(source.resolve()),
                    str(output.resolve()), str(args.exported.resolve())], check=True)
    native = sitk.ReadImage(str(output))
    assert native.GetSize() == image.GetSize()
    np.testing.assert_allclose(native.GetSpacing(), image.GetSpacing(), atol=1e-6)
    np.testing.assert_allclose(native.GetOrigin(), image.GetOrigin(), atol=1e-6)
    np.testing.assert_allclose(native.GetDirection(), image.GetDirection(), atol=1e-6)
    predictor = nnUNetPredictor(tile_step_size=.5, use_gaussian=True, use_mirroring=False,
                               perform_everything_on_device=False, device=torch.device("cpu"),
                               verbose=False, verbose_preprocessing=False, allow_tqdm=False)
    if network is None:
        predictor.initialize_from_trained_model_folder(str(args.checkpoint.resolve()), [0], "checkpoint_final.pth")
    else:
        from nnunetv2.utilities.plans_handling.plans_handler import PlansManager
        plans = PlansManager(str(args.exported / "plans.json"))
        predictor.manual_initialization(network, plans, plans.get_configuration("3d_fullres"),
                                        [network.state_dict()], json.loads((args.exported / "dataset.json").read_text()),
                                        "diagnostic", None)
    # Read back NIfTI to include the same float32 geometry serialization as C++.
    image = sitk.ReadImage(str(source))
    data = sitk.GetArrayFromImage(image)
    expected = predictor.predict_single_npy_array(data[None], {"spacing": image.GetSpacing()[::-1]}, None, None, False)
    minimum = int(np.ceil(60. / np.prod(image.GetSpacing())))
    for label in range(1,5):
        components = sitk.ConnectedComponent(sitk.GetImageFromArray((expected == label).astype(np.uint8)))
        kept = sitk.GetArrayFromImage(sitk.RelabelComponent(components, minimumObjectSize=minimum))
        expected[(expected == label) & (kept == 0)] = 0
    actual = sitk.GetArrayFromImage(native)
    if args.phantom_network:
        assert len(np.unique(actual)) == 6, "Diagnostic mask must exercise all six labels"
    disagreement = float(np.mean(actual != expected))
    print(f"Native/reference mask disagreement: {disagreement:.8%}; labels: {np.unique(actual)}")
    assert disagreement < .001, "Native pipeline differs from reference on more than 0.1% of voxels"
    reference = sitk.GetImageFromArray(expected.astype(np.uint16))
    reference.CopyInformation(image)
    sitk.WriteImage(reference, str(args.work / "reference.nii.gz"))


if __name__ == "__main__":
    main()
