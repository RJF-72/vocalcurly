// TitanVocal - UI Theme
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary
// File: Theme.h
#pragma once

#include <JuceHeader.h>

// A modern dark LookAndFeel for TitanVocal
class TitanDarkLookAndFeel : public juce::LookAndFeel_V4 {
public:
    TitanDarkLookAndFeel()
    {
        using namespace juce;
        setColour(ResizableWindow::backgroundColourId, Colour(0xFF121418));
        setColour(TextButton::buttonColourId, Colour(0xFF1E2127));
        setColour(TextButton::textColourOnId, Colours::white);
        setColour(TextButton::textColourOffId, Colours::white.withAlpha(0.9f));
        setColour(ComboBox::backgroundColourId, Colour(0xFF1E2127));
        setColour(ComboBox::textColourId, Colours::white);
        setColour(ComboBox::arrowColourId, Colours::white.withAlpha(0.9f));
        setColour(Slider::textBoxBackgroundColourId, Colour(0xFF1E2127));
        setColour(Slider::textBoxTextColourId, Colours::white);
        setColour(Slider::thumbColourId, Colour(0xFF3AA6FF));
        setColour(Slider::trackColourId, Colour(0xFF2A2E36));
        setColour(Label::textColourId, Colours::white.withAlpha(0.9f));
        setColour(ToggleButton::tickColourId, Colour(0xFF3AA6FF));
        setColour(ToggleButton::textColourId, Colours::white);
        setColour(TooltipWindow::textColourId, Colours::white);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto base = backgroundColour;
        if (isButtonDown) base = base.brighter(0.2f);
        else if (isMouseOverButton) base = base.brighter(0.1f);
        g.setColour(base);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);
        auto bounds = juce::Rectangle<float>((float) width, (float) height);
        g.setColour(findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colour(0x22FFFFFF));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        if (label.isEnabled())
            g.fillAll(juce::Colour(0x00121418));
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(label.getFont());
        g.drawFittedText(label.getText(), label.getLocalBounds(), label.getJustificationType(), 1);
    }
};