#include "PluginEditor.h"

const juce::Colour CatsHairLF::bgDark    (0xFF060610);
const juce::Colour CatsHairLF::bgPanel   (0xFF0E0E1C);
const juce::Colour CatsHairLF::accentTeal(0xFF00E5B8);
const juce::Colour CatsHairLF::accentGold(0xFFFFD700);
const juce::Colour CatsHairLF::accentRed (0xFFFF5F5F);
const juce::Colour CatsHairLF::textBright(0xFFD0D0E0);
const juce::Colour CatsHairLF::textMuted (0xFF6A6A88);
const juce::Colour CatsHairLF::textDim   (0xFF404060);
const juce::Colour CatsHairLF::border    (0xFF20203A);
const juce::Colour CatsHairLF::shadeFill (0x3300E5B8);

SpectrumGraph::SpectrumGraph(CatsHairProcessor& p) : processor(p)
{
    startTimerHz(30);
}

void SpectrumGraph::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(CatsHairLF::bgPanel);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(CatsHairLF::border);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    float padL = 50.0f, padR = 20.0f, padT = 20.0f, padB = 25.0f;
    float gx = bounds.getX() + padL;
    float gy = bounds.getY() + padT;
    float gw = bounds.getWidth() - padL - padR;
    float gh = bounds.getHeight() - padT - padB;
    auto graphArea = juce::Rectangle<float>(gx, gy, gw, gh);

    auto freqToX = [&](float freq) {
        float t = std::log(freq / minFreq) / std::log(maxFreq / minFreq);
        return gx + t * gw;
    };
    auto dbToY = [&](float db) {
        float t = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
        return gy + gh - t * gh;
    };

    g.setColour(CatsHairLF::border);
    g.drawRect(graphArea, 1.0f);

    g.setColour(CatsHairLF::textDim.withAlpha(0.3f));
    for (float freq : {50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f})
    {
        float x = freqToX(freq);
        g.drawVerticalLine((int)x, gy, gy + gh);
    }
    for (float db = -24.0f; db <= 12.0f; db += 6.0f)
    {
        float y = dbToY(db);
        g.drawHorizontalLine((int)y, gx, gx + gw);
    }

    g.setColour(CatsHairLF::textDim);
    g.setFont(9.0f);
    for (float freq : {50.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f})
    {
        float x = freqToX(freq);
        juce::String label;
        if (freq < 1000.0f)
            label = juce::String((int)freq);
        else
            label = juce::String(freq / 1000.0f, 1) + "k";
        g.drawText(label, (int)x - 15, (int)(gy + gh + 2), 30, 15,
                   juce::Justification::centred);
    }
    for (float db = -24.0f; db <= 12.0f; db += 6.0f)
    {
        float y = dbToY(db);
        g.drawText(juce::String((int)db),
                   2, (int)y - 7, 45, 14, juce::Justification::right);
    }

    juce::Path targetPath, shadePath;
    bool first = true;
    for (int i = 0; i < 512; ++i)
    {
        float freq = minFreq * std::pow(maxFreq / minFreq, i / 511.0f);
        float db = processor.getTargetDb(freq);
        float x = freqToX(freq);
        float y = dbToY(db);
        y = juce::jlimit(gy, gy + gh, y);

        if (first)
        {
            shadePath.startNewSubPath(x, gy + gh);
            shadePath.lineTo(x, y);
            targetPath.startNewSubPath(x, y);
            first = false;
        }
        else
        {
            shadePath.lineTo(x, y);
            targetPath.lineTo(x, y);
        }
    }
    shadePath.lineTo(freqToX(maxFreq), gy + gh);
    shadePath.closeSubPath();

    g.setColour(CatsHairLF::shadeFill);
    g.fillPath(shadePath);
    g.setColour(CatsHairLF::accentTeal);
    g.strokePath(targetPath, juce::PathStrokeType(2.0f));

    g.setColour(CatsHairLF::accentGold.withAlpha(0.6f));
    juce::Path spectrumPath;
    first = true;
    for (int i = 0; i < 256; ++i)
    {
        float t = i / 255.0f;
        float freq = minFreq * std::pow(maxFreq / minFreq, t);
        float x = freqToX(freq);
        float db = juce::jmap(spectrumData[i], 0.0f, 1.0f, minDb, maxDb);
        float y = dbToY(db);
        y = juce::jlimit(gy, gy + gh, y);

        if (first) { spectrumPath.startNewSubPath(x, y); first = false; }
        else spectrumPath.lineTo(x, y);
    }
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));
}

void SpectrumGraph::timerCallback()
{
    if (processor.hasNewSpectrum())
    {
        auto& spec = processor.getSpectrum();
        for (int i = 0; i < 256; ++i)
            spectrumData[i] = spectrumData[i] * 0.7f + spec.magnitude[i] * 0.3f;
        processor.clearSpectrumFlag();
        repaint();
    }
}

CatsHairEditor::CatsHairEditor(CatsHairProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), graph(p)
{
    setLookAndFeel(&laf);
    setSize(420, 500);

    auto styleSlider = [&](juce::Slider& s, const juce::Colour& accent)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 14);
        s.setColour(juce::Slider::trackColourId, accent.withAlpha(0.4f));
        s.setColour(juce::Slider::thumbColourId, accent);
        s.setColour(juce::Slider::backgroundColourId, CatsHairLF::bgPanel.brighter(0.05f));
        s.setColour(juce::Slider::textBoxTextColourId, accent);
        s.setColour(juce::Slider::textBoxBackgroundColourId, CatsHairLF::bgPanel);
        s.setColour(juce::Slider::textBoxOutlineColourId, CatsHairLF::border);
    };

    styleSlider(startFreqSlider, CatsHairLF::accentTeal);
    styleSlider(slantSlider, CatsHairLF::accentGold);
    styleSlider(pushSlider, CatsHairLF::accentRed);

    startFreqLabel.setColour(juce::Label::textColourId, CatsHairLF::textMuted);
    startFreqLabel.setFont(juce::Font(9.0f).boldened());
    startFreqLabel.setText("START", juce::dontSendNotification);

    slantLabel.setColour(juce::Label::textColourId, CatsHairLF::textMuted);
    slantLabel.setFont(juce::Font(9.0f).boldened());
    slantLabel.setText("SLANT", juce::dontSendNotification);

    pushLabel.setColour(juce::Label::textColourId, CatsHairLF::textMuted);
    pushLabel.setFont(juce::Font(9.0f).boldened());
    pushLabel.setText("PUSH", juce::dontSendNotification);

    titleLabel.setColour(juce::Label::textColourId, CatsHairLF::textMuted);
    titleLabel.setFont(juce::Font(14.0f).boldened());
    titleLabel.setText("CAT'S HAIR", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);

    phaseLabel.setColour(juce::Label::textColourId, CatsHairLF::textDim);
    phaseLabel.setFont(juce::Font(8.0f));
    phaseLabel.setText("PHASE", juce::dontSendNotification);
    phaseLabel.setJustificationType(juce::Justification::centred);

    phaseCombo.setLookAndFeel(&laf);
    phaseCombo.setColour(juce::ComboBox::backgroundColourId, CatsHairLF::bgPanel);
    phaseCombo.setColour(juce::ComboBox::textColourId, CatsHairLF::textMuted);
    phaseCombo.setColour(juce::ComboBox::arrowColourId, CatsHairLF::accentGold);
    phaseCombo.setColour(juce::ComboBox::outlineColourId, CatsHairLF::border);
    phaseCombo.setColour(juce::ComboBox::buttonColourId, CatsHairLF::bgPanel);
    phaseCombo.addItemList({"Linear", "Natural"}, 1);

    bypassBtn.setLookAndFeel(&laf);
    bypassBtn.setColour(juce::TextButton::buttonColourId, CatsHairLF::bgPanel);
    bypassBtn.setColour(juce::TextButton::buttonOnColourId, CatsHairLF::accentRed);
    bypassBtn.setColour(juce::TextButton::textColourOffId, CatsHairLF::textMuted);
    bypassBtn.setColour(juce::TextButton::textColourOnId, CatsHairLF::bgDark);
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setButtonText("BYPASS");

    addAndMakeVisible(graph);
    addAndMakeVisible(titleLabel);
    addAndMakeVisible(startFreqSlider);
    addAndMakeVisible(startFreqLabel);
    addAndMakeVisible(slantSlider);
    addAndMakeVisible(slantLabel);
    addAndMakeVisible(pushSlider);
    addAndMakeVisible(pushLabel);
    addAndMakeVisible(phaseCombo);
    addAndMakeVisible(phaseLabel);
    addAndMakeVisible(bypassBtn);

    startAttach = std::make_unique<SliderAttach>(processorRef.getAPVTS(), "startFreq", startFreqSlider);
    slantAttach = std::make_unique<SliderAttach>(processorRef.getAPVTS(), "slant", slantSlider);
    pushAttach = std::make_unique<SliderAttach>(processorRef.getAPVTS(), "push", pushSlider);
    phaseAttach = std::make_unique<ComboAttach>(processorRef.getAPVTS(), "phaseMode", phaseCombo);
    bypassAttach = std::make_unique<ButtonAttach>(processorRef.getAPVTS(), "bypass", bypassBtn);
}

CatsHairEditor::~CatsHairEditor()
{
    setLookAndFeel(nullptr);
}

void CatsHairEditor::paint(juce::Graphics& g)
{
    g.fillAll(CatsHairLF::bgDark);
    auto area = getLocalBounds().reduced(6);
    g.setColour(CatsHairLF::bgPanel);
    g.fillRoundedRectangle(area.toFloat(), 8.0f);
    g.setColour(CatsHairLF::border);
    g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.0f);
}

void CatsHairEditor::resized()
{
    auto area = getLocalBounds().reduced(6);
    auto top = area.removeFromTop(30);
    titleLabel.setBounds(top);

    auto graphArea = area.removeFromTop(240).reduced(8, 4);
    graph.setBounds(graphArea);

    auto ctrlArea = area.reduced(20, 8);

    auto startArea = ctrlArea.removeFromTop(48);
    startFreqLabel.setBounds(startArea.removeFromLeft(48));
    startFreqSlider.setBounds(startArea);

    auto slantArea = ctrlArea.removeFromTop(48);
    slantLabel.setBounds(slantArea.removeFromLeft(48));
    slantSlider.setBounds(slantArea);

    auto pushArea = ctrlArea.removeFromTop(48);
    pushLabel.setBounds(pushArea.removeFromLeft(48));
    pushSlider.setBounds(pushArea);

    auto bottomArea = ctrlArea.removeFromTop(36).removeFromLeft(240);
    phaseLabel.setBounds(bottomArea.removeFromLeft(36));
    phaseCombo.setBounds(bottomArea.removeFromLeft(100));
    bypassBtn.setBounds(bottomArea.removeFromLeft(80).reduced(4));
}
