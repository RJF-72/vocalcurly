// TitanVocal - Proprietary UI
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// See LICENSE.txt for strict proprietary licensing terms.
//
// File: PluginEditor.cpp
// Description: Implements TitanVocal editor UI, display mode selector, presets, and AI toggle.
#include "PluginEditor.h"

VocalCraftQuantumEditor::VocalCraftQuantumEditor(VocalCraftQuantumProcessor& p)
    : juce::AudioProcessorEditor(&p), audioProcessor(p), mainTabs(juce::TabbedButtonBar::TabsAtTop)
{
    setSize(900, 600);

    spectralDisplay = std::make_unique<SpectralDisplay>(audioProcessor.spectralAnalyzer, audioProcessor.apvts);
    parameterControls = std::make_unique<ParameterControls>(audioProcessor.apvts);
    addAndMakeVisible(mainTabs);

    // Tabs
    auto* mainPage = new juce::Component();
    mainTabs.addTab("Main", juce::Colours::darkgrey, mainPage, true);

    mainPage->addAndMakeVisible(*spectralDisplay);
    mainPage->addAndMakeVisible(*parameterControls);

    // Display mode selector
    addAndMakeVisible(displayModeBox);
    initializeDisplayModeSelector();

    // Buttons
    addAndMakeVisible(advancedModeButton);
    advancedModeButton.setButtonText("Advanced");
    advancedModeButton.addListener(this);

    addAndMakeVisible(presetSelector);
    addAndMakeVisible(loadPresetButton);
    addAndMakeVisible(savePresetButton);
    loadPresetButton.setButtonText("Load");
    savePresetButton.setButtonText("Save");
    loadPresetButton.addListener(this);
    savePresetButton.addListener(this);

    addAndMakeVisible(aiAssistantButton);
    aiAssistantButton.setButtonText("AI Assistant");
    aiAssistantButton.addListener(this);

    // AI enabled toggle
    addAndMakeVisible(aiEnabledToggle);
    aiEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "aiEnabled", aiEnabledToggle);

    // Meters
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(inputLabel);
    addAndMakeVisible(outputLabel);
    inputLabel.setText("In", juce::dontSendNotification);
    outputLabel.setText("Out", juce::dontSendNotification);

    startTimerHz(30);
}

VocalCraftQuantumEditor::~VocalCraftQuantumEditor() { }

void VocalCraftQuantumEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void VocalCraftQuantumEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto topBar = area.removeFromTop(32);
    advancedModeButton.setBounds(topBar.removeFromLeft(100));
    displayModeBox.setBounds(topBar.removeFromLeft(140));
    presetSelector.setBounds(topBar.removeFromLeft(200));
    loadPresetButton.setBounds(topBar.removeFromLeft(80));
    savePresetButton.setBounds(topBar.removeFromLeft(80));
    aiAssistantButton.setBounds(topBar.removeFromLeft(120));
    aiEnabledToggle.setBounds(topBar.removeFromLeft(60));

    auto meterArea = area.removeFromRight(80);
    inputLabel.setBounds(meterArea.removeFromTop(20));
    inputMeter.setBounds(meterArea.removeFromTop(120));
    outputLabel.setBounds(meterArea.removeFromTop(20));
    outputMeter.setBounds(meterArea.removeFromTop(120));

    mainTabs.setBounds(area);
    if (auto* comp = mainTabs.getCurrentContentComponent())
    {
        auto pageArea = comp->getLocalBounds().reduced(10);
        auto top = pageArea.removeFromTop(pageArea.getHeight() / 2);
        spectralDisplay->setBounds(top);
        parameterControls->setBounds(pageArea);
    }
}

void VocalCraftQuantumEditor::timerCallback()
{
    // Update meters using peak of last buffer sample magnitudes
    audioProcessor.spectralAnalyzer.computeSpectrum();
    const auto& mags = audioProcessor.spectralAnalyzer.getMagnitudes();
    float in = 0.0f;
    for (auto m : mags) in = std::max(in, m);
    inputMeter.setValue(in);
    outputMeter.setValue(in);
}

void VocalCraftQuantumEditor::buttonClicked(juce::Button* button)
{
    if (button == &advancedModeButton)
    {
        toggleAdvancedMode(true);
    }
    else if (button == &loadPresetButton)
    {
        loadPreset();
    }
    else if (button == &savePresetButton)
    {
        savePreset();
    }
    else if (button == &aiAssistantButton)
    {
        showAIAssistant();
    }
}

void VocalCraftQuantumEditor::showSection(UISection)
{
    // Placeholder: handled via tabs in this minimal implementation
}

void VocalCraftQuantumEditor::toggleAdvancedMode(bool)
{
    // Toggle additional controls visibility
}

void VocalCraftQuantumEditor::createPitchControls() {}
void VocalCraftQuantumEditor::createFormantControls() {}
void VocalCraftQuantumEditor::createTimeControls() {}
void VocalCraftQuantumEditor::createCreativeControls() {}
void VocalCraftQuantumEditor::createOutputControls() {}

void VocalCraftQuantumEditor::updateMeters() {}
void VocalCraftQuantumEditor::loadPreset() {
    juce::FileChooser chooser ("Load Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse(file));
        if (xml.get() != nullptr)
        {
            // Support simple <Parameters><Parameter id="..." value="..."/></Parameters> format
            if (xml->hasTagName("Parameters"))
            {
                forEachXmlChildElement(*xml, child)
                {
                    if (child->hasTagName("Parameter"))
                    {
                        auto id = child->getStringAttribute("id");
                        auto value = (float) child->getDoubleAttribute("value");
                        if (auto* p = audioProcessor.apvts.getParameter(id))
                            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
                    }
                }
                return;
            }

            // Fallback: try to replace APVTS state directly if compatible
            if (xml->hasTagName(audioProcessor.apvts.state.getType()))
            {
                audioProcessor.apvts.replaceState(juce::ValueTree::fromXml(*xml));
            }
        }
    }
}
void VocalCraftQuantumEditor::savePreset() {
    juce::FileChooser chooser ("Save Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    if (chooser.browseForFileToSave(true))
    {
        auto file = chooser.getResult();
        // Write simple portable format
        juce::XmlElement xmlRoot("Parameters");
        auto addParam = [&](const juce::String& id)
        {
            if (auto* p = audioProcessor.apvts.getRawParameterValue(id))
            {
                juce::XmlElement* child = new juce::XmlElement("Parameter");
                child->setAttribute("id", id);
                child->setAttribute("value", (double) p->load());
                xmlRoot.addChildElement(child);
            }
        };
        addParam("dryWet");
        addParam("outputGain");
        addParam("pitchAmount");
        addParam("pitchSpeed");
        addParam("formantShift");
        addParam("noiseAmount");
        addParam("saturation");
        addParam("aiEnabled");
        xmlRoot.writeToFile(file, {});
    }
}
void VocalCraftQuantumEditor::showAIAssistant() {}

void VocalCraftQuantumEditor::initializeDisplayModeSelector()
{
    displayModeBox.clear(juce::dontSendNotification);
    displayModeBox.addItem("Spectrogram", 1);
    displayModeBox.addItem("Waveform", 2);
    displayModeBox.addItem("Pitch", 3);
    displayModeBox.addItem("Formant", 4);
    displayModeBox.addItem("FFT", 5);
    displayModeBox.onChange = [this]
    {
        int id = displayModeBox.getSelectedId();
        if (id >= 1 && id <= 5)
            spectralDisplay->setDisplayMode(static_cast<SpectralDisplay::DisplayMode>(id - 1));
    };
    displayModeBox.setSelectedId(1, juce::sendNotification);
}