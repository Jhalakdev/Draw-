#pragma once
#include <JuceHeader.h>

class PhaseProcessor
{
public:
    void process(juce::AudioBuffer<float>& audio, bool invertLeft, bool invertRight)
    {
        auto numSamples = audio.getNumSamples();
        if (invertLeft && audio.getNumChannels() > 0)
        {
            auto* l = audio.getWritePointer(0);
            for (int s = 0; s < numSamples; ++s)
                l[s] = -l[s];
        }
        if (invertRight && audio.getNumChannels() > 1)
        {
            auto* r = audio.getWritePointer(1);
            for (int s = 0; s < numSamples; ++s)
                r[s] = -r[s];
        }
    }
};
