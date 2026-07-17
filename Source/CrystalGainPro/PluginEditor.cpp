#include "PluginEditor.h"

const juce::Colour CrystalGainProLF::bgDark    (0xFF060610);
const juce::Colour CrystalGainProLF::bgPanel   (0xFF0E0E1C);
const juce::Colour CrystalGainProLF::accentTeal(0xFF00E5B8);
const juce::Colour CrystalGainProLF::accentGold(0xFFFFD700);
const juce::Colour CrystalGainProLF::accentRed (0xFFFF5F5F);
const juce::Colour CrystalGainProLF::textBright(0xFFD0D0E0);
const juce::Colour CrystalGainProLF::textMuted (0xFF6A6A88);
const juce::Colour CrystalGainProLF::textDim   (0xFF404060);
const juce::Colour CrystalGainProLF::border    (0xFF20203A);

CrystalGainProEditor::CrystalGainProEditor(CrystalGainProProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&laf);
    setSize(380, 420);

    auto styleCombo = [&](juce::ComboBox& cb)
    {
        cb.setLookAndFeel(&laf);
        cb.setColour(juce::ComboBox::backgroundColourId, CrystalGainProLF::bgPanel);
        cb.setColour(juce::ComboBox::textColourId, CrystalGainProLF::textMuted);
        cb.setColour(juce::ComboBox::arrowColourId, CrystalGainProLF::accentGold);
        cb.setColour(juce::ComboBox::outlineColourId, CrystalGainProLF::border);
        cb.setColour(juce::ComboBox::buttonColourId, CrystalGainProLF::bgPanel);
    };

    styleCombo(charCombo);
    charCombo.addItemList({"Off", "Mister Passive", "Krane Mybiz", "West Nugget",
                           "Pool Dake", "Never 80-8", "Liquid State Solid"}, 1);

    gainFader.setSliderStyle(juce::Slider::LinearVertical);
    gainFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainFader.setColour(juce::Slider::trackColourId, CrystalGainProLF::accentTeal.withAlpha(0.4f));
    gainFader.setColour(juce::Slider::thumbColourId, CrystalGainProLF::accentTeal);
    gainFader.setColour(juce::Slider::backgroundColourId, CrystalGainProLF::bgPanel.brighter(0.05f));

    colorFader.setSliderStyle(juce::Slider::LinearVertical);
    colorFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    colorFader.setColour(juce::Slider::trackColourId, CrystalGainProLF::accentGold.withAlpha(0.4f));
    colorFader.setColour(juce::Slider::thumbColourId, CrystalGainProLF::accentGold);
    colorFader.setColour(juce::Slider::backgroundColourId, CrystalGainProLF::bgPanel.brighter(0.05f));

    titleLabel.setColour(juce::Label::textColourId, CrystalGainProLF::textMuted);
    titleLabel.setFont(juce::Font(12.0f).boldened());
    titleLabel.setText("CRYSTAL GAIN PRO", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    gainLabel.setColour(juce::Label::textColourId, CrystalGainProLF::textDim);
    gainLabel.setFont(juce::Font(8.0f));
    gainLabel.setText("GAIN", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);

    colorLabel.setColour(juce::Label::textColourId, CrystalGainProLF::textDim);
    colorLabel.setFont(juce::Font(8.0f));
    colorLabel.setText("COLOR", juce::dontSendNotification);
    colorLabel.setJustificationType(juce::Justification::centred);

    valueLabel.setColour(juce::Label::textColourId, CrystalGainProLF::accentTeal);
    valueLabel.setFont(juce::Font(11.0f).boldened());
    valueLabel.setJustificationType(juce::Justification::centred);

    colorValLabel.setColour(juce::Label::textColourId, CrystalGainProLF::accentGold);
    colorValLabel.setFont(juce::Font(11.0f).boldened());
    colorValLabel.setJustificationType(juce::Justification::centred);

    phaseBtn.setLookAndFeel(&laf);
    phaseBtn.setColour(juce::TextButton::buttonColourId, CrystalGainProLF::bgPanel);
    phaseBtn.setColour(juce::TextButton::buttonOnColourId, CrystalGainProLF::bgPanel.brighter(0.15f));
    phaseBtn.setColour(juce::TextButton::textColourOffId, CrystalGainProLF::textMuted);
    phaseBtn.setColour(juce::TextButton::textColourOnId, CrystalGainProLF::accentTeal);
    phaseBtn.setClickingTogglesState(true);

    monoBtn.setLookAndFeel(&laf);
    monoBtn.setColour(juce::TextButton::buttonColourId, CrystalGainProLF::bgPanel);
    monoBtn.setColour(juce::TextButton::buttonOnColourId, CrystalGainProLF::bgPanel.brighter(0.15f));
    monoBtn.setColour(juce::TextButton::textColourOffId, CrystalGainProLF::textMuted);
    monoBtn.setColour(juce::TextButton::textColourOnId, CrystalGainProLF::accentTeal);
    monoBtn.setClickingTogglesState(true);

    balanceSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    balanceSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    balanceSlider.setColour(juce::Slider::trackColourId, CrystalGainProLF::border);
    balanceSlider.setColour(juce::Slider::thumbColourId, CrystalGainProLF::accentTeal);
    balanceSlider.setColour(juce::Slider::backgroundColourId, CrystalGainProLF::bgPanel.brighter(0.05f));

    balanceLabel.setColour(juce::Label::textColourId, CrystalGainProLF::textDim);
    balanceLabel.setFont(juce::Font(8.0f));
    balanceLabel.setText("BAL", juce::dontSendNotification);
    balanceLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(charCombo);
    addAndMakeVisible(gainFader);
    addAndMakeVisible(colorFader);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(colorLabel);
    addAndMakeVisible(valueLabel);
    addAndMakeVisible(colorValLabel);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(phaseBtn);
    addAndMakeVisible(monoBtn);
    addAndMakeVisible(balanceSlider);
    addAndMakeVisible(balanceLabel);
    addAndMakeVisible(levelMeter);

    charAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "character", charCombo);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "gain", gainFader);
    colorAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "color", colorFader);
    phaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "phase", phaseBtn);
    monoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "mono", monoBtn);
    balanceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "balance", balanceSlider);

    startTimerHz(30);
}

CrystalGainProEditor::~CrystalGainProEditor()
{
    setLookAndFeel(nullptr);
}

void CrystalGainProEditor::paint(juce::Graphics& g)
{
    g.fillAll(CrystalGainProLF::bgDark);

    auto area = getLocalBounds().reduced(6);
    g.setColour(CrystalGainProLF::bgPanel);
    g.fillRoundedRectangle(area.toFloat(), 8.0f);
    g.setColour(CrystalGainProLF::border);
    g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.0f);

    // Section accent bars
    // Top section (title + character)
    g.setColour(CrystalGainProLF::accentGold.withAlpha(0.3f));
    g.fillRect(area.getX(), area.getY(), area.getWidth(), 2);

    // Divider line below character selector
    int divY = area.getY() + 60;
    g.setColour(CrystalGainProLF::border);
    g.drawLine(area.getX() + 10.0f, divY, area.getX() + area.getWidth() - 10.0f, divY, 1.0f);

    // Bottom bar above controls row
    int controlsY = area.getY() + area.getHeight() - 120;
    g.setColour(CrystalGainProLF::border);
    g.drawLine(area.getX() + 10.0f, controlsY, area.getX() + area.getWidth() - 10.0f, controlsY, 1.0f);
}

void CrystalGainProEditor::resized()
{
    auto area = getLocalBounds().reduced(6);

    // Title
    auto top = area.removeFromTop(24);
    titleLabel.setBounds(top);

    // Character combo
    auto charArea = area.removeFromTop(32).reduced(20, 4);
    charCombo.setBounds(charArea);

    // Faders area
    auto faderArea = area.removeFromTop(180).reduced(30, 8);

    int faderW = 80;
    auto leftFader = faderArea.removeFromLeft(faderW);
    gainFader.setBounds(leftFader.withSizeKeepingCentre(leftFader.getWidth(), leftFader.getHeight() - 20));
    gainLabel.setBounds(leftFader.removeFromBottom(16));

    auto midLabel = faderArea.removeFromLeft(40);
    valueLabel.setBounds(midLabel.removeFromTop(midLabel.getHeight() / 2));
    colorValLabel.setBounds(midLabel);

    auto rightFader = faderArea.withSizeKeepingCentre(faderW, faderArea.getHeight());
    colorFader.setBounds(rightFader.withSizeKeepingCentre(rightFader.getWidth(), rightFader.getHeight() - 20));
    colorLabel.setBounds(rightFader.removeFromBottom(16));

    // Controls row
    auto ctrlArea = area.removeFromTop(44).reduced(20, 4);
    phaseBtn.setBounds(ctrlArea.removeFromLeft(60).reduced(2));
    monoBtn.setBounds(ctrlArea.removeFromLeft(60).reduced(2));
    balanceLabel.setBounds(ctrlArea.removeFromLeft(24));
    balanceSlider.setBounds(ctrlArea.reduced(0, 6));

    // Meter
    levelMeter.setBounds(area.reduced(20, 4));
}

void CrystalGainProEditor::timerCallback()
{
    float gainDb = processorRef.getGainDb();
    float color = processorRef.getAPVTS().getRawParameterValue("color")->load();
    bool phase = processorRef.getAPVTS().getRawParameterValue("phase")->load() > 0.5f;
    bool mono = processorRef.getAPVTS().getRawParameterValue("mono")->load() > 0.5f;

    valueLabel.setText(juce::String(gainDb, 1) + " dB", juce::dontSendNotification);
    colorValLabel.setText(juce::String((int)color) + "%", juce::dontSendNotification);
    phaseBtn.setColour(juce::TextButton::textColourOffId,
                       phase ? CrystalGainProLF::accentTeal : CrystalGainProLF::textMuted);
    monoBtn.setColour(juce::TextButton::textColourOffId,
                      mono ? CrystalGainProLF::accentTeal : CrystalGainProLF::textMuted);

    levelMeter.setLevels(processorRef.getLeftLevel(), processorRef.getRightLevel());
}
