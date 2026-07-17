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
    charBlendSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    charBlendSlider.setColour(juce::Slider::trackColourId, LF::border);
    charBlendSlider.setColour(juce::Slider::thumbColourId, LF::accentGold);
    charBlendSlider.setColour(juce::Slider::backgroundColourId, LF::bgPanel);
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

    juce::ColourGradient headerGrad(LF::bgPanel.withAlpha(0.98f), 0, 0,
                                     LF::bgPanel.brighter(0.03f), 0, 52, false);
    g.setGradientFill(headerGrad);
    g.fillRect(header);

    g.setColour(LF::border);
    g.drawLine(0.0f, 51.5f, static_cast<float>(getWidth()), 51.5f, 1.0f);

    // Brand
    g.setColour(LF::accentTeal.withAlpha(0.95f));
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText("Curvex", juce::Rectangle<int>(12, 2, 64, 22),
               juce::Justification::centredLeft);
    g.setColour(LF::textMuted);
    g.setFont(juce::Font(9.0f));
    g.drawText("Draw EQ " BUILD_VERSION, juce::Rectangle<int>(12, 22, 100, 16),
               juce::Justification::centredLeft);

    // Group separator lines + labels
    int hh = 52;
    g.setColour(LF::border);
    g.drawVerticalLine(78, 0, hh);
    g.setColour(LF::textDim);
    g.setFont(juce::Font(6.0f));
    g.drawText("TRACK", 82, 2, 60, 10, juce::Justification::centred);
    g.setColour(LF::border);
    g.drawVerticalLine(220, 0, hh);
    g.setColour(LF::textDim);
    g.setFont(juce::Font(6.0f));
    g.drawText("EMULATION", 224, 2, 200, 10, juce::Justification::centredLeft);
    g.setColour(LF::border);
    g.drawVerticalLine(436, 0, hh);
    g.setColour(LF::textDim);
    g.setFont(juce::Font(6.0f));
    g.drawText("GAIN", 440, 2, 60, 10, juce::Justification::centred);
    g.setColour(LF::border);
    g.drawVerticalLine(506, 0, hh);
    g.setColour(LF::textDim);
    g.setFont(juce::Font(6.0f));
    g.drawText("HISTORY", 510, 2, 200, 10, juce::Justification::centredLeft);
    g.setColour(LF::border);
    g.drawVerticalLine(getWidth() - 176, 0, hh);
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

    // ===== HEADER =====
    int x = 80;

    // Group 1: FUNDAMENTAL + BYPASS
    trackingBtn.setBounds(x, btnY, 76, btnH);
    x += 80;
    bypassBtn.setBounds(x, btnY, 52, btnH);
    x += 64;

    // Group 2: Emulation combo + BLEND (bigger)
    charCombo.setBounds(x, btnY, 128, btnH);
    x += 132;
    charBlendSlider.setBounds(x, btnY - 2, 72, btnH + 8);
    charBlendLabel.setBounds(x, btnY - 14, 72, 12);
    x += 80;

    // Group 3: AUTO GAIN
    autoGainBtn.setBounds(x, btnY, 64, btnH);
    x += 72;

    // Group 4: Undo/Redo/Clear — wider, shifted 30% right
    x += 50;
    undoBtn.setBounds(x, btnY, 44, btnH);
    x += 48;
    redoBtn.setBounds(x, btnY, 44, btnH);
    x += 48;
    clearBtn.setBounds(x, btnY, 44, btnH);
    x += 52;

    // Pitch info right side
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
