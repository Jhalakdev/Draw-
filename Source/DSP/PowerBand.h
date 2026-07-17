#pragma once
#include <JuceHeader.h>
#include <cmath>

class PowerBand
{
public:
    PowerBand() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        resetFilters();
    }

    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }

    void setSlopeFreq(float freq)
    {
        freq = juce::jlimit(100.0f, 18000.0f, freq);
        if (std::abs(freq - slopeFreq) < 1.0f) return;
        slopeFreq = freq;
        updateFilters();
    }

    float getSlopeFreq() const { return slopeFreq; }

    void setGain(float dB) { gainDB = juce::jlimit(-24.0f, 24.0f, dB); }
    float getGain() const { return gainDB; }

    void setSaturation(float amount) { saturation = juce::jlimit(0.0f, 1.0f, amount); }
    float getSaturation() const { return saturation; }

    void process(float* channelData, int numSamples)
    {
        if (!enabled) return;

        float gainLinear = std::pow(10.0f, gainDB / 20.0f);
        float drive = 1.0f + saturation * 6.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];

            float filtered = processSlope(input);

            float dry = input - filtered;

            float boosted = filtered * gainLinear * drive;

            float ceiling = std::abs(filtered);
            if (ceiling < 0.001f) ceiling = 0.001f;

            float wet = hardLimit(boosted, ceiling);

            channelData[sample] = dry + wet;
        }
    }

private:
    double sampleRate = 44100.0;
    bool enabled = false;
    float slopeFreq = 3000.0f;
    float gainDB = 0.0f;
    float saturation = 0.5f;

    struct BiquadLP
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        void reset() { x1 = x2 = y1 = y2 = 0; }

        float process(float input)
        {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input;
            y2 = y1; y1 = output;
            return output;
        }

        void setLowpass(float freq, double sr)
        {
            float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sr);
            float cosw = std::cos(w0);
            float sinw = std::sin(w0);
            float Q = 0.7071f;
            float alpha = sinw / (2.0f * Q);

            float a0 = 1.0f + alpha;
            b0 = ((1.0f - cosw) / 2.0f) / a0;
            b1 = (1.0f - cosw) / a0;
            b2 = ((1.0f - cosw) / 2.0f) / a0;
            a1 = (-2.0f * cosw) / a0;
            a2 = (1.0f - alpha) / a0;
        }
    };

    BiquadLP stage1, stage2;

    void resetFilters()
    {
        stage1.reset();
        stage2.reset();
        updateFilters();
    }

    void updateFilters()
    {
        stage1.setLowpass(slopeFreq, sampleRate);
        stage2.setLowpass(slopeFreq, sampleRate);
    }

    float processSlope(float input)
    {
        return stage2.process(stage1.process(input));
    }

    float hardLimit(float input, float ceiling)
    {
        float absIn = std::abs(input);
        if (absIn <= ceiling) return input;

        float sign = (input >= 0.0f) ? 1.0f : -1.0f;

        float ratio = ceiling / absIn;
        float t = ratio;
        float shaped = sign * ceiling * (1.0f - (1.0f - t) * (1.0f - t) * saturation);

        return shaped;
    }
};
