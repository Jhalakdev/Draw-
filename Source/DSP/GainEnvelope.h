#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>
#include <cmath>

class GainEnvelope
{
public:
    struct Point
    {
        float frequency = 0.0f;
        float gain = 0.0f;
    };

    GainEnvelope() = default;

    static constexpr float MinFreq = 20.0f;
    static constexpr float MaxFreq = 20000.0f;
    static constexpr float MinGain = -12.0f;
    static constexpr float MaxGain = 12.0f;

    const std::vector<Point>& getPoints() const { return points; }

    void addFreehandPoints(float startFreq, float endFreq,
                           const std::vector<float>& gains)
    {
        float lo = juce::jmin(startFreq, endFreq);
        float hi = juce::jmax(startFreq, endFreq);
        lo = juce::jmax(MinFreq, lo);
        hi = juce::jmin(MaxFreq, hi);

        if (gains.empty()) return;

        auto it = std::lower_bound(points.begin(), points.end(), lo,
            [](const Point& p, float f) { return p.frequency < f; });
        auto endIt = std::upper_bound(points.begin(), points.end(), hi,
            [](float f, const Point& p) { return f < p.frequency; });
        points.erase(it, endIt);

        for (size_t i = 0; i < gains.size(); ++i)
        {
            float f = lo + (hi > lo ? static_cast<float>(i) * (hi - lo) / static_cast<float>(gains.size()) : 0.0f);
            points.push_back({ f, gains[i] });
        }

        sortAndDeduplicate();
        dirty = true;
    }

    void eraseRange(float startFreq, float endFreq)
    {
        float lo = juce::jmin(startFreq, endFreq);
        float hi = juce::jmax(startFreq, endFreq);
        auto it = std::remove_if(points.begin(), points.end(),
            [lo, hi](const Point& p) { return p.frequency >= lo && p.frequency <= hi; });
        points.erase(it, points.end());
        dirty = true;
    }

    float getGainAt(float freq) const
    {
        if (points.empty()) return 0.0f;
        if (freq <= points.front().frequency) return points.front().gain;
        if (freq >= points.back().frequency) return points.back().gain;

        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            if (freq >= points[i].frequency && freq <= points[i + 1].frequency)
            {
                float t = (freq - points[i].frequency) /
                          (points[i + 1].frequency - points[i].frequency);
                return points[i].gain + t * (points[i + 1].gain - points[i].gain);
            }
        }
        return 0.0f;
    }

    void clear()
    {
        points.clear();
        dirty = true;
        audioDirty = true;
    }

    void pushUndo()
    {
        undoStack.push_back(points);
        if (undoStack.size() > 40)
            undoStack.erase(undoStack.begin());
        redoStack.clear();
    }

    void undo()
    {
        if (undoStack.empty()) return;
        redoStack.push_back(points);
        points = undoStack.back();
        undoStack.pop_back();
        dirty = true;
        audioDirty = true;
    }

    void redo()
    {
        if (redoStack.empty()) return;
        undoStack.push_back(points);
        points = redoStack.back();
        redoStack.pop_back();
        dirty = true;
        audioDirty = true;
    }

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }

    bool isDirty() const { return dirty; }
    void clearDirty() { dirty = false; }
    bool isAudioDirty() const { return audioDirty; }
    void markAudioDirty() { audioDirty = true; }
    void clearAudioDirty() { audioDirty = false; }

    juce::ValueTree serialize() const
    {
        juce::ValueTree tree("Envelope");
        juce::String pointData;
        for (size_t i = 0; i < points.size(); ++i)
        {
            if (i > 0) pointData += ";";
            pointData += juce::String(points[i].frequency, 2) + "," +
                         juce::String(points[i].gain, 2);
        }
        tree.setProperty("points", pointData, nullptr);
        return tree;
    }

    void deserialize(const juce::ValueTree& tree)
    {
        if (!tree.isValid()) return;
        points.clear();
        juce::String pointData = tree.getProperty("points", "");
        if (pointData.isNotEmpty())
        {
            auto pairs = juce::StringArray::fromTokens(pointData, ";", "");
            for (auto& pair : pairs)
            {
                auto parts = juce::StringArray::fromTokens(pair, ",", "");
                if (parts.size() >= 2)
                    points.push_back({ parts[0].getFloatValue(), parts[1].getFloatValue() });
            }
        }
        sortAndDeduplicate();
        undoStack.clear();
        redoStack.clear();
    }

private:
    std::vector<Point> points;
    std::vector<std::vector<Point>> undoStack;
    std::vector<std::vector<Point>> redoStack;
    bool dirty = false;
    bool audioDirty = false;

    void sortAndDeduplicate()
    {
        std::sort(points.begin(), points.end(),
            [](const Point& a, const Point& b) { return a.frequency < b.frequency; });
        if (points.size() > 1)
        {
            auto it = std::unique(points.begin(), points.end(),
                [](const Point& a, const Point& b) {
                    return std::abs(a.frequency - b.frequency) < 1.0f;
                });
            points.erase(it, points.end());
        }
    }
};
