#include "PluginProcessor.h"
#include "PluginEditor.h"

CatsHairProcessor::CatsHairProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    apvts.addParameterListener("startFreq", this);
    apvts.addParameterListener("slant", this);
    apvts.addParameterListener("push", this);
    apvts.addParameterListener("phaseMode", this);
    apvts.addParameterListener("bypass", this);
}

CatsHairProcessor::~CatsHairProcessor()
{
    apvts.removeParameterListener("startFreq", this);
    apvts.removeParameterListener("slant", this);
    apvts.removeParameterListener("push", this);
    apvts.removeParameterListener("phaseMode", this);
    apvts.removeParameterListener("bypass", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout CatsHairProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("startFreq", "Start Freq",
        juce::NormalisableRange<float>(3000.0f, 8000.0f, 1.0f), 3000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("slant", "Slant",
        juce::NormalisableRange<float>(0.0f, 8.0f, 0.1f), 4.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("push", "Push",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>("phaseMode", "Phase",
        juce::StringArray{"Linear", "Natural"}, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    return layout;
}

void CatsHairProcessor::parameterChanged(const juce::String&, float)
{
    startFreq = apvts.getRawParameterValue("startFreq")->load();
    slant = apvts.getRawParameterValue("slant")->load();
    push = apvts.getRawParameterValue("push")->load();
    phaseMode = (int)apvts.getRawParameterValue("phaseMode")->load();
    bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    updateTarget = true;
}

void CatsHairProcessor::parameterChanged()
{
    startFreq = apvts.getRawParameterValue("startFreq")->load();
    slant = apvts.getRawParameterValue("slant")->load();
    push = apvts.getRawParameterValue("push")->load();
    phaseMode = (int)apvts.getRawParameterValue("phaseMode")->load();
    bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    updateTarget = true;
}

const juce::String CatsHairProcessor::getName() const { return "Cat's Hair"; }
bool CatsHairProcessor::acceptsMidi() const { return false; }
bool CatsHairProcessor::producesMidi() const { return false; }
bool CatsHairProcessor::isMidiEffect() const { return false; }
double CatsHairProcessor::getTailLengthSeconds() const { return 0.0; }
bool CatsHairProcessor::hasEditor() const { return true; }

int CatsHairProcessor::getNumPrograms() { return 1; }
int CatsHairProcessor::getCurrentProgram() { return 0; }
void CatsHairProcessor::setCurrentProgram(int) {}
const juce::String CatsHairProcessor::getProgramName(int) { return {}; }
void CatsHairProcessor::changeProgramName(int, const juce::String&) {}

void CatsHairProcessor::releaseResources() {}

void CatsHairProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;

    inputBuf.assign(fftSize, 0.0f);
    overlapBuf.assign(fftSize, 0.0f);

    window.resize(fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[i] = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * i / (fftSize - 1));

    float winSum = 0.0f;
    for (int i = 0; i < fftSize; i += hopSize)
        for (int j = 0; j < fftSize; ++j)
            winSum += window[j] * window[(i + j) % fftSize];
    overlapScale = 1.0f / (winSum / (fftSize / hopSize));

    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    fftData.resize(fftSize * 2);
    targetCurve.resize(numBins + 1);
    bufPos = 0;
    frameCounter = 0;

    parameterChanged();
    computeTargetCurve();
}

void CatsHairProcessor::computeTargetCurve()
{
    for (int i = 0; i <= numBins; ++i)
    {
        float freq = i * sampleRate / fftSize;
        targetCurve[i] = getTargetDb(freq);
    }
}

float CatsHairProcessor::getTargetDb(float freqHz) const
{
    if (freqHz < startFreq)
        return 100.0f;

    float logStart = std::log2(startFreq);
    float logEnd = std::log2(20000.0f);
    float logFreq = std::log2(freqHz);
    float t = juce::jmap(logFreq, logStart, logEnd, 0.0f, 1.0f);
    t = juce::jlimit(0.0f, 1.0f, t);
    return push - slant * t;
}

void CatsHairProcessor::processFrame()
{
    for (int i = 0; i < fftSize; ++i)
    {
        int idx = (bufPos + i) % fftSize;
        fftData[i] = inputBuf[idx] * window[i];
    }
    juce::FloatVectorOperations::clear(fftData.data() + fftSize, fftSize);

    fft->performRealOnlyForwardTransform(fftData.data());

    float shapeMag[numBins + 1];
    float shapePhase[numBins + 1];

    float normFactor = fftSize * 0.25f;

    for (int i = 0; i <= numBins; ++i)
    {
        float re = fftData[i * 2];
        float im = fftData[i * 2 + 1];
        float mag = std::sqrt(re * re + im * im);
        float phase = std::atan2(im, re);

        float targetDb = targetCurve[i];
        float magNorm = mag / normFactor;
        float magDb = juce::Decibels::gainToDecibels(magNorm, -120.0f);

        if (magDb > targetDb)
        {
            float limitedNorm = juce::Decibels::decibelsToGain(targetDb);
            mag = limitedNorm * normFactor;
        }

        shapeMag[i] = mag;
        shapePhase[i] = phase;
    }

    if (phaseMode == 0)
    {
        for (int i = 0; i <= numBins; ++i)
        {
            fftData[i * 2] = shapeMag[i] * std::cos(shapePhase[i]);
            fftData[i * 2 + 1] = shapeMag[i] * std::sin(shapePhase[i]);
        }
    }
    else
    {
        std::vector<float> cepstrum(fftSize * 2, 0.0f);
        cepstrum[0] = std::log(shapeMag[0] + 1e-15f);
        cepstrum[1] = std::log(shapeMag[numBins] + 1e-15f);
        for (int i = 1; i < numBins; ++i)
        {
            cepstrum[2 * i] = std::log(shapeMag[i] + 1e-15f);
            cepstrum[2 * i + 1] = 0.0f;
        }

        fft->performRealOnlyInverseTransform(cepstrum.data());

        for (int i = 0; i < fftSize; ++i)
        {
            if (i == 0 || i == numBins)
                cepstrum[i] *= 1.0f;
            else if (i < numBins)
                cepstrum[i] *= 2.0f;
            else
                cepstrum[i] = 0.0f;
        }

        juce::FloatVectorOperations::clear(cepstrum.data() + fftSize, fftSize);
        fft->performRealOnlyForwardTransform(cepstrum.data());

        for (int i = 0; i <= numBins; ++i)
        {
            float re, im;
            if (i == 0) { re = cepstrum[0]; im = 0.0f; }
            else if (i == numBins) { re = cepstrum[1]; im = 0.0f; }
            else { re = cepstrum[2 * i]; im = cepstrum[2 * i + 1]; }
            float minPhase = std::atan2(im, re);

            fftData[i * 2] = shapeMag[i] * std::cos(minPhase);
            fftData[i * 2 + 1] = shapeMag[i] * std::sin(minPhase);
        }
    }

    fft->performRealOnlyInverseTransform(fftData.data());

    for (int i = 0; i < fftSize; ++i)
    {
        int idx = (bufPos + i) % fftSize;
        overlapBuf[idx] += fftData[i] * window[i] * overlapScale;
    }

    {
        static constexpr int displayBins = 256;
        static constexpr float minFreq = 20.0f;
        static constexpr float maxFreq = 20000.0f;

        for (int d = 0; d < displayBins; ++d)
        {
            float freq = minFreq * std::pow(maxFreq / minFreq, d / (float)(displayBins - 1));
            int bin = (int)(freq / sampleRate * fftSize);
            bin = juce::jlimit(0, numBins, bin);

            float sum = 0.0f;
            int count = 0;
            float binFreqEnd = (bin + 2) * sampleRate / fftSize;
            for (int b = bin; b <= numBins && b * sampleRate / fftSize <= binFreqEnd; ++b)
            {
                sum += shapeMag[b];
                count++;
            }
            float avgMag = (count > 0) ? sum / count : 0.0f;
            float db = juce::Decibels::gainToDecibels(avgMag, -80.0f);
            float norm = juce::jmap(db, -80.0f, 0.0f, 0.0f, 1.0f);
            currentSpectrum.magnitude[d] = juce::jlimit(0.0f, 1.0f, norm);
        }
        spectrumReady = true;
    }
}

void CatsHairProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (updateTarget)
    {
        computeTargetCurve();
        updateTarget = false;
    }

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if (bypass || numChannels == 0)
        return;

    auto* chData = buffer.getWritePointer(0);

    juce::FloatVectorOperations::clear(overlapBuf.data(), fftSize);
    for (int s = 0; s < numSamples; ++s)
    {
        inputBuf[bufPos] = chData[s];

        if (++frameCounter >= hopSize)
        {
            frameCounter = 0;
            processFrame();
        }

        chData[s] = overlapBuf[bufPos];
        overlapBuf[bufPos] = 0.0f;

        bufPos = (bufPos + 1) % fftSize;
    }

    for (int ch = 1; ch < numChannels; ++ch)
        buffer.copyFrom(ch, 0, buffer.getReadPointer(0), numSamples);
}

juce::AudioProcessorEditor* CatsHairProcessor::createEditor()
{
    return new CatsHairEditor(*this);
}

void CatsHairProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CatsHairProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CatsHairProcessor();
}
