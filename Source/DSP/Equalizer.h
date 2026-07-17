#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class GainEnvelope;

class Equalizer
{
public:
    Equalizer();
    ~Equalizer();

    struct Zone
    {
        float startFreq = 200.0f;
        float endFreq = 2000.0f;
        bool enabled = false;
        float threshold = -24.0f;
        float ratio = 3.0f;
        float attackMs = 10.0f;
        float releaseMs = 100.0f;
        float range = 12.0f;
    };

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::dsp::AudioBlock<float>& block);

    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }

    void setEnvelope(GainEnvelope* e) { envelope = e; }
    void markDirty() { dirty = true; }
    void flushDirty();

    void setSolo(float freq) { soloFreq = freq; soloEnabled = true; markDirty(); }
    void clearSolo() { soloEnabled = false; soloFreq = -1.0f; markDirty(); }
    bool isSoloActive() const { return soloEnabled; }
    float getSoloFrequency() const { return soloFreq; }

    enum PhaseMode { Minimum = 0, Linear = 1, Natural = 2 };
    void setPhaseMode(int mode) { if (currentPhaseMode != mode) { currentPhaseMode = mode; markDirty(); } }
    int getPhaseMode() const { return currentPhaseMode; }

    float getFrequencyResponse(float freq) const;
    float getCompoundResponse(float freq) const;
    float getCompressedGain(float freq) const;

    int addZone(float freq);
    void removeZone(int idx);
    const std::vector<Zone>& getZones() const { return zones; }
    Zone& getZone(int idx) { return zones[idx]; }
    int getNumZones() const { return (int)zones.size(); }
    void rebuildBands();

private:
    bool enabled = false;
    GainEnvelope* envelope = nullptr;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool dirty = true;
    int currentPhaseMode = 0;

    bool soloEnabled = false;
    float soloFreq = 1000.0f;

    std::vector<Zone> zones;
    bool zonesDirty = true;

    static constexpr int IrLen = 512;
    static constexpr int FftOrder = 11;
    static constexpr int FftSize = 1 << FftOrder;
    static constexpr int CrossfadeLen = 256;

    std::vector<float> ir;
    std::vector<float> oldIr;
    std::vector<std::vector<float>> delayLine;
    int delayIndex = 0;
    int crossfadeRemaining = 0;

    struct DynBand
    {
        float inLp = 0.0f, inBp = 0.0f, inHp = 0.0f;
        float outLp = 0.0f, outBp = 0.0f, outHp = 0.0f;
        float g = 0.0f, R = 0.0f;
        float envelope = 0.0f;
        float gainReduction = 1.0f;
        float attackCoeff = 0.0f, releaseCoeff = 0.0f;

        void setParams(float sr, float cf, float q, float atkMs, float relMs)
        {
            g = std::tan(juce::MathConstants<float>::pi * cf / sr);
            R = 1.0f / (2.0f * q);
            attackCoeff = 1.0f - std::exp(-1.0f / (atkMs * sr / 1000.0f));
            releaseCoeff = 1.0f - std::exp(-1.0f / (relMs * sr / 1000.0f));
        }

        float svf(float input, float& lp, float& bp, float& hp)
        {
            hp = (input - lp * (1.0f + 2.0f * R * g) - bp * (2.0f * R + g * g * R))
               / (1.0f + 2.0f * R * g + g * g * R);
            bp = g * hp + bp;
            lp = g * bp + lp;
            return bp;
        }

        float processAnalysis(float input)
        {
            float bp = svf(input, inLp, inBp, inHp);
            float det = std::abs(bp);
            float diff = det - envelope;
            if (diff > 0.0f) envelope += attackCoeff * diff;
            else envelope += releaseCoeff * diff;
            return bp;
        }

        float processModulation(float eqInput)
        {
            return svf(eqInput, outLp, outBp, outHp);
        }

        void reset()
        {
            inLp = inBp = inHp = 0.0f;
            outLp = outBp = outHp = 0.0f;
            envelope = 0.0f;
            gainReduction = 1.0f;
        }
    };

    std::vector<DynBand> bands;

    void rebuild();
    void designMinimumPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate);
    void designLinearPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate);
    void designNaturalPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate);
};
