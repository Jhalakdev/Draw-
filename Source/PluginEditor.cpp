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
    charCombo.addItemList({ "Off", "Mister Passive", "Krane Mybiz", "West Nugget", "Pool Dake", "Never 80-8", "Liquid State 4k" }, 1);
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
    charBlendSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 14);
    charBlendSlider.setColour(juce::Slider::trackColourId, LF::border);
    charBlendSlider.setColour(juce::Slider::thumbColourId, LF::accentGold);
    charBlendSlider.setColour(juce::Slider::backgroundColourId, LF::bgPanel);
    charBlendSlider.setColour(juce::Slider::textBoxTextColourId, LF::accentGold);
    charBlendSlider.setColour(juce::Slider::textBoxBackgroundColourId, LF::bgPanel);
    charBlendSlider.setColour(juce::Slider::textBoxOutlineColourId, LF::border);
    addAndMakeVisible(charBlendSlider);
    charBlendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "charBlend", charBlendSlider);

    charBlendLabel.setText("BLEND", juce::dontSendNotification);
    charBlendLabel.setColour(juce::Label::textColourId, LF::textBright);
    charBlendLabel.setFont(8.0f);
    charBlendLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(charBlendLabel);

    charBlendSlider.setColour(juce::Slider::trackColourId, LF::accentGold.withAlpha(0.4f));
    charBlendSlider.setColour(juce::Slider::thumbColourId, LF::accentGold);
    charBlendSlider.setColour(juce::Slider::backgroundColourId, LF::bgPanel.brighter(0.1f));

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
    gainLabel.setFont(9.0f);
    addAndMakeVisible(gainLabel);

    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 12);
    gainSlider.setColour(juce::Slider::trackColourId, LF::accentTeal.withAlpha(0.3f));
    gainSlider.setColour(juce::Slider::thumbColourId, LF::accentTeal);
    gainSlider.setColour(juce::Slider::backgroundColourId, LF::bgPanel.brighter(0.05f));
    gainSlider.setColour(juce::Slider::textBoxTextColourId, LF::textBright);
    gainSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    gainSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "masterGain", gainSlider);

    undoBtn.onClick = [this]()
    {
        auto& env = processorRef.getEngine().getEnvelope();
        env.undo();
        env.markAudioDirty();
        eqGraph.markResponseDirty();
        eqGraph.repaint();
    };
    redoBtn.onClick = [this]()
    {
        auto& env = processorRef.getEngine().getEnvelope();
        env.redo();
        env.markAudioDirty();
        eqGraph.markResponseDirty();
        eqGraph.repaint();
    };
    clearBtn.onClick = [this]()
    {
        auto& env = processorRef.getEngine().getEnvelope();
        env.pushUndo();
        env.clear();
        env.markAudioDirty();
        eqGraph.markResponseDirty();
        eqGraph.repaint();
    };

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
    setLookAndFeel(nullptr);
    stopTimer();
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
                          bypass ? LF::accentRed : (tracking ? LF::accentTeal : LF::accentRed));

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
}

void PitchFollowEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setColour(LF::bgDark);
    g.fillRect(bounds);

    auto header = bounds.removeFromTop(52);

    // Full header background
    g.setColour(LF::bgPanel);
    g.fillRect(header);

    // Section background fills — alternating brightness
    {
        auto base = LF::bgPanel;
        g.setColour(base.brighter(0.06f));          // TRACK
        g.fillRect(80, 0, 146, 52);
        g.setColour(base.brighter(0.01f));           // EMULATION
        g.fillRect(230, 0, 222, 52);
        g.setColour(base.brighter(0.04f));           // AUTO GAIN
        g.fillRect(456, 0, 66, 52);
        g.setColour(base);                            // HISTORY
        g.fillRect(526, 0, getWidth() - 526 - 176, 52);
    }

    // ---- Colored accent bars at top of each section ----
    g.setColour(LF::accentTeal);
    g.fillRect(80, 0, 146, 2);
    g.setColour(LF::accentGold.withAlpha(0.65f));
    g.fillRect(230, 0, 222, 2);
    g.setColour(LF::accentTeal.withAlpha(0.65f));
    g.fillRect(456, 0, 66, 2);
    g.setColour(LF::accentGold.withAlpha(0.55f));
    g.fillRect(526, 0, getWidth() - 526 - 176, 2);

    // ---- Colored accent bars at bottom of each section ----
    g.setColour(LF::accentTeal.withAlpha(0.55f));
    g.fillRect(80, 49, 146, 2);
    g.setColour(LF::accentGold.withAlpha(0.35f));
    g.fillRect(230, 49, 222, 2);
    g.setColour(LF::accentTeal.withAlpha(0.35f));
    g.fillRect(456, 49, 66, 2);
    g.setColour(LF::accentGold.withAlpha(0.30f));
    g.fillRect(526, 49, getWidth() - 526 - 176, 2);

    // ---- Short separator lines aligned with button row ----
    int sepTop = 14, sepBot = 44;
    g.setColour(LF::border);
    g.drawVerticalLine(78, sepTop, sepBot);
    g.drawVerticalLine(226, sepTop, sepBot);
    g.drawVerticalLine(454, sepTop, sepBot);
    g.drawVerticalLine(524, sepTop, sepBot);
    g.drawVerticalLine(getWidth() - 176, sepTop, sepBot);

    // ---- Brand ----
    g.setColour(LF::accentTeal);
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText("Curvex", juce::Rectangle<int>(14, 5, 64, 20),
               juce::Justification::centredLeft);
    g.setColour(LF::textMuted);
    g.setFont(juce::Font(8.0f));
    g.drawText("Draw EQ " BUILD_VERSION, juce::Rectangle<int>(14, 25, 64, 12),
               juce::Justification::centredLeft);

    // Brand accent dot
    g.setColour(LF::accentTeal);
    g.fillRoundedRectangle(4.0f, 12.0f, 4.0f, 4.0f, 2.0f);

    // ---- Section labels (under accent bars) ----
    g.setColour(LF::textMuted);
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText("TRACK", 86, 6, 60, 10, juce::Justification::centredLeft);
    g.drawText("EMULATION", 236, 6, 180, 10, juce::Justification::centredLeft);
    g.drawText("AUTO GAIN", 462, 6, 54, 10, juce::Justification::centredLeft);
    g.drawText("HISTORY", 532, 6, 80, 10, juce::Justification::centredLeft);

    // Bottom border
    g.setColour(LF::border);
    g.drawLine(0.0f, 51.5f, static_cast<float>(getWidth()), 51.5f, 1.0f);
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
    auto header = area.removeFromTop(52);

    int btnY = 16;
    int btnH = 22;

    // ===== HEADER — groups mapped to paint section boundaries =====

    // Group 1: TRACKING (80-226)
    trackingBtn.setBounds(82, btnY, 90, btnH);
    bypassBtn.setBounds(178, btnY, 44, btnH);

    // Group 2: EMULATION (230-454)
    charCombo.setBounds(232, btnY, 128, btnH);
    charBlendSlider.setBounds(366, btnY - 2, 84, btnH + 8);
    charBlendLabel.setBounds(366, btnY - 14, 84, 12);

    // Group 3: AUTO GAIN (456-524)
    autoGainBtn.setBounds(458, btnY, 62, btnH);

    // Group 4: HISTORY (526-784) — evenly spread across 258px
    undoBtn.setBounds(560, btnY, 48, btnH);
    redoBtn.setBounds(630, btnY, 48, btnH);
    clearBtn.setBounds(700, btnY, 48, btnH);

    // Pitch info right side (784-960, 176px)
    auto pitchArea = header.removeFromRight(148);
    int px = pitchArea.getX();
    noteLabel.setBounds(px, 6, 44, 34);
    pitchLabel.setBounds(px + 44, 8, 70, 14);
    statusLabel.setBounds(px + 44, 24, 70, 12);

    // ===== RIGHT SIDEBAR =====
    auto sidebar = area.removeFromRight(76);
    auto meterArea = sidebar.removeFromTop(sidebar.getHeight() * 0.65f);
    levelMeter.setBounds(meterArea.reduced(2));

    gainSlider.setBounds(sidebar.withTrimmedTop(4).reduced(1));
    gainLabel.setText("MASTER", juce::dontSendNotification);
    gainLabel.setFont(9.0f);
    gainLabel.setBounds(sidebar.getX() - 2, sidebar.getY() - 14, 80, 12);

    // ===== GRAPH AREA =====
    auto graphArea = area.reduced(6, 4);
    graphArea.removeFromBottom(32);

    // Phase combo at bottom-right of graph
    phaseCombo.setBounds(graphArea.getRight() - 130, getHeight() - 24, 130, 20);

    // M/S combo at bottom-left of graph (shifted 10% right)
    msCombo.setBounds(graphArea.getX() + static_cast<int>(graphArea.getWidth() * 0.1f), getHeight() - 24, 130, 20);

    eqGraph.setBounds(graphArea);
}
