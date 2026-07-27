#include "UIComponents.h"
#include <cmath>

namespace
{
constexpr std::array<const char*, 10> presetNames {
    "OFF", "FORWARD CUT", "BACKSPIN", "CHIRP", "BABY",
    "TRANSFORM", "DRAG", "ZIGZAG", "TAPE BRAKE", "CUSTOM"
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
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat());
    auto area = getLocalBounds().reduced (18, 8);
    auto brand = area.removeFromLeft (430);
    g.setColour (ToyotomiUi::gold());
    g.setFont (ToyotomiUi::font (31.0f, true));
    g.drawText ("Toyotomi Hideyoshi", brand.removeFromTop (40), juce::Justification::centredLeft);
    g.setColour (ToyotomiUi::red().darker (0.1f));
    g.setFont (ToyotomiUi::font (13.0f, true));
    g.drawText ("TOYOTOMI HIDEYOSHI   BUCK / KRUMP SCRATCH SEQUENCER", brand,
                juce::Justification::centredLeft);

    const auto statusWidth = juce::jmax (90, area.getWidth() / 7);
    auto drawStatus = [&] (const juce::String& label, const juce::String& value)
    {
        auto box = area.removeFromLeft (statusWidth).reduced (8, 0);
        g.setColour (ToyotomiUi::muted());
        g.setFont (ToyotomiUi::font (11.0f));
        g.drawText (label, box.removeFromTop (20), juce::Justification::centredLeft);
        g.setColour (ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (18.0f));
        g.drawText (value, box, juce::Justification::centredLeft);
    };

    drawStatus ("HOST SYNC", hostSync ? "ON" : "--");
    drawStatus ("BPM", juce::String (bpm, 2));
    drawStatus ("TIME SIG", juce::String (numerator) + "/" + juce::String (denominator));
    drawStatus ("PRESET", "Init");
    drawStatus ("BYPASS", "OFF");
}

void ArtworkPanel::paint (juce::Graphics& g)
{
    g.fillAll (ToyotomiUi::background());
    if (source.isValid())
        g.drawImage (source, 0, 0, getWidth(), getHeight(), 5, 89, 307, 416, false);
    g.setColour (ToyotomiUi::border());
    g.drawRect (getLocalBounds());
}

void BarTabComponent::paint (juce::Graphics& g)
{
    const std::array<const char*, 4> labels { "1-16", "17-32", "33-48", "49-64" };
    for (int i = 0; i < 4; ++i)
    {
        auto cell = gridCell (getLocalBounds(), i, 0, 4, 1, 4);
        g.setColour (i == selectedPage ? ToyotomiUi::gold().withAlpha (0.22f) : ToyotomiUi::panel());
        g.fillRect (cell);
        g.setColour (i == selectedPage ? ToyotomiUi::gold() : ToyotomiUi::border());
        g.drawRect (cell, i == selectedPage ? 2 : 1);
        g.setColour (i == selectedPage ? ToyotomiUi::gold() : ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (15.0f));
        g.drawText (labels[static_cast<size_t> (i)], cell, juce::Justification::centred);
    }
}

void BarTabComponent::mouseDown (const juce::MouseEvent& event)
{
    selectedPage = juce::jlimit (0, 3, event.x * 4 / juce::jmax (1, getWidth()));
    repaint();
}

void BarCellComponent::configure (int number, bool isSelected, bool isPlaying)
{
    barNumber = number; selected = isSelected; playing = isPlaying; repaint();
}

void BarCellComponent::paint (juce::Graphics& g)
{
    g.fillAll (ToyotomiUi::panel());
    g.setColour (playing ? ToyotomiUi::red() : selected ? ToyotomiUi::gold() : ToyotomiUi::border());
    g.drawRect (getLocalBounds(), playing || selected ? 2 : 1);
    g.setFont (ToyotomiUi::font (12.0f));
    g.drawText ("BAR " + juce::String (barNumber), getLocalBounds().removeFromTop (22), juce::Justification::centred);
    ToyotomiUi::drawMotionGlyph (g, getLocalBounds().toFloat().reduced (7.0f, 18.0f), barNumber % 9);
    g.setFont (ToyotomiUi::font (11.0f, true));
    if (playing) g.drawText ("PLAYING", getLocalBounds().removeFromBottom (22), juce::Justification::centred);
    else if (selected) g.drawText ("SELECTED", getLocalBounds().removeFromBottom (22), juce::Justification::centred);
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
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].configure (i + 1, i + 1 == selectedBar, i + 1 == playingBar);
    repaint();
}

void BarMapComponent::paint (juce::Graphics& g)
{
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat(), "BAR MAP");
    auto footer = getLocalBounds().removeFromBottom (35).reduced (10, 0);
    g.setFont (ToyotomiUi::font (13.0f));
    g.setColour (ToyotomiUi::red());
    g.drawText ("> PLAYING BAR:  " + juce::String (playingBar), footer.removeFromLeft (footer.getWidth() / 2), juce::Justification::centredLeft);
    g.setColour (ToyotomiUi::gold());
    g.drawText ("SELECTED BAR:  " + juce::String (selectedBar), footer, juce::Justification::centred);
}

void BarMapComponent::resized()
{
    auto area = getLocalBounds().withTrimmedTop (32).withTrimmedBottom (42).reduced (7, 0);
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].setBounds (gridCell (area, i % 8, i / 8, 8, 2, 4));
}

void CountCellComponent::configure (int number, int preset, bool isSelected)
{
    countNumber = number; presetIndex = preset; selected = isSelected; repaint();
}

void CountCellComponent::paint (juce::Graphics& g)
{
    g.fillAll (ToyotomiUi::panel());
    g.setColour (selected ? ToyotomiUi::gold() : ToyotomiUi::border());
    g.drawRect (getLocalBounds(), selected ? 2 : 1);
    g.setFont (ToyotomiUi::font (15.0f));
    g.drawText (juce::String (countNumber), getLocalBounds().removeFromTop (24), juce::Justification::centred);
    ToyotomiUi::drawMotionGlyph (g, getLocalBounds().toFloat().reduced (12.0f, 19.0f), presetIndex);
    g.setFont (ToyotomiUi::font (11.0f));
    g.drawText (presetNames[static_cast<size_t> (presetIndex)], getLocalBounds().removeFromBottom (21), juce::Justification::centred);
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
    const std::array<int, 16> presets { 0,1,2,3,2,7,6,8,1,3,0,7,7,6,3,0 };
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].configure (i + 1, presets[static_cast<size_t> (i)], i + 1 == selectedCount);
    repaint();
}

void CountGridComponent::paint (juce::Graphics& g)
{
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat(), "COUNT GRID - BAR 11");
    g.setColour (ToyotomiUi::gold());
    g.setFont (ToyotomiUi::font (14.0f));
    g.drawText ("SELECTED COUNT:  " + juce::String (selectedCount), getLocalBounds().removeFromBottom (22), juce::Justification::centred);
}

void CountGridComponent::resized()
{
    auto area = getLocalBounds().withTrimmedTop (32).withTrimmedBottom (25).reduced (10, 0);
    for (int i = 0; i < 16; ++i) cells[static_cast<size_t> (i)].setBounds (gridCell (area, i % 4, i / 4, 4, 4, 3));
}

XYMotionPad::XYMotionPad()
{
    normalizedMotion.add ({ 0.30f, 0.72f });
    normalizedMotion.add ({ 0.46f, 0.62f });
    normalizedMotion.add ({ 0.58f, 0.45f });
    normalizedMotion.add ({ 0.78f, 0.20f });
}

juce::Rectangle<float> XYMotionPad::padBounds() const { return getLocalBounds().toFloat().reduced (35.0f, 38.0f).withTrimmedBottom (18.0f); }

void XYMotionPad::paint (juce::Graphics& g)
{
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat(), "XY PAD (COUNT 5)");
    const auto pad = padBounds();
    g.setColour (ToyotomiUi::border());
    g.drawRect (pad);
    g.drawLine (pad.getCentreX(), pad.getY(), pad.getCentreX(), pad.getBottom());
    g.drawLine (pad.getX(), pad.getCentreY(), pad.getRight(), pad.getCentreY());
    for (int i = 1; i <= 3; ++i) g.drawEllipse (pad.withSizeKeepingCentre (pad.getWidth() * i / 3.0f, pad.getHeight() * i / 3.0f), 0.7f);
    g.setColour (ToyotomiUi::ivory());
    g.setFont (ToyotomiUi::font (11.0f));
    g.drawText ("HIGH", pad.withHeight (16).translated (0, -18), juce::Justification::centred);
    g.drawText ("LOW", pad.withHeight (16).withY (pad.getBottom() + 2), juce::Justification::centred);

    juce::Path motion;
    for (int i = 0; i < normalizedMotion.size(); ++i)
    {
        const auto p = normalizedMotion.getReference (i);
        const auto screen = juce::Point<float> (pad.getX() + p.x * pad.getWidth(), pad.getY() + p.y * pad.getHeight());
        if (i == 0) motion.startNewSubPath (screen); else motion.lineTo (screen);
    }
    g.setColour (ToyotomiUi::gold());
    g.strokePath (motion, juce::PathStrokeType (1.5f));
    if (! normalizedMotion.isEmpty())
    {
        const auto p = normalizedMotion.getLast();
        g.drawEllipse ({ pad.getX() + p.x * pad.getWidth() - 5.0f, pad.getY() + p.y * pad.getHeight() - 5.0f, 10.0f, 10.0f }, 2.0f);
    }
}

void XYMotionPad::appendPoint (juce::Point<float> position)
{
    const auto pad = padBounds();
    if (normalizedMotion.size() >= 256) return;
    normalizedMotion.add ({ juce::jlimit (0.0f, 1.0f, (position.x - pad.getX()) / pad.getWidth()),
                            juce::jlimit (0.0f, 1.0f, (position.y - pad.getY()) / pad.getHeight()) });
    repaint();
}

void XYMotionPad::mouseDown (const juce::MouseEvent& event) { recording = padBounds().contains (event.position); if (recording) { normalizedMotion.clearQuick(); appendPoint (event.position); } }
void XYMotionPad::mouseDrag (const juce::MouseEvent& event) { if (recording) appendPoint (event.position); }
void XYMotionPad::mouseUp (const juce::MouseEvent&) { recording = false; }

void ScratchPresetPalette::paint (juce::Graphics& g)
{
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat(), "COUNT 5 PRESET");
    auto area = getLocalBounds().withTrimmedTop (36).reduced (8, 0);
    const auto rows = 4;
    for (int i = 0; i < 10; ++i)
    {
        auto cell = gridCell (area, i % 3, i / 3, 3, rows, 3);
        g.setColour (i == selectedPreset ? ToyotomiUi::gold().withAlpha (0.15f) : ToyotomiUi::panel());
        g.fillRect (cell);
        g.setColour (i == selectedPreset ? ToyotomiUi::gold() : ToyotomiUi::border());
        g.drawRect (cell, i == selectedPreset ? 2 : 1);
        ToyotomiUi::drawMotionGlyph (g, cell.toFloat().withTrimmedBottom (22.0f), i);
        g.setColour (i == selectedPreset ? ToyotomiUi::gold() : ToyotomiUi::ivory());
        g.setFont (ToyotomiUi::font (11.0f));
        g.drawText (presetNames[static_cast<size_t> (i)], cell.removeFromBottom (23), juce::Justification::centred);
    }
}

void ScratchPresetPalette::mouseDown (const juce::MouseEvent& event)
{
    auto area = getLocalBounds().withTrimmedTop (36).reduced (8, 0);
    for (int i = 0; i < 10; ++i) if (gridCell (area, i % 3, i / 3, 3, 4, 3).contains (event.getPosition())) { selectedPreset = i; repaint(); break; }
}

void CountParameterPanel::paint (juce::Graphics& g)
{
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat(), "COUNT PARAMETERS (COUNT 5)");
    auto area = getLocalBounds().withTrimmedTop (38).reduced (10, 0);
    g.setColour (ToyotomiUi::gold());
    g.setFont (ToyotomiUi::font (12.0f));
    g.drawText ("LENGTH", area.removeFromTop (18), juce::Justification::centredLeft);
    auto lengths = area.removeFromTop (36);
    const std::array<const char*, 5> labels { "1/16", "1/8", "1/4", "1/2", "1 BAR" };
    for (int i = 0; i < 5; ++i)
    {
        auto cell = gridCell (lengths, i, 0, 5, 1, 2);
        g.setColour (i == selectedLength ? ToyotomiUi::gold() : ToyotomiUi::border());
        g.drawRect (cell, i == selectedLength ? 2 : 1);
        g.setColour (i == selectedLength ? ToyotomiUi::gold() : ToyotomiUi::ivory());
        g.drawText (labels[static_cast<size_t> (i)], cell, juce::Justification::centred);
    }

    auto knobs = area.withTrimmedTop (42).withTrimmedBottom (20);
    const std::array<const char*, 3> names { "SPEED", "PITCH", "DEPTH" };
    const std::array<const char*, 3> values { "0.00", "-12.00 st", "50 %" };
    const std::array<juce::Rectangle<int>, 3> sources { juce::Rectangle<int> (878, 635, 67, 72), { 958, 635, 67, 72 }, { 1038, 635, 67, 72 } };
    for (int i = 0; i < 3; ++i)
    {
        auto column = gridCell (knobs, i, 0, 3, 1, 6);
        g.setColour (ToyotomiUi::gold());
        g.setFont (ToyotomiUi::font (12.0f));
        g.drawText (names[static_cast<size_t> (i)], column.removeFromTop (22), juce::Justification::centred);
        auto knob = column.removeFromTop (78).reduced (2);
        if (source.isValid())
        {
            const auto src = sources[static_cast<size_t> (i)];
            g.drawImage (source, knob.getX(), knob.getY(), knob.getWidth(), knob.getHeight(), src.getX(), src.getY(), src.getWidth(), src.getHeight(), false);
        }
        g.setColour (ToyotomiUi::border());
        g.drawRect (column.removeFromTop (28));
        g.setColour (ToyotomiUi::ivory());
        g.drawText (values[static_cast<size_t> (i)], column.withTrimmedBottom (column.getHeight() - 28), juce::Justification::centred);
    }
}

void CountParameterPanel::mouseDown (const juce::MouseEvent& event)
{
    if (event.y < 105 && event.y > 55) { selectedLength = juce::jlimit (0, 4, (event.x - 10) * 5 / juce::jmax (1, getWidth() - 20)); repaint(); }
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
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat(), "OUTPUT");
    auto area = getLocalBounds().withTrimmedTop (48).withTrimmedBottom (32).reduced (24, 0);
    const auto drawChannel = [&] (juce::Rectangle<int> column, float level, const char* label)
    {
        g.setColour (ToyotomiUi::ivory()); g.setFont (ToyotomiUi::font (12.0f));
        g.drawText (label, column.removeFromTop (18), juce::Justification::centred);
        const auto segments = 28;
        const auto active = juce::roundToInt (juce::jmap (level, -60.0f, 6.0f, 0.0f, static_cast<float> (segments)));
        for (int i = 0; i < segments; ++i)
        {
            auto segment = column.removeFromBottom (juce::jmax (2, column.getHeight() / (segments - i))).withTrimmedTop (2).reduced (2, 0);
            const auto lit = i < active;
            const auto colour = i > 23 ? ToyotomiUi::red() : i > 17 ? ToyotomiUi::gold() : juce::Colour (0xff4d9a46);
            g.setColour (lit ? colour : ToyotomiUi::border().darker (0.35f));
            g.fillRect (segment);
        }
    };
    drawChannel (area.removeFromLeft (area.getWidth() / 2), leftDb, "L");
    drawChannel (area, rightDb, "R");
}

void BottomStatusBar::paint (juce::Graphics& g)
{
    ToyotomiUi::drawPanel (g, getLocalBounds().toFloat());
    auto area = getLocalBounds().reduced (18, 0);
    g.setFont (ToyotomiUi::font (12.0f));
    const std::array<juce::String, 4> values { "RAZOR FACE COMPANY", "PROJECT  Toyotomi Hideyoshi", "VERSION  0.1.0", "STATUS  PASS THROUGH" };
    const auto columnWidth = area.getWidth() / static_cast<int> (values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        auto cell = i + 1 == values.size() ? area : area.removeFromLeft (columnWidth);
        g.setColour (values[i].contains ("PASS") ? ToyotomiUi::ivory() : ToyotomiUi::muted());
        g.drawText (values[i], cell, juce::Justification::centredLeft);
    }
}
