#include "PluginProcessor.h"
#include "PluginEditor.h"

CrystalGainProProcessor::CrystalGainProProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("CrystalGainPro"), createParameterLayout())
{}

CrystalGainProProcessor::~CrystalGainProProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout CrystalGainProProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("gain", 1), "Gain",
        juce::NormalisableRange<float>(-36.0f, 36.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("phase", 1), "Phase Invert", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("mono", 1), "Mono", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("balance", 1), "Balance",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("character", 1), "Character",
        juce::StringArray{"Off", "Mister Passive", "Krane Mybiz", "West Nugget",
                          "Pool Dake", "Never 80-8", "Liquid State Solid"}, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("color", 1), "Color",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f));
    return layout;
}

void CrystalGainProProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    gainProc.prepare(sampleRate, samplesPerBlock);
    character.prepare(sampleRate, samplesPerBlock);
    prepared = true;
}

void CrystalGainProProcessor::releaseResources() {}

void CrystalGainProProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    if (!prepared) prepareToPlay(getSampleRate(), numSamples);

    float gainDb = apvts.getRawParameterValue("gain")->load();
    currentGainDb = gainDb;
    bool phase = apvts.getRawParameterValue("phase")->load() > 0.5f;
    bool mono = apvts.getRawParameterValue("mono")->load() > 0.5f;
    float balance = apvts.getRawParameterValue("balance")->load();
    int charType = apvts.getRawParameterValue("character")->load();
    float color = apvts.getRawParameterValue("color")->load() / 100.0f;

    monoProc.process(buffer, mono);
    balanceProc.process(buffer, balance);
    gainProc.setGainDb(gainDb);
    gainProc.process(buffer);

    // AnalogCharacter processing
    character.setType((AnalogCharacter::Type)charType);
    character.setDrive(color);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        character.process(data, numSamples, ch);
    }

    phaseProc.process(buffer, phase, phase);

    if (numChannels > 0)
    {
        float peakL = buffer.getRMSLevel(0, 0, numSamples);
        if (numChannels > 1)
        {
            float peakR = buffer.getRMSLevel(1, 0, numSamples);
            setLevels(peakL, peakR);
        }
        else setLevels(peakL, peakL);
    }
}

juce::AudioProcessorEditor* CrystalGainProProcessor::createEditor()
{
    return new CrystalGainProEditor(*this);
}

void CrystalGainProProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void CrystalGainProProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto stream = juce::MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false);
    auto state = juce::ValueTree::readFromStream(stream);
    if (state.isValid()) apvts.replaceState(state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrystalGainProProcessor();
}
