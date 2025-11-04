// C:/Vocal Plugin/VocalCraftQuantum/Source/GUI/SpectralDisplay.h
#pragma once

#include <JuceHeader.h>
#include "../DSP/SpectralAnalyzer.h"

class SpectralDisplay : public juce::Component,
                       private juce::Timer,
                       private juce::AudioProcessorValueTreeState::Listener {
public:
    SpectralDisplay(SpectralAnalyzer& analyzer, juce::AudioProcessorValueTreeState& apvts);
    ~SpectralDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    enum DisplayMode {
        SPECTROGRAM = 0,
        WAVEFORM,
        PITCH_CONTOUR,
        FORMANT_ANALYSIS,
        REAL_TIME_FFT
    };

    void setDisplayMode(DisplayMode mode);
    void setColorScheme(const juce::ColourGradient& gradient);
    void setDecayRate(float decay); // For persistence display

    // Interactive features
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    SpectralAnalyzer& spectralAnalyzer;
    juce::AudioProcessorValueTreeState& parameters;

    DisplayMode currentMode = SPECTROGRAM;
    juce::ColourGradient colorGradient;
    float decayRate = 0.9f;

    // Display buffers
    juce::Image spectrogramImage;
    std::vector<std::vector<float>> historyBuffer;

    // Interactive regions
    juce::Rectangle<int> pitchCorrectionRegion;
    juce::Rectangle<int> formantRegion;
    juce::Rectangle<int> timeStretchRegion;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void drawSpectrogram(juce::Graphics& g);
    void drawWaveform(juce::Graphics& g);
    void drawPitchContour(juce::Graphics& g);
    void drawFormantAnalysis(juce::Graphics& g);
    void drawRealTimeFFT(juce::Graphics& g);

    void updateSpectrogram();
    void handleRegionClick(const juce::Point<int>& position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralDisplay)
};