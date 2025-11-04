// C:/Vocal Plugin/VocalCraftQuantum/Source/GUI/PluginEditor.cpp
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
    presetSelector.setBounds(topBar.removeFromLeft(200));
    loadPresetButton.setBounds(topBar.removeFromLeft(80));
    savePresetButton.setBounds(topBar.removeFromLeft(80));
    aiAssistantButton.setBounds(topBar.removeFromLeft(120));

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
        if (xml.get() != nullptr && xml->hasTagName(audioProcessor.apvts.state.getType()))
        {
            audioProcessor.apvts.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }
}
void VocalCraftQuantumEditor::savePreset() {
    juce::FileChooser chooser ("Save Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    if (chooser.browseForFileToSave(true))
    {
        auto file = chooser.getResult();
        auto state = audioProcessor.apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        xml->writeToFile(file, {});
    }
}
void VocalCraftQuantumEditor::showAIAssistant() {}