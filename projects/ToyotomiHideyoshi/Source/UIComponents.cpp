#include "UIComponents.h"
#include <cmath>

namespace
{
constexpr std::array<const char*, 9> presetNames {
    "OFF", "FORWARD CUT", "BACKSPIN", "CHIRP", "BABY",
    "TRANSFORM", "DRAG", "ZIGZAG", "TAPE BRAKE"
};

juce::Rectangle<int> gridCell (juce::Rectangle<int> area, int column, int row,
                               int columns, int rows, int gap = 3)
{
    const auto width = (area.getWidth() - gap * (columns - 1)) / columns;
    const auto height = (area.getHeight() - gap * (rows - 1)) / rows;
    return { area.getX() + column * (width + gap), area.getY() + row * (height + gap), width, height };
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

juce::Font font (float height, bool bold)
{
    juce::Font result (height, bold ? juce::Font::bold : juce::Font::plain);
    result.setTypefaceName ("Bahnschrift Condensed");
    return result;
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
{
    g.setColour (panel().withAlpha (0.97f));
    g.fillRect (bounds);
    g.setColour (border());
    g.drawRect (bounds, 1.0f);

    if (title.isNotEmpty())
    {
        g.setColour (gold());
        g.setFont (font (14.0f));
        g.drawText (title, bounds.removeFromTop (28.0f).reduced (10.0f, 0.0f),
                    juce::Justification::centredLeft, false);
    }
}

void drawMotionGlyph (juce::Graphics& g, juce::Rectangle<float> bounds, int presetIndex)
{
    bounds = bounds.reduced (7.0f, 4.0f);
    juce::Path path;
    const auto x = bounds.getX();
    const auto y = bounds.getCentreY();
    const auto w = bounds.getWidth();
    const auto h = bounds.getHeight() * 0.32f;
    path.startNewSubPath (x, y);

    switch (presetIndex)
    {
        case 0: path.lineTo (x + w, y); break;
        case 1: path.lineTo (x + w * 0.25f, y); path.lineTo (x + w * 0.42f, y - h); path.lineTo (x + w * 0.72f, y - h); path.lineTo (x + w, y); break;
        case 2:
            for (int i = 1; i <= 24; ++i)
            {
                const auto t = i / 24.0f;
                path.lineTo (x + w * t, y + std::sin (t * juce::MathConstants<float>::twoPi * 2.0f) * h * (1.0f - t));
            }
            break;
        case 3: path.cubicTo (x + w * .25f, y, x + w * .45f, y - h, x + w * .72f, y - h); path.lineTo (x + w, y - h); break;
        case 4: path.cubicTo (x + w * .25f, y, x + w * .35f, y - h, x + w * .5f, y - h); path.cubicTo (x + w * .65f, y - h, x + w * .72f, y + h, x + w, y); break;
        case 5:
            for (int i = 1; i <= 24; ++i) { const auto t = i / 24.0f; path.lineTo (x + w * t, y + std::sin (t * juce::MathConstants<float>::twoPi * 2.5f) * h); }
            break;
        case 6: path.cubicTo (x + w * .25f, y, x + w * .65f, y - h, x + w, y - h); break;
        case 7:
            for (int i = 1; i <= 6; ++i) path.lineTo (x + w * i / 6.0f, y + (i % 2 == 0 ? h : -h));
            break;
        case 8:
            for (int i = 1; i <= 6; ++i) path.lineTo (x + w * i / 6.0f, y - h + h * i / 3.0f);
            break;
        default:
            path.startNewSubPath (bounds.getCentreX(), bounds.getY());
            path.lineTo (bounds.getCentreX(), bounds.getBottom());
            path.startNewSubPath (bounds.getX(), bounds.getCentreY());
            path.lineTo (bounds.getRight(), bounds.getCentreY());
            break;
    }

    g.setColour (ivory().withAlpha (0.82f));
    g.strokePath (path, juce::PathStrokeType (1.25f));
}
}

TopBarComponent::TopBarComponent (ToyotomiHideyoshiAudioProcessor& p) : processor (p)
{
    startTimerHz (15);
}

void TopBarComponent::timerCallback()
{
    bpm = processor.getHostBpm();
    numerator = processor.getTimeSignatureNumerator();
    denominator = processor.getTimeSignatureDenominator();
    hostSync = processor.getHostSyncAvailable();
    repaint();
}

void TopBarComponent::paint (juce::Graphics& g)
{
    // These value wells cover the values baked into the image-only reference;
    // labels and the surrounding engraved chrome intentionally remain visible.
    const auto coverValue = [&] (juce::Rectangle<int> bounds, const juce::String& value,
                                 juce::Colour colour)
    {
        g.setColour (ToyotomiUi::background().withAlpha (0.92f));
        g.fillRect (bounds);
        g.setColour (colour);
        g.setFont (ToyotomiUi::font (16.0f, true));
        g.drawText (value, bounds, juce::Justification::centred);
    };

    coverValue ({ 435, 37, 74, 24 }, hostSync ? "ON" : "OFF", hostSync ? ToyotomiUi::red() : ToyotomiUi::muted());
    coverValue ({ 579, 37, 88, 24 }, juce::String (bpm, 2), ToyotomiUi::ivory());
    coverValue ({ 702, 37, 90, 24 }, juce::String (numerator) + "/" + juce::String (denominator), ToyotomiUi::ivory());
    coverValue ({ 830, 37, 215, 24 }, "CUSTOM PROJECT", ToyotomiUi::ivory());

    const auto bypass = juce::Rectangle<int> (1172, 28, 68, 33);
    const auto active = processor.getStateModel().getUiState().bypass;
    g.setColour (ToyotomiUi::background().withAlpha (0.94f));
    g.fillRoundedRectangle (bypass.toFloat(), 8.0f);
    g.setColour ((active ? ToyotomiUi::red() : ToyotomiUi::border()).withAlpha (0.86f));
    g.fillRoundedRectangle (bypass.toFloat(), 8.0f);
    g.setColour (active ? ToyotomiUi::red() : ToyotomiUi::gold().withAlpha (0.55f));
    g.drawRoundedRectangle (bypass.toFloat(), 8.0f, 1.2f);
    g.setColour (active ? ToyotomiUi::ivory() : ToyotomiUi::muted());
    g.setFont (ToyotomiUi::font (10.0f, true));
    g.drawText (active ? "ON" : "OFF", bypass, juce::Justification::centred);
}

void TopBarComponent::mouseDown (const juce::MouseEvent& event)
{
    if (juce::Rectangle<int> (1172, 28, 68, 33).contains (event.getPosition()))
    {
        const auto current = processor.getStateModel().getUiState().bypass;
        processor.getStateModel().setBypass (! current);
        repaint();
    }
}

void ArtworkPanel::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g, source);
}

void BarTabComponent::paint (juce::Graphics& g)
{
    const std::array<const char*, 4> labels { "1-16", "17-32", "33-48", "49-64" };
    for (int i = 0; i < 4; ++i)
    {
        auto cell = gridCell (getLocalBounds(), i, 0, 4, 1, 4);
        g.setColour (ToyotomiUi::background().withAlpha (0.97f));
        g.fillRect (cell);
        g.setColour (i == selectedPage ? ToyotomiUi::gold() : ToyotomiUi::border());
        g.drawRect (cell, i == selectedPage ? 2 : 1);
        g.setColour (i == selectedPage ? ToyotomiUi::gold() : ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (15.0f, true));
        g.drawText (labels[static_cast<size_t> (i)], cell, juce::Justification::centred);
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
    auto body = getLocalBounds().reduced (1);
    g.setColour (ToyotomiUi::background().withAlpha (0.97f));
    g.fillRect (body);
    g.setColour (playing ? ToyotomiUi::red() : selected ? ToyotomiUi::gold() : ToyotomiUi::border());
    g.drawRect (getLocalBounds(), playing || selected ? 2 : 1);
    g.setColour (playing ? ToyotomiUi::red() : selected ? ToyotomiUi::gold() : ToyotomiUi::ivory());
    g.setFont (ToyotomiUi::font (13.0f, true));
    g.drawText ("BAR " + juce::String (barNumber), body.removeFromTop (26), juce::Justification::centred);
    ToyotomiUi::drawMotionGlyph (g, body.withTrimmedTop (24).withTrimmedBottom (19).toFloat(), barNumber % 9);
    if (playing)
    {
        g.setColour (ToyotomiUi::red());
        g.setFont (ToyotomiUi::font (11.0f, true));
        g.drawText ("PLAYING", body.withTrimmedBottom (2).removeFromBottom (19), juce::Justification::centred);
    }
    else if (selected)
    {
        g.setColour (ToyotomiUi::gold());
        g.setFont (ToyotomiUi::font (11.0f, true));
        g.drawText ("SELECTED", body.withTrimmedBottom (2).removeFromBottom (19), juce::Justification::centred);
    }
    g.setColour (playing ? ToyotomiUi::red() : selected ? ToyotomiUi::gold() : ToyotomiUi::muted());
    g.fillEllipse ((float) getWidth() * 0.5f - 3.0f, (float) getHeight() - 15.0f, 6.0f, 6.0f);
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
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].configure (i + 1, i + 1 == selectedBar, i + 1 == playingBar);
    repaint();
}

void BarMapComponent::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void BarMapComponent::resized()
{
    auto area = getLocalBounds().withTrimmedTop (32).withTrimmedBottom (48).reduced (9, 0);
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].setBounds (gridCell (area, i % 8, i / 8, 8, 2, 4));
}

void CountCellComponent::configure (int number, int preset, bool isSelected)
{
    countNumber = number; presetIndex = preset; selected = isSelected; repaint();
}

void CountCellComponent::paint (juce::Graphics& g)
{
    auto body = getLocalBounds().reduced (1);
    g.setColour (ToyotomiUi::background().withAlpha (0.97f));
    g.fillRect (body);
    g.setColour (selected ? ToyotomiUi::gold() : ToyotomiUi::border());
    g.drawRect (getLocalBounds(), selected ? 2 : 1);
    g.setColour (selected ? ToyotomiUi::gold() : ToyotomiUi::ivory());
    g.setFont (ToyotomiUi::font (15.0f, true));
    g.drawText (juce::String (countNumber), body.removeFromTop (25), juce::Justification::centred);
    ToyotomiUi::drawMotionGlyph (g, body.withTrimmedTop (24).withTrimmedBottom (24).toFloat(), presetIndex);
    g.setColour (selected ? ToyotomiUi::gold() : ToyotomiUi::ivory());
    g.setFont (ToyotomiUi::font (11.0f, true));
    g.drawText (presetNames[static_cast<size_t> (juce::jlimit (0, 8, presetIndex))],
                body.withTrimmedBottom (2).removeFromBottom (21), juce::Justification::centred);
    if (selected)
    {
        g.setColour (ToyotomiUi::gold().withAlpha (0.18f));
        g.fillRect (getLocalBounds().reduced (2));
    }
}

void CountCellComponent::mouseDown (const juce::MouseEvent&) { if (onSelected) onSelected (countNumber); }

CountGridComponent::CountGridComponent()
{
    const std::array<int, 16> presets { 0,1,2,3,2,7,6,8,1,3,0,7,7,6,3,0 };
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
    const std::array<int, 16> presets { 0,1,2,3,2,7,6,8,1,3,0,7,7,6,3,0 };
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].configure (i + 1, presets[static_cast<size_t> (i)], i + 1 == selectedCount);
    repaint();
}

void CountGridComponent::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g, selectedCount);
}

void CountGridComponent::resized()
{
    auto area = getLocalBounds().withTrimmedTop (28).withTrimmedBottom (35).reduced (15, 0);
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].setBounds (gridCell (area, i % 4, i / 4, 4, 4, 3));
}

XYMotionPad::XYMotionPad()
{
    normalizedMotion.add ({ 0.30f, 0.72f });
    normalizedMotion.add ({ 0.46f, 0.62f });
    normalizedMotion.add ({ 0.58f, 0.45f });
    normalizedMotion.add ({ 0.78f, 0.20f });
}

juce::Rectangle<float> XYMotionPad::padBounds() const
{
    // Measured from the 1280 x 853 faceplate: this excludes the title,
    // direction labels, and the two physical action buttons below the pad.
    const auto scaleX = getWidth() / 289.0f;
    const auto scaleY = getHeight() / 249.0f;
    return { 30.0f * scaleX, 27.0f * scaleY, 243.0f * scaleX, 176.0f * scaleY };
}

void XYMotionPad::paint (juce::Graphics& g)
{
    const auto pad = padBounds();
    // The glass, frame and grid are image-first. Only recorded motion is
    // dynamic, so the pad never acquires a second, offset frame.
    g.saveState();
    g.reduceClipRegion (pad.toNearestInt());

    juce::Path motion;
    for (int i = 0; i < normalizedMotion.size(); ++i)
    {
        const auto p = normalizedMotion.getReference (i);
        const auto screen = juce::Point<float> (pad.getX() + p.x * pad.getWidth(), pad.getY() + p.y * pad.getHeight());
        if (i == 0) motion.startNewSubPath (screen); else motion.lineTo (screen);
    }
    g.setColour (ToyotomiUi::gold().withAlpha (0.90f));
    g.strokePath (motion, juce::PathStrokeType (2.0f));
    if (! normalizedMotion.isEmpty())
    {
        const auto p = normalizedMotion.getLast();
        const auto point = juce::Point<float> (pad.getX() + p.x * pad.getWidth(), pad.getY() + p.y * pad.getHeight());
        g.setColour (ToyotomiUi::gold().withAlpha (0.18f));
        g.fillEllipse (point.x - 10.0f, point.y - 10.0f, 20.0f, 20.0f);
        g.setColour (ToyotomiUi::gold());
        g.drawEllipse (point.x - 5.0f, point.y - 5.0f, 10.0f, 10.0f, 2.0f);
    }
    g.restoreState();
}

void XYMotionPad::appendPoint (juce::Point<float> position)
{
    const auto pad = padBounds();
    if (normalizedMotion.size() >= 256) return;
    normalizedMotion.add ({ juce::jlimit (0.0f, 1.0f, (position.x - pad.getX()) / pad.getWidth()),
                            juce::jlimit (0.0f, 1.0f, (position.y - pad.getY()) / pad.getHeight()) });
    repaint();
}

void XYMotionPad::mouseDown (const juce::MouseEvent& event)
{
    const auto scaleX = getWidth() / 289.0f;
    const auto scaleY = getHeight() / 249.0f;
    const auto clear = juce::Rectangle<int> (juce::roundToInt (18.0f * scaleX), juce::roundToInt (211.0f * scaleY),
                                             juce::roundToInt (124.0f * scaleX), juce::roundToInt (33.0f * scaleY));
    const auto reset = juce::Rectangle<int> (juce::roundToInt (156.0f * scaleX), juce::roundToInt (211.0f * scaleY),
                                             juce::roundToInt (114.0f * scaleX), juce::roundToInt (33.0f * scaleY));
    if (clear.contains (event.getPosition()))
    {
        normalizedMotion.clearQuick();
        if (onClearMotion) onClearMotion();
        repaint();
        return;
    }
    if (reset.contains (event.getPosition()))
    {
        normalizedMotion.clearQuick();
        if (onResetCount) onResetCount();
        repaint();
        return;
    }
    recording = padBounds().contains (event.position);
    if (recording) { normalizedMotion.clearQuick(); appendPoint (event.position); }
}
void XYMotionPad::mouseDrag (const juce::MouseEvent& event) { if (recording) appendPoint (event.position); }
void XYMotionPad::mouseUp (const juce::MouseEvent&)
{
    recording = false;
    if (onMotionChanged)
    {
        std::vector<PluginStateModel::MotionPoint> points;
        points.reserve ((size_t) normalizedMotion.size());
        for (const auto& point : normalizedMotion) points.push_back ({ point.x, point.y });
        onMotionChanged (points);
    }
}

void ScratchPresetPalette::paint (juce::Graphics& g)
{
    const auto scaleX = getWidth() / 360.0f;
    const auto scaleY = getHeight() / 313.0f;
    const auto area = juce::Rectangle<int> (juce::roundToInt (14.0f * scaleX), juce::roundToInt (32.0f * scaleY),
                                            juce::roundToInt (333.0f * scaleX), juce::roundToInt (257.0f * scaleY));
    for (int i = 0; i < 9; ++i)
    {
        auto cell = gridCell (area, i % 3, i / 3, 3, 3, 3);
        g.setColour (ToyotomiUi::background().withAlpha (0.97f));
        g.fillRect (cell);
        g.setColour (i == selectedPreset ? ToyotomiUi::gold() : ToyotomiUi::border());
        g.drawRect (cell, i == selectedPreset ? 2 : 1);
        ToyotomiUi::drawMotionGlyph (g, cell.reduced (12, 16).toFloat(), i);
        g.setColour (i == selectedPreset ? ToyotomiUi::gold() : ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (12.0f * scaleY, true));
        g.drawText (presetNames[static_cast<size_t> (i)], cell.removeFromBottom (23), juce::Justification::centred);
    }
}

void ScratchPresetPalette::mouseDown (const juce::MouseEvent& event)
{
    const auto scaleX = getWidth() / 360.0f;
    const auto scaleY = getHeight() / 313.0f;
    const auto area = juce::Rectangle<int> (juce::roundToInt (14.0f * scaleX), juce::roundToInt (32.0f * scaleY),
                                            juce::roundToInt (333.0f * scaleX), juce::roundToInt (257.0f * scaleY));
    for (int i = 0; i < 9; ++i) if (gridCell (area, i % 3, i / 3, 3, 3, 3).contains (event.getPosition())) { selectedPreset = i; repaint(); break; }
    if (onPresetSelected) onPresetSelected (selectedPreset);
}

CountParameterPanel::CountParameterPanel (ToyotomiHideyoshiAudioProcessor& p) : processor (p) {}

juce::Rectangle<float> CountParameterPanel::knobBounds (int index) const
{
    // speed/pitch/depth are equal 67 px circles at x=10/97/184 and y=165
    // within the measured 270 x 339 parameter panel.
    const auto scaleX = getWidth() / 270.0f;
    const auto scaleY = getHeight() / 339.0f;
    return { (10.0f + 87.0f * index) * scaleX, 165.0f * scaleY,
             67.0f * scaleX, 67.0f * scaleY };
}

void CountParameterPanel::paint (juce::Graphics& g)
{
    const auto& state = processor.getStateModel();
    const auto ui = state.getUiState();
    const auto& count = state.getCount (ui.selectedBar, ui.selectedCount);
    const auto scaleX = getWidth() / 270.0f;
    const auto scaleY = getHeight() / 339.0f;
    auto lengths = juce::Rectangle<int> (juce::roundToInt (12.0f * scaleX), juce::roundToInt (72.0f * scaleY),
                                         juce::roundToInt (248.0f * scaleX), juce::roundToInt (32.0f * scaleY));
    for (int i = 0; i < 5; ++i)
    {
        const auto cell = gridCell (lengths, i, 0, 5, 1, 3);
        const auto selected = i == static_cast<int> (count.length);
        g.setColour (ToyotomiUi::background().withAlpha (0.96f));
        g.fillRect (cell);
        g.setColour (selected ? ToyotomiUi::gold() : ToyotomiUi::border());
        g.drawRect (cell, selected ? 2 : 1);
    }

    const std::array<float, 3> normalized {
        count.speed / PluginStateModel::kMaxSpeed,
        (count.pitch - PluginStateModel::kMinPitch) / (PluginStateModel::kMaxPitch - PluginStateModel::kMinPitch),
        count.depth
    };
    const std::array<juce::String, 3> values {
        juce::String (count.speed, 2) + "x",
        juce::String (count.pitch, 1) + " st",
        juce::String (juce::roundToInt (count.depth * 100.0f)) + " %"
    };

    for (int i = 0; i < 3; ++i)
    {
        const auto knob = knobBounds (i);
        const auto center = knob.getCentre();
        const auto radius = knob.getWidth() * 0.5f;
        const auto start = juce::MathConstants<float>::pi * 1.25f;
        const auto sweep = juce::MathConstants<float>::pi * 1.5f;
        g.setColour (ToyotomiUi::background().withAlpha (0.97f));
        g.fillEllipse (knob.expanded (4.0f));
        g.setColour (ToyotomiUi::gold().withAlpha (0.62f));
        g.drawEllipse (knob.expanded (4.0f), 1.2f);
        g.setColour (ToyotomiUi::border());
        g.fillEllipse (knob.reduced (2.0f));
        for (int tick = 0; tick <= 10; ++tick)
        {
            const auto angle = start + sweep * tick / 10.0f;
            const auto outer = center + juce::Point<float> (std::cos (angle), std::sin (angle)) * (radius + 7.0f);
            const auto inner = center + juce::Point<float> (std::cos (angle), std::sin (angle)) * (radius + (tick % 5 == 0 ? 2.0f : 4.0f));
            g.setColour (ToyotomiUi::muted().withAlpha (0.80f));
            g.drawLine (juce::Line<float> (inner, outer), tick % 5 == 0 ? 1.5f : 0.8f);
        }
        const auto pointerAngle = start + sweep * juce::jlimit (0.0f, 1.0f, normalized[static_cast<size_t> (i)]);
        const auto pointerEnd = center + juce::Point<float> (std::cos (pointerAngle), std::sin (pointerAngle)) * (radius * 0.72f);
        g.setColour (ToyotomiUi::red());
        g.drawLine (juce::Line<float> (center, pointerEnd), 2.2f);
        g.setColour (ToyotomiUi::gold().withAlpha (0.28f));
        g.fillEllipse (center.x - 5.0f, center.y - 5.0f, 10.0f, 10.0f);
        g.setColour (ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (11.0f, true));
        g.drawText (values[static_cast<size_t> (i)], knob.withY (knob.getBottom() + 11.0f * scaleY).withHeight (21.0f * scaleY).toNearestInt(), juce::Justification::centred);
    }
}

void CountParameterPanel::updateKnob (int index, float delta)
{
    auto& state = processor.getStateModel();
    const auto ui = state.getUiState();
    const auto& count = state.getCount (ui.selectedBar, ui.selectedCount);
    if (index == 0) state.setCountSpeed (ui.selectedBar, ui.selectedCount, count.speed + delta * 0.012f);
    if (index == 1) state.setCountPitch (ui.selectedBar, ui.selectedCount, count.pitch + delta * 0.20f);
    if (index == 2) state.setCountDepth (ui.selectedBar, ui.selectedCount, count.depth + delta * 0.010f);
    repaint();
}

void CountParameterPanel::mouseDown (const juce::MouseEvent& event)
{
    const auto scaleX = getWidth() / 270.0f;
    const auto scaleY = getHeight() / 339.0f;
    auto lengths = juce::Rectangle<int> (juce::roundToInt (12.0f * scaleX), juce::roundToInt (72.0f * scaleY),
                                         juce::roundToInt (248.0f * scaleX), juce::roundToInt (32.0f * scaleY));
    if (lengths.contains (event.getPosition()))
    {
        const auto length = juce::jlimit (0, 4, (event.x - lengths.getX()) * 5 / juce::jmax (1, lengths.getWidth()));
        if (onLengthSelected) onLengthSelected (length);
        repaint();
        return;
    }
    for (int i = 0; i < 3; ++i)
        if (knobBounds (i).expanded (8.0f).contains (event.position)) { activeKnob = i; dragStartY = event.position.y; return; }
}

void CountParameterPanel::mouseDrag (const juce::MouseEvent& event)
{
    if (activeKnob >= 0) { updateKnob (activeKnob, dragStartY - event.position.y); dragStartY = event.position.y; }
}

void CountParameterPanel::mouseUp (const juce::MouseEvent&) { activeKnob = -1; }

void CountParameterPanel::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    for (int i = 0; i < 3; ++i)
        if (knobBounds (i).expanded (8.0f).contains (event.position)) { updateKnob (i, wheel.deltaY * 12.0f); return; }
}

OutputMeterComponent::OutputMeterComponent (ToyotomiHideyoshiAudioProcessor& p) : processor (p) { startTimerHz (30); }

float OutputMeterComponent::smooth (float current, float target)
{
    const auto coefficient = target > current ? 0.62f : 0.10f;
    return current + (target - current) * coefficient;
}

void OutputMeterComponent::timerCallback()
{
    const auto toDb = [] (float peak) { return juce::jlimit (-60.0f, 6.0f, juce::Decibels::gainToDecibels (juce::jmax (peak, 0.000001f), -60.0f)); };
    leftDb = smooth (leftDb, toDb (processor.consumeOutputPeak (0)));
    rightDb = smooth (rightDb, toDb (processor.consumeOutputPeak (1)));
    repaint();
}

void OutputMeterComponent::paint (juce::Graphics& g)
{
    // Fixed frame, dB legend and L/R headings remain in the faceplate. The
    // two wells and numeric readouts are fully covered and replaced here.
    const auto scaleX = getWidth() / 149.0f;
    const auto scaleY = getHeight() / 360.0f;
    const auto leftTrack = juce::Rectangle<int> (juce::roundToInt (40.0f * scaleX), juce::roundToInt (61.0f * scaleY),
                                                 juce::roundToInt (15.0f * scaleX), juce::roundToInt (255.0f * scaleY));
    const auto rightTrack = juce::Rectangle<int> (juce::roundToInt (84.0f * scaleX), juce::roundToInt (61.0f * scaleY),
                                                  juce::roundToInt (15.0f * scaleX), juce::roundToInt (255.0f * scaleY));

    const auto drawChannel = [&] (juce::Rectangle<int> track, float level)
    {
        g.setColour (juce::Colours::black.withAlpha (0.88f));
        g.fillRoundedRectangle (track.expanded (2, 2).toFloat(), 1.5f);
        g.setColour (ToyotomiUi::gold().withAlpha (0.38f));
        g.drawRoundedRectangle (track.expanded (2, 2).toFloat(), 1.5f, 1.0f);

        const auto segments = 28;
        const auto active = juce::roundToInt (juce::jmap (level, -60.0f, 6.0f, 0.0f, static_cast<float> (segments)));
        for (int i = 0; i < segments; ++i)
        {
            const auto segmentHeight = track.getHeight() / segments;
            auto segment = juce::Rectangle<int> (track.getX() + 2,
                                                  track.getBottom() - (i + 1) * segmentHeight + 1,
                                                  juce::jmax (2, track.getWidth() - 4),
                                                  juce::jmax (1, segmentHeight - 2));
            const auto lit = i < active;
            const auto colour = i > 23 ? ToyotomiUi::red() : i > 17 ? ToyotomiUi::gold() : juce::Colour (0xff4d9a46);
            g.setColour (lit ? colour.brighter (0.10f) : juce::Colour (0xff171917));
            g.fillRect (segment);
        }
    };
    drawChannel (leftTrack, leftDb);
    drawChannel (rightTrack, rightDb);

    const auto drawValue = [&] (juce::Rectangle<float> area, float level)
    {
        g.setColour (ToyotomiUi::background().withAlpha (0.97f));
        g.fillRect (area);
        g.setColour (ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (10.0f * scaleY, true));
        g.drawText (juce::String (level, 1), area.toNearestInt(), juce::Justification::centred);
    };
    drawValue ({ 25.0f * scaleX, 328.0f * scaleY, 42.0f * scaleX, 23.0f * scaleY }, leftDb);
    drawValue ({ 72.0f * scaleX, 328.0f * scaleY, 42.0f * scaleX, 23.0f * scaleY }, rightDb);
}

void BottomStatusBar::paint (juce::Graphics& g)
{
    const auto scaleX = getWidth() / 1270.0f;
    const auto scaleY = getHeight() / 76.0f;
    const auto drawStatus = [&] (juce::Rectangle<float> area, const juce::String& value, juce::Colour colour)
    {
        g.setColour (ToyotomiUi::background().withAlpha (0.94f));
        g.fillRect (area);
        g.setColour (colour);
        g.setFont (ToyotomiUi::font (12.0f * scaleY));
        g.drawText (value, area.toNearestInt(), juce::Justification::centredLeft);
    };
    drawStatus ({ 256.0f * scaleX, 20.0f * scaleY, 255.0f * scaleX, 31.0f * scaleY }, "PROJECT  Toyotomi Hideyoshi", ToyotomiUi::ivory());
    drawStatus ({ 538.0f * scaleX, 20.0f * scaleY, 150.0f * scaleX, 31.0f * scaleY }, "VERSION  0.1.0", ToyotomiUi::ivory());
    drawStatus ({ 723.0f * scaleX, 20.0f * scaleY, 180.0f * scaleX, 31.0f * scaleY }, "MODE  SEQUENCER", ToyotomiUi::ivory());
    drawStatus ({ 920.0f * scaleX, 20.0f * scaleY, 185.0f * scaleX, 31.0f * scaleY }, "STATUS  PASS THROUGH", ToyotomiUi::gold());
}
