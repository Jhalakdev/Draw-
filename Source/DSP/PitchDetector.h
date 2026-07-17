#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class PitchDetector
{
public:
    PitchDetector();
    ~PitchDetector();

    void prepare(double sampleRate, int fftOrder = 12);
    float detect(const float* input, int numSamples);

    float getFrequency() const { return currentFrequency; }
    float getConfidence() const { return currentConfidence; }
    bool isActive() const { return active; }
    void setActive(bool shouldBeActive) { active = shouldBeActive; }

    static constexpr float MinFrequency = 50.0f;
    static constexpr float MaxFrequency = 2000.0f;

private:
    double sampleRate = 44100.0;
    float currentFrequency = 0.0f;
    float currentConfidence = 0.0f;
    bool active = true;

    std::vector<float> correlationBuffer;
    std::vector<float> fftBuffer;
    std::unique_ptr<juce::dsp::FFT> fft;

    float autocorrelate(const float* input, int numSamples);
    float estimateViaFFT(const float* input, int numSamples);
    float parabolicInterpolation(const float* corr, int index, int size);
};
