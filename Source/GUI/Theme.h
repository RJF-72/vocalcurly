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
        // Typography (prefer Windows default modern font)
        setDefaultSansSerifTypefaceName("Segoe UI");

        // Design tokens
        primary = Colour(0xFF3AA6FF);
        surface = Colour(0xFF121418);
        surfaceAlt = Colour(0xFF1A1D22);
        outline = Colour(0x22FFFFFF);
        textMain = Colours::white;
        textSubtle = Colours::white.withAlpha(0.9f);
        success = Colour(0xFF3ECF8E);
        warning = Colour(0xFFFFD166);
        error   = Colour(0xFFEF476F);

        // Global colours
        setColour(ResizableWindow::backgroundColourId, surface);
        setColour(Toolbar::backgroundColourId, Colour(0xFF101316));

        // Buttons
        setColour(TextButton::buttonColourId, surfaceAlt);
        setColour(TextButton::textColourOnId, textMain);
        setColour(TextButton::textColourOffId, textSubtle);

        // ComboBox
        setColour(ComboBox::backgroundColourId, surfaceAlt);
        setColour(ComboBox::textColourId, textMain);
        setColour(ComboBox::arrowColourId, textSubtle);

        // Sliders
        setColour(Slider::textBoxBackgroundColourId, surfaceAlt);
        setColour(Slider::textBoxTextColourId, textMain);
        setColour(Slider::thumbColourId, primary);
        setColour(Slider::trackColourId, Colour(0xFF2A2E36));

        // Labels & toggles
        setColour(Label::textColourId, textSubtle);
        setColour(ToggleButton::tickColourId, primary);
        setColour(ToggleButton::textColourId, textMain);
        setColour(TooltipWindow::textColourId, textMain);

        // Tabs
        setColour(TabbedButtonBar::tabTextColourId, textSubtle);
        setColour(TabbedButtonBar::tabOutlineColourId, outline);
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
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(outline);
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        if (label.isEnabled())
            g.fillAll(surface.withAlpha(0.0f));
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(label.getFont());
        g.drawFittedText(label.getText(), label.getLocalBounds(), label.getJustificationType(), 1);
    }

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        auto area = button.getLocalBounds().toFloat();
        bool active = button.isFrontTab();
        auto bg = active ? surfaceAlt.brighter(0.08f) : surfaceAlt.darker(0.02f);
        if (isMouseDown) bg = bg.brighter(0.12f);
        else if (isMouseOver) bg = bg.brighter(0.06f);
        g.setColour(bg);
        g.fillRoundedRectangle(area.reduced(2.0f), 8.0f);
        g.setColour(outline);
        g.drawRoundedRectangle(area.reduced(2.0f), 8.0f, 1.0f);
        g.setColour(active ? textMain : textSubtle);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifTypefaceName(), 14.0f, juce::Font::plain));
        g.drawText(button.getButtonText(), area, juce::Justification::centred);
    }

private:
    juce::Colour primary, surface, surfaceAlt, outline, textMain, textSubtle, success, warning, error;
};