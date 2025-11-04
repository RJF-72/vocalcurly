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
#include "Theme.h"
#include <deque>

class TitanVocalEditor : public juce::AudioProcessorEditor,
                               private juce::Timer,
                               private juce::Button::Listener {
public:
    TitanVocalEditor(TitanVocalProcessor&);
    ~TitanVocalEditor() override;

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
    TitanVocalProcessor& audioProcessor;
    TitanDarkLookAndFeel darkTheme;

    // Toolbar
    juce::Toolbar toolbar;
    struct ToolbarIDs {
        enum Ids { tbAdvanced = 1, tbLoadPreset, tbSavePreset, tbLoadDefault, tbAIAssistant };
    };

    // Main components
    std::unique_ptr<SpectralDisplay> spectralDisplay;
    std::unique_ptr<ParameterControls> parameterControls;

    // Display mode selector
    juce::ComboBox displayModeBox;

    // AI toggle bound to APVTS
    juce::ToggleButton aiEnabledToggle { "AI" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aiEnabledAttachment;

    // AI model selection
    juce::ComboBox aiModelBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aiModelAttachment;

    // UI Sections
    juce::TabbedComponent mainTabs;

    // Control groups
    juce::GroupComponent pitchGroup, formantGroup, timeGroup, creativeGroup, outputGroup;

    // Interactive elements
    // Preset selector remains available (not in toolbar yet)
    juce::ComboBox presetSelector;

    // Metering
    juce::Slider inputMeter, outputMeter;
    juce::Label inputLabel, outputLabel;
    juce::Label statusBar;

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
    void loadDefaultPreset();
    void showAIAssistant();
    void setStatus(const juce::String& text);
    void populateToolbar();

    // Helpers
    void initializeDisplayModeSelector();

    // Keep an active FileChooser alive for async operations
    std::unique_ptr<juce::FileChooser> activeFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TitanVocalEditor)
};
