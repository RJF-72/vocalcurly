// TitanVocal - Proprietary Audio Processor
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// Licensed under strict proprietary EULA in LICENSE.txt.
//
// File: PluginProcessor.h
// Description: Main AudioProcessor for TitanVocal, parameters, DSP, and AI integration.
#pragma once

#include <JuceHeader.h>
#include "../DSP/SpectralAnalyzer.h"
#include "../AI/AIModelInterface.h"

class TitanVocalProcessor : public juce::AudioProcessor
{
public:
    TitanVocalProcessor();
    ~TitanVocalProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TitanVocal"; }
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

    // AI buffered processing
    std::deque<float> aiInputDeque[2];
    std::deque<float> aiOutputDeque[2];
    int aiFrameSize { 1024 };
    AIModelInterface::ModelType aiDefaultModel { AIModelInterface::NOISE_REDUCTION };

    // Simple formant filters per channel (F1,F2,F3)
    juce::dsp::IIR::Filter<float> formantFilters[2][3];

    void updateFormantFilters(float semitoneShift);
    AIModelInterface::ModelType getSelectedModelType() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TitanVocalProcessor)
};