#include "PluginEditorV2.h"
#include "ParameterLayout.h"
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

CellState resolveCellState (bool selected, bool playing) noexcept
{
    if (selected && playing) return selectedPlayingState;
    if (playing) return playingState;
    if (selected) return selectedState;
    return normalState;
}

// Every state image is loaded once, validated against its draw rectangle, and
// then used directly. This is deliberately not a per-paint resource lookup:
// a failed resource can no longer silently expose the black hole behind it.
struct V2AssetCatalog final
{
    juce::Image background, ring, pointer, bypassOff, bypassOn;
    std::array<std::array<juce::Image, 2>, 4> tabs;
    // Retain the proven native 56 x 80 completed BAR cells.  The later
    // shell/label split introduced a same-basename BinaryData collision and
    // produced the unreadable labels seen in the editor.
    std::array<std::array<juce::Image, 4>, 64> barCells;
    std::array<std::array<juce::Image, 2>, 10> presets;
    std::array<std::array<juce::Image, 2>, 5> lengths;
    bool barMapValid = true;

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
        load (ring,      "knob_ring_60.png",             ParameterLayout::knobBounds()[0]);
        load (pointer,   "knob_pointer_60.png",          ParameterLayout::knobBounds()[0]);
        load (bypassOff, "bypass_off.png",               { 931, 14, 80, 31 });
        load (bypassOn,  "bypass_on.png",                { 931, 14, 80, 31 });
    }
};
}

class ToyotomiHideyoshiAudioProcessorEditorV2::HitRegion final : public juce::Component
{
public:
    explicit HitRegion (std::function<void()> callback) : fn (std::move (callback)) { setOpaque (false); }
    void mouseDown (const juce::MouseEvent&) override { if (fn) fn(); }
    bool activateAt (juce::Point<int> point)
    {
        if (! getBounds().contains (point)) return false;
        if (fn) fn();
        return true;
    }
private:
    std::function<void()> fn;
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

class ToyotomiHideyoshiAudioProcessorEditorV2::XYRegion final : public juce::Component
{
public:
    explicit XYRegion (ToyotomiHideyoshiAudioProcessor& source) : processor (source) {}
    void mouseDown (const juce::MouseEvent& event) override { points.clear(); add (event.position); }
    void mouseDrag (const juce::MouseEvent& event) override { add (event.position); }
    void mouseUp (const juce::MouseEvent&) override { processor.getStateModel().setSelectedMotion (points); }
private:
    void add (juce::Point<float> point)
    {
        if (points.size() < PluginStateModel::kMaxMotionPoints)
            points.push_back ({ juce::jlimit (0.0f, 1.0f, point.x / (float) getWidth()),
                                juce::jlimit (0.0f, 1.0f, point.y / (float) getHeight()) });
    }
    ToyotomiHideyoshiAudioProcessor& processor;
    std::vector<PluginStateModel::MotionPoint> points;
};

// Same ownership pattern as Buck Raw Shit's StereoMeter: one child owns the
// entire OUTPUT panel and derives both channels in local coordinates.
class ToyotomiHideyoshiAudioProcessorEditorV2::OutputMeter final : public juce::Component,
                                                                    private juce::Timer
{
public:
    explicit OutputMeter (ToyotomiHideyoshiAudioProcessor& source)
        : processor (source), meterLed (readAsset ("meter_led_strip.png"))
    {
        setOpaque (false);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto tracks = ParameterLayout::outputMeterLocalBounds();
        const auto readouts = ParameterLayout::outputReadoutLocalBounds();
        for (size_t channel = 0; channel < tracks.size(); ++channel)
        {
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
            g.setColour (juce::Colour (0xffe3d7c5));
            g.setFont (9.0f);
            g.drawText (outputDb[channel] <= -59.5f ? "-Inf" : juce::String (outputDb[channel], 1),
                        readouts[channel], juce::Justification::centred);
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
    juce::Image meterLed;
    std::array<float, 2> outputDb {{ -60.0f, -60.0f }};
    std::array<float, 2> outputPeakDb {{ -60.0f, -60.0f }};
};

class ToyotomiHideyoshiAudioProcessorEditorV2::Surface final : public juce::Component
{
public:
    explicit Surface (ToyotomiHideyoshiAudioProcessor& source) : processor (source) {}
    bool barMapAssetsReady() const { return assets.barMapValid; }

    void paint (juce::Graphics& g) override
    {
        drawNative (g, assets.background, { 0, 0, kW, kH });
        const auto ui = processor.getStateModel().getUiState();
        const int tab = ui.selectedTab;
        const int selected = ui.selectedBar;
        const int playing = processor.getCurrentTimelineSlot();
        const bool hasSelection = PluginStateModel::hasSelectedBar (selected);

        for (int index = 0; index < 4; ++index)
            drawNative (g, assets.tabs[(size_t) index][index == tab ? 1 : 0], kTabs[(size_t) index]);

        for (int index = 0; index < 16; ++index)
        {
            const auto bar = tab * 16 + index;
            const auto state = resolveCellState (bar == selected, bar == playing);
            const auto bounds = cellBounds (index);
            g.saveState();
            g.reduceClipRegion (bounds);
            drawNative (g, assets.barCells[(size_t) bar][(size_t) state], bounds);
            g.restoreState();
        }

        // Slot zero supplies the existing effective defaults while no BAR is
        // selected; it never contributes a selected image in that state.
        const auto& slot = processor.getStateModel().getSlot (hasSelection ? selected : 0);
        for (int index = 0; index < 10; ++index)
            drawNative (g, assets.presets[(size_t) index][hasSelection && index == (int) slot.preset ? 1 : 0], kPresets[(size_t) index]);
        for (int index = 0; index < 5; ++index)
            drawNative (g, assets.lengths[(size_t) index][hasSelection && index == (int) slot.length ? 1 : 0], kLengths[(size_t) index]);

        drawNative (g, ui.bypass ? assets.bypassOn : assets.bypassOff, { 931, 14, 80, 31 });
        // The static faceplate owns the neutral XY panel and its fixed button
        // visuals. Only the trace is dynamic in V2.

        const std::array<float, 3> normalized {{ (slot.speed - .25f) / 3.75f, (slot.pitch + 12.0f) / 24.0f, slot.depth }};
        const std::array<juce::String, 3> text {{ juce::String (slot.speed, 2) + "x", juce::String (slot.pitch, 1) + " st", juce::String (juce::roundToInt (slot.depth * 100.0f)) + " %" }};
        for (int index = 0; index < 3; ++index)
        {
            const auto bounds = ParameterLayout::knobBounds()[(size_t) index];
            drawNative (g, assets.ring, bounds);
            g.saveState();
            const auto centre = bounds.getCentre().toFloat();
            const auto angle = juce::MathConstants<float>::pi * 1.25f
                             + juce::MathConstants<float>::pi * 1.5f * juce::jlimit (0.0f, 1.0f, normalized[(size_t) index]);
            g.addTransform (juce::AffineTransform::rotation (angle + juce::MathConstants<float>::halfPi, centre.x, centre.y));
            drawNative (g, assets.pointer, bounds);
            g.restoreState();
            g.setColour (juce::Colour (0xffe3d7c5));
            g.setFont (10.0f);
            g.drawText (text[(size_t) index], ParameterLayout::readoutBounds()[(size_t) index], juce::Justification::centred);
        }

        if (! slot.motion.empty())
        {
            juce::Path trace;
            for (size_t index = 0; index < slot.motion.size(); ++index)
            {
                const auto point = slot.motion[index];
                const auto location = juce::Point<float> (38.0f + point.x * 195.0f, 438.0f + point.y * 141.0f);
                if (index == 0) trace.startNewSubPath (location); else trace.lineTo (location);
            }
            g.saveState();
            g.reduceClipRegion ({ 38, 438, 195, 141 });
            g.setColour (juce::Colour (0xffd6a446));
            g.strokePath (trace, juce::PathStrokeType (1.5f));
            g.restoreState();
        }
    }

private:
    ToyotomiHideyoshiAudioProcessor& processor;
    V2AssetCatalog assets;
};

ToyotomiHideyoshiAudioProcessorEditorV2::ToyotomiHideyoshiAudioProcessorEditorV2 (ToyotomiHideyoshiAudioProcessor& source)
    : AudioProcessorEditor (&source), processor (source)
{
    surface = std::make_unique<Surface> (processor);
    addAndMakeVisible (*surface);
    surface->setInterceptsMouseClicks (false, false);
    outputMeter = std::make_unique<OutputMeter> (processor);
    addAndMakeVisible (*outputMeter);
    outputMeter->setBounds (ParameterLayout::outputPanelBounds());

    for (int index = 0; index < 4; ++index)
        addImageHit (kTabs[(size_t) index], [this, index] { processor.getStateModel().selectTab (index); });
    for (int index = 0; index < 16; ++index)
        addImageHit (cellBounds (index), [this, index] { const auto page = processor.getStateModel().getUiState().selectedTab; processor.getStateModel().selectBar (page * 16 + index); });
    for (int index = 0; index < 10; ++index)
        addImageHit (kPresets[(size_t) index], [this, index] { processor.getStateModel().setSelectedPreset ((PluginStateModel::ScratchPreset) index); });
    for (int index = 0; index < 5; ++index)
        addImageHit (kLengths[(size_t) index], [this, index] { processor.getStateModel().setSelectedLength ((PluginStateModel::NoteLength) index); });
    addImageHit ({ 931, 14, 80, 31 }, [this] { auto& state = processor.getStateModel(); state.setBypass (! state.getUiState().bypass); });
    addImageHit ({ 27, 591, 59, 23 }, [] {});
    addImageHit ({ 95, 591, 59, 23 }, [this] { processor.getStateModel().clearSelectedMotion(); });
    addImageHit ({ 159, 591, 82, 23 }, [this] { processor.getStateModel().resetSelectedSlot(); });

    for (int index = 0; index < 3; ++index)
    {
        knobs[(size_t) index] = std::make_unique<KnobRegion> (processor, index);
        addAndMakeVisible (*knobs[(size_t) index]);
        knobs[(size_t) index]->setBounds (ParameterLayout::knobBounds()[(size_t) index]);
    }
    xyInput = std::make_unique<XYRegion> (processor);
    addAndMakeVisible (*xyInput);
    xyInput->setBounds ({ 38, 438, 195, 141 });

    setResizable (false, false);
    setSize (kW, kH);
    startTimerHz (30);
}

bool ToyotomiHideyoshiAudioProcessorEditorV2::hasValidBarMapAssets() const { return surface != nullptr && surface->barMapAssetsReady(); }
bool ToyotomiHideyoshiAudioProcessorEditorV2::validateInteractiveBounds() const
{
    std::vector<juce::Rectangle<int>> expected;
    expected.insert (expected.end(), kTabs.begin(), kTabs.end());
    for (int index = 0; index < 16; ++index) expected.push_back (cellBounds (index));
    expected.insert (expected.end(), kPresets.begin(), kPresets.end());
    expected.insert (expected.end(), kLengths.begin(), kLengths.end());
    expected.push_back ({ 931, 14, 80, 31 });
    expected.push_back ({ 27, 591, 59, 23 });
    expected.push_back ({ 95, 591, 59, 23 });
    expected.push_back ({ 159, 591, 82, 23 });
    if ((int) hitRegions.size() != (int) expected.size()) return false;
    for (int index = 0; index < (int) expected.size(); ++index)
        if (hitRegions[index]->getBounds() != expected[(size_t) index]) return false;
    return knobs[0]->getBounds() == ParameterLayout::knobBounds()[0]
        && knobs[1]->getBounds() == ParameterLayout::knobBounds()[1]
        && knobs[2]->getBounds() == ParameterLayout::knobBounds()[2]
        && xyInput->getBounds() == juce::Rectangle<int> { 38, 438, 195, 141 };
}
bool ToyotomiHideyoshiAudioProcessorEditorV2::debugClickAt (juce::Point<int> point)
{
    for (auto* hit : hitRegions)
        if (hit->activateAt (point)) return true;
    return false;
}
void ToyotomiHideyoshiAudioProcessorEditorV2::paint (juce::Graphics& g) { juce::ignoreUnused (g); }
void ToyotomiHideyoshiAudioProcessorEditorV2::resized()
{
    surface->setBounds (getLocalBounds());
    outputMeter->setBounds (ParameterLayout::outputPanelBounds());
}
void ToyotomiHideyoshiAudioProcessorEditorV2::timerCallback()
{
    surface->repaint();
}

void ToyotomiHideyoshiAudioProcessorEditorV2::addImageHit (juce::Rectangle<int> bounds, std::function<void()> fn)
{
    auto* hit = new HitRegion (std::move (fn));
    hit->setBounds (bounds);
    addAndMakeVisible (hit);
    hitRegions.add (hit);
}
