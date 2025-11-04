// C:/Vocal Plugin/TitanVocal/Source/DSP/SpectralAnalyzer.h
#pragma once

#include <JuceHeader.h>

class SpectralAnalyzer
{
public:
    explicit SpectralAnalyzer(int fftOrder = 11) // 2^11 = 2048
        : fftOrder(fftOrder),
          fftSize(1 << fftOrder),
          forwardFFT(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        timeDomainBuffer.resize(fftSize);
        freqDomainBuffer.resize(fftSize * 2);
        magnitudeBuffer.resize(fftSize / 2);
    }

    void pushAudioBuffer(const float* samples, int numSamples)
    {
        // Fill time-domain buffer (simple ring)
        for (int i = 0; i < numSamples; ++i)
        {
            timeDomainBuffer[writeIndex] = samples[i];
            writeIndex = (writeIndex + 1) % fftSize;
        }
    }

    void computeSpectrum()
    {
        // Copy the latest windowed chunk into a scratch buffer
        scratchBuffer.resize(fftSize);
        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (writeIndex + i) % fftSize;
            scratchBuffer[i] = timeDomainBuffer[idx];
        }

        window.multiplyWithWindowingTable(scratchBuffer.data(), fftSize);
        std::fill(freqDomainBuffer.begin(), freqDomainBuffer.end(), 0.0f);
        std::copy(scratchBuffer.begin(), scratchBuffer.end(), freqDomainBuffer.begin());

        forwardFFT.performRealOnlyForwardTransform(freqDomainBuffer.data());

        // Compute magnitudes
        const auto* re = freqDomainBuffer.data();
        const auto* im = freqDomainBuffer.data() + 1; // interleaved real/imag? JUCE packs as [real0, imag0, real1, imag1, ...]
        // JUCE stores: bins as [real0, imag0, real1, imag1, ...]
        for (int i = 0; i < fftSize / 2; ++i)
        {
            const float real = freqDomainBuffer[2 * i];
            const float imag = freqDomainBuffer[2 * i + 1];
            magnitudeBuffer[i] = std::sqrt(real * real + imag * imag);
        }
    }

    const std::vector<float>& getMagnitudes() const { return magnitudeBuffer; }

    float estimatePitch(float sampleRate) const
    {
        // Simple max-bin frequency estimate
        int maxIndex = 0;
        float maxValue = 0.0f;
        for (int i = 1; i < (int)magnitudeBuffer.size(); ++i)
        {
            if (magnitudeBuffer[i] > maxValue)
            {
                maxValue = magnitudeBuffer[i];
                maxIndex = i;
            }
        }
        const float binHz = sampleRate / (float)fftSize;
        return maxIndex * binHz;
    }

    void getWaveform(std::vector<float>& out) const
    {
        out.resize(fftSize);
        for (int i = 0; i < fftSize; ++i)
        {
            int idx = (writeIndex + i) % fftSize;
            out[i] = timeDomainBuffer[idx];
        }
    }

private:
    int fftOrder { 11 };
    int fftSize { 2048 };
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> timeDomainBuffer;
    std::vector<float> scratchBuffer;
    std::vector<float> freqDomainBuffer;
    std::vector<float> magnitudeBuffer;
    int writeIndex { 0 };
};
// TitanVocal - Proprietary Spectral Analyzer
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// Licensed under strict proprietary EULA in LICENSE.txt.
//
// File: SpectralAnalyzer.h
// Description: FFT-based analyzer providing magnitudes and basic pitch estimation.