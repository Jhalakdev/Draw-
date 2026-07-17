#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
    : fft(std::make_unique<juce::dsp::FFT>(FFTOrder))
{
    fftInput.resize(static_cast<size_t>(FFTSize * 2), 0.0f);
    fftOutput.resize(static_cast<size_t>(FFTSize * 2), 0.0f);
    startTimerHz(20);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    stopTimer();
}

void SpectrumAnalyzer::pushSamples(const float* data, int numSamples)
{
    if (data == nullptr || numSamples <= 0) return;

    for (int i = 0; i < numSamples; ++i)
    {
        fftInput[static_cast<size_t>(writePosition)] = data[i];
        writePosition = (writePosition + 1) % FFTSize;
    }

    if (writePosition == 0)
        readyToProcess = true;
}

void SpectrumAnalyzer::processFFT()
{
    for (int i = 0; i < FFTSize; ++i)
        fftOutput[static_cast<size_t>(i)] = fftInput[static_cast<size_t>((writePosition + i) % FFTSize)];

    for (int i = 0; i < FFTSize; ++i)
    {
        float window = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * static_cast<float>(i) / static_cast<float>(FFTSize - 1));
        fftOutput[static_cast<size_t>(i)] *= window;
    }

    fft->performRealOnlyForwardTransform(fftOutput.data());

    int numBins = FFTSize / 2;
    float maxMag = 0.0f;
    int maxBin = 0;

    for (int i = 0; i < numBins && i < 512; ++i)
    {
        float re = fftOutput[static_cast<size_t>(2 * i)];
        float im = fftOutput[static_cast<size_t>(2 * i + 1)];
        float mag = std::sqrt(re * re + im * im) / static_cast<float>(FFTSize);
        float db = juce::Decibels::gainToDecibels(mag, -80.0f);
        float normalized = juce::jmap(db, -80.0f, 0.0f, 0.0f, 1.0f);

        currentSpectrum.magnitude[i] = normalized;

        if (mag > maxMag && i > 0)
        {
            maxMag = mag;
            maxBin = i;
        }
    }

    if (maxBin > 0)
        currentSpectrum.peakFreq = static_cast<float>(maxBin) * 44100.0f / static_cast<float>(FFTSize);
    currentSpectrum.peakMagnitude = maxMag;

    for (int i = 0; i < 512; ++i)
        displaySpectrum[i] = displaySpectrum[i] * 0.7f + currentSpectrum.magnitude[i] * 0.3f;

    readyToProcess = false;
}

void SpectrumAnalyzer::timerCallback()
{
    if (readyToProcess)
        processFFT();
    repaint();
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.fillAll(juce::Colour(0xFF0D0D1A));

    int numBins = juce::jmin(512, static_cast<int>(displaySpectrum.size()));
    float barWidth = static_cast<float>(area.getWidth()) / static_cast<float>(numBins);

    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = displaySpectrum[i];
        if (magnitude < 0.01f) continue;

        float height = magnitude * static_cast<float>(area.getHeight());
        float x = static_cast<float>(i) * barWidth;
        float y = static_cast<float>(area.getBottom()) - height;

        float hue = 0.7f - 0.5f * (static_cast<float>(i) / static_cast<float>(numBins));
        g.setColour(juce::Colour::fromHSV(hue, 0.8f, 0.9f, 0.8f));
        g.fillRect(static_cast<int>(x), static_cast<int>(y), juce::jmax(1, static_cast<int>(barWidth) - 1), static_cast<int>(height));
    }
}

void SpectrumAnalyzer::resized() {}
