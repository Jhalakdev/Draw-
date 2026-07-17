#include "PluginProcessor.h"
#include "PluginEditor.h"

DrawEQSimpleProcessor::DrawEQSimpleProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("DrawEQSimple"), createParameterLayout())
{
    engine.getEqualizer().setEnvelope(&engine.getEnvelope());
}

DrawEQSimpleProcessor::~DrawEQSimpleProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout DrawEQSimpleProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("tracking", 1), "Spectrum", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass EQ", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sensitivity", 1), "Sensitivity",
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("trackingSpeed", 1), "Tracking Speed",
        juce::NormalisableRange<float>(0.01f, 1.0f, 0.01f), 0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("intensity", 1), "Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("masterGain", 1), "Output Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("phaseMode", 1), "Phase Mode",
        juce::StringArray{"Minimum", "Linear", "Natural"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("msMode", 1), "M/S Mode",
        juce::StringArray{"Stereo", "Mid Only", "Side Only", "Left Only", "Right Only"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("autoGain", 1), "Auto Gain", false));
    return layout;
}

void DrawEQSimpleProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (sampleRate != lastSampleRate || samplesPerBlock != lastSamplesPerBlock || !prepared)
    {
        engine.prepare(sampleRate, samplesPerBlock);
        fftAnalyzer.prepare(sampleRate);
        lastSampleRate = sampleRate;
        lastSamplesPerBlock = samplesPerBlock;
        prepared = true;
    }
}

void DrawEQSimpleProcessor::releaseResources() {}

void DrawEQSimpleProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (!prepared)
        prepareToPlay(getSampleRate() > 0 ? getSampleRate() : 44100.0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    bool tracking = apvts.getRawParameterValue("tracking")->load() > 0.5f;
    bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    int phaseMode = apvts.getRawParameterValue("phaseMode")->load();
    int msMode = apvts.getRawParameterValue("msMode")->load();

    engine.setEnabled(tracking);
    engine.setSensitivity(apvts.getRawParameterValue("sensitivity")->load());
    engine.setTrackingSpeed(apvts.getRawParameterValue("trackingSpeed")->load());
    engine.setIntensity(apvts.getRawParameterValue("intensity")->load());

    if (numChannels > 0)
        fftAnalyzer.pushSamples(buffer.getReadPointer(0), numSamples);

    if (!bypass)
    {
        bool autoGainOn = apvts.getRawParameterValue("autoGain")->load() > 0.5f;
        std::unique_ptr<juce::AudioBuffer<float>> dryBuf;
        if (autoGainOn)
        {
            dryBuf = std::make_unique<juce::AudioBuffer<float>>(numChannels, numSamples);
            for (int ch = 0; ch < numChannels; ++ch)
                dryBuf->copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }

        if (msMode == 1 || msMode == 2)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                float l = buffer.getSample(0, s);
                float r = buffer.getSample(1, s);
                float m = (l + r) * 0.5f;
                float side = (l - r) * 0.5f;
                buffer.setSample(0, s, m);
                buffer.setSample(1, s, side);
            }
        }

        if (tracking)
        {
            engine.resetBlockDetection();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                engine.process(data, numSamples);
            }
        }

        engine.getEqualizer().setPhaseMode(phaseMode);
        engine.getEqualizer().setEnabled(true);
        engine.getEqualizer().flushDirty();

        {
            juce::dsp::AudioBlock<float> block(buffer);
            engine.getEqualizer().process(block);
        }

        if (msMode == 1 || msMode == 2)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                float m = buffer.getSample(0, s);
                float side = buffer.getSample(1, s);
                buffer.setSample(0, s, m + side);
                buffer.setSample(1, s, m - side);
            }
        }

        if (msMode == 3) buffer.clear(1, 0, numSamples);
        else if (msMode == 4) buffer.clear(0, 0, numSamples);

        if (autoGainOn && dryBuf != nullptr)
        {
            float dryRMS = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float chRMS = dryBuf->getRMSLevel(ch, 0, numSamples);
                dryRMS += chRMS * chRMS;
            }
            dryRMS = std::sqrt(dryRMS / numChannels);
            float wetRMS = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float chRMS = buffer.getRMSLevel(ch, 0, numSamples);
                wetRMS += chRMS * chRMS;
            }
            wetRMS = std::sqrt(wetRMS / numChannels);
            if (dryRMS > 1e-6f && wetRMS > 1e-6f)
            {
                float targetGain = juce::jlimit(0.251f, 3.981f, dryRMS / wetRMS);
                float sr = (float)getSampleRate();
                float attack = 1.0f - std::exp(-1.0f / (0.002f * sr));
                float release = 1.0f - std::exp(-1.0f / (0.100f * sr));
                float diff = targetGain - autoGainSmooth;
                autoGainSmooth += ((diff > 0.0f) ? attack : release) * diff;
                float gain = autoGainSmooth;
                if (std::abs(gain - 1.0f) > 0.001f)
                    buffer.applyGain(gain);
                autoGainDb = juce::Decibels::gainToDecibels(gain);
            }
        }
        else
        {
            autoGainSmooth = 1.0f;
            autoGainDb = 0.0f;
        }

        float masterGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("masterGain")->load());
        if (std::abs(masterGain - 1.0f) > 0.001f)
            buffer.applyGain(masterGain);
    }

    if (numChannels > 0)
    {
        float peakL = buffer.getRMSLevel(0, 0, numSamples);
        if (numChannels > 1)
        {
            float peakR = buffer.getRMSLevel(1, 0, numSamples);
            setLevels(peakL, peakR);
        }
        else
            setLevels(peakL, peakL);
    }
}

juce::AudioProcessorEditor* DrawEQSimpleProcessor::createEditor()
{
    return new DrawEQSimpleEditor(*this);
}

void DrawEQSimpleProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.addChild(engine.getEnvelope().serialize(), -1, nullptr);
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void DrawEQSimpleProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto stream = juce::MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false);
    auto state = juce::ValueTree::readFromStream(stream);
    if (state.isValid())
    {
        apvts.replaceState(state);
        auto envTree = state.getChildWithName("Envelope");
        if (envTree.isValid())
        {
            engine.getEnvelope().deserialize(envTree);
            engine.getEnvelope().markAudioDirty();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrawEQSimpleProcessor();
}
