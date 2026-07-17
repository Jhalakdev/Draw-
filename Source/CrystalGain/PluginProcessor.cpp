#include "PluginProcessor.h"
#include "PluginEditor.h"

CrystalGainProcessor::CrystalGainProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("CrystalGain"), createParameterLayout())
{}

CrystalGainProcessor::~CrystalGainProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout CrystalGainProcessor::createParameterLayout()
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
    return layout;
}

void CrystalGainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    gainProc.prepare(sampleRate, samplesPerBlock);
    prepared = true;
}

void CrystalGainProcessor::releaseResources() {}

void CrystalGainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    if (!prepared) prepareToPlay(getSampleRate(), numSamples);

    float gainDb = apvts.getRawParameterValue("gain")->load();
    currentGainDb = gainDb;
    bool phase = apvts.getRawParameterValue("phase")->load() > 0.5f;
    bool mono = apvts.getRawParameterValue("mono")->load() > 0.5f;
    float balance = apvts.getRawParameterValue("balance")->load();

    monoProc.process(buffer, mono);
    balanceProc.process(buffer, balance);
    gainProc.setGainDb(gainDb);
    gainProc.process(buffer);
    phaseProc.process(buffer, phase, phase);

    if (buffer.getNumChannels() > 0)
    {
        float peakL = buffer.getRMSLevel(0, 0, numSamples);
        if (buffer.getNumChannels() > 1)
        {
            float peakR = buffer.getRMSLevel(1, 0, numSamples);
            setLevels(peakL, peakR);
        }
        else setLevels(peakL, peakL);
    }
}

juce::AudioProcessorEditor* CrystalGainProcessor::createEditor()
{
    return new CrystalGainEditor(*this);
}

void CrystalGainProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void CrystalGainProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto stream = juce::MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false);
    auto state = juce::ValueTree::readFromStream(stream);
    if (state.isValid()) apvts.replaceState(state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrystalGainProcessor();
}
