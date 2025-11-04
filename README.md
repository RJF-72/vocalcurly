# TitanVocal

TitanVocal – a JUCE-based vocal repair and creative processing plugin prototype.

Project structure
- VocalCraftQuantum/Source: C++ sources for Plugin, GUI, DSP, AI, Core.
- VocalCraftQuantum/Resources: models, presets, UI assets, scripts.
- VocalCraftQuantum/Resources/Scripts/train_vocal_model.py: PyTorch training script for a multi-task transformer.
- VocalCraftQuantum/ThirdParty: optional local dependencies (JUCE, LibTorch, ONNX Runtime).

Build options
Option A: CMake standalone app
1) Install CMake and a compiler (Visual Studio 2019/2022 on Windows).
2) From C:\Vocal Plugin\VocalCraftQuantum, run:
   - cmake -S . -B build
   - cmake --build build --config Release
3) Executable target: TitanVocal_Standalone

Notes:
- CMakeLists.txt uses FetchContent to pull JUCE by default. To use a local JUCE, configure: -DJUCE_DIR=../ThirdParty/JUCE
- To enable LibTorch and ONNX Runtime, ensure they are installed and available to CMake.
  - Example: -DENABLE_TORCH=ON -DTorch_DIR="path/to/libtorch/share/cmake/Torch"
  - Example: -DENABLE_ONNX=ON -DONNXRuntime_DIR="path/to/onnxruntime/cmake"

Option B: Projucer (if preferred)
- You can create a .jucer project and add sources under VocalCraftQuantum/Source.
- Link LibTorch and ONNX Runtime via exporter settings.

Option C: CMake VST3 plugin
1) From C:\Vocal Plugin\VocalCraftQuantum, run:
   - cmake -S . -B build -DENABLE_TORCH=ON -DENABLE_ONNX=ON
   - cmake --build build --config Release --target TitanVocal_Plugin
2) The built VST3 will be copied to your system's default VST3 folder (COPY_PLUGIN_AFTER_BUILD is enabled). On Windows this is typically:
   - C:\Program Files\Common Files\VST3\TitanVocal.vst3
3) If JUCE is local, pass -DJUCE_DIR=../ThirdParty/JUCE

Runtime dependencies
- JUCE 7.x
- Optional: LibTorch (TorchScript), ONNX Runtime (CPU/CUDA)

Current features
- APVTS parameters: dryWet, outputGain, pitchAmount, pitchSpeed, formantShift, noiseAmount, saturation.
- Spectral analysis (FFT, magnitudes, simple pitch estimate).
- GUI: main tab, spectral display (waveform, FFT, scrolling spectrogram), parameter controls, basic meters.
- Audio processing: naive pitch shift (resampling), formant shaping (peaking filters), noise gate, saturation.
- AI model interface: TorchScript and ONNX Runtime support with preprocessing/postprocessing.

Presets
- Save preset: Use the "Save" button in the editor to write APVTS state to an XML file.
- Load preset: Use the "Load" button to restore APVTS state from XML.

License and copyright
- License: Proprietary and very strict (see LICENSE.txt). Non-transferable, non-sublicensable, revocable; no redistribution, no derivative works, no reverse engineering, no public benchmarking, no AI training on code or outputs, and no commercial use without written permission.
- Copyright: © 2025 Ray Flanary and Joni Marie Flanary. All rights reserved. See COPYRIGHT.txt.
- Third-party licensing: This project depends on JUCE, LibTorch/PyTorch, and ONNX Runtime. You must comply with their respective licenses. Proprietary distribution with JUCE requires a commercial JUCE license; GPL builds of JUCE require GPL-compliant distribution. See NOTICE.txt.

Training script
- train_vocal_model.py contains a complete pipeline for dataset loading, model definition, and training with checkpoints.

Next steps
- Improve pitch shifting (OLA/phase vocoder), formant processing (true vocoder), and noise reduction (spectral gating).
- Add preset management and more UI tabs.
- Integrate model inference in real-time path via AIModelInterface.