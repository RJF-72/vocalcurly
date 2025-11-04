// TitanVocal - Proprietary Audio Processor Implementation
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// See LICENSE.txt for strict proprietary licensing terms.
//
// File: PluginProcessor.cpp
// Description: Implements TitanVocal AudioProcessor with DSP chain and AI buffered processing.
#include "PluginProcessor.h"
#include "../GUI/PluginEditor.h"

TitanVocalProcessor::TitanVocalProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Default latency to frame size when AI is enabled (updated in prepareToPlay)
}

TitanVocalProcessor::~TitanVocalProcessor() = default;

void TitanVocalProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 3; ++i)
            formantFilters[ch][i].prepare(spec);

    // Initialize AI buffers
    for (int ch = 0; ch < 2; ++ch) { aiInputDeque[ch].clear(); aiOutputDeque[ch].clear(); }
    setLatencySamples(aiFrameSize);

    // Attempt to load default model if present based on selected model type
    auto modelType = getSelectedModelType();
    juce::File modelsDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("Resources").getChildFile("Models");
    juce::File defaultOnnx = modelsDir.getChildFile("default.onnx");
    juce::File defaultTorch = modelsDir.getChildFile("default.pt");
    if (defaultOnnx.existsAsFile())
        aiInterface.loadModel(modelType, defaultOnnx.getFullPathName().toStdString());
    else if (defaultTorch.existsAsFile())
        aiInterface.loadModel(modelType, defaultTorch.getFullPathName().toStdString());
}

void TitanVocalProcessor::releaseResources()
{
}

bool TitanVocalProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (out != in) return false;
    return true;
}

void TitanVocalProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto* dryWetParam = apvts.getRawParameterValue("dryWet");
    auto* outputGainParam = apvts.getRawParameterValue("outputGain");

    const float dryWet = dryWetParam ? dryWetParam->load() : 1.0f;
    const float gainDb = outputGainParam ? outputGainParam->load() : 0.0f;
    const float pitchAmt = apvts.getRawParameterValue("pitchAmount") ? apvts.getRawParameterValue("pitchAmount")->load() : 0.0f;
    const float formShift = apvts.getRawParameterValue("formantShift") ? apvts.getRawParameterValue("formantShift")->load() : 0.0f;
    const float noiseAmt = apvts.getRawParameterValue("noiseAmount") ? apvts.getRawParameterValue("noiseAmount")->load() : 0.0f;
    const float satAmt = apvts.getRawParameterValue("saturation") ? apvts.getRawParameterValue("saturation")->load() : 0.0f;
    const bool aiEnabled = apvts.getRawParameterValue("aiEnabled") ? (apvts.getRawParameterValue("aiEnabled")->load() > 0.5f) : false;
    const float gain = juce::Decibels::decibelsToGain(gainDb);

    // Update formant filters per block
    updateFormantFilters(formShift);

    // Process per channel: naive pitch shift, formant filters, noise gate, saturation
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        spectralAnalyzer.pushAudioBuffer(data, buffer.getNumSamples());

        // Copy dry signal
        std::vector<float> processed(buffer.getNumSamples());

        // Naive pitch shift by resampling with ratio up to +/- 12 semitones (one octave)
        const float maxSemis = 12.0f;
        const float semis = juce::jlimit(-maxSemis, maxSemis, (pitchAmt - 0.5f) * 2.0f * maxSemis); // map 0..1 to -12..+12
        const float ratio = std::pow(2.0f, semis / 12.0f);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float srcIndex = (float) i / ratio;
            if (srcIndex <= 0.0f) processed[i] = data[0];
            else if (srcIndex >= buffer.getNumSamples() - 1)
                processed[i] = data[buffer.getNumSamples() - 1];
            else
            {
                int idx = (int) srcIndex;
                float frac = srcIndex - (float) idx;
                processed[i] = juce::jmap(frac, data[idx], data[idx + 1]);
            }
        }

        // Formant filters
        juce::AudioBuffer<float> tempBuffer (1, (int) processed.size());
        tempBuffer.copyFrom(0, 0, processed.data(), (int) processed.size());
        juce::dsp::AudioBlock<float> block (tempBuffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        for (int i = 0; i < 3; ++i)
            formantFilters[juce::jmin(ch, 1)][i].process(ctx);
        const float* procPtr = tempBuffer.getReadPointer(0);
        std::copy(procPtr, procPtr + tempBuffer.getNumSamples(), processed.begin());

        // Noise gate (simple): threshold scales with noiseAmt
        const float baseThresh = 0.02f; // base threshold
        const float threshold = baseThresh * (1.0f - noiseAmt); // more reduction => higher attenuation below threshold
        const float attenuation = juce::jmap(noiseAmt, 0.0f, 1.0f, 1.0f, 0.2f);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (std::abs(processed[i]) < threshold)
                processed[i] *= attenuation;
        }

        // Saturation: mix between linear and tanh
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = processed[i];
            float y = std::tanh(x);
            processed[i] = (1.0f - satAmt) * x + satAmt * y;
        }

        // If AI enabled, feed input into AI buffer and produce output frames
        if (aiEnabled)
        {
            // Push input samples
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                aiInputDeque[juce::jmin(ch, 1)].push_back(data[i]);

            // Process full frames
            while ((int) aiInputDeque[juce::jmin(ch, 1)].size() >= aiFrameSize)
            {
                std::vector<float> frame;
                frame.reserve(aiFrameSize);
                for (int i = 0; i < aiFrameSize; ++i) { frame.push_back(aiInputDeque[juce::jmin(ch, 1)].front()); aiInputDeque[juce::jmin(ch, 1)].pop_front(); }

                std::map<std::string, float> aiParams {
                    { "pitchAmount", pitchAmt },
                    { "formantShift", formShift },
                    { "noiseAmount", noiseAmt },
                    { "saturation", satAmt },
                };
                auto result = aiInterface.processFrame(getSelectedModelType(), frame, aiParams);
                const auto& out = result.success && !result.processedAudio.empty() ? result.processedAudio : frame;
                for (auto v : out) aiOutputDeque[juce::jmin(ch, 1)].push_back(v);
            }
        }

        // Mix and gain
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float dry = data[i];
            float wet = processed[i];
            if (aiEnabled)
            {
                // If AI output available, use it as wet signal; otherwise fall back to processed chain
                if (!aiOutputDeque[juce::jmin(ch, 1)].empty())
                {
                    wet = aiOutputDeque[juce::jmin(ch, 1)].front();
                    aiOutputDeque[juce::jmin(ch, 1)].pop_front();
                }
            }
            data[i] = (1.0f - dryWet) * dry + dryWet * wet;
            data[i] *= gain;
        }
    }
    spectralAnalyzer.computeSpectrum();
}

juce::AudioProcessorEditor* TitanVocalProcessor::createEditor()
{
    return new TitanVocalEditor(*this);
}

void TitanVocalProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TitanVocalProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TitanVocalProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryWet", "Dry/Wet", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", juce::NormalisableRange<float>(-24.0f, 24.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchAmount", "Pitch Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchSpeed", "Pitch Speed", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("formantShift", "Formant Shift", juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseAmount", "Noise Reduction", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("saturation", "Saturation", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // AI toggle (0: off, 1: on)
    params.push_back(std::make_unique<juce::AudioParameterBool>("aiEnabled", "AI Enabled", false));

    // AI model selection
    juce::StringArray modelChoices { "Noise Reduction", "Pitch Correction", "Formant Repair", "Breath Control", "Voice Morphing", "Timing Correction" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("aiModelType", "AI Model", modelChoices, 0));

    return { params.begin(), params.end() };
}

// JUCE plugin entry point factory is defined in CreateFilter.cpp

void TitanVocalProcessor::updateFormantFilters(float semitoneShift)
{
    // Base formant centers (Hz)
    const float F1 = 500.0f;
    const float F2 = 1500.0f;
    const float F3 = 2500.0f;
    const float Q = 1.0f; // broad peaking filters

    auto shifted = [&](float f){ return f * std::pow(2.0f, semitoneShift / 12.0f); };

    for (int ch = 0; ch < 2; ++ch)
    {
        formantFilters[ch][0].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, shifted(F1), Q, 1.5f);
        formantFilters[ch][1].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, shifted(F2), Q, 1.5f);
        formantFilters[ch][2].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, shifted(F3), Q, 1.3f);
    }
}

AIModelInterface::ModelType TitanVocalProcessor::getSelectedModelType() const
{
    int idx = 0;
    if (auto* p = apvts.getParameter("aiModelType")) idx = p->getValue();
    switch (idx)
    {
        case 0: return AIModelInterface::NOISE_REDUCTION;
        case 1: return AIModelInterface::PITCH_CORRECTION;
        case 2: return AIModelInterface::FORMANT_REPAIR;
        case 3: return AIModelInterface::BREATH_CONTROL;
        case 4: return AIModelInterface::VOICE_MORPHING;
        case 5: return AIModelInterface::TIMING_CORRECTION;
        default: return AIModelInterface::NOISE_REDUCTION;
    }
}