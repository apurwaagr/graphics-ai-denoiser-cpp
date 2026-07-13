import argparse
from pathlib import Path

import torch

from train_denoiser import TinyDenoiser


def export(model_path: Path, onnx_path: Path):
    model = TinyDenoiser()
    state = torch.load(model_path, map_location="cpu")
    model.load_state_dict(state)
    model.eval()

    dummy = torch.randn(1, 3, 64, 64)
    torch.onnx.export(
        model,
        dummy,
        onnx_path,
        input_names=["noisy"],
        output_names=["denoised"],
        dynamic_axes={"noisy": {0: "batch"}, "denoised": {0: "batch"}},
        opset_version=17,
    )

    print(f"Exported ONNX model to {onnx_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", type=Path, default=Path("artifacts/tiny_denoiser.pt"))
    parser.add_argument("--onnx", type=Path, default=Path("artifacts/tiny_denoiser.onnx"))
    args = parser.parse_args()

    args.onnx.parent.mkdir(parents=True, exist_ok=True)
    export(args.model, args.onnx)
