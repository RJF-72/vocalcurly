// C:/Vocal Plugin/VocalCraftQuantum/Source/Plugin/PluginProcessor.cpp
#include "PluginProcessor.h"
#include "../GUI/PluginEditor.h"

VocalCraftQuantumProcessor::VocalCraftQuantumProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

VocalCraftQuantumProcessor::~VocalCraftQuantumProcessor() = default;

void VocalCraftQuantumProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 3; ++i)
            formantFilters[ch][i].prepare(spec);
}

void VocalCraftQuantumProcessor::releaseResources()
{
}

bool VocalCraftQuantumProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (out != in) return false;
    return true;
}

void VocalCraftQuantumProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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
        juce::dsp::AudioBlock<float> block (&processed[0], 1, (size_t) processed.size());
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        for (int i = 0; i < 3; ++i)
            formantFilters[juce::jmin(ch, 1)][i].process(ctx);

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

        // Mix and gain
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float dry = data[i];
            float wet = processed[i];
            data[i] = (1.0f - dryWet) * dry + dryWet * wet;
            data[i] *= gain;
        }
    }
    spectralAnalyzer.computeSpectrum();
}

juce::AudioProcessorEditor* VocalCraftQuantumProcessor::createEditor()
{
    return new VocalCraftQuantumEditor(*this);
}

void VocalCraftQuantumProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VocalCraftQuantumProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalCraftQuantumProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryWet", "Dry/Wet", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", juce::NormalisableRange<float>(-24.0f, 24.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchAmount", "Pitch Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchSpeed", "Pitch Speed", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("formantShift", "Formant Shift", juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseAmount", "Noise Reduction", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("saturation", "Saturation", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}

void VocalCraftQuantumProcessor::updateFormantFilters(float semitoneShift)
{
    // Base formant centers (Hz)
    const float F1 = 500.0f;
    const float F2 = 1500.0f;
    const float F3 = 2500.0f;
    const float Q = 1.0f; // broad peaking filters

    auto shifted = [&](float f){ return f * std::pow(2.0f, semitoneShift / 12.0f); };

    for (int ch = 0; ch < 2; ++ch)
    {
        *formantFilters[ch][0].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, shifted(F1), Q, 1.5f);
        *formantFilters[ch][1].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, shifted(F2), Q, 1.5f);
        *formantFilters[ch][2].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, shifted(F3), Q, 1.3f);
    }
}