#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../Source/UI/LevelMeterComponent.h"

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
    juce::TextButton phaseBtn{"PHASE"}, monoBtn{"M"};
    juce::Label balanceLabel;
    juce::Label panLabel;
    juce::Slider balanceSlider;
    LevelMeterComponent levelMeter;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> balanceAttach;
};
