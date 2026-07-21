#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CatsHairLF : public juce::LookAndFeel_V4
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
    static const juce::Colour shadeFill;
};

class SpectrumGraph : public juce::Component, public juce::Timer
{
public:
    SpectrumGraph(CatsHairProcessor& p);
    void paint(juce::Graphics&) override;
    void timerCallback() override;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -24.0f;
    static constexpr float maxDb = 12.0f;

private:
    CatsHairProcessor& processor;
    std::array<float, 256> spectrumData{};
    juce::Path targetPath, shadePath;
};

class CatsHairEditor : public juce::AudioProcessorEditor
{
public:
    CatsHairEditor(CatsHairProcessor&);
    ~CatsHairEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CatsHairProcessor& processorRef;
    CatsHairLF laf;
    SpectrumGraph graph;

    juce::Slider startFreqSlider, slantSlider, pushSlider;
    juce::Label startFreqLabel, slantLabel, pushLabel;
    juce::Label titleLabel, phaseLabel;
    juce::ComboBox phaseCombo;
    juce::TextButton bypassBtn;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttach> startAttach, slantAttach, pushAttach;
    std::unique_ptr<ComboAttach> phaseAttach;
    std::unique_ptr<ButtonAttach> bypassAttach;
};
