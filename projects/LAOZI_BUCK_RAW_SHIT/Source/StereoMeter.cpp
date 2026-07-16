#include "StereoMeter.h"

StereoMeter::StereoMeter(std::atomic<float>& leftPeakSource, std::atomic<float>& rightPeakSource)
    : leftPeak(leftPeakSource), rightPeak(rightPeakSource)
{
    setOpaque(false);
    startTimerHz(30);
}

float StereoMeter::toDb(float linear) noexcept
{
    return juce::jlimit(-60.0f, 3.0f, juce::Decibels::gainToDecibels(juce::jmax(linear, 0.000001f), -60.0f));
}

void StereoMeter::timerCallback()
{
    const auto targetLeft = toDb(leftPeak.exchange(0.0f, std::memory_order_relaxed));
    const auto targetRight = toDb(rightPeak.exchange(0.0f, std::memory_order_relaxed));
    const auto update = [] (float shown, float target)
    {
        const auto coeff = target > shown ? 0.62f : 0.10f;
        return shown + (target - shown) * coeff;
    };
    displayedLeftDb = update(displayedLeftDb, targetLeft);
    displayedRightDb = update(displayedRightDb, targetRight);
    repaint();
}

void StereoMeter::paint(juce::Graphics& g)
{
    const auto drawChannel = [&g] (juce::Rectangle<float> area, float db)
    {
        constexpr int segments = 24;
        const auto active = juce::jlimit(0, segments, static_cast<int>(std::ceil(juce::jmap(db, -60.0f, 0.0f, 0.0f, static_cast<float>(segments)))));
        const auto gap = 1.0f;
        const auto segmentHeight = (area.getHeight() - gap * static_cast<float>(segments - 1)) / static_cast<float>(segments);
        for (int i = 0; i < segments; ++i)
        {
            const auto y = area.getBottom() - (static_cast<float>(i + 1) * segmentHeight + static_cast<float>(i) * gap);
            const auto hot = i < active;
            const auto colour = i > 20 ? juce::Colour(0xfff17a8a) : juce::Colour(0xffd55b70);
            // Fully cover the darkened slot so only real audio peaks light up.
            g.setColour(hot ? colour.withAlpha(0.92f) : juce::Colour(0xff09090a));
            g.fillRoundedRectangle(area.getX(), y, area.getWidth(), segmentHeight, 0.7f);
        }
    };

    const auto bounds = getLocalBounds().toFloat();
    const auto columnWidth = bounds.getWidth() * (19.0f / 56.0f);
    const auto rightX = bounds.getWidth() * (37.0f / 56.0f);
    drawChannel({ 0.0f, 0.0f, columnWidth, bounds.getHeight() }, displayedLeftDb);
    drawChannel({ rightX, 0.0f, columnWidth, bounds.getHeight() }, displayedRightDb);
}
