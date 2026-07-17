#include "EQGraphComponent.h"

EQGraphComponent::EQGraphComponent(PitchFollowEngine& e, FFTAnalyzer& fft) : engine(e), fftAnalyzer(fft)
{
    startTimerHz(20);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

EQGraphComponent::~EQGraphComponent() { stopTimer(); }
void EQGraphComponent::resized() { responsePathDirty = true; }

void EQGraphComponent::timerCallback()
{
    if (fftAnalyzer.hasNewData())
    {
        auto& mag = fftAnalyzer.getSpectrum().magnitude;
        for (int i = 0; i < NumSpectrumBins; ++i)
        {
            float energy = juce::jlimit(0.0f, 1.0f, mag[i]);
            spectrumData[i] = spectrumData[i] * 0.92f + energy * 0.08f;
        }
        fftAnalyzer.clearNewDataFlag();
    }

    if (engine.getEnvelope().isDirty())
    {
        engine.getEnvelope().clearDirty();
        responsePathDirty = true;
    }

    repaint();
}

void EQGraphComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(juce::Colour(0xFF0C0C18));
    g.fillRoundedRectangle(area.toFloat(), 6.0f);

    auto bounds = area.reduced(12);
    bounds = bounds.withTrimmedLeft(52).withTrimmedBottom(32).withTrimmedRight(12);
    graphX = static_cast<float>(bounds.getX());
    graphY = static_cast<float>(bounds.getY());
    graphW = static_cast<float>(bounds.getWidth());
    graphH = static_cast<float>(bounds.getHeight());

    g.setColour(juce::Colour(0xFF141428));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    g.setColour(juce::Colour(0xFF1E1E36));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    drawGrid(g);
    drawZones(g);
    drawSoloMarker(g);
    drawSpectrum(g);
    drawEnvelope(g);
    drawResponse(g);
    drawTrackingDot(g);
    drawPlusIcons(g);

    if (drawTrail.size() > 1)
    {
        juce::Path trailPath;
        trailPath.startNewSubPath(drawTrail[0].first, drawTrail[0].second);
        for (size_t i = 1; i < drawTrail.size(); ++i)
            trailPath.lineTo(drawTrail[i].first, drawTrail[i].second);
        g.setColour(juce::Colour(0xFFFFD700).withAlpha(0.25f));
        g.strokePath(trailPath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colour(0xFFFFD700).withAlpha(0.8f));
        g.strokePath(trailPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    if (drawCursorX > 0.0f && drawCursorY > 0.0f)
    {
        bool erase = (dragMode == DragMode::Erase);
        auto col = erase ? juce::Colour(0xFFFF6B6B) : juce::Colour(0xFFFFD700);
        g.setColour(col.withAlpha(0.12f));
        g.fillEllipse(drawCursorX - 14.0f, drawCursorY - 14.0f, 28.0f, 28.0f);
        g.setColour(col.withAlpha(0.5f));
        g.drawEllipse(drawCursorX - 14.0f, drawCursorY - 14.0f, 28.0f, 28.0f, 1.0f);
        g.setColour(col);
        g.fillEllipse(drawCursorX - 3.0f, drawCursorY - 3.0f, 6.0f, 6.0f);
        float freq = getFrequencyForX(drawCursorX);
        float gain = getGainForY(drawCursorY);
        g.setFont(juce::Font(10.0f).boldened());
        g.setColour(juce::Colour(0xFFF0F0FF));
        g.drawText(juce::String(freq, 0) + " Hz  " + (gain > 0 ? "+" : "") + juce::String(gain, 1) + " dB",
                   juce::Rectangle<int>(static_cast<int>(drawCursorX) + 18,
                                        static_cast<int>(drawCursorY) - 22, 120, 16),
                   juce::Justification::centredLeft, false);
    }

    g.setColour(juce::Colour(0xFF6A6A88));
    g.setFont(juce::Font(9.0f));
    float freqLabels[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float f : freqLabels)
    {
        float x = getXForFrequency(f);
        if (x >= graphX && x <= graphX + graphW)
        {
            juce::String label = f >= 1000 ? juce::String(f / 1000.0f, 1) + "k" : juce::String(f, 0);
            g.setColour(juce::Colour(0xFF6A6A88));
            g.drawText(label, juce::Rectangle<int>(static_cast<int>(x) - 20, static_cast<int>(graphY + graphH) + 5, 40, 14),
                       juce::Justification::centred, false);
        }
    }

    for (float gv = MinGain; gv <= MaxGain; gv += 6.0f)
    {
        float y = getYForGain(gv);
        juce::String gainLabel = (gv > 0 ? "+" : "") + juce::String(gv, 0) + " dB";
        g.setColour(juce::Colour(0xFF6A6A88));
        g.setFont(juce::Font(9.0f));
        g.drawText(gainLabel, juce::Rectangle<int>(static_cast<int>(graphX) - 50, static_cast<int>(y) - 7, 46, 14),
                   juce::Justification::centredRight, false);
    }

    auto& eq = engine.getEqualizer();
    g.setColour(juce::Colour(0xFF3A3A58));
    g.setFont(juce::Font(8.0f));
    juce::String dynHint;
    if (eq.getNumZones() > 0)
        dynHint = "  ·  Click zone to select  ·  Drag edges to resize";
    g.drawText("Draw: click  ·  Erase: Shift+click  ·  Solo: right-click/hold  ·  + to add zone" + dynHint,
               juce::Rectangle<int>(static_cast<int>(graphX), static_cast<int>(graphY + graphH) + 22, 500, 12),
               juce::Justification::centredLeft, false);

    g.setColour(juce::Colour(0xFF3A3A58));
    g.setFont(juce::Font(8.0f));
    int nDyn = eq.getNumZones();
    juce::String dynStatus;
    for (int i = 0; i < nDyn; ++i)
    {
        if (i > 0) dynStatus += "  ";
        const auto& z = eq.getZones()[i];
        dynStatus += (z.enabled ? "DYN" : "OFF") + juce::String(i + 1);
    }
    g.drawText(dynStatus,
               juce::Rectangle<int>(static_cast<int>(graphX + graphW) - 200, static_cast<int>(graphY + graphH) + 22, 200, 12),
               juce::Justification::centredRight, false);
}

void EQGraphComponent::drawGrid(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xFF1E1E38));
    for (float gv = MinGain; gv <= MaxGain; gv += 3.0f)
    {
        float y = getYForGain(gv);
        if (y >= graphY && y <= graphY + graphH)
            g.drawHorizontalLine(static_cast<int>(y), graphX, graphX + graphW);
    }

    float freqMarks[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    for (float f : freqMarks)
    {
        float x = getXForFrequency(f);
        g.drawVerticalLine(static_cast<int>(x), graphY, graphY + graphH);
    }

    g.setColour(juce::Colour(0xFF3A3A5A));
    float zeroY = getYForGain(0.0f);
    g.drawHorizontalLine(static_cast<int>(zeroY), graphX, graphX + graphW);
}

void EQGraphComponent::drawZones(juce::Graphics& g)
{
    auto& eq = engine.getEqualizer();
    int n = eq.getNumZones();
    for (int i = 0; i < n; ++i)
    {
        const auto& z = eq.getZones()[i];
        float x1 = getXForFrequency(z.startFreq);
        float x2 = getXForFrequency(z.endFreq);
        if (x2 < graphX || x1 > graphX + graphW) continue;
        x1 = juce::jmax(graphX, x1);
        x2 = juce::jmin(graphX + graphW, x2);

        bool sel = (i == selectedZone);
        auto bandCol = z.enabled
            ? juce::Colour(0xFFFF6B6B).withAlpha(sel ? 0.15f : 0.08f)
            : juce::Colour(0xFF555570).withAlpha(sel ? 0.10f : 0.05f);

        g.setColour(bandCol);
        g.fillRect(x1, graphY, x2 - x1, graphH);

        if (z.enabled)
        {
            g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(sel ? 0.5f : 0.25f));
            g.drawVerticalLine(static_cast<int>(x1), graphY, graphY + graphH);
            g.drawVerticalLine(static_cast<int>(x2), graphY, graphY + graphH);
        }

        if (sel)
        {
            g.setColour(juce::Colour(0xFF00E5B8).withAlpha(0.4f));
            g.drawRoundedRectangle(x1 - 1, graphY - 1, x2 - x1 + 2, graphH + 2, 2.0f, 1.5f);
        }

        char buf[32];
        std::snprintf(buf, sizeof(buf), "Z%d %s", i + 1, z.enabled ? "" : "(OFF)");
        g.setFont(juce::Font(8.0f).boldened());
        g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(sel ? 0.8f : 0.4f));
        g.drawText(buf, juce::Rectangle<int>(static_cast<int>(x1) + 4, static_cast<int>(graphY) + 4,
                                              static_cast<int>(x2 - x1) - 8, 12),
                   juce::Justification::centredLeft, false);

        if (z.enabled)
        {
            juce::String info = "TH " + juce::String(z.threshold, 0) + "  R " + juce::String(z.ratio, 1) + ":1";
            g.setFont(7.0f);
            g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(0.3f));
            g.drawText(info, juce::Rectangle<int>(static_cast<int>(x1) + 4, static_cast<int>(graphY) + 16,
                                                   static_cast<int>(x2 - x1) - 8, 10),
                       juce::Justification::centredLeft, false);
        }
    }
}

void EQGraphComponent::drawPlusIcons(juce::Graphics& g)
{
    float iconY = graphY + graphH + 14.0f;
    float iconSize = 14.0f;

    float plusPositions[] = { 100, 200, 500, 1000, 2000, 5000, 10000 };
    for (int i = 0; i < 7; ++i)
    {
        float x = getXForFrequency(plusPositions[i]);
        if (x < graphX || x > graphX + graphW) continue;

        bool hover = (i == hoveredPlusIcon);
        g.setColour(juce::Colour(0xFF555570).withAlpha(hover ? 0.8f : 0.4f));
        g.drawRoundedRectangle(x - iconSize / 2.0f, iconY - iconSize / 2.0f, iconSize, iconSize, 3.0f, 1.0f);

        g.setFont(juce::Font(12.0f).boldened());
        g.drawText("+", juce::Rectangle<int>(static_cast<int>(x) - 7, static_cast<int>(iconY) - 7, 14, 14),
                   juce::Justification::centred, false);
    }
}

void EQGraphComponent::drawSpectrum(juce::Graphics& g)
{
    float barW = graphW / static_cast<float>(NumSpectrumBins);
    for (int i = 0; i < NumSpectrumBins; ++i)
    {
        if (spectrumData[i] < 0.003f) continue;
        float freq = MinFreq * std::pow(MaxFreq / MinFreq,
            static_cast<float>(i) / static_cast<float>(NumSpectrumBins));
        float x = getXForFrequency(freq);
        float height = spectrumData[i] * graphH * 0.85f;
        float y = graphY + graphH - height;
        float t = static_cast<float>(i) / static_cast<float>(NumSpectrumBins);
        float hue = 0.58f - t * 0.38f;
        float hiBoost = 0.5f + t * 0.5f;
        float alpha = 0.15f + spectrumData[i] * hiBoost;
        alpha = juce::jmin(alpha, 0.95f);
        g.setColour(juce::Colour::fromHSV(hue, 0.9f, 0.8f, alpha));
        float barWScale = 0.6f + t * 0.3f;
        g.fillRect(x - barW * barWScale * 0.5f, y, barW * barWScale, height);
        if (spectrumData[i] > 0.15f)
        {
            g.setColour(juce::Colour::fromHSV(hue, 0.6f, 1.0f, alpha * 0.35f));
            float glowW = barW * (barWScale + 0.2f);
            g.fillRect(x - glowW * 0.5f, y - 1.0f, glowW, height + 2.0f);
        }
    }
}

void EQGraphComponent::drawSoloMarker(juce::Graphics& g)
{
    auto& eq = engine.getEqualizer();
    if (!eq.isSoloActive()) return;

    float freq = eq.getSoloFrequency();
    float x = getXForFrequency(freq);
    if (x < graphX || x > graphX + graphW) return;

    g.setColour(juce::Colour(0xFF00E5B8).withAlpha(0.1f));
    g.fillRect(x - 20.0f, graphY, 40.0f, graphH);
    g.setColour(juce::Colour(0xFF00E5B8).withAlpha(0.5f));
    g.drawVerticalLine(static_cast<int>(x), graphY, graphY + graphH);
    g.setColour(juce::Colour(0xFF00E5B8).withAlpha(0.9f));
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText("SOLO", juce::Rectangle<int>(static_cast<int>(x) - 14, static_cast<int>(graphY) - 12, 28, 10),
               juce::Justification::centred, false);
}

void EQGraphComponent::drawEnvelope(juce::Graphics& g)
{
    auto& env = engine.getEnvelope();
    const auto& points = env.getPoints();
    float zeroY = getYForGain(0.0f);

    if (points.empty()) return;

    juce::Path fillPath;
    fillPath.startNewSubPath(graphX, zeroY);
    for (const auto& pt : points)
        fillPath.lineTo(getXForFrequency(pt.frequency), getYForGain(pt.gain));
    fillPath.lineTo(graphX + graphW, zeroY);
    fillPath.closeSubPath();

    g.setColour(juce::Colour(0xFFFFD700).withAlpha(0.1f));
    g.fillPath(fillPath);
    g.setColour(juce::Colour(0xFFFFD700).withAlpha(0.06f));
    g.strokePath(fillPath, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path linePath;
    bool started = false;
    for (const auto& pt : points)
    {
        float x = getXForFrequency(pt.frequency);
        float y = getYForGain(pt.gain);
        if (!started) { linePath.startNewSubPath(x, y); started = true; }
        else linePath.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xFFFFD700).withAlpha(0.25f));
    g.strokePath(linePath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xFFFFD700));
    g.strokePath(linePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (const auto& pt : points)
    {
        float x = getXForFrequency(pt.frequency);
        float y = getYForGain(pt.gain);
        g.setColour(juce::Colour(0xFFFFD700).withAlpha(0.4f));
        g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
        g.setColour(juce::Colour(0xFFFFD700));
        g.fillEllipse(x - 1.5f, y - 1.5f, 3.0f, 3.0f);
    }
}

void EQGraphComponent::drawResponse(juce::Graphics& g)
{
    auto& eq = engine.getEqualizer();
    float zeroY = getYForGain(0.0f);

    if (responsePathDirty || cachedResponsePath.isEmpty())
    {
        cachedResponsePath.clear();
        bool started = false;
        for (int px = 0; px <= static_cast<int>(graphW); px += 4)
        {
            float x = graphX + static_cast<float>(px);
            float freq = getFrequencyForX(x);
            float response = eq.getCompoundResponse(freq);
            response = juce::jlimit(MinGain, MaxGain, response);
            float y = getYForGain(response);
            if (!started) { cachedResponsePath.startNewSubPath(x, y); started = true; }
            else cachedResponsePath.lineTo(x, y);
        }
        responsePathDirty = false;
    }

    juce::Path fillPath(cachedResponsePath);
    fillPath.lineTo(graphX + graphW, zeroY);
    fillPath.lineTo(graphX, zeroY);
    fillPath.closeSubPath();
    g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(0.06f));
    g.fillPath(fillPath);
    g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(0.2f));
    g.strokePath(fillPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xFFFF6B6B));
    g.strokePath(cachedResponsePath, juce::PathStrokeType(1.5f));

    if (eq.getNumZones() > 0)
    {
        juce::Path compPath;
        bool started = false;
        for (int px = 0; px <= static_cast<int>(graphW); px += 4)
        {
            float x = graphX + static_cast<float>(px);
            float freq = getFrequencyForX(x);
            float cGain = eq.getCompressedGain(freq);
            cGain = juce::jlimit(MinGain, MaxGain, cGain);
            float y = getYForGain(cGain);
            if (!started) { compPath.startNewSubPath(x, y); started = true; }
            else compPath.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(0.6f));
        g.strokePath(compPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void EQGraphComponent::drawTrackingDot(juce::Graphics& g)
{
    float pitch = engine.getCurrentPitch();
    float conf = engine.getConfidence();
    if (pitch <= 20.0f || conf < 0.2f) return;

    float x = getXForFrequency(pitch);
    if (x < graphX || x > graphX + graphW) return;

    float envGain = engine.getCurrentEnvelopeGain();
    float y = getYForGain(envGain);

    g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(0.06f));
    g.drawVerticalLine(static_cast<int>(x), graphY, graphY + graphH);
    g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(conf * 0.4f));
    g.drawEllipse(x - 14.0f, y - 14.0f, 28.0f, 28.0f, 1.5f);
    g.setColour(juce::Colour(0xFFFF6B6B).withAlpha(0.15f));
    g.fillEllipse(x - 10.0f, y - 10.0f, 20.0f, 20.0f);
    g.setColour(juce::Colour(0xFFFF6B6B));
    g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    g.setColour(juce::Colour(0xFFF0F0FF));
    g.setFont(juce::Font(9.0f).boldened());
    const char* noteName = PitchFollowEngine::getNoteName(pitch);
    g.drawText(juce::String(noteName) + "  " + juce::String(pitch, 0) + " Hz",
               juce::Rectangle<int>(static_cast<int>(x) - 40, static_cast<int>(graphY) - 16, 80, 14),
               juce::Justification::centred, false);
}

void EQGraphComponent::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition().toFloat();
    float mx = pos.x, my = pos.y;

    auto& eq = engine.getEqualizer();

    if (mx < graphX || mx > graphX + graphW || my < graphY || my > graphY + graphH)
    {
        float iconY = graphY + graphH + 14.0f;
        float iconSize = 14.0f;
        if (my >= iconY - iconSize && my <= iconY + iconSize)
        {
            float plusFreqs[] = { 100, 200, 500, 1000, 2000, 5000, 10000 };
            for (int i = 0; i < 7; ++i)
            {
                float x = getXForFrequency(plusFreqs[i]);
                if (std::abs(mx - x) < iconSize / 2.0f)
                {
                    int idx = eq.addZone(plusFreqs[i]);
                    engine.getEnvelope().markAudioDirty();
                    selectedZone = idx;
                    markResponseDirty();
                    return;
                }
            }
        }
        return;
    }

    if (event.mods.isRightButtonDown() || event.mods.isPopupMenu())
    {
        float freq = getFrequencyForX(mx);
        eq.setSolo(freq);
        dragMode = DragMode::Solo;
        soloDragActive = true;
        drawCursorX = mx; drawCursorY = my;
        repaint();
        return;
    }

    if (event.mods.isShiftDown())
    {
        dragMode = DragMode::Erase;
        engine.getEnvelope().pushUndo();
        float freq = getFrequencyForX(mx);
        engine.getEnvelope().eraseRange(freq - 30.0f, freq + 30.0f);
        drawCursorX = mx; drawCursorY = my;
        repaint();
        return;
    }

    int nZones = eq.getNumZones();
    for (int i = 0; i < nZones; ++i)
    {
        const auto& z = eq.getZones()[i];
        float zx1 = getXForFrequency(z.startFreq);
        float zx2 = getXForFrequency(z.endFreq);

        if (mx >= zx1 - 4 && mx <= zx1 + 4)
        {
            draggingZoneEdge = i;
            draggingEdgeLeft = true;
            dragMode = DragMode::ZoneEdge;
            engine.getEnvelope().pushUndo();
            return;
        }
        if (mx >= zx2 - 4 && mx <= zx2 + 4)
        {
            draggingZoneEdge = i;
            draggingEdgeLeft = false;
            dragMode = DragMode::ZoneEdge;
            engine.getEnvelope().pushUndo();
            return;
        }
    }

    for (int i = 0; i < nZones; ++i)
    {
        const auto& z = eq.getZones()[i];
        float zx1 = getXForFrequency(z.startFreq);
        float zx2 = getXForFrequency(z.endFreq);
        if (mx >= zx1 && mx <= zx2)
        {
            selectedZone = i;
            repaint();
            return;
        }
    }

    selectedZone = -1;

    dragMode = DragMode::Draw;
    engine.getEnvelope().pushUndo();
    float freq = getFrequencyForX(mx);
    float gain = getGainForY(my);
    lastDrawFreq = freq;
    lastDrawGain = gain;
    engine.getEnvelope().addFreehandPoints(freq, freq, { gain });
    drawTrail.clear();
    drawTrail.push_back({ mx, my });
    drawCursorX = mx; drawCursorY = my;
    repaint();
}

void EQGraphComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (dragMode == DragMode::None) return;

    float mx = juce::jmax(graphX, juce::jmin(graphX + graphW, static_cast<float>(event.getPosition().x)));
    float my = juce::jmax(graphY, juce::jmin(graphY + graphH, static_cast<float>(event.getPosition().y)));

    auto& eq = engine.getEqualizer();

    if (dragMode == DragMode::ZoneEdge)
    {
        float freq = getFrequencyForX(mx);
        auto& z = eq.getZone(draggingZoneEdge);
        if (draggingEdgeLeft)
            z.startFreq = juce::jlimit(20.0f, z.endFreq - 10.0f, freq);
        else
            z.endFreq = juce::jlimit(z.startFreq + 10.0f, 20000.0f, freq);
        engine.getEnvelope().markAudioDirty();
        eq.rebuildBands();
        repaint();
        return;
    }

    if (dragMode == DragMode::Solo)
    {
        float freq = getFrequencyForX(mx);
        eq.setSolo(freq);
        drawCursorX = mx; drawCursorY = my;
        repaint();
        return;
    }

    if (dragMode == DragMode::Draw)
    {
        float freq = getFrequencyForX(mx);
        float gain = getGainForY(my);
        if (lastDrawFreq > 0.0f && std::abs(freq - lastDrawFreq) > 0.5f)
        {
            float loF = juce::jmin(lastDrawFreq, freq);
            float hiF = juce::jmax(lastDrawFreq, freq);
            bool ascending = (freq >= lastDrawFreq);
            float spanHz = hiF - loF;
            float stepHz = 20.0f;
            if (loF > 250.0f) stepHz = 18.0f;
            if (loF > 500.0f) stepHz = 15.0f;
            if (loF > 1000.0f) stepHz = 25.0f;
            if (loF > 3000.0f) stepHz = 50.0f;
            if (loF > 8000.0f) stepHz = 100.0f;
            int steps = juce::jmin(200, juce::jmax(2, static_cast<int>(spanHz / stepHz) + 1));
            std::vector<float> interpGains;
            for (int s = 0; s <= steps; ++s)
            {
                float t = static_cast<float>(s) / static_cast<float>(steps);
                float g = lastDrawGain + t * (gain - lastDrawGain);
                interpGains.push_back(ascending ? g : (gain + (1.0f - t) * (lastDrawGain - gain)));
            }
            engine.getEnvelope().addFreehandPoints(loF, hiF, interpGains);
        }
        else
            engine.getEnvelope().addFreehandPoints(freq, freq, { gain });
        lastDrawFreq = freq;
        lastDrawGain = gain;
        drawTrail.push_back({ mx, my });
    }
    else if (dragMode == DragMode::Erase)
    {
        float freq = getFrequencyForX(mx);
        engine.getEnvelope().eraseRange(freq - 30.0f, freq + 30.0f);
    }

    drawCursorX = mx; drawCursorY = my;
    repaint();
}

void EQGraphComponent::mouseUp(const juce::MouseEvent&)
{
    if (dragMode == DragMode::Solo || soloDragActive)
    {
        engine.getEqualizer().clearSolo();
        soloDragActive = false;
        dragMode = DragMode::None;
        drawCursorX = -1.0f;
        repaint();
        return;
    }

    if (dragMode == DragMode::ZoneEdge)
    {
        draggingZoneEdge = -1;
        dragMode = DragMode::None;
        engine.getEnvelope().markAudioDirty();
        return;
    }

    if (dragMode != DragMode::None)
        engine.getEnvelope().markAudioDirty();

    dragMode = DragMode::None;
    drawCursorX = -1.0f;
    lastDrawFreq = -1.0f;
    drawTrail.clear();
    repaint();
}

float EQGraphComponent::getXForFrequency(float freq) const
{
    float logMin = std::log10(MinFreq);
    float logMax = std::log10(MaxFreq);
    float logFreq = std::log10(juce::jmax(MinFreq, juce::jmin(MaxFreq, freq)));
    return graphX + ((logFreq - logMin) / (logMax - logMin)) * graphW;
}

float EQGraphComponent::getFrequencyForX(float x) const
{
    float normalized = (x - graphX) / graphW;
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return std::pow(10.0f, std::log10(MinFreq) + normalized * (std::log10(MaxFreq) - std::log10(MinFreq)));
}

float EQGraphComponent::getYForGain(float gain) const
{
    float normalized = 1.0f - (gain - MinGain) / (MaxGain - MinGain);
    return graphY + normalized * graphH;
}

float EQGraphComponent::getGainForY(float y) const
{
    float normalized = (y - graphY) / graphH;
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return MaxGain - normalized * (MaxGain - MinGain);
}
