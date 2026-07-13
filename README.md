# Graphics AI Denoiser (C++ + PyTorch)

A portfolio project that demonstrates a hybrid graphics + AI workflow:
- A modern C++ Monte Carlo path tracer producing noisy low-spp renders.
- An edge-aware denoising pass in C++ (baseline).
- A PyTorch training script for a tiny denoiser model.
- ONNX export to prepare model deployment into C++ inference runtimes.
- Unit tests and CI pipeline for software quality.

## Why this project is relevant

This project maps directly to AI-for-graphics engineering responsibilities:
- Physically inspired rendering workflow with stochastic sampling noise.
- AI model training/fine-tuning workflow.
- Software quality via tests and CI.
- Production-minded deployment path through ONNX export.

## Project structure

- src/main.cpp: App entry point, renders and denoises output images.
- src/core/renderer.*: Monte Carlo path tracing (Lambertian spheres) and PPM export.
- src/core/denoiser.*: Edge-aware bilateral denoising implementation.
- tests/test_denoiser.cpp: Unit test validating smoothing + edge preservation.
- python/train_denoiser.py: Trains a tiny CNN denoiser with PyTorch.
- python/export_onnx.py: Exports trained model to ONNX.
- .github/workflows/ci.yml: Build + test automation.

## Windows prerequisites

You need both CMake and a C++ compiler.

1. Install Visual Studio 2022 Build Tools.
2. Enable workload: Desktop development with C++.
3. Open x64 Native Tools Command Prompt for VS 2022, then run CMake commands.

If you installed portable CMake (as done in this workspace), use:

```powershell
$cmake = "C:\Users\agrawalapurw\Documents\tools\cmake\cmake-3.30.5-windows-x86_64\bin\cmake.exe"
$ctest = "C:\Users\agrawalapurw\Documents\tools\cmake\cmake-3.30.5-windows-x86_64\bin\ctest.exe"
```

## Build and run (C++)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/graphics_ai_demo
```

On Windows with Visual Studio generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\graphics_ai_demo.exe
```

The executable writes:
- output_noisy.ppm
- output_denoised.ppm

### Runtime arguments

You can pass optional arguments:

```powershell
.\graphics_ai_demo.exe [width height spp] [--wait]
```

Examples:

```powershell
.\graphics_ai_demo.exe
.\graphics_ai_demo.exe 1024 1024 32
.\graphics_ai_demo.exe 1024 1024 32 --wait
```

`--wait` keeps the console open until you press Enter (useful when running by double-click).

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Python training and ONNX export

```bash
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -r requirements.txt
python python/train_denoiser.py --epochs 100
python python/export_onnx.py
```

## Suggested roadmap

1. Add direct-light importance sampling and Russian roulette termination.
2. Add albedo/normal auxiliary buffers for feature-guided denoising.
3. Integrate ONNX Runtime in C++ for real model inference.
4. Add benchmark harness (PSNR/SSIM, runtime, memory).
5. Add GitHub Actions job for Python lint/train smoke test.
