#include "UIComponents.h"
#include <cmath>

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define TOYOTOMI_HAS_BINARY_DATA 1
#else
 #define TOYOTOMI_HAS_BINARY_DATA 0
#endif

namespace
{
juce::Rectangle<int> gridCell (juce::Rectangle<int> area, int column, int row,
                               int columns, int rows, int gap = 3)
{
    const auto width = (area.getWidth() - gap * (columns - 1)) / columns;
    const auto height = (area.getHeight() - gap * (rows - 1)) / rows;
    return { area.getX() + column * (width + gap), area.getY() + row * (height + gap), width, height };
}

juce::Image loadAsset (const void* data, int size)
{
    return juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (size));
}

const juce::Image& imageFor (const juce::String& id)
{
#if TOYOTOMI_HAS_BINARY_DATA
    if (id == "tabNormal")       { static const auto image = loadAsset (BinaryData::tab_normal_frame_png, BinaryData::tab_normal_frame_pngSize); return image; }
    if (id == "tabSelected")     { static const auto image = loadAsset (BinaryData::tab_selected_frame_png, BinaryData::tab_selected_frame_pngSize); return image; }
    if (id == "barNormal")       { static const auto image = loadAsset (BinaryData::bar_normal_frame_png, BinaryData::bar_normal_frame_pngSize); return image; }
    if (id == "barSelected")     { static const auto image = loadAsset (BinaryData::bar_selected_frame_png, BinaryData::bar_selected_frame_pngSize); return image; }
    if (id == "barPlaying")      { static const auto image = loadAsset (BinaryData::bar_playing_frame_png, BinaryData::bar_playing_frame_pngSize); return image; }
    if (id == "countNormal")     { static const auto image = loadAsset (BinaryData::count_normal_frame_png, BinaryData::count_normal_frame_pngSize); return image; }
    if (id == "countSelected")   { static const auto image = loadAsset (BinaryData::count_selected_frame_png, BinaryData::count_selected_frame_pngSize); return image; }
    if (id == "presetNormal")    { static const auto image = loadAsset (BinaryData::preset_normal_frame_png, BinaryData::preset_normal_frame_pngSize); return image; }
    if (id == "presetSelected")  { static const auto image = loadAsset (BinaryData::preset_selected_frame_png, BinaryData::preset_selected_frame_pngSize); return image; }
    if (id == "lengthNormal")    { static const auto image = loadAsset (BinaryData::length_normal_frame_png, BinaryData::length_normal_frame_pngSize); return image; }
    if (id == "lengthSelected")  { static const auto image = loadAsset (BinaryData::length_selected_frame_png, BinaryData::length_selected_frame_pngSize); return image; }
    if (id == "knobBase")        { static const auto image = loadAsset (BinaryData::knob_ring_67_png, BinaryData::knob_ring_67_pngSize); return image; }
    if (id == "knobPointer")     { static const auto image = loadAsset (BinaryData::knob_pointer_png, BinaryData::knob_pointer_pngSize); return image; }
    if (id == "meterLed")        { static const auto image = loadAsset (BinaryData::meter_led_strip_png, BinaryData::meter_led_strip_pngSize); return image; }
#else
    juce::ignoreUnused (id);
#endif
    static const juce::Image empty;
    return empty;
}

void drawImage (juce::Graphics& g, const juce::Image& image, juce::Rectangle<int> bounds)
{
    if (image.isValid())
        g.drawImageWithin (image, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), juce::RectanglePlacement::stretchToFit);
}

void drawFrame (juce::Graphics& g, const char* id, juce::Rectangle<int> bounds)
{
    drawImage (g, imageFor (id), bounds);
}
}

namespace ToyotomiUi
{
juce::Colour background() { return juce::Colour (0xff07090a); }
juce::Colour panel()      { return juce::Colour (0xff0a0c0d); }
juce::Colour ivory()      { return juce::Colour (0xffd2c6af); }
juce::Colour muted()      { return juce::Colour (0xff8f887c); }
juce::Colour gold()       { return juce::Colour (0xffd6a64d); }
juce::Colour red()        { return juce::Colour (0xffdc3228); }
juce::Colour border()     { return juce::Colour (0xff292927); }
juce::Font font (float height, bool bold) { return juce::Font (height, bold ? juce::Font::bold : juce::Font::plain); }
void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String&) {}
void drawMotionGlyph (juce::Graphics&, juce::Rectangle<float>, int) {}
bool validateEmbeddedImageAssets()
{
    static constexpr std::array<const char*, 13> ids {
        "tabNormal", "tabSelected", "barNormal", "barSelected", "barPlaying",
        "countNormal", "countSelected", "presetNormal", "presetSelected",
        "lengthNormal", "lengthSelected", "knobBase", "knobPointer"
    };
    for (const auto* id : ids)
    {
        const auto& image = imageFor (id);
        if (! image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0)
            return false;
    }
    const auto& meter = imageFor ("meterLed");
    return meter.isValid() && meter.getWidth() == 15 && meter.getHeight() == 255;
}
}

TopBarComponent::TopBarComponent (ToyotomiHideyoshiAudioProcessor& p) : processor (p) { startTimerHz (15); }
void TopBarComponent::timerCallback()
{
    bpm = processor.getHostBpm();
    numerator = processor.getTimeSignatureNumerator();
    denominator = processor.getTimeSignatureDenominator();
    hostSync = processor.getHostSyncAvailable();
}
void TopBarComponent::paint (juce::Graphics&) {}
void TopBarComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto bypass = juce::Rectangle<int> (getWidth() - juce::roundToInt (98.0f * getWidth() / 1270.0f),
                                              juce::roundToInt (28.0f * getHeight() / 78.0f),
                                              juce::roundToInt (68.0f * getWidth() / 1270.0f),
                                              juce::roundToInt (33.0f * getHeight() / 78.0f));
    if (bypass.contains (event.getPosition()))
        processor.getStateModel().setBypass (! processor.getStateModel().getUiState().bypass);
}

void ArtworkPanel::paint (juce::Graphics& g) { juce::ignoreUnused (g, source); }

void BarTabComponent::paint (juce::Graphics& g)
{
    for (int i = 0; i < 4; ++i)
    {
        const auto cell = gridCell (getLocalBounds(), i, 0, 4, 1, 4);
        drawFrame (g, "tabNormal", cell);
        if (i == selectedPage) drawFrame (g, "tabSelected", cell);
    }
}
void BarTabComponent::mouseDown (const juce::MouseEvent& event)
{
    selectedPage = juce::jlimit (0, 3, event.x * 4 / juce::jmax (1, getWidth()));
    if (onSelectedPage) onSelectedPage (selectedPage);
    repaint();
}

void BarCellComponent::configure (int number, bool isSelected, bool isPlaying)
{
    barNumber = number; selected = isSelected; playing = isPlaying; repaint();
}
void BarCellComponent::paint (juce::Graphics& g)
{
    drawFrame (g, "barNormal", getLocalBounds());
    if (playing) drawFrame (g, "barPlaying", getLocalBounds());
    else if (selected) drawFrame (g, "barSelected", getLocalBounds());
}
void BarCellComponent::mouseDown (const juce::MouseEvent&) { if (onSelected) onSelected (barNumber); }

BarMapComponent::BarMapComponent()
{
    for (int i = 0; i < 16; ++i)
    {
        cells[static_cast<size_t> (i)].onSelected = [this] (int number) { selectBar (number); };
        addAndMakeVisible (cells[static_cast<size_t> (i)]);
    }
    selectBar (selectedBar);
}
void BarMapComponent::selectBar (int number)
{
    selectedBar = number;
    if (onSelectedBar) onSelectedBar (number - 1);
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].configure (i + 1, i + 1 == selectedBar, i + 1 == playingBar);
}
void BarMapComponent::paint (juce::Graphics&) {}
void BarMapComponent::resized()
{
    const auto area = getLocalBounds().withTrimmedTop (32).withTrimmedBottom (48).reduced (9, 0);
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].setBounds (gridCell (area, i % 8, i / 8, 8, 2, 4));
}

void CountCellComponent::configure (int number, int preset, bool isSelected)
{
    countNumber = number; presetIndex = preset; selected = isSelected; repaint();
}
void CountCellComponent::paint (juce::Graphics& g)
{
    drawFrame (g, "countNormal", getLocalBounds());
    if (selected) drawFrame (g, "countSelected", getLocalBounds());
}
void CountCellComponent::mouseDown (const juce::MouseEvent&) { if (onSelected) onSelected (countNumber); }

CountGridComponent::CountGridComponent()
{
    const std::array<int, 16> presets { 0, 1, 2, 3, 2, 7, 6, 8, 1, 3, 0, 7, 7, 6, 3, 0 };
    for (int i = 0; i < 16; ++i)
    {
        cells[static_cast<size_t> (i)].onSelected = [this] (int number) { selectCount (number); };
        cells[static_cast<size_t> (i)].configure (i + 1, presets[static_cast<size_t> (i)], i + 1 == selectedCount);
        addAndMakeVisible (cells[static_cast<size_t> (i)]);
    }
}
void CountGridComponent::selectCount (int number)
{
    selectedCount = number;
    if (onSelectedCount) onSelectedCount (number - 1);
    const std::array<int, 16> presets { 0, 1, 2, 3, 2, 7, 6, 8, 1, 3, 0, 7, 7, 6, 3, 0 };
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].configure (i + 1, presets[static_cast<size_t> (i)], i + 1 == selectedCount);
}
void CountGridComponent::paint (juce::Graphics&) {}
void CountGridComponent::resized()
{
    const auto area = getLocalBounds().withTrimmedTop (28).withTrimmedBottom (35).reduced (15, 0);
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].setBounds (gridCell (area, i % 4, i / 4, 4, 4, 3));
}

XYMotionPad::XYMotionPad()
{
    normalizedMotion.add ({ 0.30f, 0.72f }); normalizedMotion.add ({ 0.46f, 0.62f });
    normalizedMotion.add ({ 0.58f, 0.45f }); normalizedMotion.add ({ 0.78f, 0.20f });
}
juce::Rectangle<float> XYMotionPad::padBounds() const
{
    return { 30.0f * getWidth() / 289.0f, 27.0f * getHeight() / 249.0f,
             243.0f * getWidth() / 289.0f, 176.0f * getHeight() / 249.0f };
}
void XYMotionPad::paint (juce::Graphics& g)
{
    const auto pad = padBounds();
    g.saveState(); g.reduceClipRegion (pad.toNearestInt());
    juce::Path motion;
    for (int i = 0; i < normalizedMotion.size(); ++i)
    {
        const auto p = normalizedMotion.getReference (i);
        const auto point = juce::Point<float> (pad.getX() + p.x * pad.getWidth(), pad.getY() + p.y * pad.getHeight());
        if (i == 0) motion.startNewSubPath (point); else motion.lineTo (point);
    }
    g.setColour (ToyotomiUi::gold().withAlpha (0.90f)); g.strokePath (motion, juce::PathStrokeType (2.0f));
    if (! normalizedMotion.isEmpty())
    {
        const auto p = normalizedMotion.getLast();
        const auto point = juce::Point<float> (pad.getX() + p.x * pad.getWidth(), pad.getY() + p.y * pad.getHeight());
        g.setColour (ToyotomiUi::gold()); g.fillEllipse (point.x - 4.0f, point.y - 4.0f, 8.0f, 8.0f);
    }
    g.restoreState();
}
void XYMotionPad::appendPoint (juce::Point<float> position)
{
    const auto pad = padBounds();
    if (normalizedMotion.size() >= PluginStateModel::kMaxMotionPoints) return;
    normalizedMotion.add ({ juce::jlimit (0.0f, 1.0f, (position.x - pad.getX()) / pad.getWidth()),
                            juce::jlimit (0.0f, 1.0f, (position.y - pad.getY()) / pad.getHeight()) });
    repaint();
}
void XYMotionPad::mouseDown (const juce::MouseEvent& event)
{
    const auto scaleX = getWidth() / 289.0f, scaleY = getHeight() / 249.0f;
    const auto clear = juce::Rectangle<int> (juce::roundToInt (18.0f * scaleX), juce::roundToInt (211.0f * scaleY), juce::roundToInt (124.0f * scaleX), juce::roundToInt (33.0f * scaleY));
    const auto reset = juce::Rectangle<int> (juce::roundToInt (156.0f * scaleX), juce::roundToInt (211.0f * scaleY), juce::roundToInt (114.0f * scaleX), juce::roundToInt (33.0f * scaleY));
    if (clear.contains (event.getPosition())) { normalizedMotion.clear(); if (onClearMotion) onClearMotion(); repaint(); return; }
    if (reset.contains (event.getPosition())) { normalizedMotion.clear(); if (onResetCount) onResetCount(); repaint(); return; }
    if (padBounds().contains (event.position)) { recording = true; normalizedMotion.clear(); appendPoint (event.position); }
}
void XYMotionPad::mouseDrag (const juce::MouseEvent& event) { if (recording) appendPoint (event.position); }
void XYMotionPad::mouseUp (const juce::MouseEvent&)
{
    if (! recording) return;
    recording = false;
    std::vector<PluginStateModel::MotionPoint> points;
    points.reserve (static_cast<size_t> (normalizedMotion.size()));
    for (const auto& p : normalizedMotion) points.push_back ({ p.x, p.y });
    if (onMotionChanged) onMotionChanged (points);
}

void ScratchPresetPalette::paint (juce::Graphics& g)
{
    const auto area = juce::Rectangle<int> (juce::roundToInt (14.0f * getWidth() / 360.0f), juce::roundToInt (32.0f * getHeight() / 313.0f), juce::roundToInt (333.0f * getWidth() / 360.0f), juce::roundToInt (257.0f * getHeight() / 313.0f));
    for (int i = 0; i < 9; ++i)
    {
        const auto cell = gridCell (area, i % 3, i / 3, 3, 3, 3);
        drawFrame (g, "presetNormal", cell);
        if (i == selectedPreset) drawFrame (g, "presetSelected", cell);
    }
}
void ScratchPresetPalette::mouseDown (const juce::MouseEvent& event)
{
    const auto area = juce::Rectangle<int> (juce::roundToInt (14.0f * getWidth() / 360.0f), juce::roundToInt (32.0f * getHeight() / 313.0f), juce::roundToInt (333.0f * getWidth() / 360.0f), juce::roundToInt (257.0f * getHeight() / 313.0f));
    for (int i = 0; i < 9; ++i) if (gridCell (area, i % 3, i / 3, 3, 3, 3).contains (event.getPosition())) { selectedPreset = i; repaint(); if (onPresetSelected) onPresetSelected (i); return; }
}

CountParameterPanel::CountParameterPanel (ToyotomiHideyoshiAudioProcessor& p) : processor (p) {}
juce::Rectangle<float> CountParameterPanel::knobBounds (int index) const
{
    return { (10.0f + 87.0f * index) * getWidth() / 270.0f, 165.0f * getHeight() / 339.0f,
             67.0f * getWidth() / 270.0f, 67.0f * getHeight() / 339.0f };
}
void CountParameterPanel::paint (juce::Graphics& g)
{
    const auto ui = processor.getStateModel().getUiState();
    const auto& count = processor.getStateModel().getCount (ui.selectedBar, ui.selectedCount);
    const auto lengths = juce::Rectangle<int> (juce::roundToInt (12.0f * getWidth() / 270.0f), juce::roundToInt (72.0f * getHeight() / 339.0f), juce::roundToInt (248.0f * getWidth() / 270.0f), juce::roundToInt (32.0f * getHeight() / 339.0f));
    for (int i = 0; i < 5; ++i)
    {
        const auto cell = gridCell (lengths, i, 0, 5, 1, 3);
        drawFrame (g, "lengthNormal", cell);
        if (i == static_cast<int> (count.length)) drawFrame (g, "lengthSelected", cell);
    }
    const std::array<float, 3> normalized { (count.speed - PluginStateModel::kMinSpeed) / (PluginStateModel::kMaxSpeed - PluginStateModel::kMinSpeed), (count.pitch - PluginStateModel::kMinPitch) / (PluginStateModel::kMaxPitch - PluginStateModel::kMinPitch), count.depth };
    const std::array<juce::String, 3> values { juce::String (count.speed, 2) + "x", juce::String (count.pitch, 1) + " st", juce::String (juce::roundToInt (count.depth * 100.0f)) + " %" };
    for (int i = 0; i < 3; ++i)
    {
        const auto knob = knobBounds (i); const auto centre = knob.getCentre();
        drawImage (g, imageFor ("knobBase"), knob.toNearestInt());
        g.saveState();
        const auto angle = juce::MathConstants<float>::pi * 1.25f + juce::MathConstants<float>::pi * 1.5f * juce::jlimit (0.0f, 1.0f, normalized[static_cast<size_t> (i)]);
        g.addTransform (juce::AffineTransform::rotation (angle + juce::MathConstants<float>::halfPi, centre.x, centre.y));
        drawImage (g, imageFor ("knobPointer"), knob.toNearestInt());
        g.restoreState();
        g.setColour (ToyotomiUi::ivory()); g.setFont (juce::Font (11.0f * getHeight() / 339.0f));
        g.drawText (values[static_cast<size_t> (i)], knob.withY (knob.getBottom() + 11.0f * getHeight() / 339.0f).withHeight (21.0f * getHeight() / 339.0f).toNearestInt(), juce::Justification::centred);
    }
}
void CountParameterPanel::updateKnob (int index, float delta)
{
    auto& state = processor.getStateModel(); const auto ui = state.getUiState(); const auto& count = state.getCount (ui.selectedBar, ui.selectedCount);
    if (index == 0) state.setCountSpeed (ui.selectedBar, ui.selectedCount, count.speed + delta * 0.012f);
    if (index == 1) state.setCountPitch (ui.selectedBar, ui.selectedCount, count.pitch + delta * 0.20f);
    if (index == 2) state.setCountDepth (ui.selectedBar, ui.selectedCount, count.depth + delta * 0.010f);
    repaint();
}
void CountParameterPanel::mouseDown (const juce::MouseEvent& event)
{
    const auto lengths = juce::Rectangle<int> (juce::roundToInt (12.0f * getWidth() / 270.0f), juce::roundToInt (72.0f * getHeight() / 339.0f), juce::roundToInt (248.0f * getWidth() / 270.0f), juce::roundToInt (32.0f * getHeight() / 339.0f));
    if (lengths.contains (event.getPosition())) { if (onLengthSelected) onLengthSelected (juce::jlimit (0, 4, (event.x - lengths.getX()) * 5 / juce::jmax (1, lengths.getWidth()))); repaint(); return; }
    for (int i = 0; i < 3; ++i) if (knobBounds (i).contains (event.position)) { activeKnob = i; dragStartY = event.position.y; return; }
}
void CountParameterPanel::mouseDoubleClick (const juce::MouseEvent& event)
{
    const auto ui = processor.getStateModel().getUiState();
    for (int i = 0; i < 3; ++i)
        if (knobBounds (i).contains (event.position))
        {
            if (i == 0) processor.getStateModel().setCountSpeed (ui.selectedBar, ui.selectedCount, 1.0f);
            if (i == 1) processor.getStateModel().setCountPitch (ui.selectedBar, ui.selectedCount, 0.0f);
            if (i == 2) processor.getStateModel().setCountDepth (ui.selectedBar, ui.selectedCount, 0.5f);
            repaint();
            return;
        }
}
void CountParameterPanel::mouseDrag (const juce::MouseEvent& event) { if (activeKnob >= 0) { updateKnob (activeKnob, dragStartY - event.position.y); dragStartY = event.position.y; } }
void CountParameterPanel::mouseUp (const juce::MouseEvent&) { activeKnob = -1; }
void CountParameterPanel::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{ for (int i = 0; i < 3; ++i) if (knobBounds (i).contains (event.position)) { updateKnob (i, wheel.deltaY * 12.0f); return; } }

OutputMeterComponent::OutputMeterComponent (ToyotomiHideyoshiAudioProcessor& p) : processor (p) { startTimerHz (30); }
float OutputMeterComponent::smooth (float current, float target) { return current + (target - current) * (target > current ? 0.62f : 0.10f); }
void OutputMeterComponent::timerCallback()
{
    const auto toDb = [] (float peak) { return juce::jlimit (-60.0f, 6.0f, juce::Decibels::gainToDecibels (juce::jmax (peak, 0.000001f), -60.0f)); };
    const auto leftTarget = toDb (processor.consumeOutputPeak (0)), rightTarget = toDb (processor.consumeOutputPeak (1));
    leftDb = smooth (leftDb, leftTarget); rightDb = smooth (rightDb, rightTarget);
    leftPeakDb = juce::jmax (leftTarget, leftPeakDb - 0.70f); rightPeakDb = juce::jmax (rightTarget, rightPeakDb - 0.70f);
    repaint();
}
void OutputMeterComponent::paint (juce::Graphics& g)
{
    const auto scaleX = getWidth() / 149.0f, scaleY = getHeight() / 360.0f;
    const auto left = juce::Rectangle<int> (juce::roundToInt (40.0f * scaleX), juce::roundToInt (61.0f * scaleY), juce::roundToInt (15.0f * scaleX), juce::roundToInt (255.0f * scaleY));
    const auto right = juce::Rectangle<int> (juce::roundToInt (84.0f * scaleX), juce::roundToInt (61.0f * scaleY), juce::roundToInt (15.0f * scaleX), juce::roundToInt (255.0f * scaleY));
    const auto drawChannel = [&] (juce::Rectangle<int> track, float level, float peak)
    {
        const auto pixels = juce::roundToInt (juce::jmap (level, -60.0f, 6.0f, 0.0f, static_cast<float> (track.getHeight())));
        if (pixels > 0) { g.saveState(); g.reduceClipRegion (track.withTop (track.getBottom() - pixels)); drawImage (g, imageFor ("meterLed"), track); g.restoreState(); }
        const auto peakY = track.getBottom() - juce::roundToInt (juce::jmap (peak, -60.0f, 6.0f, 0.0f, static_cast<float> (track.getHeight())));
        if (peak > -59.5f) { g.saveState(); g.reduceClipRegion (juce::Rectangle<int> (track.getX(), juce::jlimit (track.getY(), track.getBottom() - 2, peakY), track.getWidth(), 2)); drawImage (g, imageFor ("meterLed"), track); g.restoreState(); }
    };
    drawChannel (left, leftDb, leftPeakDb); drawChannel (right, rightDb, rightPeakDb);
}

void BottomStatusBar::paint (juce::Graphics&) {}
