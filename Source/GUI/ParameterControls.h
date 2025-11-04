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
        setOpaque(true);
        addSlider(dryWet, dryWetLabel, dryWetAttachment, "dryWet", "Dry/Wet");
        addSlider(outputGain, outputGainLabel, outputGainAttachment, "outputGain", "Output Gain (dB)");
        addSlider(pitchAmount, pitchAmountLabel, pitchAmountAttachment, "pitchAmount", "Pitch Amount");
        addSlider(pitchSpeed, pitchSpeedLabel, pitchSpeedAttachment, "pitchSpeed", "Pitch Speed");
        addSlider(formantShift, formantShiftLabel, formantShiftAttachment, "formantShift", "Formant Shift");
        addSlider(noiseAmount, noiseAmountLabel, noiseAmountAttachment, "noiseAmount", "Noise Reduction");
        addSlider(saturation, saturationLabel, saturationAttachment, "saturation", "Saturation");
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto bg = findColour(juce::ResizableWindow::backgroundColourId);

        // Card container
        juce::Path shadow; shadow.addRoundedRectangle(area.reduced(2.0f).translated(0.0f, 2.0f), 10.0f);
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillPath(shadow);
        g.setGradientFill(juce::ColourGradient(bg.darker(0.15f), area.getX(), area.getY(),
                                               bg.darker(0.10f), area.getX(), area.getBottom(), false));
        g.fillRoundedRectangle(area.reduced(2.0f), 10.0f);
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawRoundedRectangle(area.reduced(2.0f), 10.0f, 1.0f);

        // Rows separators and accent ticks
        auto accent = findColour(juce::Slider::thumbColourId);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        for (auto& r : rowRects)
        {
            auto lineY = (float) r.getBottom();
            g.drawHorizontalLine((int) lineY, area.getX()+10.0f, area.getRight()-10.0f);
            auto tick = juce::Rectangle<float>(area.getX()+12.0f, (float) r.getY()+r.getHeight()/2.0f-3.0f, 4.0f, 6.0f);
            g.setColour(accent);
            g.fillRoundedRectangle(tick, 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.06f));
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(18);
        auto rowHeight = 44;
        auto labelWidth = 140;
        auto sliderWidth = area.getWidth() - labelWidth - 28;

        rowRects.clear();
        auto place = [&](juce::Label& label, juce::Slider& slider, int row)
        {
            auto rowRect = juce::Rectangle<int>(area.getX(), area.getY() + row * rowHeight, area.getWidth(), rowHeight);
            rowRects.push_back(rowRect);

            label.setBounds(rowRect.removeFromLeft(labelWidth));
            slider.setBounds(juce::Rectangle<int>(area.getX() + labelWidth + 10, label.getY(), sliderWidth, rowHeight));
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

    std::vector<juce::Rectangle<int>> rowRects;

    void addSlider(juce::Slider& slider, juce::Label& label, std::unique_ptr<SliderAttachment>& attachment,
                   const juce::String& paramID, const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 84, 22);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, findColour(juce::ComboBox::backgroundColourId));
        slider.setColour(juce::Slider::thumbColourId, findColour(juce::Slider::thumbColourId));
        slider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2A2E36));
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, findColour(juce::Label::textColourId));
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