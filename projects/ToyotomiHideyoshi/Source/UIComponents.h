#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <utility>
#include "PluginProcessor.h"

namespace ToyotomiUi
{
juce::Colour background();
juce::Colour panel();
juce::Colour ivory();
juce::Colour muted();
juce::Colour gold();
juce::Colour red();
juce::Colour border();
juce::Font font (float height, bool bold = false);
void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String& title = {});
void drawMotionGlyph (juce::Graphics&, juce::Rectangle<float>, int presetIndex);
}

class TopBarComponent final : public juce::Component,
                              private juce::Timer
{
public:
    explicit TopBarComponent (ToyotomiHideyoshiAudioProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    ToyotomiHideyoshiAudioProcessor& processor;
    double bpm = 120.0;
    int numerator = 4;
    int denominator = 4;
    bool hostSync = false;
};

class ArtworkPanel final : public juce::Component
{
public:
    explicit ArtworkPanel (juce::Image sourceImage) : source (std::move (sourceImage)) {}
    void paint (juce::Graphics&) override;

private:
    juce::Image source;
};

class BarTabComponent final : public juce::Component
{
public:
    std::function<void (int)> onSelectedPage;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int getSelectedPage() const noexcept { return selectedPage; }

private:
    int selectedPage = 0;
};

class BarCellComponent final : public juce::Component
{
public:
    std::function<void (int)> onSelected;
    void configure (int number, bool isSelected, bool isPlaying);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    int barNumber = 1;
    bool selected = false;
    bool playing = false;
};

class BarMapComponent final : public juce::Component
{
public:
    std::function<void (int)> onSelectedBar;
    BarMapComponent();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectBar (int number);
    std::array<BarCellComponent, 16> cells;
    int selectedBar = 11;
    int playingBar = 6;
};

class CountCellComponent final : public juce::Component
{
public:
    std::function<void (int)> onSelected;
    void configure (int number, int preset, bool isSelected);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    int countNumber = 1;
    int presetIndex = 0;
    bool selected = false;
};

class CountGridComponent final : public juce::Component
{
public:
    std::function<void (int)> onSelectedCount;
    CountGridComponent();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectCount (int number);
    std::array<CountCellComponent, 16> cells;
    int selectedCount = 5;
};

class XYMotionPad final : public juce::Component
{
public:
    std::function<void (const std::vector<PluginStateModel::MotionPoint>&)> onMotionChanged;
    XYMotionPad();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<void()> onClearMotion;
    std::function<void()> onResetCount;

private:
    void appendPoint (juce::Point<float>);
    juce::Rectangle<float> padBounds() const;
    juce::Array<juce::Point<float>> normalizedMotion;
    bool recording = false;
};

class ScratchPresetPalette final : public juce::Component
{
public:
    std::function<void (int)> onPresetSelected;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    int selectedPreset = 2;
};

class CountParameterPanel final : public juce::Component
{
public:
    std::function<void (int)> onLengthSelected;
    explicit CountParameterPanel (ToyotomiHideyoshiAudioProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<float> knobBounds (int index) const;
    void updateKnob (int index, float delta);
    ToyotomiHideyoshiAudioProcessor& processor;
    int activeKnob = -1;
    float dragStartY = 0.0f;
};

class OutputMeterComponent final : public juce::Component,
                                   private juce::Timer
{
public:
    explicit OutputMeterComponent (ToyotomiHideyoshiAudioProcessor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    static float smooth (float current, float target);
    ToyotomiHideyoshiAudioProcessor& processor;
    float leftDb = -60.0f;
    float rightDb = -60.0f;
    float leftPeakDb = -60.0f;
    float rightPeakDb = -60.0f;
};

class BottomStatusBar final : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
};
