#include "FFTAnalyzer.h"

FFTAnalyzer::FFTAnalyzer()
    : fft(std::make_unique<juce::dsp::FFT>(FFTOrder))
{
    fftInput.resize(static_cast<size_t>(FFTSize), 0.0f);
    fftWindow.resize(static_cast<size_t>(FFTSize), 0.0f);
    for (int i = 0; i < FFTSize; ++i)
        fftWindow[i] = 0.5f - 0.5f * std::cos(
            juce::MathConstants<float>::twoPi * static_cast<float>(i) /
            static_cast<float>(FFTSize - 1));
    scratch.resize(static_cast<size_t>(FFTSize * 2), 0.0f);
}

FFTAnalyzer::~FFTAnalyzer() {}

void FFTAnalyzer::prepare(double sr)
{
    sampleRate = sr;
    std::fill(fftInput.begin(), fftInput.end(), 0.0f);
    writePosition = 0;
    readyToProcess = false;
    newDataReady = false;
    displaySpectrum.fill(0.0f);
    binsInitialized = false;
}

void FFTAnalyzer::initBinMapping()
{
    for (int i = 0; i < NumDisplayBins; ++i)
    {
        float freq = MinFreq * std::pow(MaxFreq / MinFreq,
            static_cast<float>(i) / static_cast<float>(NumDisplayBins));
        int bin = static_cast<int>((freq / static_cast<float>(sampleRate)) * static_cast<float>(FFTSize));
        int nextBin = static_cast<int>((freq * 1.05f / static_cast<float>(sampleRate)) * static_cast<float>(FFTSize));
        if (nextBin <= bin) nextBin = bin + 1;
        binStart[i] = juce::jlimit(0, NumBins - 1, bin);
        binEnd[i] = juce::jlimit(binStart[i] + 1, NumBins, nextBin);
    }
    binsInitialized = true;
}

void FFTAnalyzer::pushSamples(const float* data, int numSamples)
{
    if (data == nullptr || numSamples <= 0) return;

    if (!binsInitialized)
        initBinMapping();

    int toCopy = numSamples;
    int pos = writePosition;

    if (pos + toCopy <= FFTSize)
    {
        std::copy(data, data + toCopy, fftInput.begin() + pos);
        pos += toCopy;
    }
    else
    {
        int first = FFTSize - pos;
        std::copy(data, data + first, fftInput.begin() + pos);
        std::copy(data + first, data + toCopy, fftInput.begin());
        pos = (pos + toCopy) % FFTSize;
    }

    writePosition = pos;
    if (writePosition == 0)
    {
        readyToProcess = true;
        processFFT();
    }
}

void FFTAnalyzer::processFFT()
{
    int wp = writePosition;
    if (wp == 0)
        std::copy(fftInput.begin(), fftInput.end(), scratch.begin());
    else
    {
        std::copy(fftInput.begin() + wp, fftInput.end(), scratch.begin());
        std::copy(fftInput.begin(), fftInput.begin() + wp, scratch.begin() + (FFTSize - wp));
    }

    for (int i = 0; i < FFTSize; ++i)
        scratch[i] *= fftWindow[i];

    fft->performRealOnlyForwardTransform(scratch.data());

    for (int i = 0; i < NumDisplayBins; ++i)
    {
        float energy = 0.0f;
        int count = 0;
        for (int b = binStart[i]; b < binEnd[i] && b < NumBins; ++b)
        {
            float re = scratch[static_cast<size_t>(2 * b)];
            float im = scratch[static_cast<size_t>(2 * b + 1)];
            energy += re * re + im * im;
            ++count;
        }
        if (count > 0)
            energy = std::sqrt(energy / static_cast<float>(count)) / static_cast<float>(FFTSize);

        float db = juce::Decibels::gainToDecibels(energy, -80.0f);
        float normalized = juce::jmap(db, -80.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        displaySpectrum[i] = displaySpectrum[i] * 0.7f + normalized * 0.3f;
        currentSpectrum.magnitude[i] = displaySpectrum[i];
    }

    readyToProcess = false;
    newDataReady = true;
}
