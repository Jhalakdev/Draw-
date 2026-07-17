#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include "AnalogCharacter.h"

class GainEnvelope;

class Equalizer
{
public:
    Equalizer();
    ~Equalizer();

    enum Character
    {
        CharOff = 0,
        CharFormPassive = 1,
        CharCraneSong = 2,
        CharValveTube = 3,
        CharPulsetEQ = 4,
        CharConsole88 = 5,
        CharGBus = 6
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

    AnalogCharacter& getCharacterProcessor() { return charProcessor; }
    const AnalogCharacter& getCharacterProcessor() const { return charProcessor; }

    enum PhaseMode { Minimum = 0, Linear = 1, Natural = 2 };
    void setPhaseMode(int mode) { if (currentPhaseMode != mode) { currentPhaseMode = mode; markDirty(); } }
    int getPhaseMode() const { return currentPhaseMode; }

    void setCharacter(int c) { charProcessor.setType((AnalogCharacter::Type)c); }
    void setCharBlend(float b) { charProcessor.setDrive(b); }
    int getCharacter() const { return (int)charProcessor.getType(); }

    float getFrequencyResponse(float freq) const;
    float getCompoundResponse(float freq) const;

private:
    bool enabled = false;
    GainEnvelope* envelope = nullptr;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool dirty = true;
    int currentPhaseMode = 0;

    float charBlend = 0.35f;

    AnalogCharacter charProcessor;

    bool soloEnabled = false;
    float soloFreq = 1000.0f;

    static constexpr int IrLen = 256;
    static constexpr int FftOrder = 10;
    static constexpr int FftSize = 1 << FftOrder;
    static constexpr int CrossfadeLen = 128;

    std::vector<float> ir;
    std::vector<float> oldIr;
    std::vector<std::vector<float>> delayLine;
    std::vector<std::vector<float>> charScratch;
    int delayIndex = 0;
    int crossfadeRemaining = 0;

    void rebuild();
    void designMinimumPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate);
    void designLinearPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate);
    void designNaturalPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate);
};
