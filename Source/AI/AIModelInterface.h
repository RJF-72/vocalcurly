// TitanVocal - Proprietary AI Model Interface
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// Licensed under strict proprietary EULA in LICENSE.txt.
//
// File: AIModelInterface.h
// Description: Abstraction for TorchScript and ONNX Runtime models used in TitanVocal.
#pragma once

#if defined(ENABLE_TORCH)
#include <c10/util/Optional.h> // ensure c10::nullopt is visible before JUCE or std
using c10::nullopt; // prefer c10::nullopt for LibTorch optional functions
#include <torch/script.h>
#endif
#if defined(ENABLE_ONNX)
#include <onnxruntime_cxx_api.h>
#endif
#include <JuceHeader.h>
#include <vector>
#include <memory>

class AIModelInterface {
public:
    AIModelInterface();
    ~AIModelInterface();

    enum ModelType {
        PITCH_CORRECTION = 0,
        FORMANT_REPAIR,
        NOISE_REDUCTION,
        BREATH_CONTROL,
        VOICE_MORPHING,
        TIMING_CORRECTION
    };

    struct ModelConfig {
        ModelType type;
        std::string modelPath;
        int inputSize;
        int outputSize;
        float complexity;
        bool requiresGPU;
    };

    struct ProcessingResult {
        std::vector<float> processedAudio;
        std::vector<float> analysisData;
        float confidence;
        double processingTime;
        bool success;
    };

    // Model Management
    bool loadModel(ModelType type, const std::string& modelPath);
    bool isModelLoaded(ModelType type) const;
    void unloadModel(ModelType type);

    // Real-time Processing
    ProcessingResult processFrame(ModelType type, const std::vector<float>& audioFrame,
                                 const std::map<std::string, float>& parameters);

    // Batch Processing (for offline mode)
    ProcessingResult processBuffer(ModelType type, const std::vector<float>& audioBuffer,
                                  const std::map<std::string, float>& parameters);

    // Model Information
    std::vector<ModelType> getLoadedModels() const;
    ModelConfig getModelConfig(ModelType type) const;

    // Performance Optimization
    void setThreadCount(int threads);
    void setGPUMode(bool useGPU);
    void setPrecision(int precision); // 0: FP32, 1: FP16, 2: INT8

private:
    struct ModelInstance {
        ModelConfig config;
        // Pointers guarded by feature flags so the header compiles without the libraries
#if defined(ENABLE_TORCH)
        std::shared_ptr<torch::jit::script::Module> torchModel; // valid when a Torch model is loaded
#endif
#if defined(ENABLE_ONNX)
        std::unique_ptr<Ort::Session> onnxSession; // valid when an ONNX model is loaded
#endif
        bool isLoaded = false;
    };

    std::map<ModelType, ModelInstance> models;
    // ONNX environment only when ONNX is enabled
#if defined(ENABLE_ONNX)
    Ort::Env env{ ORT_LOGGING_LEVEL_WARNING, "TitanVocal" };
#endif
    bool useGPU = false;
    int threadCount = 4;
    int precision = 0;

    // Processing methods
#if defined(ENABLE_TORCH)
    ProcessingResult processWithTorch(ModelInstance& model, const std::vector<float>& audioFrame,
                                     const std::map<std::string, float>& parameters);
#endif
#if defined(ENABLE_ONNX)
    ProcessingResult processWithONNX(ModelInstance& model, const std::vector<float>& audioFrame,
                                    const std::map<std::string, float>& parameters);
#endif

    // Utility functions
    std::vector<float> preprocessAudio(const std::vector<float>& audio, int targetSize);
    std::vector<float> postprocessAudio(const std::vector<float>& processed, int originalSize);
};