#include "PluginEditor.h"

const juce::Colour DrawEQLF::bgDark    (0xFF060610);
const juce::Colour DrawEQLF::bgPanel   (0xFF0E0E1C);
const juce::Colour DrawEQLF::accentTeal(0xFF00E5B8);
const juce::Colour DrawEQLF::accentGold(0xFFFFD700);
const juce::Colour DrawEQLF::accentRed (0xFFFF5F5F);
const juce::Colour DrawEQLF::textBright(0xFFD0D0E0);
const juce::Colour DrawEQLF::textMuted (0xFF6A6A88);
const juce::Colour DrawEQLF::textDim   (0xFF404060);
const juce::Colour DrawEQLF::border    (0xFF20203A);

DrawEQSimpleEditor::DrawEQSimpleEditor(DrawEQSimpleProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p),
      eqGraph(p.getEngine(), p.getFFTAnalyzer())
{
    setLookAndFeel(&laf);
    setSize(960, 540);

    auto styleBtn = [&](juce::TextButton& btn, const juce::Colour& accent)
    {
        btn.setLookAndFeel(&laf);
        btn.setColour(juce::TextButton::buttonColourId, DrawEQLF::bgPanel);
        btn.setColour(juce::TextButton::buttonOnColourId, DrawEQLF::bgPanel.brighter(0.15f));
        btn.setColour(juce::TextButton::textColourOffId, accent);
        btn.setColour(juce::TextButton::textColourOnId, accent.brighter(0.3f));
    };

    auto styleCombo = [&](juce::ComboBox& cb)
    {
        cb.setLookAndFeel(&laf);
        cb.setColour(juce::ComboBox::backgroundColourId, DrawEQLF::bgPanel);
        cb.setColour(juce::ComboBox::textColourId, DrawEQLF::textMuted);
        cb.setColour(juce::ComboBox::arrowColourId, DrawEQLF::textMuted);
        cb.setColour(juce::ComboBox::outlineColourId, DrawEQLF::border);
        cb.setColour(juce::ComboBox::buttonColourId, DrawEQLF::bgPanel);
    };

    styleBtn(trackingBtn, DrawEQLF::accentTeal);
    trackingBtn.setClickingTogglesState(true);
    trackingBtn.onClick = [this]()
    {
        bool on = trackingBtn.getToggleState();
        processorRef.getAPVTS().getParameter("tracking")->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    styleBtn(bypassBtn, DrawEQLF::accentRed);
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.onClick = [this]()
    {
        bool on = bypassBtn.getToggleState();
        processorRef.getAPVTS().getParameter("bypass")->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    styleBtn(autoGainBtn, DrawEQLF::accentTeal);
    autoGainBtn.setClickingTogglesState(true);
    autoGainBtn.onClick = [this]()
    {
        bool on = autoGainBtn.getToggleState();
        processorRef.getAPVTS().getParameter("autoGain")->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };

    styleBtn(undoBtn, DrawEQLF::accentGold);
    styleBtn(redoBtn, DrawEQLF::accentGold);
    styleBtn(clearBtn, DrawEQLF::accentRed);

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

    styleCombo(phaseCombo);
    phaseCombo.addItemList({"Minimum", "Linear", "Natural"}, 1);

    styleCombo(msCombo);
    msCombo.addItemList({"Stereo", "Mid Only", "Side Only", "Left Only", "Right Only"}, 1);

    noteLabel.setColour(juce::Label::textColourId, DrawEQLF::accentGold);
    noteLabel.setFont(juce::Font(28.0f).boldened());

    pitchLabel.setColour(juce::Label::textColourId, DrawEQLF::textMuted);
    pitchLabel.setFont(juce::Font(9.0f));

    statusLabel.setColour(juce::Label::textColourId, DrawEQLF::textDim);
    statusLabel.setFont(juce::Font(8.0f));

    gainLabel.setColour(juce::Label::textColourId, DrawEQLF::textMuted);
    gainLabel.setFont(juce::Font(9.0f));

    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainSlider.setColour(juce::Slider::trackColourId, DrawEQLF::accentTeal.withAlpha(0.3f));
    gainSlider.setColour(juce::Slider::thumbColourId, DrawEQLF::accentTeal);
    gainSlider.setColour(juce::Slider::backgroundColourId, DrawEQLF::bgPanel.brighter(0.05f));

    addAndMakeVisible(eqGraph);
    addAndMakeVisible(levelMeter);
    addAndMakeVisible(trackingBtn);
    addAndMakeVisible(bypassBtn);
    addAndMakeVisible(phaseCombo);
    addAndMakeVisible(msCombo);
    addAndMakeVisible(undoBtn);
    addAndMakeVisible(redoBtn);
    addAndMakeVisible(clearBtn);
    addAndMakeVisible(autoGainBtn);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(noteLabel);
    addAndMakeVisible(pitchLabel);
    addAndMakeVisible(statusLabel);

    phaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "phaseMode", phaseCombo);
    msAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "msMode", msCombo);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "masterGain", gainSlider);

    startTimerHz(30);
}

DrawEQSimpleEditor::~DrawEQSimpleEditor()
{
    setLookAndFeel(nullptr);
}

void DrawEQSimpleEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setColour(DrawEQLF::bgDark);
    g.fillRect(bounds);

    auto header = bounds.removeFromTop(52);
    g.setColour(DrawEQLF::bgPanel);
    g.fillRect(header);

    // Section fills
    {
        auto base = DrawEQLF::bgPanel;
        g.setColour(base.brighter(0.06f));
        g.fillRect(80, 0, 170, 52);
        g.setColour(base.brighter(0.04f));
        g.fillRect(260, 0, 80, 52);
        g.setColour(base);
        g.fillRect(350, 0, getWidth() - 350 - 176, 52);
    }

    // Top accent bars
    g.setColour(DrawEQLF::accentTeal);
    g.fillRect(80, 0, 170, 2);
    g.setColour(DrawEQLF::accentTeal.withAlpha(0.65f));
    g.fillRect(260, 0, 80, 2);
    g.setColour(DrawEQLF::accentGold.withAlpha(0.55f));
    g.fillRect(350, 0, getWidth() - 350 - 176, 2);

    // Bottom accent bars
    g.setColour(DrawEQLF::accentTeal.withAlpha(0.55f));
    g.fillRect(80, 49, 170, 2);
    g.setColour(DrawEQLF::accentTeal.withAlpha(0.35f));
    g.fillRect(260, 49, 80, 2);
    g.setColour(DrawEQLF::accentGold.withAlpha(0.30f));
    g.fillRect(350, 49, getWidth() - 350 - 176, 2);

    // Short separators
    int sepTop = 14, sepBot = 44;
    g.setColour(DrawEQLF::border);
    g.drawVerticalLine(78, sepTop, sepBot);
    g.drawVerticalLine(256, sepTop, sepBot);
    g.drawVerticalLine(346, sepTop, sepBot);
    g.drawVerticalLine(getWidth() - 176, sepTop, sepBot);

    // Brand
    g.setColour(DrawEQLF::accentTeal);
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText("Curvex", juce::Rectangle<int>(14, 5, 64, 20), juce::Justification::centredLeft);
    g.setColour(DrawEQLF::textMuted);
    g.setFont(juce::Font(8.0f));
    g.drawText("Draw EQ " BUILD_VERSION, juce::Rectangle<int>(14, 25, 64, 12), juce::Justification::centredLeft);
    g.setColour(DrawEQLF::accentTeal);
    g.fillRoundedRectangle(4.0f, 12.0f, 4.0f, 4.0f, 2.0f);

    // Section labels
    g.setColour(DrawEQLF::textMuted);
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText("TRACK", 86, 6, 60, 10, juce::Justification::centredLeft);
    g.drawText("AUTO GAIN", 266, 6, 54, 10, juce::Justification::centredLeft);
    g.drawText("HISTORY", 356, 6, 80, 10, juce::Justification::centredLeft);

    // Bottom border
    g.setColour(DrawEQLF::border);
    g.drawLine(0.0f, 51.5f, (float)getWidth(), 51.5f, 1.0f);
}

void DrawEQSimpleEditor::paintOverChildren(juce::Graphics& g)
{
    bool bypass = processorRef.getAPVTS().getRawParameterValue("bypass")->load() > 0.5f;
    if (bypass)
    {
        g.setColour(juce::Colour(0xFF060610).withAlpha(0.55f));
        g.fillRect(getLocalBounds());
        g.setColour(DrawEQLF::accentRed.withAlpha(0.8f));
        g.setFont(juce::Font(18.0f).boldened());
        g.drawText("BYPASSED", getLocalBounds(), juce::Justification::centred, false);
    }
}

void DrawEQSimpleEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(52);

    int btnY = 16, btnH = 22;

    // TRACK (80-256)
    trackingBtn.setBounds(82, btnY, 100, btnH);
    bypassBtn.setBounds(188, btnY, 50, btnH);

    // AUTO GAIN (260-346)
    autoGainBtn.setBounds(262, btnY, 74, btnH);

    // HISTORY (350-784)
    undoBtn.setBounds(400, btnY, 50, btnH);
    redoBtn.setBounds(470, btnY, 50, btnH);
    clearBtn.setBounds(540, btnY, 50, btnH);

    // Pitch
    auto pitchArea = header.removeFromRight(148);
    int px = pitchArea.getX();
    noteLabel.setBounds(px, 6, 44, 34);
    pitchLabel.setBounds(px + 44, 8, 70, 14);
    statusLabel.setBounds(px + 44, 24, 70, 12);

    // Sidebar
    auto sidebar = area.removeFromRight(76);
    auto meterArea = sidebar.removeFromTop(sidebar.getHeight() * 0.65f);
    levelMeter.setBounds(meterArea.reduced(2));
    gainSlider.setBounds(sidebar.withTrimmedTop(4).reduced(1));
    gainLabel.setText("MASTER", juce::dontSendNotification);
    gainLabel.setBounds(sidebar.getX() - 2, sidebar.getY() - 14, 80, 12);

    // Graph
    auto graphArea = area.reduced(6, 4);
    graphArea.removeFromBottom(32);
    phaseCombo.setBounds(graphArea.getRight() - 130, getHeight() - 24, 130, 20);
    msCombo.setBounds(graphArea.getX() + static_cast<int>(graphArea.getWidth() * 0.1f), getHeight() - 24, 130, 20);
    eqGraph.setBounds(graphArea);
}

void DrawEQSimpleEditor::timerCallback()
{
    auto& apvts = processorRef.getAPVTS();
    bool tracking = apvts.getRawParameterValue("tracking")->load() > 0.5f;
    bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    bool autoGainOn = apvts.getRawParameterValue("autoGain")->load() > 0.5f;

    autoGainBtn.setColour(juce::TextButton::textColourOffId,
                          autoGainOn ? DrawEQLF::accentGold : DrawEQLF::textBright);
    trackingBtn.setToggleState(tracking, juce::dontSendNotification);
    bypassBtn.setToggleState(bypass, juce::dontSendNotification);
    bypassBtn.setColour(juce::TextButton::textColourOffId,
                        bypass ? DrawEQLF::accentRed :
                        (tracking ? DrawEQLF::accentTeal : DrawEQLF::accentRed));

    statusLabel.setText(tracking ? "TRACKING" : "IDLE", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, tracking ? DrawEQLF::textMuted : DrawEQLF::accentRed);

    float pitch = processorRef.getCurrentPitch();
    float conf = processorRef.getCurrentConfidence();
    if (tracking && pitch > 0.0f && conf > 0.3f)
    {
        noteLabel.setText(juce::MidiMessage::getMidiNoteName(juce::MidiMessage::getMidiNoteInHertz(pitch), true, true, 3),
                          juce::dontSendNotification);
        pitchLabel.setText(juce::String(pitch, 0) + " Hz", juce::dontSendNotification);
    }
    else
    {
        noteLabel.setText("--", juce::dontSendNotification);
        pitchLabel.setText("--- Hz", juce::dontSendNotification);
    }

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
    autoGainBtn.setToggleState(autoGainOn, juce::dontSendNotification);

    levelMeter.setLevels(processorRef.getLeftLevel(), processorRef.getRightLevel());
    eqGraph.repaint();
    repaint();
}
