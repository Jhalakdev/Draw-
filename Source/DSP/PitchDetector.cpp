#include "PitchDetector.h"

PitchDetector::PitchDetector() {}

PitchDetector::~PitchDetector() {}

void PitchDetector::prepare(double sr, int fftOrder)
{
    sampleRate = sr;
    int fftSize = 1 << fftOrder;
    correlationBuffer.resize(static_cast<size_t>(fftSize * 2), 0.0f);
    fftBuffer.resize(static_cast<size_t>(fftSize * 2), 0.0f);
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
}

float PitchDetector::detect(const float* input, int numSamples)
{
    if (!active || input == nullptr || numSamples <= 0)
        return 0.0f;

    float freq = autocorrelate(input, numSamples);

    if (freq >= MinFrequency && freq <= MaxFrequency)
    {
        currentFrequency = 0.8f * currentFrequency + 0.2f * freq;
        currentConfidence = juce::jlimit(0.0f, 1.0f, currentConfidence + 0.05f);
    }
    else
    {
        currentConfidence = juce::jmax(0.0f, currentConfidence - 0.02f);
    }

    return currentFrequency;
}

float PitchDetector::autocorrelate(const float* input, int numSamples)
{
    int minLag = static_cast<int>(sampleRate / MaxFrequency);
    int maxLag = static_cast<int>(sampleRate / MinFrequency);
    int bufSize = static_cast<int>(correlationBuffer.size());

    if (maxLag >= bufSize)
        maxLag = bufSize - 1;

    float bestCorr = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        float energy1 = 0.0f;
        float energy2 = 0.0f;
        int count = numSamples - lag;

        if (count <= 0) continue;

        for (int i = 0; i < count; ++i)
        {
            corr += input[i] * input[i + lag];
            energy1 += input[i] * input[i];
            energy2 += input[i + lag] * input[i + lag];
        }

        float norm = std::sqrt(energy1 * energy2);
        if (norm > 0.0f)
            corr /= norm;

        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestLag = lag;
        }
    }

    if (bestLag > 1 && bestCorr > 0.3f)
    {
        float refined = parabolicInterpolation(nullptr, bestLag, 0);
        if (refined > 0)
            return static_cast<float>(sampleRate) / refined;
    }

    return 0.0f;
}

float PitchDetector::estimateViaFFT(const float* input, int numSamples)
{
    if (!fft) return 0.0f;

    int fftSize = fft->getSize();
    std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);

    int samplesToUse = juce::jmin(numSamples, fftSize);
    for (int i = 0; i < samplesToUse; ++i)
        fftBuffer[static_cast<size_t>(i)] = input[i];

    fft->performRealOnlyForwardTransform(fftBuffer.data());

    int numBins = fftSize / 2;
    float maxMag = 0.0f;
    int maxBin = 0;
    for (int i = 2; i < numBins; ++i)
    {
        float re = fftBuffer[static_cast<size_t>(2 * i)];
        float im = fftBuffer[static_cast<size_t>(2 * i + 1)];
        float mag = std::sqrt(re * re + im * im);
        if (mag > maxMag)
        {
            maxMag = mag;
            maxBin = i;
        }
    }

    if (maxBin > 0)
        return static_cast<float>(maxBin) * static_cast<float>(sampleRate) / static_cast<float>(fftSize);

    return 0.0f;
}

float PitchDetector::parabolicInterpolation(const float* /*corr*/, int index, int /*size*/)
{
    if (index <= 0) return static_cast<float>(index);

    float alpha = correlationBuffer[static_cast<size_t>(index - 1)];
    float beta = correlationBuffer[static_cast<size_t>(index)];
    float gamma = correlationBuffer[static_cast<size_t>(index + 1)];

    float denom = 2.0f * (2.0f * beta - alpha - gamma);
    if (std::abs(denom) < 1e-10f)
        return static_cast<float>(index);

    return static_cast<float>(index) + (alpha - gamma) / denom;
}
