#pragma once
#include <JuceHeader.h>
#include "DSP/PitchFollowEngine.h"
#include "UI/SpectrumAnalyzer.h"

class PitchFollowEQAudioProcessor : public juce::AudioProcessor
{
public:
    PitchFollowEQAudioProcessor();
    ~PitchFollowEQAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    float getAutoGainDb() const { return autoGainDb; }

    PitchFollowEngine& getEngine() { return engine; }
    SpectrumAnalyzer& getSpectrumAnalyzer() { return spectrumAnalyzer; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    float getCurrentPitch() const { return engine.getCurrentPitch(); }
    float getCurrentConfidence() const { return engine.getConfidence(); }
    float getLeftLevel() const { return leftLevel; }
    float getRightLevel() const { return rightLevel; }

private:
    PitchFollowEngine engine;
    SpectrumAnalyzer spectrumAnalyzer;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    bool prepared = false;
    double lastSampleRate = 0;
    int lastSamplesPerBlock = 0;
    float leftLevel = 0.0f, rightLevel = 0.0f;

    float autoGainDb = 0.0f;
    float autoGainSmooth = 1.0f;

    void setLevels(float l, float r) { leftLevel = l; rightLevel = r; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchFollowEQAudioProcessor)
};
