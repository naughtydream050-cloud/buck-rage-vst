#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>

class SilverKnobLAF : public juce::LookAndFeel_V4
{
public:
    SilverKnobLAF()
    {
        int sz = 0;
        auto* data = BinaryData::getNamedResource("knob_base_png", sz);
        if (data)
            knobImg = juce::ImageCache::getFromMemory(data, sz);
    }

    void drawRotarySlider(juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos,
        float startAng, float endAng,
        juce::Slider&) override
    {
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float r  = juce::jmin(width, height) * 0.5f - 2.f;
        const float angle = startAng + sliderPos * (endAng - startAng);

        if (knobImg.isValid())
        {
            juce::Graphics::ScopedSaveState ss(g);
            g.addTransform(juce::AffineTransform::rotation(angle, cx, cy));
            const float sz = r * 2.f;
            g.drawImage(knobImg,
                (int)(cx - r), (int)(cy - r), (int)sz, (int)sz,
                0, 0, knobImg.getWidth(), knobImg.getHeight());
        }
        else
        {
            g.setColour(juce::Colour(0xff888888));
            g.fillEllipse(cx - r, cy - r, r*2, r*2);
            g.setColour(juce::Colours::black);
            g.drawEllipse(cx - r, cy - r, r*2, r*2, 2.f);
            const float dotR = 4.f;
            const float dotX = cx + (r - 10.f) * std::sin(angle) - dotR;
            const float dotY = cy - (r - 10.f) * std::cos(angle) - dotR;
            g.setColour(juce::Colour(0xffcc0000));
            g.fillEllipse(dotX, dotY, dotR*2, dotR*2);
        }
    }

private:
    juce::Image knobImg;
};
