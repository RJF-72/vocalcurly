// TitanVocal - Proprietary Parameter Definitions
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// Licensed under strict proprietary EULA in LICENSE.txt.
//
// File: QuantumParameters.h
// Description: Centralized parameter IDs and ranges for TitanVocal.
#pragma once

#include <JuceHeader.h>
#include <map>
#include <string>

struct QuantumParameters
{
    // Pitch Correction Parameters
    struct PitchParams {
        float amount = 0.0f;           // 0.0 (none) to 1.0 (full correction)
        float speed = 0.5f;            // Correction speed (slow to fast)
        bool scaleLock = true;         // Lock to musical scale
        float formantPreservation = 0.8f; // Preserve original formants
        float humanize = 0.3f;         // Humanization amount
        std::string scaleType = "Chromatic"; // Scale type
    } pitch;

    // Time Correction Parameters
    struct TimeParams {
        bool enabled = false;
        float strength = 0.7f;
        float grooveAmount = 0.0f;     // Humanize/groove
        bool transientPreservation = true;
        float timingTolerance = 0.1f;  // Timing tolerance in ms
    } time;

    // Formant Control Parameters
    struct FormantParams {
        float shift = 0.0f;            // -1.0 (down) to +1.0 (up)
        float preservation = 0.9f;
        bool intelligentRepair = true;
        float resonance = 0.5f;        // Formant resonance
        float bandwidth = 0.5f;        // Formant bandwidth
    } formant;

    // Noise Reduction Parameters
    struct NoiseParams {
        float amount = 0.0f;
        float spectralSmoothing = 0.5f;
        bool adaptive = true;
        float threshold = -60.0f;      // dB threshold
        float reduction = 12.0f;       // dB reduction
    } noise;

    // Breath Control Parameters
    struct BreathParams {
        float reduction = 0.0f;
        float smoothing = 0.5f;
        bool naturalRecovery = true;
        float threshold = -40.0f;      // Breath detection threshold
        float attack = 10.0f;          // ms attack
        float release = 100.0f;        // ms release
    } breath;

    // Creative Effects Parameters
    struct CreativeParams {
        float harmonyAmount = 0.0f;
        float thickness = 0.0f;
        float morphAmount = 0.0f;
        int voiceModel = 0;            // Selected voice model
        float stereoWidth = 0.0f;
        float saturation = 0.0f;
    } creative;

    // Output Control Parameters
    struct OutputParams {
        float dryWet = 1.0f;           // 0.0 (dry) to 1.0 (wet)
        float outputGain = 0.0f;       // dB
        bool autoGain = true;
        float limiterThreshold = -1.0f; // dB
        bool dither = false;
    } output;

    // Advanced Parameters
    struct AdvancedParams {
        int processingQuality = 2;     // 0: Low, 1: Medium, 2: High
        bool realTimeMode = true;
        float latencyCompensation = 0.0f; // ms
        bool multiThreading = true;
        int bufferSize = 512;          // Samples
    } advanced;

    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);

    // Validation
    bool validate() const;
};

class ParameterManager {
public:
    ParameterManager();

    void setParameter(const std::string& path, float value);
    float getParameter(const std::string& path) const;

    void loadPreset(const juce::File& presetFile);
    void savePreset(const juce::File& presetFile);

    // AI-assisted parameter adjustment
    void applyAISuggestions(const std::vector<std::pair<std::string, float>>& suggestions);

private:
    QuantumParameters currentParams;
    std::map<std::string, juce::RangedAudioParameter*> parameterMap;
};