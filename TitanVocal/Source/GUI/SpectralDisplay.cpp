// C:/Vocal Plugin/VocalCraftQuantum/Source/GUI/SpectralDisplay.cpp
#include "SpectralDisplay.h"

SpectralDisplay::SpectralDisplay(SpectralAnalyzer& analyzer, juce::AudioProcessorValueTreeState& apvts)
    : spectralAnalyzer(analyzer), parameters(apvts)
{
    setOpaque(true);
    colorGradient = juce::ColourGradient(juce::Colours::blue, 0, 0, juce::Colours::red, 100, 0, false);
    spectrogramImage = juce::Image(juce::Image::RGB, 800, 400, true);
    startTimerHz(30);
}

SpectralDisplay::~SpectralDisplay() {}

void SpectralDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    switch (currentMode)
    {
        case SPECTROGRAM:     drawSpectrogram(g); break;
        case WAVEFORM:        drawWaveform(g); break;
        case PITCH_CONTOUR:   drawPitchContour(g); break;
        case FORMANT_ANALYSIS:drawFormantAnalysis(g); break;
        case REAL_TIME_FFT:   drawRealTimeFFT(g); break;
    }
}

void SpectralDisplay::resized()
{
    spectrogramImage = juce::Image(juce::Image::RGB, getWidth(), getHeight(), true);
}

void SpectralDisplay::setDisplayMode(DisplayMode mode)
{
    currentMode = mode;
}

void SpectralDisplay::setColorScheme(const juce::ColourGradient& gradient)
{
    colorGradient = gradient;
}

void SpectralDisplay::setDecayRate(float decay)
{
    decayRate = juce::jlimit(0.0f, 1.0f, decay);
}

void SpectralDisplay::mouseDown(const juce::MouseEvent& event)
{
    handleRegionClick(event.getPosition());
}

void SpectralDisplay::mouseDrag(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
}

void SpectralDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(event, wheel);
}

void SpectralDisplay::timerCallback()
{
    if (currentMode == SPECTROGRAM)
        updateSpectrogram();
    repaint();
}

void SpectralDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(parameterID, newValue);
}

void SpectralDisplay::drawSpectrogram(juce::Graphics& g)
{
    g.drawImage(spectrogramImage, getLocalBounds().toFloat());
}

void SpectralDisplay::drawWaveform(juce::Graphics& g)
{
    std::vector<float> waveform;
    spectralAnalyzer.getWaveform(waveform);
    auto area = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white);
    juce::Path p;
    p.startNewSubPath(area.getX(), area.getCentreY());
    const float dx = area.getWidth() / (float) waveform.size();
    for (size_t i = 0; i < waveform.size(); ++i)
    {
        float x = area.getX() + dx * (float) i;
        float y = area.getCentreY() - waveform[i] * (area.getHeight() / 2.0f);
        p.lineTo(x, y);
    }
    g.strokePath(p, juce::PathStrokeType(1.5f));
}

void SpectralDisplay::drawPitchContour(juce::Graphics& g)
{
    // Simple pitch indicator using max-bin
    const float sr = 44100.0f; // if needed, pass sample rate
    float pitchHz = spectralAnalyzer.estimatePitch(sr);
    g.setColour(juce::Colours::yellow);
    g.drawText(juce::String("Pitch: ") + juce::String(pitchHz, 1) + " Hz", getLocalBounds(), juce::Justification::centred);
}

void SpectralDisplay::drawFormantAnalysis(juce::Graphics& g)
{
    // Placeholder
    g.setColour(juce::Colours::lightgreen);
    g.drawText("Formant Analysis", getLocalBounds(), juce::Justification::centred);
}

void SpectralDisplay::drawRealTimeFFT(juce::Graphics& g)
{
    const auto& mags = spectralAnalyzer.getMagnitudes();
    auto area = getLocalBounds().toFloat();
    g.setColour(juce::Colours::orange);
    if (mags.empty()) return;
    juce::Path p;
    p.startNewSubPath(area.getX(), area.getBottom());
    const float dx = area.getWidth() / (float) mags.size();
    for (size_t i = 0; i < mags.size(); ++i)
    {
        float x = area.getX() + dx * (float) i;
        float y = area.getBottom() - std::log1p(mags[i]) * 20.0f; // log scale
        p.lineTo(x, juce::jmax(area.getY(), y));
    }
    g.strokePath(p, juce::PathStrokeType(1.5f));
}

void SpectralDisplay::updateSpectrogram()
{
    const auto& mags = spectralAnalyzer.getMagnitudes();
    if (mags.empty()) return;

    // Scroll left, draw new column at right
    spectrogramImage.moveImageSection(0, 0, 1, 0, spectrogramImage.getWidth() - 1, spectrogramImage.getHeight());

    int x = spectrogramImage.getWidth() - 1;
    for (int y = 0; y < spectrogramImage.getHeight(); ++y)
    {
        int bin = juce::jmap(y, 0, spectrogramImage.getHeight() - 1, (int)mags.size() - 1, 0);
        float v = std::log1p(mags[(size_t)bin]);
        juce::Colour c = juce::Colour::fromHSV(juce::jlimit(0.0f, 1.0f, v * 0.05f), 0.9f, 0.9f, 1.0f);
        spectrogramImage.setPixel(x, y, c.overlaidWith(spectrogramImage.getPixel(x, y).withAlpha(decayRate)));
    }
}

void SpectralDisplay::handleRegionClick(const juce::Point<int>& position)
{
    juce::ignoreUnused(position);
}
// TitanVocal - Proprietary Spectral Display Implementation
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// See LICENSE.txt for strict proprietary licensing terms.
//
// File: SpectralDisplay.cpp
// Description: Rendering logic for spectral visualization modes.