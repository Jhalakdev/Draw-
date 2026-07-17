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
    static constexpr int Decimation = 4;
    static constexpr int MaxDecimated = 128;

    int decimatedCount = numSamples / Decimation;
    if (decimatedCount < 8) return 0.0f;
    if (decimatedCount > MaxDecimated) decimatedCount = MaxDecimated;

    float decimated[MaxDecimated];
    for (int i = 0; i < decimatedCount; ++i)
    {
        float sum = 0.0f;
        for (int j = 0; j < Decimation; ++j)
            sum += input[i * Decimation + j];
        decimated[i] = sum * 0.25f;
    }

    float effectiveSr = static_cast<float>(sampleRate) * (1.0f / static_cast<float>(Decimation));
    int minLag = static_cast<int>(effectiveSr / MaxFrequency);
    int maxLag = static_cast<int>(effectiveSr / MinFrequency);
    int bufSize = static_cast<int>(correlationBuffer.size());

    if (minLag < 1) minLag = 1;
    if (maxLag >= bufSize) maxLag = bufSize - 1;
    if (maxLag >= decimatedCount) maxLag = decimatedCount - 1;

    float bestCorr = 0.0f;
    int bestLag = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        float energy1 = 0.0f;
        float energy2 = 0.0f;
        int count = decimatedCount - lag;

        for (int i = 0; i < count; ++i)
        {
            corr += decimated[i] * decimated[i + lag];
            energy1 += decimated[i] * decimated[i];
            energy2 += decimated[i + lag] * decimated[i + lag];
        }

        float norm = std::sqrt(energy1 * energy2);
        if (norm > 0.0f)
            corr /= norm;

        correlationBuffer[static_cast<size_t>(lag)] = corr;

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
            return effectiveSr / refined;
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
