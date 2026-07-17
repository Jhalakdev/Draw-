#pragma once
#include <JuceHeader.h>

class BalanceProcessor
{
public:
    void process(juce::AudioBuffer<float>& audio, float balance)
    {
        if (audio.getNumChannels() < 2 || std::abs(balance) < 0.001f)
            return;

        auto numSamples = audio.getNumSamples();
        float leftGain = 1.0f, rightGain = 1.0f;
        if (balance < 0.0f)
            rightGain = 1.0f + balance;
        else
            leftGain = 1.0f - balance;

        auto* l = audio.getWritePointer(0);
        auto* r = audio.getWritePointer(1);
        for (int s = 0; s < numSamples; ++s)
        {
            l[s] *= leftGain;
            r[s] *= rightGain;
        }
    }
};
