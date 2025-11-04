// TitanVocal - Proprietary UI
// Copyright (c) 2025 Ray Flanary and Joni Marie Flanary. All rights reserved.
// See LICENSE.txt for strict proprietary licensing terms.
//
// File: PluginEditor.cpp
// Description: Implements TitanVocal editor UI, display mode selector, presets, and AI toggle.
#include "PluginEditor.h"
// Icon-based toolbar item with hover/press feedback and vector-drawn icons
class IconToolbarItem : public juce::ToolbarItemComponent {
public:
    IconToolbarItem(int itemId, const juce::String& label, std::function<void()> onClick)
        : juce::ToolbarItemComponent(itemId, label, true, false), callback(std::move(onClick)) {}

    bool getToolbarItemSizes(int toolbarDepth, bool isVertical, int& preferredSize,
                             int& minSize, int& maxSize) override
    {
        juce::ignoreUnused(isVertical);
        const int h = juce::jmax(28, toolbarDepth - 6);
        preferredSize = h + 72; // icon + label padding
        minSize = h + 56;
        maxSize = h + 92;
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto bg = findColour(juce::Toolbar::backgroundColourId);
        auto accent = juce::Colours::silver.withAlpha(0.25f);
        auto hoverAccent = juce::Colours::white.withAlpha(0.10f);
        auto pressAccent = juce::Colours::white.withAlpha(0.18f);

        // Card background
        g.setColour(bg.darker(0.10f));
        g.fillRoundedRectangle(bounds, 6.0f);
        if (isMouseButtonDown())
        {
            g.setColour(pressAccent);
            g.fillRoundedRectangle(bounds, 6.0f);
        }
        else if (isMouseOver(true))
        {
            g.setColour(hoverAccent);
            g.fillRoundedRectangle(bounds, 6.0f);
        }

        // Icon + label layout
        auto iconArea = bounds.withSizeKeepingCentre(26.0f, 26.0f).translated(-22.0f, 0.0f);
        auto textArea = juce::Rectangle<float>(bounds.getX() + bounds.getWidth() / 2.0f - 6.0f,
                                               bounds.getY(), bounds.getWidth() / 2.0f + 6.0f, bounds.getHeight());

        // Draw icon (accent on hover/press)
        auto iconColour = juce::Colours::lightgrey;
        if (isMouseButtonDown() || isMouseOver(true))
            iconColour = findColour(juce::Slider::thumbColourId).withAlpha(0.95f);
        g.setColour(iconColour);
        drawIcon(g, iconArea.toNearestInt());

        // Draw label
        g.setColour(juce::Colours::whitesmoke);
        g.drawText(getName(), textArea, juce::Justification::centredLeft);
    }

    void mouseUp(const juce::MouseEvent&) override { if (callback) callback(); }
    void resized() override {}

private:
    std::function<void()> callback;

    void drawIcon(juce::Graphics& g, juce::Rectangle<int> r)
    {
        const int id = getItemId();
        juce::Path path;
        const float cx = r.getCentreX();
        const float cy = r.getCentreY();
        const float s = juce::jmin(r.getWidth(), r.getHeight()) * 0.42f;

        auto addGear = [&] {
            for (int i = 0; i < 8; ++i)
            {
                float a = juce::MathConstants<float>::twoPi * (float) i / 8.0f;
                float x1 = cx + std::cos(a) * (s * 0.9f);
                float y1 = cy + std::sin(a) * (s * 0.9f);
                float x2 = cx + std::cos(a) * (s * 1.2f);
                float y2 = cy + std::sin(a) * (s * 1.2f);
                path.addTriangle(x1, y1, x2, y2, cx, cy);
            }
            path.addEllipse(cx - s * 0.65f, cy - s * 0.65f, s * 1.3f, s * 1.3f);
            juce::Path inner; inner.addEllipse(cx - s * 0.35f, cy - s * 0.35f, s * 0.7f, s * 0.7f);
            path.addPath(inner);
        };

        auto addFolder = [&] {
            juce::Rectangle<float> b((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight());
            auto top = b.removeFromTop(b.getHeight() * 0.45f);
            juce::Path p;
            p.addRoundedRectangle(b.reduced(2.0f), 3.0f);
            p.addRoundedRectangle(top.reduced(4.0f).translated(6.0f, 0.0f), 3.0f);
            path.addPath(p);
        };

        auto addSave = [&] {
            juce::Path p; p.addRoundedRectangle(r.toFloat().reduced(3.0f), 3.0f);
            juce::Rectangle<float> notch((float)r.getX()+6, (float)r.getY()+6, (float)r.getWidth()-12, 10.0f);
            p.addRectangle(notch);
            p.addEllipse((float)r.getCentreX()-6, (float)r.getBottom()-16, 12.0f, 12.0f);
            path.addPath(p);
        };

        auto addRefresh = [&] {
            juce::Path p; p.addArc(cx - s, cy - s, 2*s, 2*s, juce::MathConstants<float>::pi*0.2f, juce::MathConstants<float>::pi*1.7f, true);
            juce::Line<float> l(cx + s*0.7f, cy - s*0.1f, cx + s*0.95f, cy + s*0.15f);
            p.addArrow(l, 4.0f, 8.0f, 4.0f);
            path.addPath(p);
        };

        auto addSparkles = [&] {
            juce::Path p;
            for (int i=0;i<4;++i){
                float a = juce::MathConstants<float>::twoPi * i / 4.0f;
                p.addStar(juce::Point<float>(cx + std::cos(a)*s*0.4f, cy + std::sin(a)*s*0.4f), 5, s*0.15f, s*0.35f);
            }
            path.addPath(p);
        };

        switch (id)
        {
            case TitanVocalEditor::ToolbarIDs::tbAdvanced:     addGear(); break;
            case TitanVocalEditor::ToolbarIDs::tbLoadPreset:   addFolder(); break;
            case TitanVocalEditor::ToolbarIDs::tbSavePreset:   addSave(); break;
            case TitanVocalEditor::ToolbarIDs::tbLoadDefault:  addRefresh(); break;
            case TitanVocalEditor::ToolbarIDs::tbAIAssistant:  addSparkles(); break;
            default: break;
        }
        g.strokePath(path, juce::PathStrokeType(1.6f));
    }
};

// Palette switcher toolbar item: shows swatches and opens a menu to select
class PaletteToolbarItem : public juce::ToolbarItemComponent {
public:
    PaletteToolbarItem(int itemId, std::function<void(int)> onSelect)
        : juce::ToolbarItemComponent(itemId, "Palette", true, false), selectCallback(std::move(onSelect)) {}

    bool getToolbarItemSizes(int toolbarDepth, bool isVertical, int& preferredSize,
                             int& minSize, int& maxSize) override
    {
        juce::ignoreUnused(isVertical);
        const int h = juce::jmax(28, toolbarDepth - 6);
        preferredSize = h + 80;
        minSize = h + 60;
        maxSize = h + 100;
        return true;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto bg = findColour(juce::Toolbar::backgroundColourId);
        g.setColour(bg.darker(0.10f));
        g.fillRoundedRectangle(bounds, 6.0f);
        if (isMouseButtonDown())
            g.setColour(juce::Colours::white.withAlpha(0.18f));
        else if (isMouseOver(true))
            g.setColour(juce::Colours::white.withAlpha(0.10f));
        if (isMouseOver(true) || isMouseButtonDown())
            g.fillRoundedRectangle(bounds, 6.0f);

        auto swArea = bounds.reduced(8.0f);
        auto swW = (swArea.getWidth() - 24.0f) / 3.0f;
        auto swH = swArea.getHeight() - 12.0f;
        juce::Rectangle<float> s1(swArea.getX(), swArea.getY()+6.0f, swW, swH);
        juce::Rectangle<float> s2 = s1.translated(swW + 12.0f, 0.0f);
        juce::Rectangle<float> s3 = s2.translated(swW + 12.0f, 0.0f);

        // Classic
        g.setGradientFill(juce::ColourGradient(juce::Colours::blue, s1.getX(), s1.getY(),
                                               juce::Colours::red, s1.getRight(), s1.getBottom(), false));
        g.fillRoundedRectangle(s1, 4.0f);
        // Fire
        g.setGradientFill(juce::ColourGradient(juce::Colours::black, s2.getX(), s2.getY(),
                                               juce::Colours::yellow, s2.getRight(), s2.getBottom(), false));
        g.fillRoundedRectangle(s2, 4.0f);
        // Viridis-ish
        g.setGradientFill(juce::ColourGradient(juce::Colours::blue, s3.getX(), s3.getY(),
                                               juce::Colours::yellow, s3.getRight(), s3.getBottom(), false));
        g.fillRoundedRectangle(s3, 4.0f);

        g.setColour(juce::Colours::whitesmoke);
        g.drawText("Palette", bounds.reduced(6.0f), juce::Justification::topLeft);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Classic");
        menu.addItem(2, "Fire");
        menu.addItem(3, "Viridis");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result){ if (result > 0 && selectCallback) selectCallback(result); });
    }

    void resized() override {}

private:
    std::function<void(int)> selectCallback;
};

class TitanToolbarFactory : public juce::ToolbarItemFactory {
public:
    explicit TitanToolbarFactory(TitanVocalEditor& ed) : editor(ed) {}

    void getAllItemIds(juce::Array<int>& ids) override
    {
        ids.add(TitanVocalEditor::ToolbarIDs::tbAdvanced);
        ids.add(TitanVocalEditor::ToolbarIDs::tbPalette);
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
                return new IconToolbarItem(itemId, "Advanced", [this]{ editor.toggleAdvancedMode(true); });
            case IDs::tbPalette:
                return new PaletteToolbarItem(itemId, [this](int preset){ editor.setSpectrogramPalettePreset(preset); });
            case IDs::tbLoadPreset:
                return new IconToolbarItem(itemId, "Load", [this]{ editor.loadPreset(); });
            case IDs::tbSavePreset:
                return new IconToolbarItem(itemId, "Save", [this]{ editor.savePreset(); });
            case IDs::tbLoadDefault:
                return new IconToolbarItem(itemId, "Load Default", [this]{ editor.loadDefaultPreset(); });
            case IDs::tbAIAssistant:
                return new IconToolbarItem(itemId, "AI Assistant", [this]{ editor.showAIAssistant(); });
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
    toolbar.setColour(juce::Toolbar::backgroundColourId, juce::Colour(0xFF101316));
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
    // Default to warm 'Fire' palette per user preference (red/yellow)
    spectralDisplay->setColorSchemePreset(2);
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

void TitanVocalEditor::setSpectrogramPalettePreset(int presetId)
{
    if (spectralDisplay)
        spectralDisplay->setColorSchemePreset(presetId);
    switch (presetId)
    {
        case 1: setStatus("Palette: Classic"); break;
        case 2: setStatus("Palette: Fire"); break;
        case 3: setStatus("Palette: Viridis"); break;
        default: setStatus("Palette: Custom"); break;
    }
}

void TitanVocalEditor::populateToolbar()
{
    toolbar.clear();
    TitanToolbarFactory factory(*this);
    toolbar.addDefaultItems(factory);
}