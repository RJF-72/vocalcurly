// TitanVocal - Proprietary UI
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// This source code is licensed under the strict proprietary EULA described in LICENSE.txt.
// Unauthorized copying, distribution, modification, sublicensing, reverse engineering,
// benchmarking, or use for AI training is prohibited without explicit written permission.
//
// File: PluginEditor.h
// Description: Main editor UI for TitanVocal, including controls and spectral display.
#pragma once

#include <JuceHeader.h>
#include "../Plugin/PluginProcessor.h"
#include "SpectralDisplay.h"
#include "ParameterControls.h"

class VocalCraftQuantumEditor : public juce::AudioProcessorEditor,
                               private juce::Timer,
                               private juce::Button::Listener {
public:
    VocalCraftQuantumEditor(VocalCraftQuantumProcessor&);
    ~VocalCraftQuantumEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // UI Sections
    enum UISection {
        MAIN_DISPLAY = 0,
        PITCH_CONTROLS,
        FORMANT_CONTROLS,
        TIME_CONTROLS,
        CREATIVE_CONTROLS,
        OUTPUT_CONTROLS
    };

    void showSection(UISection section);
    void toggleAdvancedMode(bool advanced);

private:
    VocalCraftQuantumProcessor& audioProcessor;

    // Main components
    std::unique_ptr<SpectralDisplay> spectralDisplay;
    std::unique_ptr<ParameterControls> parameterControls;

    // Display mode selector
    juce::ComboBox displayModeBox;

    // AI toggle bound to APVTS
    juce::ToggleButton aiEnabledToggle { "AI" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aiEnabledAttachment;

    // UI Sections
    juce::TabbedComponent mainTabs;

    // Control groups
    juce::GroupComponent pitchGroup, formantGroup, timeGroup, creativeGroup, outputGroup;

    // Interactive elements
    juce::TextButton advancedModeButton;
    juce::ComboBox presetSelector;
    juce::TextButton loadPresetButton, savePresetButton;
    juce::TextButton aiAssistantButton;

    // Metering
    juce::Slider inputMeter, outputMeter;
    juce::Label inputLabel, outputLabel;

    void timerCallback() override;
    void buttonClicked(juce::Button* button) override;

    void createPitchControls();
    void createFormantControls();
    void createTimeControls();
    void createCreativeControls();
    void createOutputControls();

    void updateMeters();
    void loadPreset();
    void savePreset();
    void showAIAssistant();

    // Helpers
    void initializeDisplayModeSelector();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCraftQuantumEditor)
};