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

    void setButtonImage(juce::Image image)
    {
        buttonImage = std::move(image);
        repaint();
    }

    void setSelected(bool shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        juce::ignoreUnused(isMouseOverButton);

        if (!buttonImage.isValid())
            return;

        if (!selected && !isButtonDown)
            return;

        auto bounds = getLocalBounds().toFloat();

        if (isButtonDown)
            bounds = bounds.reduced(kPressedInset).translated(0.0f, kPressedOffsetY);

        g.setOpacity(isButtonDown ? kPressedOpacity : kSelectedOpacity);
        g.drawImage(buttonImage, bounds, juce::RectanglePlacement::stretchToFit, false);

        if (isButtonDown)
        {
            g.setColour(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f, kPressedShadeAlpha));
            g.fillRect(getLocalBounds().withHeight(kPressedShadeHeight));
        }
    }

private:
    static constexpr float kPressedInset = 1.0f;
    static constexpr float kPressedOffsetY = 1.5f;
    static constexpr float kSelectedOpacity = 1.0f;
    static constexpr float kPressedOpacity = 0.82f;
    static constexpr float kPressedShadeAlpha = 0.18f;
    static constexpr int kPressedShadeHeight = 4;

    juce::Image buttonImage;
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
