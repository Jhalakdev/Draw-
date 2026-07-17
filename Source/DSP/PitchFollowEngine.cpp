#include "PitchFollowEngine.h"

PitchFollowEngine::PitchFollowEngine()
{
    eq.setEnvelope(&envelope);
}

PitchFollowEngine::~PitchFollowEngine() {}

void PitchFollowEngine::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    pitchDetector.prepare(sr);
    eq.prepare(sr, samplesPerBlock);
}

void PitchFollowEngine::process(float* channelData, int numSamples)
{
    if (!enabled)
        return;

    pitchDetector.detect(channelData, numSamples);
    float freq = pitchDetector.getFrequency();
    float conf = pitchDetector.getConfidence();

    float smoothCoeff = trackingSpeed * 0.3f;
    smoothedPitch += smoothCoeff * (freq - smoothedPitch);
    smoothedConfidence += smoothCoeff * (conf - smoothedConfidence);

    if (smoothedConfidence > 0.3f && smoothedPitch > 0.0f)
    {
        currentEnvelopeGain = envelope.getGainAt(smoothedPitch);
        pitchInRange = true;
    }
    else
    {
        pitchInRange = false;
        currentEnvelopeGain = 0.0f;
    }
}

float PitchFollowEngine::getFrequencyToMidiNote(float freq)
{
    if (freq <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2(freq / 440.0f);
}

const char* PitchFollowEngine::getNoteName(float freq)
{
    if (freq <= 0.0f) return "---";

    float midi = 69.0f + 12.0f * std::log2(freq / 440.0f);
    int noteNum = static_cast<int>(std::round(midi)) % 12;
    int octave = static_cast<int>(std::round(midi)) / 12 - 1;

    const char* notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    static char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%s%d", notes[noteNum], octave);
    return buffer;
}
