// C:/Vocal Plugin/TitanVocal/Source/GUI/ParameterControls.h
#pragma once

#include <JuceHeader.h>

class ParameterControls : public juce::Component
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    explicit ParameterControls(juce::AudioProcessorValueTreeState& apvts)
        : apvts(apvts)
    {
        addSlider(dryWet, dryWetLabel, dryWetAttachment, "dryWet", "Dry/Wet");
        addSlider(outputGain, outputGainLabel, outputGainAttachment, "outputGain", "Output Gain (dB)");
        addSlider(pitchAmount, pitchAmountLabel, pitchAmountAttachment, "pitchAmount", "Pitch Amount");
        addSlider(pitchSpeed, pitchSpeedLabel, pitchSpeedAttachment, "pitchSpeed", "Pitch Speed");
        addSlider(formantShift, formantShiftLabel, formantShiftAttachment, "formantShift", "Formant Shift");
        addSlider(noiseAmount, noiseAmountLabel, noiseAmountAttachment, "noiseAmount", "Noise Reduction");
        addSlider(saturation, saturationLabel, saturationAttachment, "saturation", "Saturation");
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        auto rowHeight = 40;
        auto labelWidth = 140;
        auto sliderWidth = area.getWidth() - labelWidth - 20;

        auto place = [&](juce::Label& label, juce::Slider& slider, int row)
        {
            label.setBounds(area.getX(), area.getY() + row * rowHeight, labelWidth, rowHeight);
            slider.setBounds(area.getX() + labelWidth + 10, area.getY() + row * rowHeight, sliderWidth, rowHeight);
        };

        int r = 0;
        place(dryWetLabel, dryWet, r++);
        place(outputGainLabel, outputGain, r++);
        place(pitchAmountLabel, pitchAmount, r++);
        place(pitchSpeedLabel, pitchSpeed, r++);
        place(formantShiftLabel, formantShift, r++);
        place(noiseAmountLabel, noiseAmount, r++);
        place(saturationLabel, saturation, r++);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider dryWet, outputGain, pitchAmount, pitchSpeed, formantShift, noiseAmount, saturation;
    juce::Label dryWetLabel, outputGainLabel, pitchAmountLabel, pitchSpeedLabel, formantShiftLabel, noiseAmountLabel, saturationLabel;

    std::unique_ptr<SliderAttachment> dryWetAttachment, outputGainAttachment, pitchAmountAttachment, pitchSpeedAttachment, formantShiftAttachment, noiseAmountAttachment, saturationAttachment;

    void addSlider(juce::Slider& slider, juce::Label& label, std::unique_ptr<SliderAttachment>& attachment,
                   const juce::String& paramID, const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
        addAndMakeVisible(slider);
        attachment = std::make_unique<SliderAttachment>(apvts, paramID, slider);
    }
};
// TitanVocal - Proprietary Parameter Controls
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// Licensed under strict proprietary EULA in LICENSE.txt.
//
// File: ParameterControls.h
// Description: UI components to control APVTS parameters for TitanVocal.