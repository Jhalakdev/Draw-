#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../UI/LevelMeterComponent.h"

class CrystalGainLF : public juce::LookAndFeel_V4
{
public:
    static const juce::Colour bgDark;
    static const juce::Colour bgPanel;
    static const juce::Colour accentTeal;
    static const juce::Colour textBright;
    static const juce::Colour textMuted;
    static const juce::Colour textDim;
    static const juce::Colour border;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& bgColour, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto col = bgColour;
        if (isButtonDown || button.getToggleState()) col = col.brighter(0.25f);
        else if (isMouseOver) col = col.brighter(0.1f);
        g.setColour(col);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xFF2A2A44));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, juce::Slider&) override
    {
        auto cx = x + w * 0.5f, cy = y + h * 0.5f, r = jmin(w, h) * 0.4f;
        g.setColour(bgPanel.brighter(0.08f));
        g.fillEllipse(cx - r, cy - r, r * 2, r * 2);
        g.setColour(border);
        g.drawEllipse(cx - r, cy - r, r * 2, r * 2, 1.0f);
        float angle = startAngle + pos * (endAngle - startAngle);
        g.setColour(accentTeal);
        g.drawLine(cx, cy, cx + r * 0.85f * std::cos(angle), cy + r * 0.85f * std::sin(angle), 2.0f);
    }
};

class CrystalGainEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    CrystalGainEditor(CrystalGainProcessor& p);
    ~CrystalGainEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CrystalGainProcessor& processorRef;
    CrystalGainLF laf;

    juce::Slider gainKnob;
    juce::Label gainLabel, valueLabel;
    juce::TextButton phaseBtn{"\u2205"}, monoBtn{"M"};
    juce::Label balanceLabel;
    juce::Slider balanceSlider;
    LevelMeterComponent levelMeter;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> balanceAttach;
};
