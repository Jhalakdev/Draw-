#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/EQGraphComponent.h"
#include "UI/LevelMeterComponent.h"

#define BUILD_VERSION "v28"

class PitchFollowEQLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static const juce::Colour bgDark;
    static const juce::Colour bgPanel;
    static const juce::Colour accentTeal;
    static const juce::Colour accentGold;
    static const juce::Colour accentRed;
    static const juce::Colour textBright;
    static const juce::Colour textMuted;
    static const juce::Colour textDim;
    static const juce::Colour border;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& bgColour, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto col = bgColour;
        bool toggled = button.getToggleState();
        if (isButtonDown || toggled) col = col.brighter(0.25f);
        else if (isMouseOver) col = col.brighter(0.1f);
        g.setColour(col);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xFF2A2A44));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }
};

class PitchFollowEQAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PitchFollowEQAudioProcessorEditor(PitchFollowEQAudioProcessor&);
    ~PitchFollowEQAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    PitchFollowEQAudioProcessor& processorRef;
    PitchFollowEQLookAndFeel lnf;
    EQGraphComponent eqGraph;
    LevelMeterComponent levelMeter;

    juce::TextButton trackingBtn { "FUNDAMENTAL" };
    juce::TextButton bypassBtn   { "BYPASS" };
    juce::TextButton autoGainBtn { "AUTO GAIN" };
    juce::TextButton undoBtn     { "UNDO" };
    juce::TextButton redoBtn     { "REDO" };
    juce::TextButton clearBtn    { "CLEAR" };

    juce::Label pitchLabel;
    juce::Label noteLabel;
    juce::Label statusLabel;

    juce::ComboBox charCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> charAttachment;
    juce::Slider charBlendSlider;
    juce::Label charBlendLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> charBlendAttachment;

    juce::Label gainLabel;
    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> trackingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::ComboBox phaseCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseAttachment;

    juce::ComboBox msCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> msAttachment;

    void setupSlider(juce::Slider& s, juce::Colour thumb);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchFollowEQAudioProcessorEditor)
};
