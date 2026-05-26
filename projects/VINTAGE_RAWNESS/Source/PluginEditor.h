#pragma once
#include <JuceHeader.h>
#include "ImageKnob.h"
#include "PluginProcessor.h"

class PresetButtonOverlay final : public juce::Button
{
public:
    explicit PresetButtonOverlay(const juce::String& name) : juce::Button(name)
    {
        setOpaque(false);
        setWantsKeyboardFocus(true);
    }

    void setSelected(bool shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        if (!selected && !isMouseOverButton && !isButtonDown)
            return;

        auto bounds = getLocalBounds().toFloat().reduced(kOuterInset);

        if (isButtonDown)
            bounds = bounds.translated(0.0f, kPressedOffsetY).reduced(kPressedInset);

        const auto fillAlpha = isButtonDown ? kPressedFillAlpha : (selected ? kSelectedFillAlpha : kHoverFillAlpha);
        const auto strokeAlpha = isButtonDown ? kPressedStrokeAlpha : (selected ? kSelectedStrokeAlpha : kHoverStrokeAlpha);

        g.setColour(juce::Colour::fromFloatRGBA(0.08f, 0.07f, 0.06f, isButtonDown ? kPressedShadowAlpha : 0.0f));
        if (isButtonDown)
            g.fillRoundedRectangle(bounds.translated(0.0f, -kPressedOffsetY), kCornerRadius);

        g.setColour(juce::Colour::fromFloatRGBA(0.86f, 0.82f, 0.68f, fillAlpha));
        g.fillRoundedRectangle(bounds, kCornerRadius);

        g.setColour(juce::Colour::fromFloatRGBA(0.92f, 0.88f, 0.70f, strokeAlpha));
        g.drawRoundedRectangle(bounds, kCornerRadius, isButtonDown ? kPressedStrokeWidth : kStrokeWidth);

        if (isButtonDown)
        {
            g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, kPressedTopShadeAlpha));
            g.drawLine(bounds.getX() + 4.0f, bounds.getY() + 2.0f,
                       bounds.getRight() - 4.0f, bounds.getY() + 2.0f,
                       kPressedStrokeWidth);
            g.setColour(juce::Colour::fromFloatRGBA(1.0f, 0.94f, 0.76f, kPressedBottomHighlightAlpha));
            g.drawLine(bounds.getX() + 4.0f, bounds.getBottom() - 2.0f,
                       bounds.getRight() - 4.0f, bounds.getBottom() - 2.0f,
                       kStrokeWidth);
        }
    }

private:
    static constexpr float kOuterInset = 2.0f;
    static constexpr float kPressedInset = 1.5f;
    static constexpr float kPressedOffsetY = 1.5f;
    static constexpr float kCornerRadius = 4.0f;
    static constexpr float kStrokeWidth = 1.5f;
    static constexpr float kPressedStrokeWidth = 2.0f;
    static constexpr float kHoverFillAlpha = 0.10f;
    static constexpr float kSelectedFillAlpha = 0.18f;
    static constexpr float kPressedFillAlpha = 0.24f;
    static constexpr float kHoverStrokeAlpha = 0.20f;
    static constexpr float kSelectedStrokeAlpha = 0.34f;
    static constexpr float kPressedStrokeAlpha = 0.46f;
    static constexpr float kPressedShadowAlpha = 0.20f;
    static constexpr float kPressedTopShadeAlpha = 0.28f;
    static constexpr float kPressedBottomHighlightAlpha = 0.22f;

    bool selected { false };
};

class VintageRawnessEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VintageRawnessEditor(VintageRawnessProcessor&);
    ~VintageRawnessEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Preset
    {
        const char* name;
        float dirt;
        float crush;
        float wobble;
    };

    static constexpr int kPresetCount = 4;

    void setupKnob(ImageKnobSlider&, juce::Image, float neutralValue);
    void applyPreset(int presetIndex);
    void setParameterValue(const char* parameterId, float value);
    void updatePresetSelection(int presetIndex);

    VintageRawnessProcessor& proc;

    ImageKnobSlider dirtKnob;
    ImageKnobSlider crushKnob;
    ImageKnobSlider wobbleKnob;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttach dirtAttach;
    SliderAttach crushAttach;
    SliderAttach wobbleAttach;

    PresetButtonOverlay preset1 { "Vintage Hype" };
    PresetButtonOverlay preset2 { "Nasty Chain" };
    PresetButtonOverlay preset3 { "MF Heaveness" };
    PresetButtonOverlay preset4 { "Bout" };

    juce::Image faceplateImage;
    int selectedPreset { 0 };

    static constexpr Preset presets[kPresetCount] {
        { "Vintage Hype", 0.30f, 0.20f, 0.10f },
        { "Nasty Chain", 0.80f, 0.70f, 0.40f },
        { "MF Heaveness", 0.40f, 0.30f, 0.90f },
        { "Bout", 0.60f, 0.50f, 0.20f }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VintageRawnessEditor)
};
