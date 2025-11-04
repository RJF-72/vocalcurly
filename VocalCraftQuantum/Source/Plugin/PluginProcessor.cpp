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
    const float gain = juce::Decibels::decibelsToGain(gainDb);

    // Simple dry/wet and output gain
    // For now, processed signal == dry (no AI processing yet)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        spectralAnalyzer.pushAudioBuffer(data, buffer.getNumSamples());
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float dry = data[i];
            float wet = dry; // placeholder
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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchAmount", "Pitch Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchSpeed", "Pitch Speed", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("formantShift", "Formant Shift", juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseAmount", "Noise Reduction", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("saturation", "Saturation", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}