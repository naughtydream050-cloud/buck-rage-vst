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

        const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        const auto alpha = selected ? 0.26f : (isButtonDown ? 0.18f : 0.12f);
        g.setColour(juce::Colour::fromFloatRGBA(0.85f, 0.82f, 0.70f, alpha));
        g.drawRoundedRectangle(bounds, 4.0f, selected ? 2.0f : 1.5f);
    }

private:
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
