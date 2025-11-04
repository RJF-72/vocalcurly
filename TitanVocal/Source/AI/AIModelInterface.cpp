// C:/Vocal Plugin/TitanVocal/Source/AI/AIModelInterface.cpp
#include "AIModelInterface.h"
#include <iostream>

AIModelInterface::AIModelInterface() {
    // ONNX Runtime C++ API uses RAII via Ort::Env; no extra init needed beyond member env
}

AIModelInterface::~AIModelInterface() {
    // Cleanup models
    for (auto& [type, instance] : models) {
        if (instance.isLoaded) {
            // Proper cleanup would be needed for each model type
        }
    }
}

bool AIModelInterface::loadModel(ModelType type, const std::string& modelPath) {
    try {
        ModelInstance instance;
        instance.config.type = type;
        instance.config.modelPath = modelPath;

        bool loaded = false;

#if defined(ENABLE_TORCH)
        // Try loading as Torch model first (if enabled)
        try {
            auto module = torch::jit::load(modelPath);
            instance.torchModel = std::make_shared<torch::jit::script::Module>(std::move(module));
            instance.isLoaded = true;
            loaded = true;
            std::cout << "Loaded Torch model: " << modelPath << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Failed to load as Torch model: " << e.what() << std::endl;
        }
#endif

#if defined(ENABLE_ONNX)
        if (!loaded) {
            // Try loading as ONNX model (if enabled)
            try {
                Ort::SessionOptions sessionOptions;
                sessionOptions.SetIntraOpNumThreads(threadCount);
                sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

                if (useGPU) {
                    // Configure GPU settings if available (requires CUDA provider linked)
                    Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(sessionOptions, 0));
                }

                // Precision handling note: ONNX Runtime expects model/tensor dtypes.
                // Here we keep input as float (FP32). FP16/INT8 would require model conversion.

                instance.onnxSession = std::make_unique<Ort::Session>(env, modelPath.c_str(), sessionOptions);
                instance.isLoaded = true;
                loaded = true;
                std::cout << "Loaded ONNX model: " << modelPath << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Failed to load as ONNX model: " << e.what() << std::endl;
            }
        }
#endif

        if (!loaded) {
            std::cout << "No AI backends enabled or model failed to load: " << modelPath << std::endl;
            return false;
        }

        models[type] = instance;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
        return false;
    }
}

AIModelInterface::ProcessingResult AIModelInterface::processFrame(
    ModelType type, const std::vector<float>& audioFrame,
    const std::map<std::string, float>& parameters) {

    ProcessingResult result;
    result.success = false;

    auto it = models.find(type);
    if (it == models.end() || !it->second.isLoaded) {
        return result;
    }

    auto& model = it->second;

    try {
#if defined(ENABLE_TORCH)
        if (model.torchModel) {
            result = processWithTorch(model, audioFrame, parameters);
        }
#endif
#if defined(ENABLE_ONNX)
        else if (model.onnxSession) {
            result = processWithONNX(model, audioFrame, parameters);
        }
#endif
    } catch (const std::exception& e) {
        std::cerr << "Error processing frame: " << e.what() << std::endl;
    }

    return result;
}

#if defined(ENABLE_TORCH)
AIModelInterface::ProcessingResult AIModelInterface::processWithTorch(
    ModelInstance& model, const std::vector<float>& audioFrame,
    const std::map<std::string, float>& parameters) {

    ProcessingResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Convert to torch tensor
        torch::Tensor inputTensor = torch::from_blob(
            const_cast<float*>(audioFrame.data()),
            {1, static_cast<int64_t>(audioFrame.size())}
        ).clone();

        // Prepare parameters tensor if needed
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(inputTensor);

        // Add parameters if the model expects them
        if (!parameters.empty()) {
            std::vector<float> paramValues;
            for (const auto& [key, value] : parameters) {
                (void)key; // suppress unused warning
                paramValues.push_back(value);
            }
            torch::Tensor paramTensor = torch::tensor(paramValues).unsqueeze(0);
            inputs.push_back(paramTensor);
        }

        // Run inference
        auto output = model.torchModel->forward(inputs);

        if (output.isTensor()) {
            auto outputTensor = output.toTensor();
            result.processedAudio = std::vector<float>(
                outputTensor.data_ptr<float>(),
                outputTensor.data_ptr<float>() + outputTensor.numel()
            );
            result.success = true;
        }

    } catch (const std::exception& e) {
        std::cerr << "Torch processing error: " << e.what() << std::endl;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTime = std::chrono::duration<double>(endTime - startTime).count();

    return result;
}
#endif

#if defined(ENABLE_ONNX)
AIModelInterface::ProcessingResult AIModelInterface::processWithONNX(
    ModelInstance& model, const std::vector<float>& audioFrame,
    const std::map<std::string, float>& parameters)
{
    ProcessingResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Prepare input data (pad/trim to a reasonable size if config known)
        const int targetSize = model.config.inputSize > 0 ? model.config.inputSize : (int)audioFrame.size();
        std::vector<float> inputPadded = preprocessAudio(audioFrame, targetSize);

        // Create tensor shape [1, N]
        std::array<int64_t, 2> inputShape { 1, (int64_t)inputPadded.size() };

        Ort::AllocatorWithDefaultOptions allocator;
        size_t numInputs = model.onnxSession->GetInputCount();
        size_t numOutputs = model.onnxSession->GetOutputCount();

        const char* inputName = nullptr;
        std::vector<const char*> inputNames;
        for (size_t i = 0; i < numInputs; ++i)
        {
            auto name = model.onnxSession->GetInputNameAllocated(i, allocator);
            inputNames.push_back(name.get());
            if (i == 0) inputName = name.get();
        }

        std::vector<const char*> outputNames;
        for (size_t i = 0; i < numOutputs; ++i)
        {
            auto name = model.onnxSession->GetOutputNameAllocated(i, allocator);
            outputNames.push_back(name.get());
        }

        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memInfo, inputPadded.data(), inputPadded.size(), inputShape.data(), inputShape.size());

        std::vector<Ort::Value> inputs;
        inputs.emplace_back(std::move(inputTensor));

        // Parameters tensor (optional) if model expects a second input
        if (!parameters.empty() && numInputs > 1)
        {
            std::vector<float> paramValues;
            paramValues.reserve(parameters.size());
            for (const auto& kv : parameters) paramValues.push_back(kv.second);
            std::array<int64_t, 2> paramShape{ 1, (int64_t)paramValues.size() };
            Ort::Value paramTensor = Ort::Value::CreateTensor<float>(memInfo, paramValues.data(), paramValues.size(), paramShape.data(), paramShape.size());
            inputs.emplace_back(std::move(paramTensor));
        }

        // Run
        auto outputValues = model.onnxSession->Run(Ort::RunOptions{ nullptr }, inputNames.data(), inputs.data(), inputs.size(), outputNames.data(), outputNames.size());

        if (!outputValues.empty() && outputValues[0].IsTensor())
        {
            float* outData = outputValues[0].GetTensorMutableData<float>();
            auto outTypeInfo = outputValues[0].GetTensorTypeAndShapeInfo();
            std::vector<int64_t> outShape = outTypeInfo.GetShape();
            size_t outCount = 1;
            for (auto d : outShape) outCount *= (size_t)d;

            std::vector<float> processed(outData, outData + outCount);
            result.processedAudio = postprocessAudio(processed, (int)audioFrame.size());
            result.success = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "ONNX processing error: " << e.what() << std::endl;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTime = std::chrono::duration<double>(endTime - startTime).count();
    return result;
}
#endif

// Utility and management methods
bool AIModelInterface::isModelLoaded(ModelType type) const {
    auto it = models.find(type);
    return it != models.end() && it->second.isLoaded;
}

void AIModelInterface::unloadModel(ModelType type) {
    auto it = models.find(type);
    if (it != models.end()) {
        models.erase(it);
    }
}

std::vector<AIModelInterface::ModelType> AIModelInterface::getLoadedModels() const {
    std::vector<ModelType> loaded;
    for (const auto& kv : models) {
        if (kv.second.isLoaded) loaded.push_back(kv.first);
    }
    return loaded;
}

AIModelInterface::ModelConfig AIModelInterface::getModelConfig(ModelType type) const {
    auto it = models.find(type);
    if (it != models.end()) return it->second.config;
    return {};
}

void AIModelInterface::setThreadCount(int threads) { threadCount = std::max(1, threads); }
void AIModelInterface::setGPUMode(bool gpu) { useGPU = gpu; }
void AIModelInterface::setPrecision(int p) { precision = p; }

std::vector<float> AIModelInterface::preprocessAudio(const std::vector<float>& audio, int targetSize) {
    std::vector<float> out;
    out.resize(targetSize);
    if ((int)audio.size() >= targetSize) {
        std::copy(audio.begin(), audio.begin() + targetSize, out.begin());
    } else {
        std::copy(audio.begin(), audio.end(), out.begin());
        std::fill(out.begin() + audio.size(), out.end(), 0.0f);
    }
    return out;
}

std::vector<float> AIModelInterface::postprocessAudio(const std::vector<float>& processed, int originalSize) {
    if ((int)processed.size() <= originalSize) return processed;
    return std::vector<float>(processed.begin(), processed.begin() + originalSize);
}

AIModelInterface::ProcessingResult AIModelInterface::processBuffer(
    ModelType type, const std::vector<float>& audioBuffer,
    const std::map<std::string, float>& parameters)
{
    // Simple chunking over frames
    ProcessingResult finalResult;
    finalResult.success = false;
    const size_t frameSize = 2048;
    std::vector<float> output;
    output.reserve(audioBuffer.size());
    for (size_t pos = 0; pos < audioBuffer.size(); pos += frameSize)
    {
        size_t end = std::min(audioBuffer.size(), pos + frameSize);
        std::vector<float> frame(audioBuffer.begin() + pos, audioBuffer.begin() + end);
        auto r = processFrame(type, frame, parameters);
        if (!r.success) {
            // if any frame fails, mark but continue
        }
        output.insert(output.end(), r.processedAudio.begin(), r.processedAudio.end());
        finalResult.success = true;
    }
    finalResult.processedAudio = std::move(output);
    return finalResult;
}