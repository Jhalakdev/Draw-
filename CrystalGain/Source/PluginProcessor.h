#pragma once
#include <JuceHeader.h>
#include "../../Shared/GainProcessor.h"
#include "../../Shared/PhaseProcessor.h"
#include "../../Shared/MonoProcessor.h"
#include "../../Shared/BalanceProcessor.h"
#include "../../Source/UI/LevelMeterComponent.h"

class CrystalGainProcessor : public juce::AudioProcessor
{
public:
    CrystalGainProcessor();
    ~CrystalGainProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Crystal Gain"; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    float getLeftLevel() const { return leftLevel; }
    float getRightLevel() const { return rightLevel; }
    float getGainDb() const { return currentGainDb; }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    GainProcessor gainProc;
    PhaseProcessor phaseProc;
    MonoProcessor monoProc;
    BalanceProcessor balanceProc;

    double sr = 44100.0;
    float leftLevel = 0.0f, rightLevel = 0.0f, currentGainDb = 0.0f;
    bool prepared = false;
    void setLevels(float l, float r) { leftLevel = l; rightLevel = r; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrystalGainProcessor)
};
