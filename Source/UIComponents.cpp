#include "UIComponents.h"
#include "UiSpec.h"
#include <cmath>
#include <iostream>

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
    // juce_add_binary_data exposes resource names as C++ identifiers, not
    // filesystem names (e.g. "tab_strip_selected_1_16_png").
    const auto resourceName = filename.replaceCharacters (".- ", "___");
    int size = 0;
    const auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), size);
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
    if (id == "knobBase") { static const auto i = loadAsset ("knob_ring_48.png"); return i; }
    if (id == "knobPointer") { static const auto i = loadAsset ("knob_pointer_48.png"); return i; }
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

juce::Rectangle<int> localLayout (const juce::String& child, const juce::String& parent)
{
    return RuntimeLayout::localBounds (child, parent);
}

const juce::Image& presetCellImage (int preset, bool selected)
{
    static const auto normal = [] { const std::array<juce::String, 10> names { "off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom" }; std::array<juce::Image, 10> r; for (int i = 0; i < 10; ++i) r[static_cast<size_t> (i)] = loadAsset ("preset_" + names[static_cast<size_t> (i)] + "_normal_102x79.png"); r[2] = loadAsset ("preset_backspin_normal_neutral_102x79.png"); return r; }();
    static const auto gold = [] { const std::array<juce::String, 10> names { "off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom" }; std::array<juce::Image, 10> r; for (int i = 0; i < 10; ++i) r[static_cast<size_t> (i)] = loadAsset ("preset_" + names[static_cast<size_t> (i)] + "_selected_102x79.png"); return r; }();
    return (selected ? gold : normal)[static_cast<size_t> (juce::jlimit (0, 9, preset))];
}

const juce::Image& bypassImage (bool enabled)
{
    static const auto off = loadAsset ("bypass_off.png");
    static const auto on = loadAsset ("bypass_on.png");
    return enabled ? on : off;
}

const juce::Image& lengthImage (int length, bool selected)
{
    static const auto normal = [] { const std::array<juce::String, 5> names { "1_16", "1_8", "1_4", "1_2", "1_bar" }; std::array<juce::Image, 5> r; for (int i = 0; i < 5; ++i) r[static_cast<size_t> (i)] = loadAsset ("length_" + names[static_cast<size_t> (i)] + "_normal_40x33.png"); return r; }();
    static const auto gold = [] { const std::array<juce::String, 5> names { "1_16", "1_8", "1_4", "1_2", "1_bar" }; std::array<juce::Image, 5> r; for (int i = 0; i < 5; ++i) r[static_cast<size_t> (i)] = loadAsset ("length_" + names[static_cast<size_t> (i)] + "_selected_40x33.png"); return r; }();
    return (selected ? gold : normal)[static_cast<size_t> (juce::jlimit (0, 4, length))];
}

const std::array<juce::Rectangle<int>, 5>& lengthButtonBounds()
{
    static const std::array<juce::Rectangle<int>, 5> bounds {{
        RuntimeLayout::localBounds ("Length.1_16", "CountParameters"),
        RuntimeLayout::localBounds ("Length.1_8", "CountParameters"),
        RuntimeLayout::localBounds ("Length.1_4", "CountParameters"),
        RuntimeLayout::localBounds ("Length.1_2", "CountParameters"),
        RuntimeLayout::localBounds ("Length.1_BAR", "CountParameters")
    }};
    return bounds;
}

const std::array<juce::Rectangle<int>, 3>& parameterReadoutBounds()
{
    static const std::array<juce::Rectangle<int>, 3> bounds {{
        RuntimeLayout::localBounds ("Readout.Speed", "CountParameters"),
        RuntimeLayout::localBounds ("Readout.Pitch", "CountParameters"),
        RuntimeLayout::localBounds ("Readout.Depth", "CountParameters")
    }};
    return bounds;
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

void drawPresetPreview (juce::Graphics& g, juce::Rectangle<float> bounds,
                        const PluginStateModel::TimelineSlot& slot, juce::Colour colour)
{
    const auto preset = slot.preset;
    if (preset == PluginStateModel::ScratchPreset::off)
        return;

    const auto point = [&bounds] (float x, float y)
    {
        return juce::Point<float> (bounds.getX() + bounds.getWidth() * x,
                                   bounds.getY() + bounds.getHeight() * y);
    };
    juce::Path path;

    if (preset == PluginStateModel::ScratchPreset::custom)
    {
        if (slot.motion.empty())
        {
            const auto c = bounds.getCentre();
            g.setColour (colour);
            g.drawLine (c.x - 4.0f, c.y, c.x + 4.0f, c.y, 1.25f);
            g.drawLine (c.x, c.y - 4.0f, c.x, c.y + 4.0f, 1.25f);
            return;
        }
        path.startNewSubPath (point (slot.motion.front().x, 1.0f - slot.motion.front().y));
        for (size_t i = 1; i < slot.motion.size(); ++i)
            path.lineTo (point (slot.motion[i].x, 1.0f - slot.motion[i].y));
    }
    else
    {
        switch (preset)
        {
            case PluginStateModel::ScratchPreset::forwardCut:
                path.startNewSubPath (point (0.02f, .73f)); path.lineTo (point (.28f, .73f)); path.lineTo (point (.36f, .23f)); path.lineTo (point (.69f, .23f)); path.lineTo (point (.78f, .73f)); path.lineTo (point (.98f, .73f)); break;
            case PluginStateModel::ScratchPreset::backspin:
                path.startNewSubPath (point (.98f, .32f)); path.cubicTo (point (.73f, .16f), point (.45f, .81f), point (.06f, .70f)); break;
            case PluginStateModel::ScratchPreset::chirp:
                path.startNewSubPath (point (.02f, .78f)); path.cubicTo (point (.30f, .77f), point (.64f, .28f), point (.98f, .18f)); break;
            case PluginStateModel::ScratchPreset::baby:
                path.startNewSubPath (point (.02f, .70f)); path.cubicTo (point (.23f, .70f), point (.28f, .27f), point (.50f, .35f)); path.cubicTo (point (.70f, .43f), point (.75f, .73f), point (.98f, .70f)); break;
            case PluginStateModel::ScratchPreset::transform:
                path.startNewSubPath (point (.02f, .54f)); path.cubicTo (point (.18f, .12f), point (.33f, .93f), point (.50f, .51f)); path.cubicTo (point (.67f, .10f), point (.82f, .91f), point (.98f, .47f)); break;
            case PluginStateModel::ScratchPreset::drag:
                path.startNewSubPath (point (.02f, .25f)); path.cubicTo (point (.30f, .27f), point (.63f, .69f), point (.98f, .78f)); break;
            case PluginStateModel::ScratchPreset::zigzag:
                path.startNewSubPath (point (.02f, .74f)); path.lineTo (point (.23f, .24f)); path.lineTo (point (.46f, .75f)); path.lineTo (point (.69f, .23f)); path.lineTo (point (.98f, .74f)); break;
            case PluginStateModel::ScratchPreset::tapeBrake:
                path.startNewSubPath (point (.02f, .26f)); path.lineTo (point (.28f, .26f)); path.lineTo (point (.39f, .48f)); path.lineTo (point (.61f, .48f)); path.lineTo (point (.73f, .73f)); path.lineTo (point (.98f, .73f)); break;
            default: break;
        }
    }

    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (1.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
    static constexpr std::array<const char*, 15> ids {
        "tabStrip1", "tabStrip2", "tabStrip3", "tabStrip4", "barBaseNormal", "barBaseSelected",
        "barBasePlaying", "barBaseSelectedPlaying", "xyNeutral", "clear", "reset", "outputNeutral",
        "knobBase", "knobPointer", "meterLed"
    };
    for (const auto* id : ids)
    {
        const auto& image = imageFor (id);
        if (! image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0)
        {
            std::cerr << "ASSET_FAIL id=" << id << '\n';
            return false;
        }
    }
    for (int bar = 0; bar < 64; ++bar)
    {
        const auto& label = barLabelImage (bar);
        if (! label.isValid() || label.getWidth() <= 0 || label.getHeight() <= 0)
        {
            std::cerr << "ASSET_FAIL bar=" << (bar + 1) << '\n';
            return false;
        }
    }
    for (int preset = 0; preset < 10; ++preset)
    {
        const auto& normal = presetCellImage (preset, false);
        const auto& selected = presetCellImage (preset, true);
        const std::array<juce::String, 10> componentNames { "Preset.OFF", "Preset.FORWARD_CUT", "Preset.BACKSPIN", "Preset.CHIRP", "Preset.BABY", "Preset.TRANSFORM", "Preset.DRAG", "Preset.ZIGZAG", "Preset.TAPE_BRAKE", "Preset.CUSTOM" };
        const auto expected = RuntimeLayout::bounds (componentNames[static_cast<size_t> (preset)]);
        if (! normal.isValid() || ! selected.isValid()
            || normal.getWidth() != expected.getWidth() || normal.getHeight() != expected.getHeight()
            || selected.getWidth() != expected.getWidth() || selected.getHeight() != expected.getHeight())
        {
            std::cerr << "ASSET_FAIL preset=" << preset << '\n';
            return false;
        }
    }
    for (const auto enabled : { false, true })
    {
        const auto& image = bypassImage (enabled);
        const auto expected = RuntimeLayout::bounds ("Bypass");
        if (! image.isValid() || image.getWidth() != expected.getWidth() || image.getHeight() != expected.getHeight())
        {
            std::cerr << "ASSET_FAIL bypass=" << enabled << '\n';
            return false;
        }
    }

    // The approved master-default exports retain their source alpha at a few
    // antialiased tab edges. Validate native dimensions here; drawImageAt keeps
    // the assets at their authored 1:1 coordinates without resampling them.
    for (const auto* id : { "tabStrip1", "tabStrip2", "tabStrip3", "tabStrip4" })
    {
        const auto& strip = imageFor (id);
        if (strip.getWidth() != RuntimeLayout::bounds ("BarTabs").getWidth()
            || strip.getHeight() != RuntimeLayout::bounds ("BarTabs").getHeight())
        {
            std::cerr << "ASSET_DIM_FAIL tab=" << id << ' '
                      << strip.getWidth() << 'x' << strip.getHeight() << '\n';
            return false;
        }
    }
    for (const auto* id : { "barBaseNormal", "barBaseSelected", "barBasePlaying", "barBaseSelectedPlaying" })
    {
        const auto& cell = imageFor (id);
        if (cell.getWidth() != RuntimeLayout::bounds ("BarMap.Cell.01").getWidth()
            || cell.getHeight() != RuntimeLayout::bounds ("BarMap.Cell.01").getHeight())
        {
            std::cerr << "ASSET_DIM_FAIL cell=" << id << ' '
                      << cell.getWidth() << 'x' << cell.getHeight() << '\n';
            return false;
        }
    }
    for (const auto& [id, component] : std::array<std::pair<const char*, const char*>, 3> {{
             { "xyNeutral", "XYPad" }, { "clear", "XYPad.Clear" }, { "reset", "XYPad.Reset" } }})
    {
        const auto& image = imageFor (id);
        const auto expected = RuntimeLayout::bounds (component);
        if (image.getWidth() != expected.getWidth() || image.getHeight() != expected.getHeight())
        {
            std::cerr << "ASSET_DIM_FAIL " << id << ' ' << image.getWidth() << 'x' << image.getHeight() << '\n';
            return false;
        }
    }
    const auto& output = imageFor ("outputNeutral");
    const auto expectedOutput = RuntimeLayout::bounds ("Output");
    if (output.getWidth() != expectedOutput.getWidth() || output.getHeight() != expectedOutput.getHeight())
    {
        std::cerr << "ASSET_DIM_FAIL output " << output.getWidth() << 'x' << output.getHeight() << '\n';
        return false;
    }
    for (const auto* id : { "knobBase", "knobPointer" })
    {
        const auto& knob = imageFor (id);
        if (knob.getWidth() != RuntimeLayout::bounds ("Knob.Speed").getWidth()
            || knob.getHeight() != RuntimeLayout::bounds ("Knob.Speed").getHeight())
        {
            std::cerr << "ASSET_DIM_FAIL knob=" << id << ' '
                      << knob.getWidth() << 'x' << knob.getHeight() << '\n';
            return false;
        }
    }
    for (int length = 0; length < 5; ++length)
    {
        for (const auto selected : { false, true })
        {
            const auto& image = lengthImage (length, selected);
            if (! image.isValid() || image.getWidth() != lengthButtonBounds()[static_cast<size_t> (length)].getWidth()
                || image.getHeight() != lengthButtonBounds()[static_cast<size_t> (length)].getHeight())
            {
                std::cerr << "ASSET_DIM_FAIL length=" << length
                          << " selected=" << selected << '\n';
                return false;
            }

            for (int y = 0; y < image.getHeight(); ++y)
                for (int x = 0; x < image.getWidth(); ++x)
                    if (image.getPixelAt (x, y).getAlpha() != 255)
                    {
                        std::cerr << "ASSET_ALPHA_FAIL length=" << length
                                  << " selected=" << selected << '\n';
                        return false;
                    }
        }
    }
    return true;
}

bool validateLengthButtonGeometry()
{
    static constexpr std::array<const char*, 5> labels { "1/16", "1/8", "1/4", "1/2", "1 BAR" };

    const auto& lengthBounds = lengthButtonBounds();
    bool valid = true;
    for (int i = 0; i < static_cast<int> (lengthBounds.size()); ++i)
    {
        // Every state is rendered from this exact source rectangle.  The hit
        // target is assigned from the same rectangle in CountParameterPanel::resized.
        const auto normalBounds = lengthBounds[static_cast<size_t> (i)];
        const auto selectedBounds = lengthBounds[static_cast<size_t> (i)];
        const auto hitBounds = lengthBounds[static_cast<size_t> (i)];
        std::cerr << "LENGTH_GEOMETRY " << labels[static_cast<size_t> (i)]
                  << " normal=(" << normalBounds.getX() << ',' << normalBounds.getY() << ','
                  << normalBounds.getWidth() << ',' << normalBounds.getHeight() << ')'
                  << " selected=(" << selectedBounds.getX() << ',' << selectedBounds.getY() << ','
                  << selectedBounds.getWidth() << ',' << selectedBounds.getHeight() << ')'
                  << " hit=(" << hitBounds.getX() << ',' << hitBounds.getY() << ','
                  << hitBounds.getWidth() << ',' << hitBounds.getHeight() << ")\n";

        valid = valid && normalBounds == selectedBounds && selectedBounds == hitBounds;
        valid = valid && normalBounds.getWidth() == 32 && normalBounds.getHeight() == 26;
    }
    return valid;
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
void TopBarComponent::paint (juce::Graphics& g)
{
    // BYPASS is state-isolated from the preset palette and uses its supplied
    // off/on images at their native canonical bounds.
    const auto& image = bypassImage (processor.getStateModel().getUiState().bypass);
    const auto bounds = localLayout ("Bypass", "Header");
    drawImage (g, image, bounds);
}
void TopBarComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto bypassBounds = localLayout ("Bypass", "Header");
    if (bypassBounds.contains (event.getPosition()))
    {
        processor.getStateModel().setBypass (! processor.getStateModel().getUiState().bypass);
        repaint (bypassBounds);
    }
}

void ArtworkPanel::paint (juce::Graphics& g) { juce::ignoreUnused (g, source); }

void BarTabComponent::paint (juce::Graphics& g)
{
    static constexpr std::array<const char*, 4> stripIds { "tabStrip1", "tabStrip2", "tabStrip3", "tabStrip4" };
    const auto& strip = imageFor (stripIds[static_cast<size_t> (selectedPage)]);
    drawImage (g, strip, getLocalBounds());
}
void BarTabComponent::mouseDown (const juce::MouseEvent& event)
{
    if (getLocalBounds() != RuntimeLayout::bounds ("BarTabs").withPosition (0, 0))
        return;

    for (int i = 0; i < 4; ++i)
    {
        const auto zone = localLayout ("BarTab." + juce::String (i + 1), "BarTabs");
        if (! zone.contains (event.getPosition()))
            continue;

        selectedPage = static_cast<int> (i);
        if (onSelectedPage) onSelectedPage (selectedPage);
        repaint();
        break;
    }
}

void BarCellComponent::configure (int globalBar, bool isSelected, bool isPlaying, PluginStateModel::TimelineSlot preview)
{
    globalBarIndex = juce::jlimit (0, 63, globalBar);
    selected = isSelected;
    playing = isPlaying;
    previewSlot = std::move (preview);
    repaint();
}
void BarCellComponent::paint (juce::Graphics& g)
{
    const juce::Graphics::ScopedSaveState clipped (g);
    g.reduceClipRegion (getLocalBounds());
    const auto stateId = playing ? (selected ? "barBaseSelectedPlaying" : "barBasePlaying")
                                 : (selected ? "barBaseSelected" : "barBaseNormal");
    const auto& background = imageFor (stateId);
    drawImage (g, background, getLocalBounds());

    const auto& label = barLabelImage (globalBarIndex);
    if (label.isValid())
        g.drawImageAt (label, (getWidth() - label.getWidth()) / 2, 0);

    const auto previewBounds = localLayout ("BarMap.Cell.01.Preview", "BarMap.Cell.01").toFloat();
    const auto previewColour = playing ? ToyotomiUi::red()
                                       : (selected ? ToyotomiUi::gold() : ToyotomiUi::ivory());
    g.saveState();
    g.reduceClipRegion (previewBounds.toNearestInt());
    drawPresetPreview (g, previewBounds, previewSlot, previewColour);
    g.restoreState();
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
    const auto nextTab = juce::jlimit (0, 3, selectedTab);
    const auto nextSelected = juce::jlimit (0, 63, selectedGlobalBar);
    const auto nextPlaying = juce::jlimit (-1, 63, playingGlobalBar);
    if (nextTab == displayTab && nextSelected == selectedBar && nextPlaying == playingBar)
        return;
    displayTab = nextTab;
    selectedBar = nextSelected;
    playingBar = nextPlaying;
    refreshCells();
}
void BarMapComponent::setSlotPreview (std::function<PluginStateModel::TimelineSlot (int)> provider)
{
    for (int bar = 0; bar < PluginStateModel::kNumBars; ++bar)
        previews[static_cast<size_t> (bar)] = provider (bar);
    refreshCells();
}
bool BarMapComponent::hasReferenceCellBounds() const
{
    if (getBounds() != RuntimeLayout::bounds ("BarMap"))
        return false;
    for (int i = 0; i < 16; ++i)
    {
        const auto expected = localLayout ("BarMap.Cell." + juce::String::formatted ("%02d", i + 1), "BarMap");
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
        cells[static_cast<size_t> (i)].configure (globalBar, globalBar == selectedBar, globalBar == playingBar,
                                                   previews[static_cast<size_t> (globalBar)]);
    }
}
void BarMapComponent::paint (juce::Graphics& g) { juce::ignoreUnused (g); }
void BarMapComponent::resized()
{
    for (int i = 0; i < 16; ++i)
        cells[static_cast<size_t> (i)].setBounds (localLayout ("BarMap.Cell." + juce::String::formatted ("%02d", i + 1), "BarMap"));
    refreshCells();
}

void QuotePanel::paint (juce::Graphics& g)
{
    if (! sourceImage.isValid()) return;
    // This image was prepared once for the former central panel dimensions. Keep
    // it native-size so it cannot cross into the adjacent parameter panel.
    if (sourceImage.getWidth() != getWidth() || sourceImage.getHeight() != getHeight())
        return;
    g.drawImageAt (sourceImage, 0, 0);
}

XYMotionPad::XYMotionPad()
{
}
void XYMotionPad::setMotion (const std::vector<PluginStateModel::MotionPoint>& motion)
{
    normalizedMotion.clearQuick();
    for (const auto point : motion)
        normalizedMotion.add ({ juce::jlimit (0.0f, 1.0f, point.x), juce::jlimit (0.0f, 1.0f, point.y) });
    repaint();
}
juce::Rectangle<float> XYMotionPad::padBounds() const
{
    return localLayout ("XYPad.Input", "XYPad").toFloat();
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
    const auto clear = localLayout ("XYPad.Clear", "XYPad");
    const auto reset = localLayout ("XYPad.Reset", "XYPad");
    if (clear.contains (event.getPosition())) { normalizedMotion.clear(); if (onClearMotion) onClearMotion(); repaint(); return; }
    if (reset.contains (event.getPosition())) { normalizedMotion.clear(); if (onResetSlot) onResetSlot(); repaint(); return; }
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
    static const std::array<juce::String, 10> names { "OFF", "FORWARD_CUT", "BACKSPIN", "CHIRP", "BABY", "TRANSFORM", "DRAG", "ZIGZAG", "TAPE_BRAKE", "CUSTOM" };
    const auto activePreset = presetProvider != nullptr ? juce::jlimit (0, 9, presetProvider()) : 0;
    for (int i = 0; i < 10; ++i)
        drawImage (g, presetCellImage (i, i == activePreset), localLayout ("Preset." + names[static_cast<size_t> (i)], "PresetPanel"));
}
void ScratchPresetPalette::mouseDown (const juce::MouseEvent& event)
{
    static const std::array<juce::String, 10> names { "OFF", "FORWARD_CUT", "BACKSPIN", "CHIRP", "BABY", "TRANSFORM", "DRAG", "ZIGZAG", "TAPE_BRAKE", "CUSTOM" };
    for (int i = 0; i < 10; ++i)
        if (localLayout ("Preset." + names[static_cast<size_t> (i)], "PresetPanel").contains (event.getPosition()))
            { if (onPresetSelected) onPresetSelected (i); repaint(); return; }
}

CountParameterPanel::CountParameterPanel (ToyotomiHideyoshiAudioProcessor& p) : processor (p)
{
    static const std::array<const char*, 5> ids {{ "length-1-16", "length-1-8", "length-1-4", "length-1-2", "length-1-bar" }};
    for (int i = 0; i < static_cast<int> (lengthHitTargets.size()); ++i)
    {
        auto target = std::make_unique<TransparentLengthHitTarget> (ids[static_cast<size_t> (i)]);
        target->onClick = [this, i]
        {
            if (onLengthSelected != nullptr)
                onLengthSelected (i);
        };
        addAndMakeVisible (*target);
        lengthHitTargets[static_cast<size_t> (i)] = std::move (target);
    }
}

void CountParameterPanel::resized()
{
    const auto& bounds = lengthButtonBounds();
    for (int i = 0; i < static_cast<int> (lengthHitTargets.size()); ++i)
    {
        const auto lengthBounds = bounds[static_cast<size_t> (i)];
        lengthHitTargets[static_cast<size_t> (i)]->setBounds (lengthBounds);
        jassert (lengthHitTargets[static_cast<size_t> (i)]->getBounds() == lengthBounds);
    }
}

juce::Rectangle<float> CountParameterPanel::knobBounds (int index) const
{
    static const std::array<juce::String, 3> names { "Speed", "Pitch", "Depth" };
    return localLayout ("Knob." + names[static_cast<size_t> (juce::jlimit (0, 2, index))], "CountParameters").toFloat();
}
void CountParameterPanel::paint (juce::Graphics& g)
{
    const auto ui = processor.getStateModel().getUiState();
    const auto& count = processor.getStateModel().getSlot (ui.selectedBar);
    const auto& lengths = lengthButtonBounds();
    const auto selectedLength = juce::jlimit (0, 4, static_cast<int> (count.length));
    for (int i = 0; i < 5; ++i)
        drawImage (g, lengthImage (i, i == selectedLength), lengths[static_cast<size_t> (i)]);
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
        g.drawText (values[static_cast<size_t> (i)], parameterReadoutBounds()[static_cast<size_t> (i)], juce::Justification::centred);
    }
}
void CountParameterPanel::updateKnob (int index, float delta)
{
    auto& state = processor.getStateModel(); const auto ui = state.getUiState(); const auto& count = state.getSlot (ui.selectedBar);
    if (index == 0) state.setSlotSpeed (ui.selectedBar, count.speed + delta * 0.012f);
    if (index == 1) state.setSlotPitch (ui.selectedBar, count.pitch + delta * 0.20f);
    if (index == 2) state.setSlotDepth (ui.selectedBar, count.depth + delta * 0.010f);
    repaint();
}
void CountParameterPanel::mouseDown (const juce::MouseEvent& event)
{
    for (int i = 0; i < 3; ++i) if (knobBounds (i).contains (event.position)) { showLiveValues = true; activeKnob = i; dragStartY = event.position.y; repaint(); return; }
}
void CountParameterPanel::mouseDoubleClick (const juce::MouseEvent& event)
{
    const auto ui = processor.getStateModel().getUiState();
    for (int i = 0; i < 3; ++i)
        if (knobBounds (i).contains (event.position))
        {
            if (i == 0) processor.getStateModel().setSlotSpeed (ui.selectedBar, 1.0f);
            if (i == 1) processor.getStateModel().setSlotPitch (ui.selectedBar, 0.0f);
            if (i == 2) processor.getStateModel().setSlotDepth (ui.selectedBar, 0.5f);
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
    const auto left = localLayout ("Output.LeftTrack", "Output");
    const auto right = localLayout ("Output.RightTrack", "Output");
    const auto drawChannel = [&] (juce::Rectangle<int> track, float level, float peak)
    {
        const auto pixels = juce::roundToInt (juce::jmap (level, -60.0f, 6.0f, 0.0f, static_cast<float> (track.getHeight())));
        const auto& led = imageFor ("meterLed");
        const auto ledBounds = juce::Rectangle<int> (track.getX(), track.getBottom() - led.getHeight(), led.getWidth(), led.getHeight());
        if (pixels > 0) { g.saveState(); g.reduceClipRegion (track.withTop (track.getBottom() - pixels)); drawImage (g, led, ledBounds); g.restoreState(); }
        const auto peakY = track.getBottom() - juce::roundToInt (juce::jmap (peak, -60.0f, 6.0f, 0.0f, static_cast<float> (track.getHeight())));
        if (peak > -59.5f) { g.saveState(); g.reduceClipRegion (juce::Rectangle<int> (track.getX(), juce::jlimit (track.getY(), track.getBottom() - 2, peakY), track.getWidth(), 2)); drawImage (g, led, ledBounds); g.restoreState(); }
    };
    drawChannel (left, leftDb, leftPeakDb); drawChannel (right, rightDb, rightPeakDb);
}

void BottomStatusBar::paint (juce::Graphics& g) { drawFrame (g, "bottomDefault", getLocalBounds()); }
