#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../UI/EQGraphComponent.h"
#include "../UI/LevelMeterComponent.h"

#define BUILD_VERSION "v1"

class DrawEQLF : public juce::LookAndFeel_V4
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

class DrawEQSimpleEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    DrawEQSimpleEditor(DrawEQSimpleProcessor& p);
    ~DrawEQSimpleEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    DrawEQSimpleProcessor& processorRef;
    DrawEQLF laf;

    juce::TextButton trackingBtn{"FUNDAMENTAL"};
    juce::TextButton bypassBtn{"BYPASS"};
    juce::ComboBox phaseCombo, msCombo;
    juce::TextButton undoBtn{"UNDO"}, redoBtn{"REDO"}, clearBtn{"CLEAR"};
    juce::TextButton autoGainBtn{"AUTO"};

    juce::Slider gainSlider;
    juce::Label gainLabel, noteLabel, pitchLabel, statusLabel;
    LevelMeterComponent levelMeter;
    EQGraphComponent eqGraph;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> trackingBtnAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassBtnAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> msAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;

    void styleBtn(juce::TextButton& btn, const juce::Colour& accent);
    void styleCombo(juce::ComboBox& cb);
};
