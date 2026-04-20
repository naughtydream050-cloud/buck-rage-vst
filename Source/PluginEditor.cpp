#include "PluginEditor.h"
#include <BinaryData.h>

BuckRageEditor::BuckRageEditor(BuckRageProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      buckAttach(p.apvts, "buck", buckKnob),
      rageAttach(p.apvts, "rage", rageKnob)
{
    setSize(992, 496);

    {
        int sz = 0;
        auto* data = BinaryData::getNamedResource("background_buck_rage_png", sz);
        if (data)
            bgImage = juce::ImageCache::getFromMemory(data, sz);
    }

    for (auto* k : { &buckKnob, &rageKnob })
    {
        k->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        k->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        k->setLookAndFeel(&knobLAF);
        addAndMakeVisible(k);
    }

    startTimerHz(30);
}

BuckRageEditor::~BuckRageEditor()
{
    buckKnob.setLookAndFeel(nullptr);
    rageKnob.setLookAndFeel(nullptr);
}

void BuckRageEditor::resized()
{
    buckKnob.setBounds(98,  163, 155, 155);
    rageKnob.setBounds(738, 163, 155, 155);
}

void BuckRageEditor::paint(juce::Graphics& g)
{
    if (bgImage.isValid())
    {
        g.drawImageAt(bgImage, 0, 0);
    }
    else
    {
        g.fillAll(juce::Colour(0xff0a0a0a));
        g.setColour(juce::Colours::red);
        g.setFont(14.f);
        g.drawText("BinaryData load failed", getLocalBounds(), juce::Justification::centred);
    }
}
