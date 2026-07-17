#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "WDFCircuits.h"

class AnalogCharacter
{
public:
    enum Type
    {
        Off = 0,
        FormPassive,
        CraneSong,
        ValveTube,
        PulsetEQ,
        Console88,
        GBus
    };

    AnalogCharacter();
    ~AnalogCharacter() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(float* data, int numSamples, int channel = 0);
    void setType(Type t);
    Type getType() const { return currentType; }
    void setDrive(float d) { drive = juce::jlimit(0.0f, 1.0f, d); }
    float getFrequencyResponse(float freq) const;

private:
    Type currentType = Off;
    double sampleRate = 44100.0;
    float drive = 0.35f;

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        void setLowShelf(float freq, float gainDB, float q, float sr);
        void setHighShelf(float freq, float gainDB, float q, float sr);
        void setPeak(float freq, float gainDB, float q, float sr);
        void setHighCut(float freq, float q, float sr);
        void setLowCut(float freq, float q, float sr);
        void setPassthrough();
        float process(float x) {
            float y = b0 * x + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
            z2 = z1; z1 = y; return y; }
        void reset() { z1 = z2 = 0.0f; }
    };

    static constexpr int NumFilters = 8;
    Biquad filters[NumFilters];
    int filterCount = 0;

    // ===== CIRCUIT-LEVEL SATURATION STAGE =====
    struct CircuitStage
    {
        float envelope = 0.0f;
        float attackCoeff = 0.0f, releaseCoeff = 0.0f;
        float slewState = 0.0f;
        int satType = 0;
        float baseDrive = 0.0f;

        // Grid current rectification (tube)
        float gridCharge = 0.0f;
        float gridLeak = 0.999f;

        // Transformer hysteresis
        float coreFlux = 0.0f;
        float prevIn = 0.0f;

        // Dielectric absorption
        float capMemLow = 0.0f;
        float capMemMid = 0.0f;

        // Inter-stage loading (simulates passive filter impedance)
        float loading = 0.0f;

        void setTimes(float atkMs, float relMs, float sr);
        float process(float x, int type, float drive, float psmGain, float loadFromPrev);
        void reset();
    };

    CircuitStage inputStage;
    CircuitStage lowStage;
    CircuitStage midStage;
    CircuitStage highStage;
    CircuitStage outputStage;

    // Inter-stage loading coefficients
    float loadingAmount = 0.0f;

    // WDF engines (one per channel for stereo mismatch)
    std::unique_ptr<WDFCharacterEngine> wdfEngine;
    std::unique_ptr<WDFCharacterEngine> wdfEngine2;
    float wdfTolerance = 10.0f;
    bool useWDF = false;
    float wdfMakeupGain = 10.0f;
    float dcBlockerZ[2] = { 0.0f, 0.0f };
    float dcPrevX[2] = { 0.0f, 0.0f };

    void loadPreset();
    void resetState();

    float saturate(int type, float x, float drive) const;
    float saturateTube(float x, float drive) const;
    float saturateClean(float x, float drive) const;
    float saturateTubeAggressive(float x, float drive) const;
    float saturateConsole(float x, float drive) const;
    float saturateTransformer(float x, float drive) const;
    float saturateVCA(float x, float drive) const;
    float saturateOpamp(float x, float drive) const;
};
