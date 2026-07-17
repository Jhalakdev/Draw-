#pragma once
#include "Equalizer.h"
#include "PitchDetector.h"
#include "GainEnvelope.h"
#include <JuceHeader.h>

class PitchFollowEngine
{
public:
    PitchFollowEngine();
    ~PitchFollowEngine();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(float* channelData, int numSamples);

    void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
    bool isEnabled() const { return enabled; }

    void setSensitivity(float s) { sensitivity = juce::jlimit(0.1f, 1.0f, s); }
    float getSensitivity() const { return sensitivity; }

    void setTrackingSpeed(float s) { trackingSpeed = juce::jlimit(0.01f, 1.0f, s); }
    float getTrackingSpeed() const { return trackingSpeed; }

    void setIntensity(float i) { intensity = juce::jlimit(0.0f, 1.0f, i); }
    float getIntensity() const { return intensity; }

    float getCurrentPitch() const { return smoothedPitch; }
    float getConfidence() const { return smoothedConfidence; }

    Equalizer& getEqualizer() { return eq; }
    const Equalizer& getEqualizer() const { return eq; }

    GainEnvelope& getEnvelope() { return envelope; }
    const GainEnvelope& getEnvelope() const { return envelope; }

    float getCurrentEnvelopeGain() const { return currentEnvelopeGain; }

    static float getFrequencyToMidiNote(float freq);
    static const char* getNoteName(float freq);

private:
    Equalizer eq;
    PitchDetector pitchDetector;
    GainEnvelope envelope;
    bool enabled = false;
    float sensitivity = 0.5f;
    float trackingSpeed = 0.3f;
    float intensity = 0.6f;
    double sampleRate = 44100.0;

    float currentEnvelopeGain = 0.0f;
    bool pitchInRange = false;
    float smoothedPitch = 0.0f;
    float smoothedConfidence = 0.0f;
};
