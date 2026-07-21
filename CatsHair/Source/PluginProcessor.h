#pragma once
#include <JuceHeader.h>

class CatsHairProcessor : public juce::AudioProcessor,
                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    CatsHairProcessor();
    ~CatsHairProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    void parameterChanged(const juce::String& paramID, float newValue) override;

    struct SpectrumData
    {
        std::array<float, 256> magnitude{};
    };
    const SpectrumData& getSpectrum() const { return currentSpectrum; }
    bool hasNewSpectrum() const { return spectrumReady; }
    void clearSpectrumFlag() { spectrumReady = false; }

    float getTargetDb(float freqHz) const;

private:
    juce::AudioProcessorValueTreeState apvts;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int hopSize = fftSize / 4;
    static constexpr int numBins = fftSize / 2;
    static constexpr int numDisplayBins = 256;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> inputBuf;
    std::vector<float> overlapBuf;
    std::vector<float> window;
    std::vector<float> fftData;
    int bufPos = 0;
    int frameCounter = 0;
    double sampleRate = 48000.0;
    float overlapScale = 2.0f / 3.0f;

    bool updateTarget = true;
    std::vector<float> targetCurve;

    bool spectrumReady = false;
    SpectrumData currentSpectrum;

    void computeTargetCurve();

    void processFrame();

    void parameterChanged();

    float startFreq = 3000.0f;
    float slant = 4.0f;
    float push = 0.0f;
    int phaseMode = 0;
    bool bypass = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CatsHairProcessor)
};
