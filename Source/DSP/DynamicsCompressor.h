#pragma once
#include <JuceHeader.h>
#include <cmath>

class DynamicsCompressor
{
public:
    DynamicsCompressor() = default;

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        envelope = 0.0f;
    }

    void setThreshold(float dB) { threshold = dB; }
    void setRatio(float r) { ratio = juce::jmax(1.0f, r); }
    void setAttack(float ms) { attackCoeff = std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate))); }
    void setRelease(float ms) { releaseCoeff = std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sampleRate))); }
    void setKnee(float dB) { knee = dB; }
    void setMakeupGain(float dB) { makeupGain = dB; }
    void setClip(bool shouldClip) { clipping = shouldClip; }
    void setClipLevel(float dB) { clipLevel = dB; }

    float getThreshold() const { return threshold; }
    float getRatio() const { return ratio; }
    float getAttack() const { return attackMs; }
    float getRelease() const { return releaseMs; }
    float getKnee() const { return knee; }
    float getMakeupGain() const { return makeupGain; }
    bool isClipping() const { return clipping; }
    float getReduction() const { return currentReduction; }

    float processSample(float input)
    {
        float inputDB = 20.0f * std::log10(std::max(1e-10f, std::abs(input)));
        float targetGain = 0.0f;

        if (inputDB > threshold + knee / 2.0f)
        {
            targetGain = threshold + (inputDB - threshold) / ratio - inputDB;
        }
        else if (inputDB > threshold - knee / 2.0f)
        {
            float x = inputDB - threshold + knee / 2.0f;
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

private:
    double sampleRate = 44100.0;
    float threshold = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float knee = 6.0f;
    float makeupGain = 0.0f;
    bool clipping = false;
    float clipLevel = -0.5f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
    float currentReduction = 0.0f;
};
