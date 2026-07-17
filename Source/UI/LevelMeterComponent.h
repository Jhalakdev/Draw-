#pragma once
#include <JuceHeader.h>

class LevelMeterComponent : public juce::Component, public juce::Timer
{
public:
    LevelMeterComponent()
    {
        startTimerHz(30);
    }

    ~LevelMeterComponent() override { stopTimer(); }

    void setLevels(float left, float right)
    {
        levelL = left;
        levelR = right;
    }

    void timerCallback() override
    {
        double now = juce::Time::getMillisecondCounterHiRes();
        const double decayMs = 800.0;

        auto decay = [&](float& peak, double& peakTime, float current)
        {
            if (current > peak)
            {
                peak = current;
                peakTime = now;
            }
            else if (now - peakTime > decayMs)
            {
                peak *= 0.92f;
            }
        };

        decay(peakL, peakLTime, levelL);
        decay(peakR, peakRTime, levelR);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float w = bounds.getWidth();
        float h = bounds.getHeight();

        auto inner = bounds.reduced(2.0f);
        g.setColour(juce::Colour(0xFF0C0C18));
        g.fillRoundedRectangle(inner, 5.0f);
        g.setColour(juce::Colour(0xFF1E1E36));
        g.drawRoundedRectangle(inner, 5.0f, 1.0f);

        float meterW = (w - 18.0f) * 0.5f;
        float meterTop = 20.0f;
        float meterH = h - meterTop - 28.0f;

        drawMeter(g, 8.0f, meterTop, meterW, meterH, levelL, peakL, "L");
        drawMeter(g, 10.0f + meterW, meterTop, meterW, meterH, levelR, peakR, "R");

        g.setColour(juce::Colour(0xFF3A3A58));
        g.setFont(juce::Font(9.0f));

        float labelY = meterTop - 2.0f;
        g.drawText("L", juce::Rectangle<float>(8.0f, labelY - 12.0f, meterW, 10.0f), juce::Justification::centred);
        g.drawText("R", juce::Rectangle<float>(10.0f + meterW, labelY - 12.0f, meterW, 10.0f), juce::Justification::centred);

        for (int db = -24; db <= 0; db += 6)
        {
            float n = juce::jmap(static_cast<float>(db), MinDB, MaxDB, 0.0f, 1.0f);
            float y = meterTop + meterH * (1.0f - n);
            g.setColour(juce::Colour(0xFF2A2A44));
            g.drawHorizontalLine(static_cast<int>(y), 6.0f, 6.0f + meterW * 2.0f + 4.0f);
            g.setColour(juce::Colour(0xFF555570));
            g.drawText(juce::String(db), juce::Rectangle<float>(0.0f, y - 5.0f, 8.0f, 10.0f), juce::Justification::centredLeft);
        }
    }

    static constexpr float MaxDB = 0.0f;
    static constexpr float MinDB = -30.0f;

private:
    float levelL = 0.0f, levelR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    double peakLTime = 0.0, peakRTime = 0.0;

    static juce::Colour getMeterColour(float normalized, float alpha)
    {
        if (normalized > 0.85f)
            return juce::Colour::fromFloatRGBA(1.0f, 0.25f, 0.2f, alpha);
        if (normalized > 0.65f)
            return juce::Colour::fromFloatRGBA(1.0f, 0.75f, 0.1f, alpha);
        float t = normalized / 0.65f;
        float r = 0.2f + t * 0.3f;
        float gv = 0.85f - t * 0.3f;
        float b = 0.3f + t * 0.1f;
        return juce::Colour::fromFloatRGBA(r, gv, b, alpha);
    }

    void drawMeter(juce::Graphics& g, float x, float y, float w, float h,
                   float level, float peak, const char* channel)
    {
        float levelDB = juce::Decibels::gainToDecibels(level, MinDB);
        float norm = juce::jmap(levelDB, MinDB, MaxDB, 0.0f, 1.0f);
        norm = juce::jlimit(0.0f, 1.0f, norm);

        float peakDB = juce::Decibels::gainToDecibels(peak, MinDB);
        float peakNorm = juce::jmap(peakDB, MinDB, MaxDB, 0.0f, 1.0f);
        peakNorm = juce::jlimit(0.0f, 1.0f, peakNorm);

        float barW = w;
        float barH = h;

        juce::ColourGradient grad(juce::Colour(0xFF141428), x, y + h,
                                  juce::Colour(0xFF141428), x, y, false);
        grad.addColour(0.0, juce::Colour(0xFF141428));
        grad.addColour(1.0, juce::Colour(0xFF141428));
        g.setGradientFill(grad);
        g.fillRoundedRectangle(x, y, barW, barH, 2.0f);

        g.saveState();
        g.reduceClipRegion(x + 1.0f, y + 1.0f, barW - 2.0f, barH - 2.0f);

        for (float ny = 0; ny <= 1.0f; ny += 0.005f)
        {
            if (ny > norm) break;
            float py = y + barH - ny * barH;
            float alpha = 0.5f + ny * 0.4f;
            g.setColour(getMeterColour(ny, alpha));
            g.fillRect(x + 1.0f, py - 1.0f, barW - 2.0f, 2.0f);
        }

        if (peakNorm > 0.01f)
        {
            float peakY = y + barH - peakNorm * barH;
            g.setColour(juce::Colour(0xFFFFF0F0));
            g.fillRect(x + 1.0f, peakY - 1.5f, barW - 2.0f, 2.5f);
        }

        g.restoreState();

        g.setColour(juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.08f));
        g.fillRoundedRectangle(x, y, barW, barH, 2.0f);
        g.setColour(juce::Colour(0xFF2A2A44));
        g.drawRoundedRectangle(x, y, barW, barH, 2.0f, 1.0f);
    }
};
