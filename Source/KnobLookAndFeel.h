#pragma once
#include <JuceHeader.h>

/**
 * BUCK RAGE - SilverKnobLAF v2.3
 * Stitch [MainKnob_PerfectCircle] - squareBounds + constexpr palette + ScopedSaveState rotation
 * Fix v2.3: center-crop (srcSz=getHeight, srcX=centred) -> no squish; programmatic dot removed
 */
class SilverKnobLAF : public juce::LookAndFeel_V4
{
public:
    static constexpr float kSideRatio    = 0.90f;
    static constexpr float kInnerRatio   = 0.76f;
    static constexpr float kDotRadiusR   = 0.055f;
    static constexpr float kDotDistRatio = 0.35f;
    static constexpr int   kHairlines    = 24;

    static constexpr juce::uint32 kColOuter = 0xff1a1a1a;
    static constexpr juce::uint32 kColHigh  = 0xffe8e8e8;
    static constexpr juce::uint32 kColLow   = 0xff787878;
    static constexpr juce::uint32 kColMid1  = 0xffd0d0d0;
    static constexpr juce::uint32 kColMid2  = 0xff999999;
    static constexpr juce::uint32 kColGlow  = 0xaaff0000;
    static constexpr juce::uint32 kColDot   = 0xffff2222;
    static constexpr double kGStop1 = 0.35;
    static constexpr double kGStop2 = 0.70;
    static constexpr float kGlowMul = 1.6f;
    static constexpr float kHlX = 0.45f, kHlY = 0.65f, kHlW = 0.70f, kHlH = 0.50f;

    SilverKnobLAF() {
        int sz = 0;
        auto* data = BinaryData::getNamedResource("knob_base_png", sz);
        if (data) knobImg = juce::ImageCache::getFromMemory(data, sz);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAng, float endAng,
                          juce::Slider&) override
    {
        using namespace juce;
        const auto  raw  = Rectangle<float>((float)x, (float)y, (float)width, (float)height);
        const float side = jmin(raw.getWidth(), raw.getHeight()) * kSideRatio;
        const auto  sq   = raw.withSizeKeepingCentre(side, side);
        const float cx   = raw.getCentreX();
        const float cy   = raw.getCentreY();
        const float ang  = startAng + sliderPos * (endAng - startAng)
                           - MathConstants<float>::halfPi;
        const float dot  = side * kDotRadiusR;

        auto drawDot = [&](float dist) {
            const float px = cx + dist * std::cos(ang);
            const float py = cy + dist * std::sin(ang);
            g.setColour(Colour(kColGlow));
            g.fillEllipse(px-dot*kGlowMul, py-dot*kGlowMul, dot*kGlowMul*2.f, dot*kGlowMul*2.f);
            g.setColour(Colour(kColDot));
            g.fillEllipse(px-dot, py-dot, dot*2.f, dot*2.f);
            g.setColour(Colours::white.withAlpha(0.6f));
            g.fillEllipse(px-dot*kHlX, py-dot*kHlY, dot*kHlW, dot*kHlH);
        };

        if (knobImg.isValid()) {
            // v2.3: center-crop -> correct aspect ratio; dot removed (built into asset)
            const float rotCtx = startAng + sliderPos * (endAng - startAng);
            Graphics::ScopedSaveState savedState(g);
            g.addTransform(AffineTransform::rotation(rotCtx, cx, cy));
            const float srcSz = (float)knobImg.getHeight();
            const float srcX  = ((float)knobImg.getWidth() - srcSz) * 0.5f;
            g.drawImage(knobImg, sq.getX(), sq.getY(), sq.getWidth(), sq.getHeight(),
                        srcX, 0.f, srcSz, srcSz);
            return;
        }

        g.setColour(Colour(kColOuter));
        g.fillEllipse(sq);
        g.setColour(Colours::white.withAlpha(0.12f));
        g.drawEllipse(sq.reduced(1.f), 1.5f);
        g.setColour(Colours::black.withAlpha(0.8f));
        g.drawEllipse(sq.reduced(2.5f), 1.f);

        const float inner = side * kInnerRatio;
        const auto  inR   = raw.withSizeKeepingCentre(inner, inner);
        ColourGradient grad(Colour(kColHigh), inR.getCentreX(), inR.getCentreY(),
                            Colour(kColLow),  inR.getRight(),   inR.getBottom(), true);
        grad.addColour(kGStop1, Colour(kColMid1));
        grad.addColour(kGStop2, Colour(kColMid2));
        g.setGradientFill(grad);
        g.fillEllipse(inR);

        g.setColour(Colours::white.withAlpha(0.08f));
        const float r = inner * 0.5f;
        for (int i = 0; i < kHairlines; ++i) {
            const float a = (float)i * MathConstants<float>::twoPi / (float)kHairlines;
            g.drawLine(cx + r * 0.85f * std::cos(a), cy + r * 0.85f * std::sin(a),
                       cx + r * std::cos(a),          cy + r * std::sin(a), 0.8f);
        }

        drawDot(side * kDotDistRatio);
    }

private:
    juce::Image knobImg;
};
