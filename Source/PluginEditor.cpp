#include "PluginEditor.h"

using LF = PitchFollowEQLookAndFeel;

const juce::Colour LF::bgDark      (0xFF060610);
const juce::Colour LF::bgPanel     (0xFF0E0E1C);
const juce::Colour LF::accentTeal  (0xFF00E5B8);
const juce::Colour LF::accentGold  (0xFFFFD700);
const juce::Colour LF::accentRed   (0xFFFF5F5F);
const juce::Colour LF::textBright  (0xFFEEEEFF);
const juce::Colour LF::textMuted   (0xFF6A6A88);
const juce::Colour LF::textDim     (0xFF404060);
const juce::Colour LF::border      (0xFF20203A);

void PitchFollowEQAudioProcessorEditor::setupSlider(juce::Slider& s, juce::Colour thumb)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 12);
    s.setColour(juce::Slider::trackColourId, LF::border);
    s.setColour(juce::Slider::thumbColourId, thumb);
    s.setColour(juce::Slider::textBoxTextColourId, LF::textBright);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour(juce::Slider::backgroundColourId, LF::bgPanel);
}

PitchFollowEQAudioProcessorEditor::PitchFollowEQAudioProcessorEditor(PitchFollowEQAudioProcessor& p)
    : AudioProcessorEditor(p), processorRef(p), eqGraph(p.getEngine(), p.getFFTAnalyzer())
{
    setLookAndFeel(&lnf);
    setSize(960, 520);
    setResizable(true, true);
    setResizeLimits(800, 450, 1600, 1000);

    addAndMakeVisible(eqGraph);
    addAndMakeVisible(levelMeter);

    auto styleBtn = [this](juce::TextButton& btn, juce::Colour textCol, juce::Colour bg)
    {
        btn.setColour(juce::TextButton::buttonColourId, bg);
        btn.setColour(juce::TextButton::buttonOnColourId, bg.brighter(0.2f));
        btn.setColour(juce::TextButton::textColourOffId, textCol);
        btn.setColour(juce::TextButton::textColourOnId, textCol.brighter(0.5f));
        btn.setLookAndFeel(&lnf);
        addAndMakeVisible(btn);
    };

    styleBtn(trackingBtn, LF::accentTeal, LF::bgPanel);
    trackingBtn.setClickingTogglesState(true);
    trackingBtn.onClick = [this]()
    {
        bool on = trackingBtn.getToggleState();
        processorRef.getAPVTS().getParameter("tracking")->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    styleBtn(bypassBtn, LF::accentRed, LF::bgPanel);
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.onClick = [this]()
    {
        bool on = bypassBtn.getToggleState();
        processorRef.getAPVTS().getParameter("bypass")->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    styleBtn(autoGainBtn, LF::accentTeal, LF::bgPanel);
    autoGainBtn.setClickingTogglesState(true);
    autoGainBtn.onClick = [this]()
    {
        bool on = autoGainBtn.getToggleState();
        processorRef.getAPVTS().getParameter("autoGain")->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    autoGainBtn.setTooltip("Auto Gain — maintains constant perceived level");

    styleBtn(undoBtn, LF::accentGold, LF::bgPanel);
    styleBtn(redoBtn, LF::accentGold, LF::bgPanel);
    styleBtn(clearBtn, LF::accentRed, LF::bgPanel);

    phaseCombo.setEditableText(false);
    phaseCombo.addItemList({ "Minimum Phase", "Linear Phase", "Natural Phase" }, 1);
    phaseCombo.setSelectedItemIndex(0);
    phaseCombo.setColour(juce::ComboBox::backgroundColourId, LF::bgPanel);
    phaseCombo.setColour(juce::ComboBox::textColourId, LF::textBright);
    phaseCombo.setColour(juce::ComboBox::arrowColourId, LF::textMuted);
    phaseCombo.setColour(juce::ComboBox::outlineColourId, LF::border);
    phaseCombo.setColour(juce::ComboBox::buttonColourId, LF::bgPanel);
    addAndMakeVisible(phaseCombo);
    phaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "phaseMode", phaseCombo);

    msCombo.setEditableText(false);
    msCombo.addItemList({ "Stereo", "Mid Only", "Side Only", "Left Only", "Right Only" }, 1);
    msCombo.setSelectedItemIndex(0);
    msCombo.setColour(juce::ComboBox::backgroundColourId, LF::bgPanel);
    msCombo.setColour(juce::ComboBox::textColourId, LF::textBright);
    msCombo.setColour(juce::ComboBox::arrowColourId, LF::textMuted);
    msCombo.setColour(juce::ComboBox::outlineColourId, LF::border);
    msCombo.setColour(juce::ComboBox::buttonColourId, LF::bgPanel);
    addAndMakeVisible(msCombo);
    msAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "msMode", msCombo);

    charCombo.setEditableText(false);
    charCombo.addItemList({ "Off", "Mister Passive", "Krane Mybiz", "West Nugget", "Pool Dake", "Never 80-8", "Liquid State Solid" }, 1);
    charCombo.setSelectedItemIndex(0);
    charCombo.setColour(juce::ComboBox::backgroundColourId, LF::bgPanel);
    charCombo.setColour(juce::ComboBox::textColourId, LF::textBright);
    charCombo.setColour(juce::ComboBox::arrowColourId, LF::textMuted);
    charCombo.setColour(juce::ComboBox::outlineColourId, LF::border);
    charCombo.setColour(juce::ComboBox::buttonColourId, LF::bgPanel);
    addAndMakeVisible(charCombo);
    charAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "character", charCombo);

    charBlendSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    charBlendSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    charBlendSlider.setColour(juce::Slider::trackColourId, LF::border);
    charBlendSlider.setColour(juce::Slider::thumbColourId, LF::accentGold);
    charBlendSlider.setColour(juce::Slider::backgroundColourId, LF::bgPanel);
    addAndMakeVisible(charBlendSlider);
    charBlendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "charBlend", charBlendSlider);

    charBlendLabel.setText("BLEND", juce::dontSendNotification);
    charBlendLabel.setColour(juce::Label::textColourId, LF::textDim);
    charBlendLabel.setFont(7.0f);
    charBlendLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(charBlendLabel);

    noteLabel.setText("--", juce::dontSendNotification);
    noteLabel.setColour(juce::Label::textColourId, LF::accentGold);
    noteLabel.setFont(24.0f);
    addAndMakeVisible(noteLabel);

    pitchLabel.setText("-- Hz", juce::dontSendNotification);
    pitchLabel.setColour(juce::Label::textColourId, LF::textBright);
    pitchLabel.setFont(10.0f);
    addAndMakeVisible(pitchLabel);

    statusLabel.setText("", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, LF::textDim);
    statusLabel.setFont(9.0f);
    addAndMakeVisible(statusLabel);

    gainLabel.setText("GAIN", juce::dontSendNotification);
    gainLabel.setColour(juce::Label::textColourId, LF::textMuted);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setFont(8.0f);
    addAndMakeVisible(gainLabel);

    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 12);
    gainSlider.setColour(juce::Slider::trackColourId, LF::border);
    gainSlider.setColour(juce::Slider::thumbColourId, LF::accentTeal);
    gainSlider.setColour(juce::Slider::textBoxTextColourId, LF::textBright);
    gainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    gainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    gainSlider.setColour(juce::Slider::backgroundColourId, LF::bgPanel);
    addAndMakeVisible(gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "masterGain", gainSlider);

    undoBtn.onClick = [this]()
    {
        auto& env = processorRef.getEngine().getEnvelope();
        env.undo();
        eqGraph.markResponseDirty();
        eqGraph.repaint();
    };
    redoBtn.onClick = [this]()
    {
        auto& env = processorRef.getEngine().getEnvelope();
        env.redo();
        eqGraph.markResponseDirty();
        eqGraph.repaint();
    };
    clearBtn.onClick = [this]()
    {
        auto& env = processorRef.getEngine().getEnvelope();
        env.pushUndo();
        env.clear();
        eqGraph.markResponseDirty();
        eqGraph.repaint();
    };

    // Zone panel
    zonePanel.setVisible(false);
    addAndMakeVisible(zonePanel);

    zoneTitle.setColour(juce::Label::textColourId, LF::accentRed);
    zoneTitle.setFont(juce::Font(10.0f).boldened());
    zonePanel.addAndMakeVisible(zoneTitle);

    styleBtn(zoneDynBtn, LF::accentGold, LF::bgPanel);
    zoneDynBtn.setClickingTogglesState(true);
    zonePanel.addAndMakeVisible(zoneDynBtn);
    zoneDynBtn.onClick = [this]()
    {
        int idx = eqGraph.getSelectedZone();
        if (idx < 0) return;
        auto& z = processorRef.getEngine().getEqualizer().getZone(idx);
        z.enabled = zoneDynBtn.getToggleState();
        processorRef.getEngine().getEqualizer().rebuildBands();
        eqGraph.markResponseDirty();
    };

    zoneDeleteBtn.setColour(juce::TextButton::buttonColourId, LF::bgPanel);
    zoneDeleteBtn.setColour(juce::TextButton::textColourOffId, LF::accentRed);
    zoneDeleteBtn.setLookAndFeel(&lnf);
    zonePanel.addAndMakeVisible(zoneDeleteBtn);
    zoneDeleteBtn.onClick = [this]()
    {
        int idx = eqGraph.getSelectedZone();
        if (idx < 0) return;
        processorRef.getEngine().getEqualizer().removeZone(idx);
        eqGraph.setSelectedZone(-1);
        eqGraph.markResponseDirty();
    };

    auto zoneSliderStyle = [this](juce::Slider& s, juce::Label& lbl, const char* text, juce::Colour col)
    {
        setupSlider(s, col);
        lbl.setText(text, juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, LF::textDim);
        lbl.setFont(8.0f);
        lbl.setJustificationType(juce::Justification::centredRight);
        zonePanel.addAndMakeVisible(s);
        zonePanel.addAndMakeVisible(lbl);
    };

    zoneSliderStyle(zoneThreshSlider, zoneThreshLabel, "TH", LF::accentTeal);
    zoneSliderStyle(zoneRatioSlider, zoneRatioLabel, "RT", LF::accentGold);
    zoneSliderStyle(zoneAttackSlider, zoneAttackLabel, "AT", LF::textBright);
    zoneSliderStyle(zoneReleaseSlider, zoneReleaseLabel, "RL", LF::textBright);
    zoneSliderStyle(zoneRangeSlider, zoneRangeLabel, "RN", LF::accentRed);

    zoneThreshSlider.setRange(-60.0f, 0.0f, 0.5f);
    zoneThreshSlider.onValueChange = [this]() { updateZonePanel(eqGraph.getSelectedZone()); };
    zoneRatioSlider.setRange(1.0f, 20.0f, 0.5f);
    zoneRatioSlider.onValueChange = [this]() { updateZonePanel(eqGraph.getSelectedZone()); };
    zoneAttackSlider.setRange(0.1f, 500.0f, 0.1f);
    zoneAttackSlider.setSkewFactor(0.3f);
    zoneAttackSlider.onValueChange = [this]() { updateZonePanel(eqGraph.getSelectedZone()); };
    zoneReleaseSlider.setRange(5.0f, 2000.0f, 1.0f);
    zoneReleaseSlider.setSkewFactor(0.4f);
    zoneReleaseSlider.onValueChange = [this]() { updateZonePanel(eqGraph.getSelectedZone()); };
    zoneRangeSlider.setRange(1.0f, 36.0f, 0.5f);
    zoneRangeSlider.onValueChange = [this]() { updateZonePanel(eqGraph.getSelectedZone()); };

    startTimerHz(15);
}

PitchFollowEQAudioProcessorEditor::~PitchFollowEQAudioProcessorEditor()
{
    trackingBtn.setLookAndFeel(nullptr);
    bypassBtn.setLookAndFeel(nullptr);
    autoGainBtn.setLookAndFeel(nullptr);
    undoBtn.setLookAndFeel(nullptr);
    redoBtn.setLookAndFeel(nullptr);
    clearBtn.setLookAndFeel(nullptr);
    charBlendSlider.setLookAndFeel(nullptr);
    zoneDynBtn.setLookAndFeel(nullptr);
    zoneDeleteBtn.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    stopTimer();
}

void PitchFollowEQAudioProcessorEditor::updateZonePanel(int idx)
{
    if (idx < 0 || idx >= processorRef.getEngine().getEqualizer().getNumZones())
    {
        zonePanel.setVisible(false);
        return;
    }

    auto& z = processorRef.getEngine().getEqualizer().getZone(idx);
    zonePanel.setVisible(true);
    zoneTitle.setText("Z" + juce::String(idx + 1), juce::dontSendNotification);
    zoneDynBtn.setToggleState(z.enabled, juce::dontSendNotification);
    zoneDynBtn.setButtonText(z.enabled ? "ON" : "OFF");

    if (!zoneThreshSlider.isMouseButtonDown())
        zoneThreshSlider.setValue(z.threshold, juce::dontSendNotification);
    else z.threshold = (float)zoneThreshSlider.getValue();

    if (!zoneRatioSlider.isMouseButtonDown())
        zoneRatioSlider.setValue(z.ratio, juce::dontSendNotification);
    else z.ratio = (float)zoneRatioSlider.getValue();

    if (!zoneAttackSlider.isMouseButtonDown())
        zoneAttackSlider.setValue(z.attackMs, juce::dontSendNotification);
    else z.attackMs = (float)zoneAttackSlider.getValue();

    if (!zoneReleaseSlider.isMouseButtonDown())
        zoneReleaseSlider.setValue(z.releaseMs, juce::dontSendNotification);
    else z.releaseMs = (float)zoneReleaseSlider.getValue();

    if (!zoneRangeSlider.isMouseButtonDown())
        zoneRangeSlider.setValue(z.range, juce::dontSendNotification);
    else z.range = (float)zoneRangeSlider.getValue();

    processorRef.getEngine().getEqualizer().rebuildBands();
}

void PitchFollowEQAudioProcessorEditor::timerCallback()
{
    float pitch = processorRef.getCurrentPitch();
    float conf = processorRef.getCurrentConfidence();

    if (pitch > 20.0f && conf > 0.3f)
    {
        pitchLabel.setText(juce::String(pitch, 1) + " Hz", juce::dontSendNotification);
        noteLabel.setText(PitchFollowEngine::getNoteName(pitch), juce::dontSendNotification);
        noteLabel.setAlpha(juce::jmin(1.0f, conf * 1.5f));
    }
    else
    {
        pitchLabel.setText("-- Hz", juce::dontSendNotification);
    noteLabel.setText("--", juce::dontSendNotification);
        noteLabel.setAlpha(0.3f);
    }

    bool tracking = processorRef.getAPVTS().getRawParameterValue("tracking")->load() > 0.5f;
    bool bypass = processorRef.getAPVTS().getRawParameterValue("bypass")->load() > 0.5f;
    bool autoGainOn = processorRef.getAPVTS().getRawParameterValue("autoGain")->load() > 0.5f;
    int msMode = (int)processorRef.getAPVTS().getRawParameterValue("msMode")->load();

    trackingBtn.setToggleState(tracking, juce::dontSendNotification);
    bypassBtn.setToggleState(bypass, juce::dontSendNotification);
    autoGainBtn.setToggleState(autoGainOn, juce::dontSendNotification);
    autoGainBtn.setColour(juce::TextButton::textColourOnId,
                          autoGainOn ? LF::accentGold : LF::textBright);

    statusLabel.setText(tracking ? (bypass ? "BYPASSED" : "ACTIVE") : "IDLE", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
                          bypass ? LF::accentRed : (tracking ? LF::accentTeal : LF::textDim));

    auto& env = processorRef.getEngine().getEnvelope();
    if (env.isAudioDirty())
    {
        env.clearAudioDirty();
        processorRef.getEngine().getEqualizer().markDirty();
        processorRef.getEngine().getEqualizer().flushDirty();
        eqGraph.markResponseDirty();
    }

    undoBtn.setEnabled(env.canUndo());
    redoBtn.setEnabled(env.canRedo());

    levelMeter.setLevels(processorRef.getLeftLevel(), processorRef.getRightLevel());

    float blendVal = processorRef.getAPVTS().getRawParameterValue("charBlend")->load();
    charBlendLabel.setText(juce::String(static_cast<int>(blendVal)) + "%", juce::dontSendNotification);

    int sel = eqGraph.getSelectedZone();
    updateZonePanel(sel);
    if (sel < 0 && zonePanel.isVisible())
        zonePanel.setVisible(false);
}

void PitchFollowEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setColour(LF::bgDark);
    g.fillRect(bounds);

    auto header = bounds.removeFromTop(44);

    juce::ColourGradient headerGrad(LF::bgPanel.withAlpha(0.98f), 0, 0,
                                     LF::bgPanel.brighter(0.03f), 0, 44, false);
    g.setGradientFill(headerGrad);
    g.fillRect(header);

    g.setColour(LF::border);
    g.drawLine(0.0f, 43.5f, static_cast<float>(getWidth()), 43.5f, 1.0f);

    // Brand
    g.setColour(LF::accentTeal.withAlpha(0.95f));
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText("Curvex", juce::Rectangle<int>(12, 2, 64, 22),
               juce::Justification::centredLeft);
    g.setColour(LF::textMuted);
    g.setFont(juce::Font(9.0f));
    g.drawText("Draw EQ " BUILD_VERSION, juce::Rectangle<int>(12, 22, 100, 16),
               juce::Justification::centredLeft);

    // Separator lines between groups
    g.setColour(LF::border);
    int h2 = 44;
    g.drawVerticalLine(216, 10, h2 - 10);
    g.drawVerticalLine(getWidth() - 176, 10, h2 - 10);
}

void PitchFollowEQAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    bool bypass = processorRef.getAPVTS().getRawParameterValue("bypass")->load() > 0.5f;
    if (bypass)
    {
        g.setColour(juce::Colour(0xFF060610).withAlpha(0.55f));
        g.fillRect(getLocalBounds());
        g.setColour(LF::accentRed.withAlpha(0.8f));
        g.setFont(juce::Font(18.0f).boldened());
        g.drawText("BYPASSED", getLocalBounds(), juce::Justification::centred, false);
    }
}

void PitchFollowEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(44);

    int btnY = 12;
    int btnH = 20;

    // ===== HEADER =====
    int x = 88;

    // Group 1: SPECTRUM + BYPASS
    trackingBtn.setBounds(x, btnY, 64, btnH);
    x += 68;
    bypassBtn.setBounds(x, btnY, 52, btnH);
    x += 64;

    // Group 2: Character combo + BLEND
    charCombo.setBounds(x, btnY, 108, btnH);
    x += 112;
    charBlendSlider.setBounds(x, btnY + 2, 48, btnH - 4);
    charBlendLabel.setBounds(x, btnY - 8, 48, 10);
    x += 56;

    // Group 3: Undo/Redo/Clear
    undoBtn.setBounds(x, btnY, 38, btnH);
    x += 42;
    redoBtn.setBounds(x, btnY, 38, btnH);
    x += 42;
    clearBtn.setBounds(x, btnY, 38, btnH);
    x += 50;

    // Pitch info right side
    auto pitchArea = header.removeFromRight(160);
    int px = pitchArea.getX();
    noteLabel.setBounds(px, 4, 44, 34);
    pitchLabel.setBounds(px + 44, 6, 70, 14);
    statusLabel.setBounds(px + 44, 20, 70, 12);

    // ===== LEFT: Phase combo below brand =====
    phaseCombo.setBounds(10, 50, 110, 20);

    // ===== RIGHT SIDEBAR =====
    auto sidebar = area.removeFromRight(72);
    auto meterArea = sidebar.removeFromTop(sidebar.getHeight() * 0.55f);
    levelMeter.setBounds(meterArea.reduced(2));

    // Auto Gain button above gain slider
    auto agArea = sidebar.removeFromTop(28);
    autoGainBtn.setBounds(agArea.reduced(4, 4));

    gainSlider.setBounds(sidebar.withTrimmedTop(4).reduced(4));
    gainLabel.setBounds(sidebar.getX() - 4, sidebar.getY() - 12, 72, 10);

    // Sync AG toggle state position
    bool autoGainOn = processorRef.getAPVTS().getRawParameterValue("autoGain")->load() > 0.5f;
    autoGainBtn.setToggleState(autoGainOn, juce::dontSendNotification);

    // ===== BOTTOM: M/S combo centered =====
    int msW = 140;
    int msH = 20;
    msCombo.setBounds((getWidth() - msW) / 2, getHeight() - 26, msW, msH);

    // ===== ZONE STRIP + GRAPH =====
    int zoneH = 36;
    bool showZone = zonePanel.isVisible();
    auto graphArea = area.reduced(6, 4);
    graphArea.removeFromBottom(30); // space for bottom M/S combo

    if (showZone)
    {
        auto bottomStrip = graphArea.removeFromBottom(zoneH);
        zonePanel.setBounds(bottomStrip);

        int zx = bottomStrip.getX() + 4;
        zoneTitle.setBounds(zx, 0, 30, bottomStrip.getHeight());
        zoneDynBtn.setBounds(zx + 32, 4, 28, bottomStrip.getHeight() - 8);
        zoneDeleteBtn.setBounds(zx + 64, 4, 20, bottomStrip.getHeight() - 8);

        int sliderY = 4;
        int sliderH = bottomStrip.getHeight() - 8;
        int sliderW = 62;
        int labelW = 18;
        int gap = 3;
        int sx = zx + 90;

        auto place = [&](juce::Slider& s, juce::Label& l)
        {
            l.setBounds(sx, sliderY, labelW, sliderH);
            s.setBounds(sx + labelW + 2, sliderY, sliderW, sliderH);
            sx += labelW + 2 + sliderW + gap;
        };
        place(zoneThreshSlider, zoneThreshLabel);
        place(zoneRatioSlider, zoneRatioLabel);
        place(zoneAttackSlider, zoneAttackLabel);
        place(zoneReleaseSlider, zoneReleaseLabel);
        place(zoneRangeSlider, zoneRangeLabel);
    }
    else
    {
        zonePanel.setBounds(0, 0, 0, 0);
    }

    eqGraph.setBounds(graphArea);
}
