// TitanVocal - Proprietary UI
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// See LICENSE.txt for strict proprietary licensing terms.
//
// File: PluginEditor.cpp
// Description: Implements TitanVocal editor UI, display mode selector, presets, and AI toggle.
#include "PluginEditor.h"
// Minimal custom Toolbar item to avoid drawable dependencies
class ActionToolbarItem : public juce::ToolbarItemComponent {
public:
    ActionToolbarItem(int itemId, const juce::String& label, std::function<void()> onClick)
        : juce::ToolbarItemComponent(itemId, label, true, false), callback(std::move(onClick)) {}

    bool getToolbarItemSizes(int toolbarDepth, bool isVertical, int& preferredSize,
                             int& minSize, int& maxSize) override
    {
        juce::ignoreUnused(isVertical);
        preferredSize = 110; minSize = 90; maxSize = 140;
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto base = findColour(juce::TextButton::buttonColourId);
        g.setColour(base);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(findColour(juce::TextButton::textColourOnId));
        g.drawText(getName(), getLocalBounds(), juce::Justification::centred);
    }

    void mouseUp(const juce::MouseEvent&) override { if (callback) callback(); }
    void resized() override {}

private:
    std::function<void()> callback;
};

class TitanToolbarFactory : public juce::ToolbarItemFactory {
public:
    explicit TitanToolbarFactory(TitanVocalEditor& ed) : editor(ed) {}

    void getAllItemIds(juce::Array<int>& ids) override
    {
        ids.add(TitanVocalEditor::ToolbarIDs::tbAdvanced);
        ids.add(TitanVocalEditor::ToolbarIDs::tbLoadPreset);
        ids.add(TitanVocalEditor::ToolbarIDs::tbSavePreset);
        ids.add(TitanVocalEditor::ToolbarIDs::tbLoadDefault);
        ids.add(TitanVocalEditor::ToolbarIDs::tbAIAssistant);
    }

    void getDefaultItemIds(juce::Array<int>& ids) override { getAllItemIds(ids); }

    juce::ToolbarItemComponent* createItem(int itemId) override
    {
        using IDs = TitanVocalEditor::ToolbarIDs;
        switch (itemId)
        {
            case IDs::tbAdvanced:
                return new ActionToolbarItem(itemId, "Advanced", [this]{ editor.toggleAdvancedMode(true); });
            case IDs::tbLoadPreset:
                return new ActionToolbarItem(itemId, "Load", [this]{ editor.loadPreset(); });
            case IDs::tbSavePreset:
                return new ActionToolbarItem(itemId, "Save", [this]{ editor.savePreset(); });
            case IDs::tbLoadDefault:
                return new ActionToolbarItem(itemId, "Load Default", [this]{ editor.loadDefaultPreset(); });
            case IDs::tbAIAssistant:
                return new ActionToolbarItem(itemId, "AI Assistant", [this]{ editor.showAIAssistant(); });
            default:
                break;
        }
        return nullptr;
    }

private:
    TitanVocalEditor& editor;
};

TitanVocalEditor::TitanVocalEditor(TitanVocalProcessor& p)
    : juce::AudioProcessorEditor(&p), audioProcessor(p), mainTabs(juce::TabbedButtonBar::TabsAtTop)
{
    setSize(900, 600);
    setLookAndFeel(&darkTheme);

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

    // Top toolbar (items to be populated progressively)
    addAndMakeVisible(toolbar);
    populateToolbar();

    // Preset selector (kept outside toolbar for now)
    addAndMakeVisible(presetSelector);

    // AI enabled toggle
    addAndMakeVisible(aiEnabledToggle);
    aiEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "aiEnabled", aiEnabledToggle);

    // AI model selector
    addAndMakeVisible(aiModelBox);
    aiModelBox.addItem("Noise Red.", 1);
    aiModelBox.addItem("Pitch Corr.", 2);
    aiModelBox.addItem("Formant Rep.", 3);
    aiModelBox.addItem("Breath Ctrl.", 4);
    aiModelBox.addItem("Voice Morph.", 5);
    aiModelBox.addItem("Timing Corr.", 6);
    aiModelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "aiModelType", aiModelBox);

    // Default preset will be accessible via toolbar (coming soon)

    // Meters
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(inputLabel);
    addAndMakeVisible(outputLabel);
    inputLabel.setText("In", juce::dontSendNotification);
    outputLabel.setText("Out", juce::dontSendNotification);

    // Status bar
    statusBar.setJustificationType(juce::Justification::centredLeft);
    statusBar.setText("Ready", juce::dontSendNotification);
    addAndMakeVisible(statusBar);

    startTimerHz(30);
}

TitanVocalEditor::~TitanVocalEditor() { }

void TitanVocalEditor::paint(juce::Graphics& g)
{
    auto bg = findColour(juce::ResizableWindow::backgroundColourId);
    g.fillAll(bg);

    // Top header accent
    juce::Rectangle<float> header(0.0f, 0.0f, (float) getWidth(), 36.0f);
    juce::Colour accent = juce::Colour(0xFF0F1115);
    g.setGradientFill(juce::ColourGradient(accent, 0, 0, accent.brighter(0.06f), getWidth(), 0, false));
    g.fillRect(header);
}

void TitanVocalEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto status = area.removeFromBottom(24);
    statusBar.setBounds(status);

    // Toolbar row
    auto toolbarRow = area.removeFromTop(36);
    toolbar.setBounds(toolbarRow);

    // Controls row under toolbar (FlexBox for responsiveness)
    auto controlsRow = area.removeFromTop(36);
    {
        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
        fb.alignItems = juce::FlexBox::AlignItems::stretch;
        fb.items.add(juce::FlexItem(displayModeBox).withMinWidth(120.0f).withMaxWidth(180.0f).withHeight((float) controlsRow.getHeight()).withMargin(juce::FlexItem::Margin(0, 6, 0, 0)));
        fb.items.add(juce::FlexItem(presetSelector).withMinWidth(180.0f).withMaxWidth(260.0f).withHeight((float) controlsRow.getHeight()).withMargin(juce::FlexItem::Margin(0, 6, 0, 0)));
        fb.items.add(juce::FlexItem(aiEnabledToggle).withMinWidth(60.0f).withMaxWidth(90.0f).withHeight((float) controlsRow.getHeight()).withMargin(juce::FlexItem::Margin(0, 6, 0, 0)));
        fb.items.add(juce::FlexItem(aiModelBox).withMinWidth(120.0f).withMaxWidth(180.0f).withHeight((float) controlsRow.getHeight()));
        fb.performLayout(controlsRow);
    }

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

void TitanVocalEditor::timerCallback()
{
    // Update meters using peak of last buffer sample magnitudes
    audioProcessor.spectralAnalyzer.computeSpectrum();
    const auto& mags = audioProcessor.spectralAnalyzer.getMagnitudes();
    float in = 0.0f;
    for (auto m : mags) in = std::max(in, m);
    inputMeter.setValue(in);
    outputMeter.setValue(in);
}

void TitanVocalEditor::buttonClicked(juce::Button* button)
{
    juce::ignoreUnused(button);
}

void TitanVocalEditor::showSection(UISection)
{
    // Placeholder: handled via tabs in this minimal implementation
}

void TitanVocalEditor::toggleAdvancedMode(bool)
{
    // Toggle additional controls visibility
}

void TitanVocalEditor::createPitchControls() {}
void TitanVocalEditor::createFormantControls() {}
void TitanVocalEditor::createTimeControls() {}
void TitanVocalEditor::createCreativeControls() {}
void TitanVocalEditor::createOutputControls() {}

void TitanVocalEditor::updateMeters() {}
void TitanVocalEditor::loadPreset() {
    // Use async file chooser to avoid JUCE_MODAL_LOOPS_PERMITTED requirements in plugin hosts
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Load Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    activeFileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        // Release chooser after callback completes
        activeFileChooser.reset();

        if (! file.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                   "Load Preset",
                                                   "No file selected or file does not exist.");
            return;
        }

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
                setStatus("Preset loaded");
                return;
            }

            // Fallback: try to replace APVTS state directly if compatible
            if (xml->hasTagName(audioProcessor.apvts.state.getType()))
            {
                audioProcessor.apvts.replaceState(juce::ValueTree::fromXml(*xml));
                setStatus("Preset state applied");
            }
            else
            {
                setStatus("Unsupported preset format");
            }
        }
        else
        {
            setStatus("Failed to parse preset file");
        }
    });
}
void TitanVocalEditor::savePreset() {
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Save Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");

    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
    activeFileChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto resultFile = chooser.getResult();
        // Release chooser after callback completes
        activeFileChooser.reset();

        if (resultFile == juce::File())
        {
            setStatus("No file selected");
            return;
        }

        auto file = resultFile;
        if (! file.hasFileExtension(".xml"))
            file = file.withFileExtension("xml");

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

        if (file.existsAsFile() && file.getSize() > 0)
            setStatus("Preset saved: " + file.getFileName());
        else
            setStatus("Failed to save preset");
    });
}
void TitanVocalEditor::showAIAssistant() {}

void TitanVocalEditor::initializeDisplayModeSelector()
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
    // Optional: map number keys to quick color schemes for spectrogram
    spectralDisplay->setColorSchemePreset(1);
}

void TitanVocalEditor::loadDefaultPreset()
{
    juce::File defaultPreset("C:/Vocal Plugin/TitanVocal/Resources/Presets/Default.xml");
    if (! defaultPreset.existsAsFile())
    {
        setStatus("Default preset missing, choose a preset file");
        loadPreset();
        return;
    }
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse(defaultPreset));
    if (xml.get() != nullptr)
    {
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
        if (xml->hasTagName(audioProcessor.apvts.state.getType()))
            audioProcessor.apvts.replaceState(juce::ValueTree::fromXml(*xml));
        setStatus("Default preset applied");
    }
}

void TitanVocalEditor::setStatus(const juce::String& text)
{
    statusBar.setText(text, juce::dontSendNotification);
}

void TitanVocalEditor::populateToolbar()
{
    toolbar.clear();
    TitanToolbarFactory factory(*this);
    toolbar.addDefaultItems(factory);
}