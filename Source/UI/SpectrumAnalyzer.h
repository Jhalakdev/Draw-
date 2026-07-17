#pragma once
#include <JuceHeader.h>

class SpectrumAnalyzer : public juce::Component, public juce::Timer
{
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void pushSamples(const float* data, int numSamples);

    struct SpectrumData
    {
        std::array<float, 512> magnitude{};
        float peakFreq = 0.0f;
        float peakMagnitude = 0.0f;
    };

    SpectrumData getCurrentSpectrum() const { return currentSpectrum; }

private:
    static constexpr int FFTOrder = 11;
    static constexpr int FFTSize = 1 << FFTOrder;
    std::unique_ptr<juce::dsp::FFT> fft;

    std::vector<float> fftInput;
    std::vector<float> fftOutput;
    int writePosition = 0;
    bool readyToProcess = false;

    SpectrumData currentSpectrum;
    std::array<float, 512> displaySpectrum{};

    void processFFT();
};
