#pragma once
#include <JuceHeader.h>
#include <cmath>

class BandCompressor
{
public:
    BandCompressor() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;
        this->samplesPerBlock = samplesPerBlock;
        updateFilters();
    }

    void setRange(float minFreq, float maxFreq)
    {
        if (std::abs(minFreq - this->minFreq) < 1.0f &&
            std::abs(maxFreq - this->maxFreq) < 1.0f)
            return;
        this->minFreq = minFreq;
        this->maxFreq = maxFreq;
        updateFilters();
    }

    void setThreshold(float dB) { threshold = dB; }
    void setRatio(float r) { ratio = juce::jmax(1.0f, r); }
    void setAttack(float ms) { attackMs = ms; }
    void setRelease(float ms) { releaseMs = ms; }
    void setKnee(float dB) { knee = dB; }
    void setMakeupGain(float dB) { makeupGain = dB; }
    void setClip(bool shouldClip) { clipping = shouldClip; }
    void setClipLevel(float dB) { clipLevel = dB; }
    void setEnvelopeGain(float gain) { currentEnvelopeGain = gain; }

    float getThreshold() const { return threshold; }
    float getRatio() const { return ratio; }
    float getAttack() const { return attackMs; }
    float getRelease() const { return releaseMs; }
    float getKnee() const { return knee; }
    float getMakeupGain() const { return makeupGain; }
    bool isClipping() const { return clipping; }
    float getReduction() const { return currentReduction; }

    void process(float* channelData, int numSamples)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            float band = bandpassFilter.process(input);
            float complement = input - band;
            float compressed = compressSample(band);
            channelData[sample] = complement + compressed;
        }
    }

private:
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    float minFreq = 100.0f;
    float maxFreq = 250.0f;

    float threshold = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float knee = 6.0f;
    float makeupGain = 0.0f;
    bool clipping = false;
    float clipLevel = -0.5f;

    float envelope = 0.0f;
    float currentReduction = 0.0f;
    float currentEnvelopeGain = 0.0f;

    struct BiquadFilter
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

        void setBandpass(float freq, float Q, double sr)
        {
            float w0 = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sr);
            float alpha = std::sin(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;

            b0 = alpha / a0;
            b1 = 0.0f;
            b2 = -alpha / a0;
            a1 = -2.0f * std::cos(w0) / a0;
            a2 = (1.0f - alpha) / a0;
        }
    };

    BiquadFilter bandpassFilter;

    void updateFilters()
    {
        if (sampleRate <= 0) return;
        float center = std::sqrt(minFreq * maxFreq);
        float q = center / juce::jmax(1.0f, maxFreq - minFreq);
        q = juce::jlimit(0.1f, 10.0f, q);
        bandpassFilter.setBandpass(center, q, sampleRate);
    }

    float compressSample(float input)
    {
        float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(sampleRate)));
        float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

        float inputDB = 20.0f * std::log10(std::max(1e-10f, std::abs(input)));
        float targetGain = 0.0f;

        float effectiveThreshold = threshold - currentEnvelopeGain;

        if (inputDB > effectiveThreshold + knee / 2.0f)
        {
            targetGain = effectiveThreshold + (inputDB - effectiveThreshold) / ratio - inputDB;
        }
        else if (inputDB > effectiveThreshold - knee / 2.0f)
        {
            float x = inputDB - effectiveThreshold + knee / 2.0f;
            targetGain = (1.0f / ratio - 1.0f) * (x * x) / (2.0f * knee);
        }

        float coeff = (targetGain < envelope) ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * targetGain;

        float gainDB = envelope + makeupGain;

        if (clipping)
        {
            float outputDB = inputDB + gainDB;
            if (outputDB > clipLevel)
                gainDB += clipLevel - outputDB;
        }

        currentReduction = envelope;

        float gainLinear = std::pow(10.0f, gainDB / 20.0f);
        return input * gainLinear;
    }
};
