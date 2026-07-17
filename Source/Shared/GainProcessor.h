#pragma once
#include <JuceHeader.h>

class GainProcessor
{
public:
    void prepare(double sampleRate, int samplesPerBlock)
    {
        sr = sampleRate;
        buffer.setSize(2, samplesPerBlock, false, true, false);
        smoothGain = 1.0f;
    }

    void setGainDb(float db)
    {
        targetGain = juce::Decibels::decibelsToGain(juce::jlimit(-36.0f, 36.0f, db));
    }

    void process(juce::AudioBuffer<float>& audio)
    {
        auto numSamples = audio.getNumSamples();
        float coeff = 1.0f - std::exp(-1.0f / (0.005f * (float)sr));
        for (int s = 0; s < numSamples; ++s)
        {
            float diff = targetGain - smoothGain;
            smoothGain += coeff * diff;
            if (std::abs(smoothGain - 1.0f) > 0.0001f)
            {
                for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                {
                    auto* d = audio.getWritePointer(ch);
                    d[s] *= smoothGain;
                }
            }
        }
    }

    void reset() { smoothGain = 1.0f; }

private:
    double sr = 44100.0;
    float smoothGain = 1.0f;
    float targetGain = 1.0f;
    juce::AudioBuffer<float> buffer;
};
