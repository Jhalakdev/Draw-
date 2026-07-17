#include "EnvelopeEditorComponent.h"

static const juce::Colour bgDark      (0xFF080810);
static const juce::Colour bgPanel     (0xFF0E0E1A);
static const juce::Colour bgGrid      (0xFF101020);
static const juce::Colour accentTeal  (0xFF00E5B8);
static const juce::Colour accentRed   (0xFFFF6B6B);
static const juce::Colour textBright  (0xFFF0F0FF);
static const juce::Colour textMuted   (0xFF6E6E8A);
static const juce::Colour textDim     (0xFF444460);
static const juce::Colour border      (0xFF222238);

EnvelopeEditorComponent::EnvelopeEditorComponent(GainEnvelope& env)
    : envelope(env)
{
    setupButtons();
}

EnvelopeEditorComponent::~EnvelopeEditorComponent() {}

void EnvelopeEditorComponent::timerCallback() {}

void EnvelopeEditorComponent::setupButtons()
{
    auto styleBtn = [&](juce::TextButton& btn, juce::Colour col, const juce::String& text) {
        btn.setButtonText(text);
        btn.setColour(juce::TextButton::buttonColourId, col.withAlpha(0.2f));
        btn.setColour(juce::TextButton::textColourOffId, col);
        btn.setLookAndFeel(nullptr);
        addAndMakeVisible(btn);
    };

    styleBtn(pencilBtn, accentTeal, "PEN");
    styleBtn(eraserBtn, accentRed, "ERASE");
    styleBtn(undoBtn, textMuted, "UNDO");
    styleBtn(redoBtn, textMuted, "REDO");
    styleBtn(clearBtn, textMuted, "CLEAR");
    styleBtn(closeBtn, accentRed, "CLOSE");

    pencilBtn.setClickingTogglesState(false);
    eraserBtn.setClickingTogglesState(false);

    modeLabel.setText("PENCIL", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, accentTeal);
    modeLabel.setFont(juce::Font(9.0f).boldened());
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);

    pencilBtn.onClick = [this]() { setMode(false); };
    eraserBtn.onClick = [this]() { setMode(true); };

    clearBtn.onClick = [this]() {
        envelope.pushUndo();
        envelope.clear();
        repaint();
    };

    undoBtn.onClick = [this]() { envelope.undo(); repaint(); };
    redoBtn.onClick = [this]() { envelope.redo(); repaint(); };
    closeBtn.onClick = [this]() { if (onClose) onClose(); };

    titleLabel.setText("Draw gain curve over the tracking range",
                       juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, textMuted);
    titleLabel.setFont(juce::Font(11.0f));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    setMode(false);
}

void EnvelopeEditorComponent::setMode(bool erase)
{
    erasing = erase;
    pencilBtn.setColour(juce::TextButton::buttonColourId,
                        erase ? textDim : accentTeal.withAlpha(0.2f));
    pencilBtn.setColour(juce::TextButton::textColourOffId,
                        erase ? textDim : accentTeal);
    eraserBtn.setColour(juce::TextButton::buttonColourId,
                        erase ? accentRed.withAlpha(0.2f) : textDim);
    eraserBtn.setColour(juce::TextButton::textColourOffId,
                        erase ? accentRed : textDim);
    modeLabel.setText(erase ? "ERASER" : "PENCIL", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, erase ? accentRed : accentTeal);
}

void EnvelopeEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(44);

    titleLabel.setBounds(topBar.getX() + 14, topBar.getY() + 4, 300, 20);

    int btnW = 52, btnH = 24;
    int x = topBar.getRight() - 14;

    closeBtn.setBounds(x - btnW, topBar.getY() + 10, btnW, btnH); x -= btnW + 4;
    clearBtn.setBounds(x - btnW, topBar.getY() + 10, btnW, btnH); x -= btnW + 4;
    redoBtn.setBounds(x - btnW, topBar.getY() + 10, btnW, btnH); x -= btnW + 4;
    undoBtn.setBounds(x - btnW, topBar.getY() + 10, btnW, btnH); x -= btnW + 4;
    eraserBtn.setBounds(x - btnW, topBar.getY() + 10, btnW, btnH); x -= btnW + 4;
    pencilBtn.setBounds(x - btnW, topBar.getY() + 10, btnW, btnH);

    modeLabel.setBounds(pencilBtn.getX(), topBar.getY() + 36, 80, 12);

    auto drawArea = area.reduced(18, 10).withTrimmedLeft(55).withTrimmedBottom(16);
    drawAreaX = drawArea.getX();
    drawAreaY = drawArea.getY();
    drawAreaW = drawArea.getWidth();
    drawAreaH = drawArea.getHeight();
}

void EnvelopeEditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(bgDark);
    drawGrid(g);
    drawEnvelope(g);
}

void EnvelopeEditorComponent::drawGrid(juce::Graphics& g)
{
    auto bounds = juce::Rectangle<int>(drawAreaX, drawAreaY, drawAreaW, drawAreaH);
    g.setColour(bgGrid);
    g.fillRect(bounds);

    for (float gv = GainEnvelope::MinGain; gv <= GainEnvelope::MaxGain; gv += 3.0f)
    {
        int y = static_cast<int>(yForGain(gv));
        g.setColour(border.withAlpha(0.6f));
        g.drawHorizontalLine(y, static_cast<float>(drawAreaX),
                             static_cast<float>(drawAreaX + drawAreaW));
    }

    g.setColour(textDim);
    g.drawHorizontalLine(static_cast<int>(yForGain(0.0f)),
                         static_cast<float>(drawAreaX), static_cast<float>(drawAreaX + drawAreaW));

    float rangeMin = envelope.getMinFreq();
    float rangeMax = envelope.getMaxFreq();
    float rangeWidth = rangeMax - rangeMin;
    int numTicks = juce::jmin(10, static_cast<int>(rangeWidth / 10.0f));
    if (numTicks < 2) numTicks = 2;

    for (int i = 0; i <= numTicks; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(numTicks);
        float freq = rangeMin + t * rangeWidth;
        int x = static_cast<int>(xForFreq(freq));

        g.setColour(border.withAlpha(0.5f));
        g.drawVerticalLine(x, static_cast<float>(drawAreaY),
                           static_cast<float>(drawAreaY + drawAreaH));

        g.setColour(textMuted);
        g.setFont(9.0f);
        juce::String label = freq >= 1000.0f
            ? juce::String(freq / 1000.0f, 1) + "k"
            : juce::String(freq, 0);
        g.drawText(label, x - 18, drawAreaY + drawAreaH + 2, 36, 12,
                   juce::Justification::centred, false);
    }

    for (float gv = GainEnvelope::MinGain; gv <= GainEnvelope::MaxGain; gv += 6.0f)
    {
        int y = static_cast<int>(yForGain(gv));
        g.setColour(textMuted);
        g.setFont(9.0f);
        juce::String label = (gv > 0 ? "+" : "") + juce::String(gv, 0) + "dB";
        g.drawText(label, drawAreaX - 52, y - 7, 48, 14,
                   juce::Justification::centredRight, false);
    }

    g.setColour(accentTeal.withAlpha(0.1f));
    g.drawRect(bounds, 1);
}

void EnvelopeEditorComponent::drawEnvelope(juce::Graphics& g)
{
    const auto& points = envelope.getPoints();
    float rangeMin = envelope.getMinFreq();
    float rangeMax = envelope.getMaxFreq();

    if (!points.empty())
    {
        juce::Path filledPath;
        filledPath.startNewSubPath(xForFreq(rangeMin), yForGain(0.0f));
        for (const auto& pt : points)
            filledPath.lineTo(xForFreq(pt.frequency), yForGain(pt.gain));
        filledPath.lineTo(xForFreq(rangeMax), yForGain(0.0f));
        filledPath.closeSubPath();

        g.setColour(accentTeal.withAlpha(0.1f));
        g.fillPath(filledPath);

        juce::Path linePath;
        bool started = false;
        for (const auto& pt : points)
        {
            float x = xForFreq(pt.frequency);
            float y = yForGain(pt.gain);
            if (!started) { linePath.startNewSubPath(x, y); started = true; }
            else linePath.lineTo(x, y);
        }
        g.setColour(accentTeal);
        g.strokePath(linePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    if (drawing && !erasing && !currentStrokeGains.empty() && lastDrawnFreq > 0.0f)
    {
        float startFreq = currentStrokeStartFreq;
        float endFreq = lastDrawnFreq;
        float freqStep = (endFreq - startFreq) / static_cast<float>(currentStrokeGains.size());

        juce::Path strokePath;
        bool started = false;
        for (size_t i = 0; i < currentStrokeGains.size(); ++i)
        {
            float freq = startFreq + static_cast<float>(i) * freqStep;
            float x = xForFreq(freq);
            float y = yForGain(currentStrokeGains[i]);
            if (!started) { strokePath.startNewSubPath(x, y); started = true; }
            else strokePath.lineTo(x, y);
        }

        g.setColour(accentTeal.withAlpha(0.3f));
        g.strokePath(strokePath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

        float cursorX = xForFreq(endFreq);
        float cursorY = yForGain(currentStrokeGains.back());
        g.setColour(textBright);
        g.fillEllipse(cursorX - 4.0f, cursorY - 4.0f, 8.0f, 8.0f);
        g.setColour(accentTeal);
        g.drawEllipse(cursorX - 6.0f, cursorY - 6.0f, 12.0f, 12.0f, 1.5f);
    }
}

void EnvelopeEditorComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown()) return;
    auto pos = event.getPosition();
    if (pos.x < drawAreaX || pos.x > drawAreaX + drawAreaW) return;
    if (pos.y < drawAreaY || pos.y > drawAreaY + drawAreaH) return;

    envelope.pushUndo();
    drawing = true;
    lastDrawnFreq = -1.0f;
    currentStrokeGains.clear();
    currentStrokeStartFreq = freqForX(static_cast<float>(pos.x));

    if (erasing)
    {
        float freq = freqForX(static_cast<float>(pos.x));
        float halfWidth = envelope.getRangeWidth() * 0.02f;
        envelope.eraseRange(freq - halfWidth, freq + halfWidth);
        repaint();
    }
}

void EnvelopeEditorComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!drawing) return;
    auto pos = event.getPosition();
    float x = juce::jlimit(static_cast<float>(drawAreaX),
                            static_cast<float>(drawAreaX + drawAreaW),
                            static_cast<float>(pos.x));
    float freq = freqForX(x);

    if (erasing)
    {
        float halfWidth = envelope.getRangeWidth() * 0.02f;
        envelope.eraseRange(freq - halfWidth, freq + halfWidth);
        repaint();
        return;
    }

    float gain = juce::jlimit(GainEnvelope::MinGain, GainEnvelope::MaxGain,
                               gainForY(static_cast<float>(pos.y)));

    if (lastDrawnFreq < 0.0f)
    {
        lastDrawnFreq = freq;
        currentStrokeStartFreq = freq;
        currentStrokeGains.clear();
    }

    float step = 5.0f;

    if (std::abs(freq - lastDrawnFreq) >= step)
    {
        currentStrokeGains.push_back(gain);
        lastDrawnFreq = freq;
    }

    repaint();
}

void EnvelopeEditorComponent::mouseUp(const juce::MouseEvent&)
{
    if (!drawing) return;
    drawing = false;

    if (!erasing && !currentStrokeGains.empty())
    {
        envelope.addFreehandPoints(currentStrokeStartFreq, lastDrawnFreq, currentStrokeGains);
        envelope.smooth();
    }

    currentStrokeGains.clear();
    lastDrawnFreq = -1.0f;
    repaint();
}

float EnvelopeEditorComponent::freqForX(float x) const
{
    float normalized = (x - static_cast<float>(drawAreaX)) / static_cast<float>(drawAreaW);
    return envelope.getMinFreq() + juce::jlimit(0.0f, 1.0f, normalized) * envelope.getRangeWidth();
}

float EnvelopeEditorComponent::xForFreq(float freq) const
{
    float normalized = (freq - envelope.getMinFreq()) / envelope.getRangeWidth();
    return static_cast<float>(drawAreaX) + juce::jlimit(0.0f, 1.0f, normalized) * static_cast<float>(drawAreaW);
}

float EnvelopeEditorComponent::gainForY(float y) const
{
    float normalized = (y - static_cast<float>(drawAreaY)) / static_cast<float>(drawAreaH);
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return GainEnvelope::MaxGain - normalized * (GainEnvelope::MaxGain - GainEnvelope::MinGain);
}

float EnvelopeEditorComponent::yForGain(float gain) const
{
    float normalized = (gain - GainEnvelope::MinGain) / (GainEnvelope::MaxGain - GainEnvelope::MinGain);
    return static_cast<float>(drawAreaY) + (1.0f - normalized) * static_cast<float>(drawAreaH);
}
