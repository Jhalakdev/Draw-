#pragma once
#include <JuceHeader.h>
#include "../DSP/GainEnvelope.h"

class EnvelopeEditorComponent : public juce::Component, public juce::Timer
{
public:
    EnvelopeEditorComponent(GainEnvelope& env);
    ~EnvelopeEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void setOnClose(std::function<void()> callback) { onClose = std::move(callback); }

private:
    GainEnvelope& envelope;
    std::function<void()> onClose;

    int drawAreaX = 0, drawAreaY = 0, drawAreaW = 0, drawAreaH = 0;

    bool drawing = false;
    bool erasing = false;
    float lastDrawnFreq = -1.0f;
    float currentStrokeStartFreq = 0.0f;
    std::vector<float> currentStrokeGains;

    juce::TextButton pencilBtn { "PEN" };
    juce::TextButton eraserBtn { "ERASE" };
    juce::TextButton undoBtn { "UNDO" };
    juce::TextButton redoBtn { "REDO" };
    juce::TextButton clearBtn { "CLEAR" };
    juce::TextButton closeBtn { "CLOSE" };
    juce::Label modeLabel;
    juce::Label titleLabel;

    void setupButtons();
    void setMode(bool erasing);
    void drawGrid(juce::Graphics& g);
    void drawEnvelope(juce::Graphics& g);

    float freqForX(float x) const;
    float xForFreq(float freq) const;
    float gainForY(float y) const;
    float yForGain(float gain) const;
};
