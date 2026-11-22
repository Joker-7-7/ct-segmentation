"""Compare the native resampler against nnU-Net's configured reference."""
import argparse
import struct
import subprocess
import tempfile
from pathlib import Path
import numpy as np
from nnunetv2.preprocessing.resampling.default_resampling import resample_data_or_seg


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("probe", type=Path)
    args = parser.parse_args()
    rng = np.random.default_rng(124)
    cases = [(s, t, order, axis)
             for s, t in [((7, 9, 11), (13, 6, 17)), ((1, 3, 8), (4, 9, 2)),
                          ((9, 5, 4), (3, 12, 10)), ((3, 4, 5), (3, 4, 5))]
             for order in (1, 3) for axis in (-1, 0, 1, 2)]
    max_error = 0
    with tempfile.TemporaryDirectory() as temp:
        source, output = Path(temp) / "input.bin", Path(temp) / "output.bin"
        for shape, target, order, axis in cases:
            data = rng.normal(size=shape).astype(np.float32)
            source.write_bytes(struct.pack("<8i", *shape, *target, order, axis) + data.tobytes())
            subprocess.run([str(args.probe.resolve()), str(source), str(output)], check=True)
            actual = np.fromfile(output, dtype=np.float32).reshape(target)
            expected = resample_data_or_seg(data[None], target, False, axis if axis >= 0 else None,
                                           order, axis >= 0, order_z=0)[0]
            error = float(np.max(np.abs(actual - expected)))
            max_error = max(error, max_error)
            np.testing.assert_allclose(actual, expected, atol=2e-5, rtol=2e-5,
                                       err_msg=str((shape, target, order, axis)))
    print(f"Passed {len(cases)} resampling cases; maximum absolute error {max_error}")


if __name__ == "__main__":
    main()
