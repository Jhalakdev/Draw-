#include "PluginEditor.h"

const juce::Colour CrystalGainLF::bgDark    (0xFF060610);
const juce::Colour CrystalGainLF::bgPanel   (0xFF0E0E1C);
const juce::Colour CrystalGainLF::accentTeal(0xFF00E5B8);
const juce::Colour CrystalGainLF::textBright(0xFFD0D0E0);
const juce::Colour CrystalGainLF::textMuted (0xFF6A6A88);
const juce::Colour CrystalGainLF::textDim   (0xFF404060);
const juce::Colour CrystalGainLF::border    (0xFF20203A);

CrystalGainEditor::CrystalGainEditor(CrystalGainProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&laf);
    setSize(280, 360);

    gainKnob.setSliderStyle(juce::Slider::LinearVertical);
    gainKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    gainKnob.setColour(juce::Slider::trackColourId, CrystalGainLF::accentTeal.withAlpha(0.4f));
    gainKnob.setColour(juce::Slider::thumbColourId, CrystalGainLF::accentTeal);
    gainKnob.setColour(juce::Slider::backgroundColourId, CrystalGainLF::bgPanel.brighter(0.05f));
    gainKnob.setColour(juce::Slider::textBoxTextColourId, CrystalGainLF::accentTeal);
    gainKnob.setColour(juce::Slider::textBoxBackgroundColourId, CrystalGainLF::bgPanel);
    gainKnob.setColour(juce::Slider::textBoxOutlineColourId, CrystalGainLF::border);

    gainLabel.setColour(juce::Label::textColourId, CrystalGainLF::textMuted);
    gainLabel.setFont(juce::Font(10.0f).boldened());
    gainLabel.setText("CRYSTAL GAIN", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);

    valueLabel.setColour(juce::Label::textColourId, CrystalGainLF::accentTeal);
    valueLabel.setFont(juce::Font(24.0f).boldened());
    valueLabel.setJustificationType(juce::Justification::centred);

    phaseBtn.setLookAndFeel(&laf);
    phaseBtn.setColour(juce::TextButton::buttonColourId, CrystalGainLF::bgPanel);
    phaseBtn.setColour(juce::TextButton::buttonOnColourId, CrystalGainLF::bgPanel.brighter(0.15f));
    phaseBtn.setColour(juce::TextButton::textColourOffId, CrystalGainLF::textMuted);
    phaseBtn.setColour(juce::TextButton::textColourOnId, CrystalGainLF::accentTeal);
    phaseBtn.setClickingTogglesState(true);

    monoBtn.setLookAndFeel(&laf);
    monoBtn.setColour(juce::TextButton::buttonColourId, CrystalGainLF::bgPanel);
    monoBtn.setColour(juce::TextButton::buttonOnColourId, CrystalGainLF::bgPanel.brighter(0.15f));
    monoBtn.setColour(juce::TextButton::textColourOffId, CrystalGainLF::textMuted);
    monoBtn.setColour(juce::TextButton::textColourOnId, CrystalGainLF::accentTeal);
    monoBtn.setClickingTogglesState(true);

    balanceSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    balanceSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 36, 16);
    balanceSlider.setColour(juce::Slider::trackColourId, CrystalGainLF::border);
    balanceSlider.setColour(juce::Slider::thumbColourId, CrystalGainLF::accentTeal);
    balanceSlider.setColour(juce::Slider::backgroundColourId, CrystalGainLF::bgPanel.brighter(0.05f));
    balanceSlider.setColour(juce::Slider::textBoxTextColourId, CrystalGainLF::accentTeal);
    balanceSlider.setColour(juce::Slider::textBoxBackgroundColourId, CrystalGainLF::bgPanel);
    balanceSlider.setColour(juce::Slider::textBoxOutlineColourId, CrystalGainLF::border);

    balanceLabel.setColour(juce::Label::textColourId, CrystalGainLF::textDim);
    balanceLabel.setFont(juce::Font(8.0f));
    balanceLabel.setText("BAL", juce::dontSendNotification);
    balanceLabel.setJustificationType(juce::Justification::centred);

    panLabel.setColour(juce::Label::textColourId, CrystalGainLF::accentTeal);
    panLabel.setFont(juce::Font(10.0f).boldened());
    panLabel.setText("C", juce::dontSendNotification);
    panLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(gainKnob);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(valueLabel);
    addAndMakeVisible(phaseBtn);
    addAndMakeVisible(monoBtn);
    addAndMakeVisible(balanceSlider);
    addAndMakeVisible(balanceLabel);
    addAndMakeVisible(panLabel);
    addAndMakeVisible(levelMeter);

    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "gain", gainKnob);
    phaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "phase", phaseBtn);
    monoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "mono", monoBtn);
    balanceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "balance", balanceSlider);

    startTimerHz(30);
}

CrystalGainEditor::~CrystalGainEditor()
{
    setLookAndFeel(nullptr);
}

void CrystalGainEditor::paint(juce::Graphics& g)
{
    g.fillAll(CrystalGainLF::bgDark);

    auto area = getLocalBounds().reduced(6);
    g.setColour(CrystalGainLF::bgPanel);
    g.fillRoundedRectangle(area.toFloat(), 8.0f);
    g.setColour(CrystalGainLF::border);
    g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.0f);

    // Top accent bar
    g.setColour(CrystalGainLF::accentTeal.withAlpha(0.4f));
    g.fillRect(area.getX(), area.getY(), area.getWidth(), 2);
}

void CrystalGainEditor::resized()
{
    auto area = getLocalBounds().reduced(6);
    auto top = area.removeFromTop(28);
    gainLabel.setBounds(top);

    auto faderArea = area.removeFromTop(170).reduced(30, 8);
    gainKnob.setBounds(faderArea.removeFromLeft(70));
    valueLabel.setBounds(faderArea);

    auto btnRow = area.removeFromTop(36).reduced(30, 0);
    phaseBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(4));
    monoBtn.setBounds(btnRow.reduced(4));

    auto balArea = area.removeFromTop(36).reduced(30, 4);
    panLabel.setBounds(balArea.removeFromLeft(40));
    balanceLabel.setBounds(balArea.removeFromLeft(24));
    balanceSlider.setBounds(balArea.reduced(0, 6));

    levelMeter.setBounds(area.reduced(20, 4));
}

void CrystalGainEditor::timerCallback()
{
    float gainDb = processorRef.getGainDb();
    float balance = processorRef.getAPVTS().getRawParameterValue("balance")->load();

    valueLabel.setText(juce::String(gainDb, 1) + " dB", juce::dontSendNotification);

    if (std::abs(balance) < 0.01f)
        panLabel.setText("C", juce::dontSendNotification);
    else if (balance < 0.0f)
        panLabel.setText("L" + juce::String((int)std::round(-balance * 100.0f)), juce::dontSendNotification);
    else
        panLabel.setText("R" + juce::String((int)std::round(balance * 100.0f)), juce::dontSendNotification);

    levelMeter.setLevels(processorRef.getLeftLevel(), processorRef.getRightLevel());
}
