#include "Equalizer.h"
#include "GainEnvelope.h"
#include <cmath>
#include <algorithm>
#include <complex>

Equalizer::Equalizer()
    : ir(IrLen, 0.0f), oldIr(IrLen, 0.0f)
{
    ir[0] = 1.0f;
    oldIr[0] = 1.0f;
}

Equalizer::~Equalizer() {}

void Equalizer::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    charProcessor.prepare(sampleRate, samplesPerBlock);
    int maxChannels = 2;
    delayLine.resize(maxChannels);
    for (auto& dl : delayLine)
        dl.resize(IrLen, 0.0f);
    delayIndex = 0;
    charScratch.resize(maxChannels);
    for (auto& ch : charScratch)
        ch.resize(samplesPerBlock, 0.0f);
}

void Equalizer::process(juce::dsp::AudioBlock<float>& block)
{
    if (!enabled || envelope == nullptr) return;

    int numChannels = static_cast<int>(block.getNumChannels());
    int numSamples = static_cast<int>(block.getNumSamples());

    if (static_cast<size_t>(numChannels) > delayLine.size())
    {
        delayLine.resize(numChannels);
        for (auto& dl : delayLine)
            dl.resize(IrLen, 0.0f);
    }

    if (crossfadeRemaining > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer(static_cast<size_t>(ch));
            auto& dl = delayLine[ch];

            for (int s = 0; s < numSamples; ++s)
            {
                dl[delayIndex] = data[s];

                float newOut = 0.0f, oldOut = 0.0f;
                int idx = delayIndex;
                for (int i = 0; i < IrLen; ++i)
                {
                    newOut += ir[i] * dl[idx];
                    oldOut += oldIr[i] * dl[idx];
                    if (--idx < 0) idx = IrLen - 1;
                }

                int cfPos = crossfadeRemaining + s;
                float out;
                if (cfPos < CrossfadeLen)
                {
                    float t = static_cast<float>(cfPos) / static_cast<float>(CrossfadeLen);
                    out = oldOut * (1.0f - t) + newOut * t;
                }
                else
                    out = newOut;

                data[s] = out;
                delayIndex = (delayIndex + 1) % IrLen;
            }
        }
        crossfadeRemaining -= numSamples;
        if (crossfadeRemaining < 0) crossfadeRemaining = 0;
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer(static_cast<size_t>(ch));
            auto& dl = delayLine[ch];

            for (int s = 0; s < numSamples; ++s)
            {
                dl[delayIndex] = data[s];

                float out = 0.0f;
                int idx = delayIndex;
                for (int i = 0; i < IrLen; ++i)
                {
                    out += ir[i] * dl[idx];
                    if (--idx < 0) idx = IrLen - 1;
                }

                data[s] = out;
                delayIndex = (delayIndex + 1) % IrLen;
            }
        }
    }

    if (charProcessor.getType() != AnalogCharacter::Off)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = block.getChannelPointer(static_cast<size_t>(ch));
            charProcessor.process(data, numSamples, ch);
        }
    }
}

void Equalizer::flushDirty()
{
    if (dirty)
    {
        rebuild();
        dirty = false;
    }
}

void Equalizer::rebuild()
{
    if (envelope == nullptr) return;

    int numBins = FftSize / 2 + 1;
    std::vector<float> mag(numBins);

    for (int k = 0; k < numBins; ++k)
    {
        float freq = static_cast<float>(k) * static_cast<float>(currentSampleRate) / static_cast<float>(FftSize);

        if (soloEnabled)
        {
            float ratio = freq / soloFreq;
            float ri = 1.0f / ratio;
            float bandpassSq = 1.0f / ((ratio - ri) * (ratio - ri) * 4.0f + 1.0f);
            float gainDB = 10.0f * std::log10(std::max(1e-8f, bandpassSq));
            gainDB = juce::jlimit(-60.0f, 0.0f, gainDB);
            mag[k] = gainDB;
        }
        else
        {
            float gainDB = envelope->getGainAt(freq);
            mag[k] = gainDB;
        }
    }

    oldIr = ir;

    std::vector<float> newIr(IrLen, 0.0f);
    if (currentPhaseMode == Linear)
        designLinearPhaseFIR(mag.data(), numBins, newIr.data(), IrLen, currentSampleRate);
    else if (currentPhaseMode == Natural)
        designNaturalPhaseFIR(mag.data(), numBins, newIr.data(), IrLen, currentSampleRate);
    else
        designMinimumPhaseFIR(mag.data(), numBins, newIr.data(), IrLen, currentSampleRate);

    ir = newIr;
    crossfadeRemaining = CrossfadeLen;
}

void Equalizer::designMinimumPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate)
{
    juce::dsp::FFT fft(FftOrder);
    std::vector<std::complex<float>> buf(FftSize);

    for (int i = 0; i < FftSize; ++i)
    {
        int k = (i <= FftSize / 2) ? i : FftSize - i;
        float linearMag = std::pow(10.0f, magnitude[k] / 20.0f);
        buf[i] = std::log(std::max(1e-8f, linearMag));
    }

    fft.perform(buf.data(), buf.data(), true);

    for (int i = 0; i < FftSize; ++i)
    {
        float w;
        if (i == 0 || i == FftSize / 2)
            w = 1.0f;
        else if (i < FftSize / 2)
            w = 2.0f;
        else
            w = 0.0f;
        buf[i] *= w;
    }

    fft.perform(buf.data(), buf.data(), false);

    for (int i = 0; i < FftSize; ++i)
    {
        float re = buf[i].real();
        float im = buf[i].imag();
        float expRe = std::exp(re);
        buf[i] = std::complex<float>(expRe * std::cos(im), expRe * std::sin(im));
    }

    fft.perform(buf.data(), buf.data(), true);

    for (int i = 0; i < outputLen; ++i)
        output[i] = buf[i].real();
}

void Equalizer::designLinearPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate)
{
    juce::ignoreUnused(sampleRate);
    juce::dsp::FFT fft(FftOrder);
    std::vector<std::complex<float>> buf(FftSize);

    for (int i = 0; i < FftSize; ++i)
    {
        int k = (i <= FftSize / 2) ? i : FftSize - i;
        float linearMag = std::pow(10.0f, magnitude[k] / 20.0f);
        buf[i] = std::complex<float>(linearMag, 0.0f);
    }

    fft.perform(buf.data(), buf.data(), false);

    float scale = 1.0f / FftSize;
    int halfLen = outputLen / 2;

    for (int i = 0; i < outputLen; ++i)
    {
        int srcIdx;
        if (i <= halfLen)
            srcIdx = halfLen - i;
        else
            srcIdx = FftSize - (i - halfLen);
        srcIdx = juce::jlimit(0, FftSize - 1, srcIdx);
        output[i] = buf[srcIdx].real() * scale;

        float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / (outputLen - 1)));
        output[i] *= window;
    }
}

void Equalizer::designNaturalPhaseFIR(float* magnitude, int numBins, float* output, int outputLen, double sampleRate)
{
    juce::ignoreUnused(sampleRate);
    juce::dsp::FFT fft(FftOrder);
    std::vector<std::complex<float>> buf(FftSize);

    for (int i = 0; i < FftSize; ++i)
    {
        int k = (i <= FftSize / 2) ? i : FftSize - i;
        float linearMag = std::pow(10.0f, magnitude[k] / 20.0f);
        buf[i] = std::log(std::max(1e-8f, linearMag));
    }

    fft.perform(buf.data(), buf.data(), true);

    for (int i = 0; i < FftSize; ++i)
    {
        float w;
        if (i == 0 || i == FftSize / 2)
            w = 1.0f;
        else if (i < FftSize / 2)
            w = 1.4f;
        else
            w = 0.0f;
        buf[i] *= w;
    }

    fft.perform(buf.data(), buf.data(), false);

    for (int i = 0; i < FftSize; ++i)
    {
        float re = buf[i].real();
        float im = buf[i].imag();
        float expRe = std::exp(re);
        buf[i] = std::complex<float>(expRe * std::cos(im), expRe * std::sin(im));
    }

    fft.perform(buf.data(), buf.data(), true);

    for (int i = 0; i < outputLen; ++i)
        output[i] = buf[i].real();
}

float Equalizer::getFrequencyResponse(float freq) const
{
    if (envelope == nullptr) return 0.0f;
    return envelope->getGainAt(freq);
}

float Equalizer::getCompoundResponse(float freq) const
{
    float w = juce::MathConstants<float>::twoPi * freq / static_cast<float>(currentSampleRate);

    float sumRe = 0.0f;
    float sumIm = 0.0f;

    for (int n = 0; n < IrLen; ++n)
    {
        float angle = w * static_cast<float>(n);
        sumRe += ir[n] * std::cos(angle);
        sumIm -= ir[n] * std::sin(angle);
    }

    float magSq = sumRe * sumRe + sumIm * sumIm;
    return 10.0f * std::log10(std::max(1e-10f, magSq));
}
