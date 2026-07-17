#pragma once
#include <JuceHeader.h>
#include "../DSP/PitchFollowEngine.h"
#include "../DSP/FFTAnalyzer.h"

class EQGraphComponent : public juce::Component, public juce::Timer
{
public:
    EQGraphComponent(PitchFollowEngine& engine, FFTAnalyzer& fftAnalyzer);
    ~EQGraphComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void markResponseDirty() { responsePathDirty = true; repaint(); }

    int getSelectedZone() const { return selectedZone; }
    void setSelectedZone(int idx) { selectedZone = idx; repaint(); }

    float getXForFrequency(float freq) const;
    float getFrequencyForX(float x) const;
    float getYForGain(float gain) const;
    float getGainForY(float y) const;

    static constexpr float MinFreq = 20.0f;
    static constexpr float MaxFreq = 20000.0f;
    static constexpr float MinGain = -18.0f;
    static constexpr float MaxGain = 18.0f;

private:
    PitchFollowEngine& engine;
    FFTAnalyzer& fftAnalyzer;

    float graphX = 0.0f, graphY = 0.0f, graphW = 0.0f, graphH = 0.0f;
    int selectedZone = -1;
    int draggingZoneEdge = -1;
    bool draggingEdgeLeft = false;

    enum class DragMode { None, Draw, Erase, Solo, ZoneEdge, ZoneDrag };
    DragMode dragMode = DragMode::None;

    void drawGrid(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g);
    void drawEnvelope(juce::Graphics& g);
    void drawResponse(juce::Graphics& g);
    void drawZones(juce::Graphics& g);
    void drawTrackingDot(juce::Graphics& g);
    void drawSoloMarker(juce::Graphics& g);
    void drawPlusIcons(juce::Graphics& g);

    static constexpr int NumSpectrumBins = 256;
    std::array<float, NumSpectrumBins> spectrumData{};
    float drawCursorX = -1.0f;
    float drawCursorY = -1.0f;
    float lastDrawFreq = -1.0f;
    float lastDrawGain = 0.0f;
    std::vector<std::pair<float, float>> drawTrail;

    juce::Path cachedResponsePath;
    bool responsePathDirty = true;
    bool soloDragActive = false;
    int hoveredPlusIcon = -1;
};
