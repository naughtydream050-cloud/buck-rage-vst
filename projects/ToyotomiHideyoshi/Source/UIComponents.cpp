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

juce::Image loadAsset (const juce::String& filename)
{
#if TOYOTOMI_HAS_BINARY_DATA
    int size = 0;
    const auto* data = BinaryData::getNamedResource (filename.toRawUTF8(), size);
    return data != nullptr ? juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (size)) : juce::Image {};
#else
    juce::ignoreUnused (filename);
    return {};
#endif
}

const juce::Image& imageFor (const juce::String& id)
{
    if (id == "tabStrip1") { static const auto i = loadAsset ("tab_strip_selected_1_16.png"); return i; }
    if (id == "tabStrip2") { static const auto i = loadAsset ("tab_strip_selected_17_32.png"); return i; }
    if (id == "tabStrip3") { static const auto i = loadAsset ("tab_strip_selected_33_48.png"); return i; }
    if (id == "tabStrip4") { static const auto i = loadAsset ("tab_strip_selected_49_64.png"); return i; }
    if (id == "barBaseNormal") { static const auto i = loadAsset ("bar_cell_shell_normal_clean_72x94.png"); return i; }
    if (id == "barBaseSelected") { static const auto i = loadAsset ("bar_cell_shell_selected_clean_72x94.png"); return i; }
    if (id == "barBasePlaying") { static const auto i = loadAsset ("bar_cell_shell_playing_72x94.png"); return i; }
    if (id == "barBaseSelectedPlaying") { static const auto i = loadAsset ("bar_cell_shell_selected_playing_72x94.png"); return i; }
    if (id == "xyNeutral") { static const auto i = loadAsset ("xy_neutral_base_288x256.png"); return i; }
    if (id == "clear") { static const auto i = loadAsset ("clear_normal_73x30.png"); return i; }
    if (id == "reset") { static const auto i = loadAsset ("reset_normal_102x30.png"); return i; }
    if (id == "outputNeutral") { static const auto i = loadAsset ("output_neutral_base_140x343.png"); return i; }
    if (id == "knobBase") { static const auto i = loadAsset ("knob_ring_67.png"); return i; }
    if (id == "knobPointer") { static const auto i = loadAsset ("knob_pointer.png"); return i; }
    if (id == "meterLed") { static const auto i = loadAsset ("meter_led_strip.png"); return i; }
    static const juce::Image empty;
    return empty;
}

const juce::Image& barLabelImage (int globalBarIndex)
{
    static const auto labels = []
    {
        std::array<juce::Image, 64> result;
        for (int i = 0; i < 64; ++i)
            result[static_cast<size_t> (i)] = loadAsset (juce::String::formatted ("bar_label_%02d.png", i + 1));
        return result;
    }();
    return labels[static_cast<size_t> (juce::jlimit (0, 63, globalBarIndex))];
}

const juce::Image& countCellImage (int oneBasedCount, bool selected)
{
    static const auto normal = [] { std::array<juce::Image, 16> r; for (int i = 0; i < 16; ++i) r[static_cast<size_t> (i)] = loadAsset (juce::String::formatted ("count_%02d_normal_117x72.png", i + 1)); return r; }();
    static const auto gold = [] { std::array<juce::Image, 16> r; for (int i = 0; i < 16; ++i) r[static_cast<size_t> (i)] = loadAsset (juce::String::formatted ("count_%02d_selected_117x72.png", i + 1)); return r; }();
    return (selected ? gold : normal)[static_cast<size_t> (juce::jlimit (1, 16, oneBasedCount) - 1)];
}

const juce::Image& presetCellImage (int preset, bool selected)
{
    static const auto normal = [] { const std::array<juce::String, 10> names { "off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom" }; std::array<juce::Image, 10> r; for (int i = 0; i < 10; ++i) r[static_cast<size_t> (i)] = loadAsset ("preset_" + names[static_cast<size_t> (i)] + "_normal_" + (i == 9 ? "102x55.png" : "102x79.png")); return r; }();
    static const auto gold = [] { const std::array<juce::String, 10> names { "off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom" }; std::array<juce::Image, 10> r; for (int i = 0; i < 10; ++i) r[static_cast<size_t> (i)] = loadAsset ("preset_" + names[static_cast<size_t> (i)] + "_selected_102x79.png"); return r; }();
    return (selected ? gold : normal)[static_cast<size_t> (juce::jlimit (0, 9, preset))];
}

const juce::Image& lengthImage (int length, bool selected)
{
    static const auto normal = [] { const std::array<juce::String, 5> names { "1_16", "1_8", "1_4", "1_2", "1_bar" }; std::array<juce::Image, 5> r; for (int i = 0; i < 5; ++i) r[static_cast<size_t> (i)] = loadAsset ("length_" + names[static_cast<size_t> (i)] + "_normal_40x33.png"); return r; }();
    static const auto gold = [] { const std::array<juce::String, 5> names { "1_16", "1_8", "1_4", "1_2", "1_bar" }; std::array<juce::Image, 5> r; for (int i = 0; i < 5; ++i) r[static_cast<size_t> (i)] = loadAsset ("length_" + names[static_cast<size_t> (i)] + "_selected_40x33.png"); return r; }();
    return (selected ? gold : normal)[static_cast<size_t> (juce::jlimit (0, 4, length))];
}

void drawImage (juce::Graphics& g, const juce::Image& image, juce::Rectangle<int> bounds)
{
    // Every state asset is authored at its final pixel dimensions.  Refuse a
    // mismatch instead of silently resampling a label or moving a hit target.
    if (image.isValid() && image.getWidth() == bounds.getWidth() && image.getHeight() == bounds.getHeight())
        g.drawImageAt (image, bounds.getX(), bounds.getY());
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
    static constexpr std::array<const char*, 11> ids {
        "tabStrip1", "tabStrip2", "tabStrip3", "tabStrip4", "barBaseNormal", "barBaseSelected",
        "barBasePlaying", "barBaseSelectedPlaying", "xyNeutral", "clear", "outputNeutral"
    };
    for (const auto* id : ids)
    {
        const auto& image = imageFor (id);
        if (! image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0)
            return false;
    }
    for (int bar = 0; bar < 64; ++bar)
    {
        const auto& label = barLabelImage (bar);
        if (! label.isValid() || label.getWidth() != 72 || label.getHeight() != 22)
            return false;
    }

    // The approved master-default exports retain their source alpha at a few
    // antialiased tab edges. Validate native dimensions here; drawImageAt keeps
    // the assets at their authored 1:1 coordinates without resampling them.
    for (const auto* id : { "tabStrip1", "tabStrip2", "tabStrip3", "tabStrip4" })
    {
        const auto& strip = imageFor (id);
        if (strip.getWidth() != 542 || strip.getHeight() != 34)
            return false;
    }
    for (const auto* id : { "barBaseNormal", "barBaseSelected", "barBasePlaying", "barBaseSelectedPlaying" })
    {
        const auto& cell = imageFor (id);
        if (cell.getWidth() != 72 || cell.getHeight() != 94)
            return false;
    }
    return true;
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
void TopBarComponent::paint (juce::Graphics& g) { juce::ignoreUnused (g); }
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
    static constexpr std::array<const char*, 4> stripIds { "tabStrip1", "tabStrip2", "tabStrip3", "tabStrip4" };
    const auto& strip = imageFor (stripIds[static_cast<size_t> (selectedPage)]);
    if (strip.isValid() && getLocalBounds().getWidth() == 542 && getLocalBounds().getHeight() == 34)
        g.drawImageAt (strip, 0, 0);
}
void BarTabComponent::mouseDown (const juce::MouseEvent& event)
{
    // These hit zones are the 1280 x 853 reference-image tab bounds, expressed
    // relative to this 542 x 35 component. Rendering and input therefore use
    // the exact same geometry as the one-strip image overlays.
    static const std::array<juce::Rectangle<float>, 4> referenceHitZones {{
        {   0.0f, 0.0f, 132.0f, 34.0f },
        { 135.0f, 0.0f, 132.0f, 34.0f },
        { 272.0f, 0.0f, 132.0f, 34.0f },
        { 410.0f, 0.0f, 132.0f, 34.0f }
    }};

    // The source strip and its hit regions are native 542 x 35 pixels.  Do
    // not transform the zones: a transformed hit test can disagree with the
    // one-to-one overlay and leak a page change from an adjacent tab.
    if (getWidth() != 542 || getHeight() != 34)
        return;

    const auto point = event.getPosition().toFloat();

    for (size_t i = 0; i < referenceHitZones.size(); ++i)
    {
        const auto zone = referenceHitZones[i];
        if (! zone.contains (point))
            continue;

        selectedPage = static_cast<int> (i);
        if (onSelectedPage) onSelectedPage (selectedPage);
        repaint();
        break;
    }
}

void BarCellComponent::configure (int globalBar, bool isSelected, bool isPlaying)
{
    globalBarIndex = juce::jlimit (0, 63, globalBar);
    selected = isSelected;
    playing = isPlaying;
    repaint();
}
void BarCellComponent::paint (juce::Graphics& g)
{
    const juce::Graphics::ScopedSaveState clipped (g);
    g.reduceClipRegion (getLocalBounds());
    const auto stateId = playing ? (selected ? "barBaseSelectedPlaying" : "barBasePlaying")
                                 : (selected ? "barBaseSelected" : "barBaseNormal");
    const auto& background = imageFor (stateId);
    if (background.isValid() && getWidth() == 72 && getHeight() == 94)
        g.drawImageAt (background, 0, 0);

    const auto& label = barLabelImage (globalBarIndex);
    if (label.isValid() && label.getWidth() == 72 && label.getHeight() == 22)
        g.drawImageAt (label, 0, 0);
}
void BarCellComponent::mouseDown (const juce::MouseEvent&) { if (onSelected) onSelected (globalBarIndex); }

BarMapComponent::BarMapComponent()
{
    for (int i = 0; i < 16; ++i)
      {
          cells[static_cast<size_t> (i)].onSelected = [this] (int number) { selectBar (number); };
          addAndMakeVisible (cells[static_cast<size_t> (i)]);
      }
      refreshCells();
}
void BarMapComponent::setDisplayState (int selectedTab, int selectedGlobalBar, int playingGlobalBar)
{
    displayTab = juce::jlimit (0, 3, selectedTab);
    selectedBar = juce::jlimit (0, 63, selectedGlobalBar);
    playingBar = juce::jlimit (-1, 63, playingGlobalBar);
    refreshCells();
}
bool BarMapComponent::hasReferenceCellBounds() const
{
    if (getWidth() != 608 || getHeight() != 266)
        return false;
    for (int i = 0; i < 16; ++i)
    {
        const auto expected = juce::Rectangle<int> (6 + (i % 8) * 75,
                                                     32 + (i / 8) * 97,
                                                     72, 94);
        if (cells[static_cast<size_t> (i)].getBounds() != expected)
            return false;
    }
    return true;
}
void BarMapComponent::selectBar (int globalBar)
{
    selectedBar = juce::jlimit (0, 63, globalBar);
    refreshCells();
    if (onSelectedBar) onSelectedBar (selectedBar);
}
void BarMapComponent::refreshCells()
{
    for (int i = 0; i < 16; ++i)
    {
        const auto globalBar = displayTab * 16 + i;
        cells[static_cast<size_t> (i)].setVisible (true);
        cells[static_cast<size_t> (i)].configure (globalBar, globalBar == selectedBar, globalBar == playingBar);
    }
}
void BarMapComponent::paint (juce::Graphics& g) { juce::ignoreUnused (g); }
void BarMapComponent::resized()
{
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].setBounds (6 + (i % 8) * 75, 32 + (i / 8) * 97, 72, 94);
    refreshCells();
}

void CountCellComponent::configure (int number, int preset, bool isSelected)
{
    countNumber = number; presetIndex = preset; selected = isSelected; repaint();
}
void CountCellComponent::paint (juce::Graphics& g)
{
    drawImage (g, countCellImage (countNumber, selected), getLocalBounds());
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
void CountGridComponent::setSelectedCount (int zeroBasedCount)
{
    selectedCount = juce::jlimit (0, 15, zeroBasedCount) + 1;
    const std::array<int, 16> presets { 0, 1, 2, 3, 2, 7, 6, 8, 1, 3, 0, 7, 7, 6, 3, 0 };
    for (int i = 0; i < 16; ++i)
    {
        cells[static_cast<size_t> (i)].setVisible (true);
        cells[static_cast<size_t> (i)].configure (i + 1, presets[static_cast<size_t> (i)], i + 1 == selectedCount);
    }
}
void CountGridComponent::selectCount (int number)
{
    showOverlay = true;
    setSelectedCount (number - 1);
    if (onSelectedCount) onSelectedCount (number - 1);
}
void CountGridComponent::paint (juce::Graphics& g) { juce::ignoreUnused (g); }
void CountGridComponent::resized()
{
    // Authored cell assets are final 117 x 72 pixels. Keep input and image
    // bounds identical on the fixed 1280 x 853 canvas.
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].setBounds (12 + (i % 4) * 122,
                                                   13 + (i / 4) * 84,
                                                   117, 72);
}

XYMotionPad::XYMotionPad()
{
}
juce::Rectangle<float> XYMotionPad::padBounds() const
{
    return { 30.0f, 27.0f, 243.0f, 176.0f };
}
void XYMotionPad::paint (juce::Graphics& g)
{
    drawFrame (g, "xyNeutral", getLocalBounds());
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
    const auto clear = juce::Rectangle<int> (17, 218, 73, 30);
    const auto reset = juce::Rectangle<int> (178, 218, 102, 30);
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
    static const std::array<juce::Rectangle<int>, 10> cells {{
        { 7, 37, 102, 79 }, { 112, 37, 102, 79 }, { 217, 37, 102, 79 },
        { 7, 120, 102, 79 }, { 112, 120, 102, 79 }, { 217, 120, 102, 79 },
        { 7, 205, 102, 79 }, { 112, 205, 102, 79 }, { 217, 205, 102, 79 },
        { 7, 289, 102, 55 }
    }};
    for (int i = 0; i < 10; ++i)
        drawImage (g, presetCellImage (i, i == selectedPreset && i != 9), cells[static_cast<size_t> (i)]);
}
void ScratchPresetPalette::mouseDown (const juce::MouseEvent& event)
{
    static const std::array<juce::Rectangle<int>, 10> cells {{ { 7,37,102,79 }, {112,37,102,79}, {217,37,102,79}, {7,120,102,79}, {112,120,102,79}, {217,120,102,79}, {7,205,102,79}, {112,205,102,79}, {217,205,102,79}, {7,289,102,55} }};
    for (int i = 0; i < 10; ++i) if (cells[static_cast<size_t> (i)].contains (event.getPosition())) { selectedPreset = i; repaint(); if (onPresetSelected) onPresetSelected (i); return; }
}

CountParameterPanel::CountParameterPanel (ToyotomiHideyoshiAudioProcessor& p) : processor (p) {}
juce::Rectangle<float> CountParameterPanel::knobBounds (int index) const
{
    return { 10.0f + 87.0f * index, 165.0f, 67.0f, 67.0f };
}
void CountParameterPanel::paint (juce::Graphics& g)
{
    const auto ui = processor.getStateModel().getUiState();
    const auto& count = processor.getStateModel().getCount (ui.selectedBar, ui.selectedCount);
    static const std::array<juce::Rectangle<int>, 5> lengths {{ {10,72,40,33}, {55,72,40,33}, {99,72,40,33}, {144,72,40,33}, {192,72,40,33} }};
    for (int i = 0; i < 5; ++i)
        drawImage (g, lengthImage (i, i == static_cast<int> (count.length)), lengths[static_cast<size_t> (i)]);
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
        g.setColour (ToyotomiUi::ivory()); g.setFont (juce::Font (11.0f));
        g.drawText (values[static_cast<size_t> (i)], knob.withY (knob.getBottom() + 11.0f).withHeight (21.0f).toNearestInt(), juce::Justification::centred);
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
    static const std::array<juce::Rectangle<int>, 5> lengths {{ {10,72,40,33}, {55,72,40,33}, {99,72,40,33}, {144,72,40,33}, {192,72,40,33} }};
    for (int i = 0; i < 5; ++i) if (lengths[static_cast<size_t> (i)].contains (event.getPosition())) { if (onLengthSelected) onLengthSelected (i); repaint(); return; }
    for (int i = 0; i < 3; ++i) if (knobBounds (i).contains (event.position)) { showLiveValues = true; activeKnob = i; dragStartY = event.position.y; repaint(); return; }
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
    drawFrame (g, "outputNeutral", getLocalBounds());
    const auto left = juce::Rectangle<int> (40, 61, 15, 255);
    const auto right = juce::Rectangle<int> (84, 61, 15, 255);
    const auto drawChannel = [&] (juce::Rectangle<int> track, float level, float peak)
    {
        const auto pixels = juce::roundToInt (juce::jmap (level, -60.0f, 6.0f, 0.0f, static_cast<float> (track.getHeight())));
        if (pixels > 0) { g.saveState(); g.reduceClipRegion (track.withTop (track.getBottom() - pixels)); drawImage (g, imageFor ("meterLed"), track); g.restoreState(); }
        const auto peakY = track.getBottom() - juce::roundToInt (juce::jmap (peak, -60.0f, 6.0f, 0.0f, static_cast<float> (track.getHeight())));
        if (peak > -59.5f) { g.saveState(); g.reduceClipRegion (juce::Rectangle<int> (track.getX(), juce::jlimit (track.getY(), track.getBottom() - 2, peakY), track.getWidth(), 2)); drawImage (g, imageFor ("meterLed"), track); g.restoreState(); }
    };
    drawChannel (left, leftDb, leftPeakDb); drawChannel (right, rightDb, rightPeakDb);
}

void BottomStatusBar::paint (juce::Graphics& g) { drawFrame (g, "bottomDefault", getLocalBounds()); }
