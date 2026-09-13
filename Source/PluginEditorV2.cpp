#include "PluginEditorV2.h"
#include "GeneratedLayout.h"
#include <array>
#include <cmath>

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
#endif

namespace
{
constexpr int kW = 1024, kH = 683;
constexpr int kCellWidth = 56, kCellHeight = 80;
const std::array<int, 8> kCellX { 259, 317, 378, 437, 494, 553, 611, 670 };
const std::array<int, 2> kCellY { 137, 221 };
const std::array<juce::Rectangle<int>, 4> kTabs {{{251,74,105,27},{360,74,105,27},{470,74,106,27},{580,74,105,27}}};
const std::array<juce::Rectangle<int>, 10> kPresets {{{750,100,84,64},{836,100,84,64},{924,100,84,64},{750,166,84,64},{836,166,84,64},{924,166,84,64},{750,232,84,64},{836,232,84,64},{924,232,84,64},{750,296,84,64}}};
const std::array<juce::Rectangle<int>, 5> kLengths {{{742,425,32,26},{773,425,32,26},{803,425,32,26},{834,425,32,26},{864,425,32,26}}};
const std::array<const char*, 10> kPresetNames {{"off","forward_cut","backspin","chirp","baby","transform","drag","zigzag","tape_brake","custom"}};
const std::array<const char*, 5> kLengthNames {{"1_16","1_8","1_4","1_2","1_bar"}};
const std::array<const char*, 4> kTabNames {{"1_16","17_32","33_48","49_64"}};

juce::String resourceName (juce::String filename)
{
    // Keep this exactly aligned with JUCE's makeBinaryDataIdentifierName():
    // dots/spaces become underscores and every other non-identifier character
    // (including '-') is removed.  In particular, runtime-manifest.json is
    // registered as runtimemanifest_json, not runtime_manifest_json.
    return juce::File (filename).getFileName()
        .replaceCharacters (" .", "__")
        .retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789");
}

juce::Image readAsset (const juce::String& filename)
{
   #if __has_include(<BinaryData.h>)
    int bytes = 0;
    const auto* data = BinaryData::getNamedResource (resourceName (filename).toRawUTF8(), bytes);
    return data != nullptr ? juce::ImageFileFormat::loadFrom (data, (size_t) bytes) : juce::Image {};
   #else
    juce::ignoreUnused (filename);
    return {};
   #endif
}

juce::var readJsonAsset (const juce::String& filename)
{
   #if __has_include(<BinaryData.h>)
    int bytes = 0;
    const auto* data = BinaryData::getNamedResource (resourceName (filename).toRawUTF8(), bytes);
    return data != nullptr ? juce::JSON::parse (juce::String::fromUTF8 (static_cast<const char*> (data), bytes)) : juce::var {};
   #else
    juce::ignoreUnused (filename);
    return {};
   #endif
}

juce::var property (const juce::var& object, const juce::String& key)
{
    if (const auto* dynamic = object.getDynamicObject())
        return dynamic->getProperty (juce::Identifier (key));

    return {};
}

bool nativeSizeIs (const juce::var& asset, int width, int height)
{
    const auto sizeValue = property (asset, "native_size");
    if (const auto* size = sizeValue.getArray())
        return size->size() == 2 && (int) size->getReference (0) == width && (int) size->getReference (1) == height;

    return false;
}

bool hasNativeSize (const juce::Image& image, juce::Rectangle<int> bounds)
{
    return image.isValid() && image.getWidth() == bounds.getWidth() && image.getHeight() == bounds.getHeight();
}

void drawNative (juce::Graphics& g, const juce::Image& image, juce::Rectangle<int> bounds)
{
    if (hasNativeSize (image, bounds))
        g.drawImageAt (image, bounds.getX(), bounds.getY());
}

juce::Rectangle<int> cellBounds (int index)
{
    return { kCellX[(size_t) (index % 8)], kCellY[(size_t) (index / 8)], kCellWidth, kCellHeight };
}

enum CellState { normalState, selectedState, playingState, selectedPlayingState };
enum XYButtonState { xyNormalState, xyPressedState, xyActiveState };
enum XYButtonId { xyRecordButton, xyClearButton, xyResetViewButton, xyNoButton = -1 };

CellState resolveCellState (bool selected, bool playing) noexcept
{
    if (selected && playing) return selectedPlayingState;
    if (playing) return playingState;
    if (selected) return selectedState;
    return normalState;
}

// Every dynamic value is assembled from source-derived sprites. There is no
// JUCE font fallback: a missing native glyph must fail validation visibly,
// never silently replace the approved visual language.
struct NativeDisplayAssets final
{
    std::array<juce::Image, 10> digits;
    juce::Image dot, minus, slash, s, t, percent, init, minusInf;

    void load()
    {
        for (int digit = 0; digit < 10; ++digit)
            digits[(size_t) digit] = readAsset ("toy_display_digit_" + juce::String (digit) + ".png");
        dot = readAsset ("toy_display_dot.png"); minus = readAsset ("toy_display_minus.png");
        slash = readAsset ("toy_display_slash.png"); s = readAsset ("toy_display_s.png");
        t = readAsset ("toy_display_t.png"); percent = readAsset ("toy_display_percent.png");
        init = readAsset ("toy_display_init.png"); minusInf = readAsset ("toy_display_minus_inf.png");
    }

    const juce::Image* imageFor (juce_wchar character) const
    {
        if (character >= '0' && character <= '9') return &digits[(size_t) (character - '0')];
        if (character == '.') return &dot; if (character == '-') return &minus;
        if (character == '/') return &slash; if (character == 's') return &s;
        if (character == 't') return &t; if (character == '%') return &percent;
        return nullptr;
    }
};

bool drawNativeValue (juce::Graphics& g, const NativeDisplayAssets& assets, const juce::String& value,
                      juce::Rectangle<int> bounds, const float targetHeight,
                      juce::Justification justification = juce::Justification::centred)
{
    const auto drawWhole = [&g, bounds, targetHeight, justification] (const juce::Image& image)
    {
        if (! image.isValid()) return false;
        const auto scale = juce::jmin (1.0f, targetHeight / (float) image.getHeight());
        const auto target = justification.appliedToRectangle ({ 0, 0, juce::roundToInt (image.getWidth() * scale),
            juce::roundToInt (image.getHeight() * scale) }, bounds);
        g.drawImage (image, target.getX(), target.getY(), target.getWidth(), target.getHeight(),
                     0, 0, image.getWidth(), image.getHeight(), false);
        return true;
    };
    if (value == "Init") return drawWhole (assets.init);
    if (value == "-Inf") return drawWhole (assets.minusInf);

    struct Glyph { const juce::Image* image = nullptr; bool space = false; };
    juce::Array<Glyph> glyphs; float maximumHeight = 0.0f;
    for (auto characters = value.getCharPointer(); ! characters.isEmpty();)
    {
        const auto character = characters.getAndAdvance();
        if (character == ' ') { glyphs.add ({ nullptr, true }); continue; }
        const auto* image = assets.imageFor (character);
        if (image == nullptr || ! image->isValid()) return false;
        glyphs.add ({ image, false }); maximumHeight = juce::jmax (maximumHeight, (float) image->getHeight());
    }
    if (glyphs.isEmpty() || maximumHeight <= 0.0f) return false;
    const auto scale = juce::jmin (1.0f, targetHeight / maximumHeight);
    float width = 0.0f;
    for (const auto& glyph : glyphs) width += glyph.space ? targetHeight * 0.28f : glyph.image->getWidth() * scale;
    const auto target = justification.appliedToRectangle ({ 0, 0, juce::roundToInt (width),
        juce::roundToInt (maximumHeight * scale) }, bounds);
    float x = (float) target.getX();
    for (const auto& glyph : glyphs)
    {
        if (glyph.space) { x += targetHeight * 0.28f; continue; }
        const auto glyphWidth = glyph.image->getWidth() * scale;
        g.drawImage (*glyph.image, juce::roundToInt (x), target.getY(), juce::roundToInt (glyphWidth),
                     juce::roundToInt (glyph.image->getHeight() * scale), 0, 0,
                     glyph.image->getWidth(), glyph.image->getHeight(), false);
        x += glyphWidth;
    }
    return true;
}

// Every state image is loaded once, validated against its draw rectangle, and
// then used directly. This is deliberately not a per-paint resource lookup:
// a failed resource can no longer silently expose the black hole behind it.
struct V2AssetCatalog final
{
    juce::Image background, ring, pointer, bypassOff, bypassOn;
    juce::Image headerBpmBacking, headerTimeSigBacking, headerPresetBacking, hostSyncLampOn, hostSyncLampOff;
    std::array<juce::Image, 3> parameterReadoutBackings;
    NativeDisplayAssets nativeDisplay;
    std::array<std::array<juce::Image, 2>, 4> tabs;
    // Retain the proven native 56 x 80 completed BAR cells.  The later
    // shell/label split introduced a same-basename BinaryData collision and
    // produced the unreadable labels seen in the editor.
    std::array<std::array<juce::Image, 4>, 64> barCells;
    std::array<juce::Image, 4> barShells;
    std::array<juce::Image, 10> barMinis;
    std::array<std::array<juce::Image, 2>, 10> presets;
    std::array<std::array<juce::Image, 2>, 5> lengths;
    std::array<std::array<juce::Image, 3>, 3> xyButtons;
    bool barMapValid = true, xyButtonAssetsValid = true;

    void load (juce::Image& destination, const juce::String& name, juce::Rectangle<int> bounds)
    {
        destination = readAsset (name);
        juce::ignoreUnused (bounds);
    }

    bool loadBarMap (juce::Image& destination, const juce::String& name, juce::Rectangle<int> bounds)
    {
        destination = readAsset (name);
        const auto valid = hasNativeSize (destination, bounds);
        barMapValid = barMapValid && valid;
        return valid;
    }

    void loadXYButton (XYButtonId button, XYButtonState state, const juce::String& name,
                       juce::Rectangle<int> bounds)
    {
        auto& image = xyButtons[(size_t) button][(size_t) state];
        image = readAsset (name);
        xyButtonAssetsValid = xyButtonAssetsValid && hasNativeSize (image, bounds);
    }

    void loadBarMapContract()
    {
        const auto root = readJsonAsset ("runtime-manifest.json");
        const auto barMap = property (root, "barMap");
        const auto bars = property (barMap, "bars").getArray();
        const auto shells = property (barMap, "shells");
        const auto placements = property (barMap, "placements");

        const auto manifestShape = bars != nullptr && bars->size() == 64
                   && nativeSizeIs (property (shells, "normal"), 56, 80)
                   && nativeSizeIs (property (shells, "selected"), 56, 80)
                   && nativeSizeIs (property (shells, "playing"), 56, 80)
                   && nativeSizeIs (property (shells, "selected_playing"), 56, 80)
                   && property (placements, "label").isArray()
                   && property (placements, "mini").isArray();
        barMapValid = barMapValid && manifestShape;

        const std::array<const char*, 4> stateNames {{ "normal", "selected", "playing", "selected_playing" }};

        for (int bar = 0; bar < 64; ++bar)
        {
            const auto record = bars != nullptr && bar < bars->size() ? bars->getReference (bar) : juce::var {};
            barMapValid = barMapValid
                       && (int) property (record, "bar_id") == bar + 1;

            for (int state = 0; state < 4; ++state)
            {
                const auto stateAsset = property (record, juce::String (stateNames[(size_t) state]) + "_asset");
                barMapValid = barMapValid && nativeSizeIs (stateAsset, 56, 80);
                loadBarMap (barCells[(size_t) bar][(size_t) state],
                            juce::File (property (stateAsset, "file").toString()).getFileName(),
                            { 0, 0, 56, 80 });
            }
        }
    }

    V2AssetCatalog()
    {
        loadBarMap (background, "static_faceplate_1024x683.png", { 0, 0, kW, kH });
        for (int index = 0; index < 4; ++index)
        {
            loadBarMap (tabs[(size_t) index][0], "tab_" + juce::String (kTabNames[(size_t) index]) + "_normal.png", kTabs[(size_t) index]);
            loadBarMap (tabs[(size_t) index][1], "tab_" + juce::String (kTabNames[(size_t) index]) + "_selected.png", kTabs[(size_t) index]);
        }
        loadBarMapContract();
        const std::array<const char*, 4> shellNames {{ "normal", "selected", "playing", "selected_playing" }};
        for (int state = 0; state < 4; ++state)
            loadBarMap (barShells[(size_t) state], "bar_cell_shell_" + juce::String (shellNames[(size_t) state]) + "_56x80.png", { 0, 0, 56, 80 });
        for (int preset = 0; preset < 10; ++preset)
            loadBarMap (barMinis[(size_t) preset], "bar_mini_" + juce::String (kPresetNames[(size_t) preset]) + ".png", { 0, 0, 40, 20 });
        for (int index = 0; index < 10; ++index)
        {
            const auto prefix = "preset_" + juce::String (kPresetNames[(size_t) index]) + "_";
            load (presets[(size_t) index][0], prefix + "normal.png",   kPresets[(size_t) index]);
            load (presets[(size_t) index][1], prefix + "selected.png", kPresets[(size_t) index]);
        }
        for (int index = 0; index < 5; ++index)
        {
            const auto prefix = "length_" + juce::String (kLengthNames[(size_t) index]) + "_";
            load (lengths[(size_t) index][0], prefix + "normal.png",   kLengths[(size_t) index]);
            load (lengths[(size_t) index][1], prefix + "selected.png", kLengths[(size_t) index]);
        }
        load (ring,      "knob_ring_60.png",             GeneratedLayout::speedKnobBounds());
        load (pointer,   "knob_pointer_60.png",          GeneratedLayout::speedKnobBounds());
        load (bypassOff, "bypass_off.png",               { 931, 14, 80, 31 });
        load (bypassOn,  "bypass_on.png",                { 931, 14, 80, 31 });
        load (headerBpmBacking, "toy_display_header_bpm_backing.png", GeneratedLayout::bpmValueBounds());
        load (headerTimeSigBacking, "toy_display_header_timesig_backing.png", GeneratedLayout::timeSigValueBounds());
        load (headerPresetBacking, "toy_display_header_preset_backing.png", GeneratedLayout::presetValueBounds());
        load (hostSyncLampOn, "toy_display_host_sync_lamp_on.png", GeneratedLayout::hostSyncLampBounds());
        load (hostSyncLampOff, "toy_display_host_sync_lamp_off.png", GeneratedLayout::hostSyncLampBounds());
        load (parameterReadoutBackings[0], "toy_display_speed_readout_backing.png", GeneratedLayout::speedReadoutBounds());
        load (parameterReadoutBackings[1], "toy_display_pitch_readout_backing.png", GeneratedLayout::pitchReadoutBounds());
        load (parameterReadoutBackings[2], "toy_display_depth_readout_backing.png", GeneratedLayout::depthReadoutBounds());
        nativeDisplay.load();
        const std::array<juce::Rectangle<int>, 3> xyBounds {{ GeneratedLayout::xyRecBounds(),
            GeneratedLayout::xyClearBounds(), GeneratedLayout::xyResetViewBounds() }};
        const std::array<const char*, 3> xyNames {{ "rec", "clear", "reset_view" }};
        const std::array<const char*, 3> xyStates {{ "normal", "pressed", "active" }};
        for (int button = 0; button < 3; ++button)
            for (int state = 0; state < 3; ++state)
                loadXYButton ((XYButtonId) button, (XYButtonState) state,
                              "xy_" + juce::String (xyNames[(size_t) button]) + "_"
                                  + juce::String (xyStates[(size_t) state]) + ".png",
                              xyBounds[(size_t) button]);
    }
};
}

class ToyotomiHideyoshiAudioProcessorEditorV2::HitRegion final : public juce::Component
{
public:
    HitRegion (std::function<void()> downCallback, std::function<void()> upCallback = {})
        : down (std::move (downCallback)), up (std::move (upCallback)) { setOpaque (false); }
    void mouseDown (const juce::MouseEvent&) override { if (down) down(); }
    void mouseUp (const juce::MouseEvent&) override { if (up) up(); }
    bool pressAt (juce::Point<int> point)
    {
        if (! getBounds().contains (point)) return false;
        if (down) down();
        return true;
    }
    bool releaseAt (juce::Point<int> point)
    {
        if (! getBounds().contains (point)) return false;
        if (up) up();
        return true;
    }
private:
    std::function<void()> down, up;
};

class ToyotomiHideyoshiAudioProcessorEditorV2::KnobRegion final : public juce::Component
{
public:
    KnobRegion (ToyotomiHideyoshiAudioProcessor& source, int knobIndex) : processor (source), index (knobIndex) {}
    void mouseDown (const juce::MouseEvent& event) override { previousY = event.position.y; }
    void mouseDrag (const juce::MouseEvent& event) override
    {
        const auto delta = previousY - event.position.y;
        previousY = event.position.y;
        auto& state = processor.getStateModel();
        const auto bar = state.getUiState().selectedBar;
        if (! PluginStateModel::hasSelectedBar (bar))
            return;
        const auto& slot = state.getSlot (bar);
        if (index == 0) state.setSlotSpeed (bar, slot.speed + delta * .012f);
        else if (index == 1) state.setSlotPitch (bar, slot.pitch + delta * .20f);
        else state.setSlotDepth (bar, slot.depth + delta * .01f);
    }
private:
    ToyotomiHideyoshiAudioProcessor& processor;
    int index;
    float previousY = 0.0f;
};

class ToyotomiHideyoshiAudioProcessorEditorV2::BpmRegion final : public juce::Component
{
public:
    BpmRegion (std::function<void (double)> dragCallback, std::function<void()> editCallback)
        : drag (std::move (dragCallback)), edit (std::move (editCallback)) {}
    void mouseDown (const juce::MouseEvent& event) override { previousY = event.position.y; }
    void mouseDrag (const juce::MouseEvent& event) override
    {
        const auto delta = previousY - event.position.y;
        previousY = event.position.y;
        if (drag) drag (delta * 0.25);
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (edit) edit(); }
private:
    std::function<void (double)> drag;
    std::function<void()> edit;
    float previousY = 0.0f;
};

class ToyotomiHideyoshiAudioProcessorEditorV2::XYRegion final : public juce::Component
{
public:
    explicit XYRegion (std::function<void(juce::Point<float>)> callback) : update (std::move (callback)) {}
    void mouseDown (const juce::MouseEvent& event) override { if (update) update (event.position); }
    void mouseDrag (const juce::MouseEvent& event) override { if (update) update (event.position); }
private:
    std::function<void(juce::Point<float>)> update;
};

// Same ownership pattern as Buck Raw Shit's StereoMeter: one child owns the
// entire OUTPUT panel and derives both channels in local coordinates.
class ToyotomiHideyoshiAudioProcessorEditorV2::OutputMeter final : public juce::Component,
                                                                    private juce::Timer
{
public:
    explicit OutputMeter (ToyotomiHideyoshiAudioProcessor& source)
        : processor (source), meterLeds {{ readAsset ("output_meter_left.png"), readAsset ("output_meter_right.png") }},
          readoutBackings {{ readAsset ("toy_display_output_l_readout_backing.png"), readAsset ("toy_display_output_r_readout_backing.png") }}
    {
        nativeDisplay.load();
        setOpaque (false);
        startTimerHz (30);
    }

    void setLevelsForVisualTest (float left, float right)
    {
        outputDb = {{ juce::jlimit (-60.0f, 6.0f, left), juce::jlimit (-60.0f, 6.0f, right) }};
        outputPeakDb = outputDb;
    }

    void paint (juce::Graphics& g) override
    {
        const auto origin = GeneratedLayout::outputPanelBounds().getPosition();
        const std::array<juce::Rectangle<int>, 2> tracks {{
            GeneratedLayout::outputLBounds().translated (-origin.x, -origin.y),
            GeneratedLayout::outputRBounds().translated (-origin.x, -origin.y)
        }};
        const std::array<juce::Rectangle<int>, 2> readouts {{
            GeneratedLayout::outputLReadoutBounds().translated (-origin.x, -origin.y),
            GeneratedLayout::outputRReadoutBounds().translated (-origin.x, -origin.y)
        }};
        for (size_t channel = 0; channel < tracks.size(); ++channel)
        {
            const auto& meterLed = meterLeds[channel];
            const auto track = tracks[channel];
            const auto pixels = juce::roundToInt (juce::jmap (outputDb[channel], -60.0f, 6.0f,
                                                               0.0f, (float) track.getHeight()));
            const auto stripBounds = juce::Rectangle<int> (track.getX(), track.getY(),
                                                             meterLed.getWidth(), meterLed.getHeight());
            if (pixels > 0)
            {
                g.saveState();
                g.reduceClipRegion (track.withTop (track.getBottom() - pixels));
                drawNative (g, meterLed, stripBounds);
                g.restoreState();
            }
            if (outputPeakDb[channel] > -59.5f)
            {
                const auto peakY = track.getBottom() - juce::roundToInt (
                    juce::jmap (outputPeakDb[channel], -60.0f, 6.0f, 0.0f, (float) track.getHeight()));
                g.saveState();
                g.reduceClipRegion ({ track.getX(), juce::jlimit (track.getY(), track.getBottom() - 2, peakY), track.getWidth(), 2 });
                drawNative (g, meterLed, stripBounds);
                g.restoreState();
            }
            drawNative (g, readoutBackings[channel], readouts[channel]);
            drawNativeValue (g, nativeDisplay, outputDb[channel] <= -59.5f ? "-Inf" : juce::String (outputDb[channel], 1),
                             readouts[channel], 8.0f);
        }
    }

private:
    void timerCallback() override
    {
        const auto toDb = [] (float peak)
        {
            return juce::jlimit (-60.0f, 6.0f,
                                 juce::Decibels::gainToDecibels (juce::jmax (peak, 0.000001f), -60.0f));
        };
        const std::array<float, 2> target {{ toDb (processor.consumeOutputPeak (0)),
                                               toDb (processor.consumeOutputPeak (1)) }};
        for (size_t channel = 0; channel < target.size(); ++channel)
        {
            outputDb[channel] += (target[channel] - outputDb[channel]) * (target[channel] > outputDb[channel] ? 0.62f : 0.10f);
            outputPeakDb[channel] = juce::jmax (target[channel], outputPeakDb[channel] - 0.70f);
        }
        repaint();
    }

    ToyotomiHideyoshiAudioProcessor& processor;
    std::array<juce::Image, 2> meterLeds;
    std::array<juce::Image, 2> readoutBackings;
    NativeDisplayAssets nativeDisplay;
    std::array<float, 2> outputDb {{ -60.0f, -60.0f }};
    std::array<float, 2> outputPeakDb {{ -60.0f, -60.0f }};
};

class ToyotomiHideyoshiAudioProcessorEditorV2::Surface final : public juce::Component
{
public:
    Surface (ToyotomiHideyoshiAudioProcessor& source, const bool& recordingState,
             const int& pressedButtonState)
        : processor (source), xyRecording (recordingState), xyPressedButton (pressedButtonState) {}
    bool barMapAssetsReady() const { return assets.barMapValid && assets.xyButtonAssetsValid; }

    void paint (juce::Graphics& g) override
    {
        drawNative (g, assets.background, { 0, 0, kW, kH });
        const auto ui = processor.getStateModel().getUiState();
        const int tab = ui.selectedTab;
        const int selected = ui.selectedBar;
        const int playing = processor.getCurrentTimelineSlot();
        const bool hasSelection = PluginStateModel::hasSelectedBar (selected);

        for (int index = 0; index < 4; ++index)
            drawNative (g, assets.tabs[(size_t) index][ui.tabHighlight == index ? 1 : 0], kTabs[(size_t) index]);

        for (int index = 0; index < 16; ++index)
        {
            const auto bar = tab * 16 + index;
            const auto state = resolveCellState (bar == selected, bar == playing);
            const auto bounds = cellBounds (index);
            // Keep the proven completed cell: standalone label assets are
            // ambiguous in BinaryData and corrupt the BAR text. Replace only
            // the mini's backing pixels, then draw the selected preset mini.
            drawNative (g, assets.barCells[(size_t) bar][(size_t) state], bounds);
            g.saveState();
            g.reduceClipRegion ({ bounds.getX() + 8, bounds.getY() + 36, 40, 20 });
            drawNative (g, assets.barShells[(size_t) state], bounds);
            g.restoreState();
            drawNative (g, assets.barMinis[(size_t) static_cast<int> (processor.getStateModel().getSlot (bar).preset)],
                        { bounds.getX() + 8, bounds.getY() + 36, 40, 20 });
        }

        // Slot zero supplies the existing effective defaults while no BAR is
        // selected; it never contributes a selected image in that state.
        const auto& slot = processor.getStateModel().getSlot (hasSelection ? selected : 0);
        for (int index = 0; index < 10; ++index)
            drawNative (g, assets.presets[(size_t) index][hasSelection && index == (int) slot.preset ? 1 : 0], kPresets[(size_t) index]);
        for (int index = 0; index < 5; ++index)
            drawNative (g, assets.lengths[(size_t) index][hasSelection && index == (int) slot.length ? 1 : 0], kLengths[(size_t) index]);

        drawNative (g, ui.bypass ? assets.bypassOn : assets.bypassOff, { 931, 14, 80, 31 });
        const auto hostSync = processor.isHostSyncEnabled();
        drawNative (g, assets.headerBpmBacking, GeneratedLayout::bpmValueBounds());
        drawNativeValue (g, assets.nativeDisplay, juce::String (processor.getEffectiveBpm(), 2),
                         GeneratedLayout::bpmValueBounds(), 12.0f);
        drawNative (g, assets.headerTimeSigBacking, GeneratedLayout::timeSigValueBounds());
        drawNativeValue (g, assets.nativeDisplay, juce::String (processor.getEffectiveTimeSignatureNumerator())
                                                          + "/" + juce::String (processor.getEffectiveTimeSignatureDenominator()),
                         GeneratedLayout::timeSigValueBounds(), 12.0f);
        drawNative (g, assets.headerPresetBacking, GeneratedLayout::presetValueBounds());
        drawNativeValue (g, assets.nativeDisplay, ui.projectPresetId == 0 ? "Init" : "Init",
                         GeneratedLayout::presetValueBounds(), 10.0f, juce::Justification::centredLeft);
        drawNative (g, hostSync ? assets.hostSyncLampOn : assets.hostSyncLampOff, GeneratedLayout::hostSyncLampBounds());
        // The static faceplate owns the neutral XY panel and fixed labels.

        const std::array<float, 3> normalized {{ (slot.speed - .25f) / 3.75f, (slot.pitch + 12.0f) / 24.0f, slot.depth }};
        const std::array<juce::String, 3> text {{ juce::String (slot.speed, 2), juce::String (slot.pitch, 2) + " st", juce::String (juce::roundToInt (slot.depth * 100.0f)) + " %" }};
        for (int index = 0; index < 3; ++index)
        {
            const auto knobBounds = index == 0 ? GeneratedLayout::speedKnobBounds()
                                 : index == 1 ? GeneratedLayout::pitchKnobBounds()
                                              : GeneratedLayout::depthKnobBounds();
            const auto readoutBounds = index == 0 ? GeneratedLayout::speedReadoutBounds()
                                     : index == 1 ? GeneratedLayout::pitchReadoutBounds()
                                                  : GeneratedLayout::depthReadoutBounds();
            drawNative (g, assets.ring, knobBounds);
            g.saveState();
            const auto centre = knobBounds.getCentre().toFloat();
            const auto angle = -juce::MathConstants<float>::pi * 0.75f
                             + juce::MathConstants<float>::pi * 1.5f * juce::jlimit (0.0f, 1.0f, normalized[(size_t) index]);
            g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));
            drawNative (g, assets.pointer, knobBounds);
            g.restoreState();
            drawNative (g, assets.parameterReadoutBackings[(size_t) index], readoutBounds);
            drawNativeValue (g, assets.nativeDisplay, text[(size_t) index], readoutBounds, 8.0f);
        }

        if (hasSelection)
        {
            const auto pad = GeneratedLayout::xyPadBounds();
            const auto toPoint = [&pad] (const PluginStateModel::MotionPoint& point)
            {
                return juce::Point<float> (pad.getX() + point.x * (float) (pad.getWidth() - 1),
                                            pad.getBottom() - 1 - point.y * (float) (pad.getHeight() - 1));
            };
            g.saveState();
            g.reduceClipRegion (pad);
            if (slot.xyMotionExists && ! slot.xyMotion.empty())
            {
                juce::Path trace;
                for (size_t index = 0; index < slot.xyMotion.size(); ++index)
                {
                    const auto location = toPoint (slot.xyMotion[index]);
                    if (index == 0) trace.startNewSubPath (location); else trace.lineTo (location);
                }
                g.setColour (juce::Colour (0xffd6a446));
                g.strokePath (trace, juce::PathStrokeType (1.25f));
            }
            const PluginStateModel::MotionPoint current { slot.currentX, slot.currentY };
            const auto currentPoint = toPoint (current);
            g.setColour (juce::Colour (0xffd6a446));
            g.fillEllipse (currentPoint.x - 2.0f, currentPoint.y - 2.0f, 4.0f, 4.0f);
            g.restoreState();
        }

        const std::array<juce::Rectangle<int>, 3> xyBounds {{ GeneratedLayout::xyRecBounds(),
            GeneratedLayout::xyClearBounds(), GeneratedLayout::xyResetViewBounds() }};
        for (int button = 0; button < 3; ++button)
        {
            const auto active = button == xyRecordButton && xyRecording;
            const auto state = xyPressedButton == button ? xyPressedState
                             : active ? xyActiveState : xyNormalState;
            drawNative (g, assets.xyButtons[(size_t) button][(size_t) state], xyBounds[(size_t) button]);
        }
    }

private:
    ToyotomiHideyoshiAudioProcessor& processor;
    const bool& xyRecording;
    const int& xyPressedButton;
    V2AssetCatalog assets;
};

ToyotomiHideyoshiAudioProcessorEditorV2::ToyotomiHideyoshiAudioProcessorEditorV2 (ToyotomiHideyoshiAudioProcessor& source)
    : AudioProcessorEditor (&source), processor (source)
{
    surface = std::make_unique<Surface> (processor, xyRecording, xyPressedButton);
    addAndMakeVisible (*surface);
    surface->setInterceptsMouseClicks (false, false);
    outputMeter = std::make_unique<OutputMeter> (processor);
    addAndMakeVisible (*outputMeter);
    outputMeter->setBounds (GeneratedLayout::outputPanelBounds());

    for (int index = 0; index < 4; ++index)
        addImageHit (kTabs[(size_t) index], [this, index] { processor.getStateModel().selectTab (index); });
    for (int index = 0; index < 16; ++index)
        addImageHit (cellBounds (index), [this, index] { stopXYRecording(); const auto page = processor.getStateModel().getUiState().selectedTab; processor.getStateModel().selectBar (page * 16 + index); });
    for (int index = 0; index < 10; ++index)
        addImageHit (kPresets[(size_t) index], [this, index] { processor.getStateModel().setSelectedPreset ((PluginStateModel::ScratchPreset) index); });
    for (int index = 0; index < 5; ++index)
        addImageHit (kLengths[(size_t) index], [this, index] { processor.getStateModel().setSelectedLength ((PluginStateModel::NoteLength) index); });
    addImageHit ({ 931, 14, 80, 31 }, [this] { auto& state = processor.getStateModel(); state.setBypass (! state.getUiState().bypass); });
    addImageHit (GeneratedLayout::hostSyncBounds(), [this] { processor.setHostSyncEnabled (! processor.isHostSyncEnabled()); });
    addImageHit (GeneratedLayout::presetSelectorBounds(), [this] { showProjectPresetMenu(); });
    addImageHit (GeneratedLayout::presetPrevBounds(), [this] { stepProjectPreset (-1); });
    addImageHit (GeneratedLayout::presetNextBounds(), [this] { stepProjectPreset (1); });
    addXYButtonImageHit (GeneratedLayout::xyRecBounds(), xyRecordButton, [this] { toggleXYRecording(); });
    addXYButtonImageHit (GeneratedLayout::xyClearBounds(), xyClearButton, [this] { processor.getStateModel().clearSelectedBarXYMotion(); });
    addXYButtonImageHit (GeneratedLayout::xyResetViewBounds(), xyResetViewButton,
                         [this] { processor.getStateModel().resetSelectedBarXYPosition(); });

    for (int index = 0; index < 3; ++index)
    {
        knobs[(size_t) index] = std::make_unique<KnobRegion> (processor, index);
        addAndMakeVisible (*knobs[(size_t) index]);
        knobs[(size_t) index]->setBounds (index == 0 ? GeneratedLayout::speedKnobBounds()
                                      : index == 1 ? GeneratedLayout::pitchKnobBounds()
                                                   : GeneratedLayout::depthKnobBounds());
    }
    bpmInput = std::make_unique<BpmRegion> (
        [this] (double delta) { adjustInternalBpm (delta); },
        [this] { beginBpmTextEdit(); });
    addAndMakeVisible (*bpmInput);
    bpmInput->setBounds (GeneratedLayout::bpmBounds());
    xyInput = std::make_unique<XYRegion> ([this] (juce::Point<float> point) { updateXYFromPad (point); });
    addAndMakeVisible (*xyInput);
    xyInput->setBounds (GeneratedLayout::xyPadBounds());

    setResizable (false, false);
    setSize (kW, kH);
    startTimerHz (30);
}

ToyotomiHideyoshiAudioProcessorEditorV2::~ToyotomiHideyoshiAudioProcessorEditorV2() { stopXYRecording(); }

bool ToyotomiHideyoshiAudioProcessorEditorV2::hasValidBarMapAssets() const { return surface != nullptr && surface->barMapAssetsReady(); }
void ToyotomiHideyoshiAudioProcessorEditorV2::debugSetOutputMeterDb (float left, float right) { outputMeter->setLevelsForVisualTest (left, right); }
bool ToyotomiHideyoshiAudioProcessorEditorV2::validateInteractiveBounds() const
{
    std::vector<juce::Rectangle<int>> expected;
    expected.insert (expected.end(), kTabs.begin(), kTabs.end());
    for (int index = 0; index < 16; ++index) expected.push_back (cellBounds (index));
    expected.insert (expected.end(), kPresets.begin(), kPresets.end());
    expected.insert (expected.end(), kLengths.begin(), kLengths.end());
    expected.push_back ({ 931, 14, 80, 31 });
    expected.push_back (GeneratedLayout::hostSyncBounds());
    expected.push_back (GeneratedLayout::presetSelectorBounds());
    expected.push_back (GeneratedLayout::presetPrevBounds());
    expected.push_back (GeneratedLayout::presetNextBounds());
    expected.push_back (GeneratedLayout::xyRecBounds());
    expected.push_back (GeneratedLayout::xyClearBounds());
    expected.push_back (GeneratedLayout::xyResetViewBounds());
    if ((int) hitRegions.size() != (int) expected.size()) return false;
    for (int index = 0; index < (int) expected.size(); ++index)
        if (hitRegions[index]->getBounds() != expected[(size_t) index]) return false;
    return knobs[0]->getBounds() == GeneratedLayout::speedKnobBounds()
        && knobs[1]->getBounds() == GeneratedLayout::pitchKnobBounds()
        && knobs[2]->getBounds() == GeneratedLayout::depthKnobBounds()
        && bpmInput != nullptr && bpmInput->getBounds() == GeneratedLayout::bpmBounds()
        && xyInput->getBounds() == GeneratedLayout::xyPadBounds();
}
bool ToyotomiHideyoshiAudioProcessorEditorV2::debugXYAt (juce::Point<int> point, double elapsedSeconds)
{
    const auto bounds = GeneratedLayout::xyPadBounds();
    if (! bounds.contains (point)
        || ! PluginStateModel::hasSelectedBar (processor.getStateModel().getUiState().selectedBar)) return false;
    updateXYFromPad ((point - bounds.getPosition()).toFloat(), elapsedSeconds);
    return true;
}
void ToyotomiHideyoshiAudioProcessorEditorV2::toggleXYRecording()
{
    const auto selectedBar = processor.getStateModel().getUiState().selectedBar;
    if (! PluginStateModel::hasSelectedBar (selectedBar)) return;
    if (xyRecording) { stopXYRecording(); return; }
    xyRecording = true; xyRecordingHasPoint = false;
    xyRecordingBar = selectedBar;
    xyRecordingStartMilliseconds = juce::Time::getMillisecondCounterHiRes();
}
void ToyotomiHideyoshiAudioProcessorEditorV2::stopXYRecording()
{
    xyRecording = false; xyRecordingHasPoint = false;
    xyRecordingBar = PluginStateModel::kNoSelectedBar;
    xyRecordingStartMilliseconds = 0.0;
}
void ToyotomiHideyoshiAudioProcessorEditorV2::updateXYFromPad (juce::Point<float> point, double elapsedSeconds)
{
    auto& state = processor.getStateModel();
    const auto selectedBar = state.getUiState().selectedBar;
    if (! PluginStateModel::hasSelectedBar (selectedBar)) return;
    if (xyRecording && selectedBar != xyRecordingBar) stopXYRecording();
    const auto bounds = GeneratedLayout::xyPadBounds();
    const auto x = juce::jlimit (0.0f, 1.0f, point.x / (float) (bounds.getWidth() - 1));
    const auto y = juce::jlimit (0.0f, 1.0f, 1.0f - point.y / (float) (bounds.getHeight() - 1));
    state.setSelectedXYPosition (x, y);
    if (! xyRecording) return;
    const auto elapsed = elapsedSeconds >= 0.0 ? elapsedSeconds
                       : (juce::Time::getMillisecondCounterHiRes() - xyRecordingStartMilliseconds) / 1000.0;
    if (! xyRecordingHasPoint) { state.beginSelectedXYMotion (x, y); xyRecordingHasPoint = true; }
    else state.appendSelectedXYMotion (x, y, elapsed);
}
bool ToyotomiHideyoshiAudioProcessorEditorV2::debugClickAt (juce::Point<int> point)
{
    for (auto* hit : hitRegions)
        if (hit->pressAt (point)) return hit->releaseAt (point);
    return false;
}
bool ToyotomiHideyoshiAudioProcessorEditorV2::debugMouseDownAt (juce::Point<int> point)
{
    for (auto* hit : hitRegions)
        if (hit->pressAt (point)) return true;
    return false;
}
bool ToyotomiHideyoshiAudioProcessorEditorV2::debugMouseUpAt (juce::Point<int> point)
{
    for (auto* hit : hitRegions)
        if (hit->releaseAt (point)) return true;
    return false;
}
void ToyotomiHideyoshiAudioProcessorEditorV2::paint (juce::Graphics& g) { juce::ignoreUnused (g); }
void ToyotomiHideyoshiAudioProcessorEditorV2::resized()
{
    surface->setBounds (getLocalBounds());
    outputMeter->setBounds (GeneratedLayout::outputPanelBounds());
}
void ToyotomiHideyoshiAudioProcessorEditorV2::timerCallback()
{
    if (xyRecording && processor.getStateModel().getUiState().selectedBar != xyRecordingBar)
        stopXYRecording();
    surface->repaint();
}

void ToyotomiHideyoshiAudioProcessorEditorV2::adjustInternalBpm (double delta)
{
    if (processor.isHostSyncEnabled()) return;
    auto& state = processor.getStateModel();
    state.setInternalBpm (state.getUiState().internalBpm + delta);
    surface->repaint();
}

void ToyotomiHideyoshiAudioProcessorEditorV2::beginBpmTextEdit()
{
    if (processor.isHostSyncEnabled() || bpmEditor != nullptr) return;
    bpmEditor = std::make_unique<juce::TextEditor> ("Internal BPM");
    bpmEditor->setText (juce::String (processor.getStateModel().getUiState().internalBpm, 2), false);
    bpmEditor->setSelectAllWhenFocused (true);
    bpmEditor->setInputRestrictions (7, "0123456789.");
    bpmEditor->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff090b0c));
    bpmEditor->setColour (juce::TextEditor::textColourId, juce::Colour (0xffe3d7c5));
    bpmEditor->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    bpmEditor->onReturnKey = [this] { commitBpmTextEdit(); };
    bpmEditor->onEscapeKey = [this] { bpmEditor.reset(); surface->repaint(); };
    bpmEditor->onFocusLost = [this] { commitBpmTextEdit(); };
    bpmEditor->setBounds (GeneratedLayout::bpmBounds().reduced (2, 7));
    addAndMakeVisible (*bpmEditor);
    bpmEditor->grabKeyboardFocus();
}

void ToyotomiHideyoshiAudioProcessorEditorV2::commitBpmTextEdit()
{
    if (bpmEditor == nullptr) return;
    const auto value = bpmEditor->getText().getDoubleValue();
    if (! processor.isHostSyncEnabled()) processor.getStateModel().setInternalBpm (value);
    bpmEditor.reset();
    surface->repaint();
}

void ToyotomiHideyoshiAudioProcessorEditorV2::showProjectPresetMenu()
{
    juce::PopupMenu menu;
    const auto selected = processor.getStateModel().getUiState().projectPresetId == 0;
    menu.addItem (1, "Init", true, selected);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [this] (int result)
                        {
                            if (result == 1) processor.getStateModel().setProjectPresetId (0);
                            surface->repaint();
                        });
}

void ToyotomiHideyoshiAudioProcessorEditorV2::stepProjectPreset (int direction)
{
    juce::ignoreUnused (direction);
    // Init is deliberately the only item until factory preset contents exist.
    processor.getStateModel().setProjectPresetId (0);
    surface->repaint();
}

void ToyotomiHideyoshiAudioProcessorEditorV2::addImageHit (juce::Rectangle<int> bounds, std::function<void()> fn)
{
    auto* hit = new HitRegion (std::move (fn));
    hit->setBounds (bounds);
    addAndMakeVisible (hit);
    hitRegions.add (hit);
}

void ToyotomiHideyoshiAudioProcessorEditorV2::addXYButtonImageHit (juce::Rectangle<int> bounds, int button,
                                                                     std::function<void()> fn)
{
    auto* hit = new HitRegion (
        [this, button, fn = std::move (fn)] () mutable
        {
            xyPressedButton = button;
            if (fn) fn();
            surface->repaint();
        },
        [this] { xyPressedButton = xyNoButton; surface->repaint(); });
    hit->setBounds (bounds);
    addAndMakeVisible (hit);
    hitRegions.add (hit);
}
