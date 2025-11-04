// C:/Vocal Plugin/VocalCraftQuantum/Source/Plugin/PluginProcessor.h
#pragma once

#include <JuceHeader.h>
#include "../DSP/SpectralAnalyzer.h"
#include "../AI/AIModelInterface.h"

class VocalCraftQuantumProcessor : public juce::AudioProcessor
{
public:
    VocalCraftQuantumProcessor();
    ~VocalCraftQuantumProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VocalCraftQuantum"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Analysis
    SpectralAnalyzer spectralAnalyzer;

private:
    AIModelInterface aiInterface;
    double currentSampleRate { 44100.0 };

    // Simple formant filters per channel (F1,F2,F3)
    juce::dsp::IIR::Filter<float> formantFilters[2][3];

    void updateFormantFilters(float semitoneShift);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCraftQuantumProcessor)
};