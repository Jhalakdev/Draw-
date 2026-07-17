#pragma once
#include <JuceHeader.h>

class MonoProcessor
{
public:
    void process(juce::AudioBuffer<float>& audio, bool mono)
    {
        if (!mono || audio.getNumChannels() < 2)
            return;

        auto numSamples = audio.getNumSamples();
        for (int s = 0; s < numSamples; ++s)
        {
            float sum = (audio.getSample(0, s) + audio.getSample(1, s)) * 0.5f;
            audio.setSample(0, s, sum);
            audio.setSample(1, s, sum);
        }
    }
};
