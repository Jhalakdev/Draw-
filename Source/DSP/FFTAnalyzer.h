#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class FFTAnalyzer
{
public:
    FFTAnalyzer();
    ~FFTAnalyzer();

    void prepare(double sampleRate);
    void pushSamples(const float* data, int numSamples);

    struct SpectrumData
    {
        std::array<float, 256> magnitude{};
    };

    const SpectrumData& getSpectrum() const { return currentSpectrum; }
    bool hasNewData() const { return newDataReady; }
    void clearNewDataFlag() { newDataReady = false; }

private:
    void processFFT();

    static constexpr int FFTOrder = 10;
    static constexpr int FFTSize = 1 << FFTOrder;
    static constexpr int NumBins = FFTSize / 2;
    static constexpr int NumDisplayBins = 256;
    static constexpr float MinFreq = 20.0f;
    static constexpr float MaxFreq = 20000.0f;

    double sampleRate = 44100.0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fftInput;
    std::vector<float> fftWindow;
    std::vector<float> scratch;
    int writePosition = 0;
    bool readyToProcess = false;
    bool newDataReady = false;

    SpectrumData currentSpectrum;
    std::array<float, NumDisplayBins> displaySpectrum{};
    std::array<int, NumDisplayBins> binStart{};
    std::array<int, NumDisplayBins> binEnd{};
    bool binsInitialized = false;

    void initBinMapping();
};
