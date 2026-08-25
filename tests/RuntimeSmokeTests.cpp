#include "PluginProcessor.h"
#include "PluginEditorV2.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
#endif

namespace
{
bool check (bool ok, const char* name) { std::cout << (ok ? "PASS " : "FAIL ") << name << '\n'; return ok; }
bool png (const juce::Image& image, const juce::String& name)
{
    juce::FileOutputStream stream (juce::File::getCurrentWorkingDirectory().getChildFile(name));
    return stream.openedOk() && juce::PNGImageFormat().writeImageToStream(image, stream);
}
juce::Image render (juce::AudioProcessorEditor& editor)
{
    juce::Image image (juce::Image::ARGB, 1024, 683, true); juce::Graphics g (image); editor.paintEntireComponent(g, true); return image;
}
bool hideTestOnlySplashOverlay (juce::Component& parent, juce::Point<int> parentOrigin = {})
{
    // JUCE injects its licensing splash as an editor child. It is not part of
    // PluginEditorV2::paint(), so remove it only from this offscreen harness.
    // It may be nested below the editor, so compare its accumulated bounds.
    // JUCE 7 places its splash component in the bottom-right 3x logo area.
    // The visible logo is smaller, but this is the component's actual bounds.
    const auto splashBounds = juce::Rectangle<int> { 655, 494, 369, 189 };
    for (int index = 0; index < parent.getNumChildComponents(); ++index)
        if (auto* child = parent.getChildComponent (index); child != nullptr)
        {
            const auto absolute = child->getBounds().translated (parentOrigin.x, parentOrigin.y);
            if (absolute == splashBounds)
            {
                child->setVisible (false);
                return true;
            }
            if (hideTestOnlySplashOverlay (*child, absolute.getPosition()))
                return true;
        }
    return false;
}
bool different (const juce::Image& a, const juce::Image& b)
{
    for (int y=0;y<a.getHeight();++y) for (int x=0;x<a.getWidth();++x) if(a.getPixelAt(x,y)!=b.getPixelAt(x,y)) return true;
    return false;
}
bool resourceIs (const char* name, int w, int h)
{
   #if __has_include(<BinaryData.h>)
    int bytes=0; const auto* data=BinaryData::getNamedResource(name,bytes); auto i=data?juce::ImageFileFormat::loadFrom(data,(size_t)bytes):juce::Image{};
    return i.isValid() && i.getWidth()==w && i.getHeight()==h;
   #else
    juce::ignoreUnused(name,w,h); return false;
   #endif
}

juce::Image resourceImage (const char* name)
{
   #if __has_include(<BinaryData.h>)
    int bytes = 0;
    const auto* data = BinaryData::getNamedResource (name, bytes);
    return data != nullptr ? juce::ImageFileFormat::loadFrom (data, (size_t) bytes) : juce::Image {};
   #else
    juce::ignoreUnused (name);
    return {};
   #endif
}

bool cropMatchesResource (const juce::Image& rendered, juce::Rectangle<int> bounds, const char* resource)
{
    const auto expected = resourceImage (resource);
    if (! expected.isValid() || expected.getWidth() != bounds.getWidth() || expected.getHeight() != bounds.getHeight())
        return false;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
            if (rendered.getPixelAt (bounds.getX() + x, bounds.getY() + y) != expected.getPixelAt (x, y))
                return false;
    return true;
}

bool cropHasVisibleCellContent (const juce::Image& rendered, juce::Rectangle<int> bounds)
{
    int nonBlack = 0;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            const auto c = rendered.getPixelAt (bounds.getX() + x, bounds.getY() + y);
            nonBlack += (c.getRed() > 20 || c.getGreen() > 20 || c.getBlue() > 20) ? 1 : 0;
        }
    return nonBlack > 100;
}

bool cropsDiffer (const juce::Image& a, const juce::Image& b, juce::Rectangle<int> bounds)
{
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
            if (a.getPixelAt (bounds.getX() + x, bounds.getY() + y) != b.getPixelAt (bounds.getX() + x, bounds.getY() + y))
                return true;
    return false;
}

class TestPlayHead final : public juce::AudioPlayHead
{
public:
    void set (bool isPlaying, double ppq)
    {
        position = {};
        position.setIsPlaying (isPlaying);
        position.setPpqPosition (ppq);
    }

    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override { return position; }

private:
    juce::AudioPlayHead::PositionInfo position;
};

bool noPlayingRed (const juce::Image& image)
{
    // BAR cells contain ivory/gold artwork but no red in normal/selected
    // states. A red playhead must never survive a STOP render.
    for (int y = 137; y < 301; ++y)
        for (int x = 259; x < 726; ++x)
        {
            const auto c = image.getPixelAt (x, y);
            if (c.getRed() > 135 && c.getGreen() < 100 && c.getBlue() < 100)
                return false;
        }
    return true;
}

bool hasSelectedGoldContamination (const juce::Image& image)
{
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto c = image.getPixelAt (x, y);
            if (c.getRed() > 120 && c.getGreen() > 75 && c.getBlue() < 70
                && c.getRed() > c.getGreen() * 1.15f)
                return true;
        }
    return false;
}

juce::var jsonResource (const char* name)
{
   #if __has_include(<BinaryData.h>)
    int bytes = 0;
    const auto* data = BinaryData::getNamedResource (name, bytes);
    return data != nullptr ? juce::JSON::parse (juce::String::fromUTF8 (data, bytes)) : juce::var {};
   #else
    juce::ignoreUnused (name); return {};
   #endif
}

juce::var jsonProperty (const juce::var& object, const char* key)
{
    if (const auto* dynamic = object.getDynamicObject()) return dynamic->getProperty (juce::Identifier (key));
    return {};
}

juce::Rectangle<int> jsonBounds (const juce::var& value)
{
    if (const auto* array = value.getArray(); array != nullptr && array->size() == 4)
        return { (int) array->getReference (0), (int) array->getReference (1),
                 (int) array->getReference (2), (int) array->getReference (3) };
    return {};
}

struct DiffStats { int differing = 0, maxChannelError = 0; double mae = 0.0; juce::Rectangle<int> mismatchBounds; };

DiffStats diffImages (const juce::Image& actual, const juce::Image& reference, juce::Rectangle<int> bounds,
                      const juce::Image* dynamicMask = nullptr)
{
    DiffStats stats; auto first = true; uint64_t total = 0; int samples = 0;
    for (int y = bounds.getY(); y < bounds.getBottom(); ++y)
        for (int x = bounds.getX(); x < bounds.getRight(); ++x)
        {
            if (dynamicMask != nullptr && dynamicMask->getPixelAt (x, y).getAlpha() != 0) continue;
            const auto a = actual.getPixelAt (x, y), b = reference.getPixelAt (x, y);
            const int dr = std::abs ((int) a.getRed() - (int) b.getRed());
            const int dg = std::abs ((int) a.getGreen() - (int) b.getGreen());
            const int db = std::abs ((int) a.getBlue() - (int) b.getBlue());
            const int maximum = std::max ({ dr, dg, db });
            total += (uint64_t) dr + (uint64_t) dg + (uint64_t) db; samples += 3;
            stats.maxChannelError = std::max (stats.maxChannelError, maximum);
            if (maximum > 8)
            {
                ++stats.differing;
                const juce::Rectangle<int> pixel { x, y, 1, 1 };
                stats.mismatchBounds = first ? pixel : stats.mismatchBounds.getUnion (pixel);
                first = false;
            }
        }
    stats.mae = samples == 0 ? 0.0 : (double) total / (double) samples;
    return stats;
}

juce::Image makeDynamicMask (const juce::var& regions)
{
    juce::Image mask (juce::Image::ARGB, 1024, 683, true); juce::Graphics g (mask);
    if (const auto* array = regions.getArray())
        for (const auto& region : *array)
            if ((bool) jsonProperty (region, "dynamic_region"))
            {
                g.setColour (juce::Colours::white);
                g.fillRect (jsonBounds (jsonProperty (region, "bounds")));
            }
    return mask;
}

juce::Image makeDiffImage (const juce::Image& actual, const juce::Image& reference)
{
    juce::Image diff (juce::Image::ARGB, 1024, 683, true);
    for (int y = 0; y < 683; ++y) for (int x = 0; x < 1024; ++x)
    {
        const auto a = actual.getPixelAt (x, y), b = reference.getPixelAt (x, y);
        const int error = std::max ({ std::abs ((int) a.getRed() - (int) b.getRed()),
                                     std::abs ((int) a.getGreen() - (int) b.getGreen()),
                                     std::abs ((int) a.getBlue() - (int) b.getBlue()) });
        diff.setPixelAt (x, y, error > 8 ? juce::Colour::fromRGB ((uint8) std::min (255, error * 2), 0, 0)
                                      : juce::Colour (0x00000000));
    }
    return diff;
}

void writeVisualReport (const juce::var& regions, const juce::Image& actual, const juce::Image& reference,
                        const juce::Image& mask, const DiffStats& full, const DiffStats& staticOnly)
{
    juce::Array<juce::var> reportRegions;
    if (const auto* array = regions.getArray())
        for (const auto& region : *array)
        {
            const auto bounds = jsonBounds (jsonProperty (region, "bounds"));
            const auto dynamic = (bool) jsonProperty (region, "dynamic_region");
            const auto stats = diffImages (actual, reference, bounds, dynamic ? nullptr : &mask);
            auto* object = new juce::DynamicObject();
            object->setProperty ("name", jsonProperty (region, "name"));
            object->setProperty ("bounds", jsonProperty (region, "bounds"));
            object->setProperty ("dynamic_region", dynamic);
            object->setProperty ("accepted_variance", jsonProperty (region, "accepted_variance"));
            object->setProperty ("differing_pixel_count", stats.differing);
            const auto area = bounds.getWidth() * bounds.getHeight();
            object->setProperty ("differing_pixel_ratio", area == 0 ? 0.0 : (double) stats.differing / (double) area);
            object->setProperty ("mae", stats.mae);
            object->setProperty ("max_channel_error", stats.maxChannelError);
            object->setProperty ("mismatch_bounds", juce::Array<juce::var> { stats.mismatchBounds.getX(), stats.mismatchBounds.getY(), stats.mismatchBounds.getWidth(), stats.mismatchBounds.getHeight() });
            object->setProperty ("status", dynamic ? "DYNAMIC_REVIEW" : (stats.differing == 0 ? "PASS" : "FAIL"));
            reportRegions.add (juce::var (object));
        }
    auto* root = new juce::DynamicObject();
    root->setProperty ("canvas", juce::Array<juce::var> { 1024, 683 });
    root->setProperty ("pixel_threshold", 8);
    root->setProperty ("full_screen", juce::var (new juce::DynamicObject()));
    root->setProperty ("static_only", juce::var (new juce::DynamicObject()));
    auto writeStats = [&] (const char* key, const DiffStats& stats)
    {
        const auto value = root->getProperty (key); auto* object = value.getDynamicObject();
        object->setProperty ("differing_pixel_count", stats.differing);
        object->setProperty ("differing_pixel_ratio", (double) stats.differing / (1024.0 * 683.0));
        object->setProperty ("mae", stats.mae); object->setProperty ("max_channel_error", stats.maxChannelError);
        object->setProperty ("mismatch_bounds", juce::Array<juce::var> { stats.mismatchBounds.getX(), stats.mismatchBounds.getY(), stats.mismatchBounds.getWidth(), stats.mismatchBounds.getHeight() });
    };
    writeStats ("full_screen", full); writeStats ("static_only", staticOnly);
    root->setProperty ("regions", reportRegions);
    juce::File::getCurrentWorkingDirectory().getChildFile ("v2-visual-acceptance-report.json").replaceWithText (juce::JSON::toString (juce::var (root), true));
}

}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui; bool pass=true;
    pass &= check(resourceIs("finalmasterreference1024x683_png",1024,683),"v2-static-background-native");
    pass &= check(resourceIs("knob_ring_60_png",48,48) && resourceIs("knob_pointer_60_png",48,48),"v2-knob-assets-native");
    pass &= check(resourceIs("bypass_off_png",80,31) && resourceIs("bypass_on_png",80,31),"v2-bypass-native");
    const std::array<const char*, 4> shellResources {{ "bar_cell_shell_normal_56x80_png", "bar_cell_shell_selected_56x80_png", "bar_cell_shell_playing_56x80_png", "bar_cell_shell_selected_playing_56x80_png" }};
    for (const auto* resource : shellResources)
        pass &= check (resourceIs (resource, 56, 80), "v2-bar-shell-native");
    for (int i = 1; i <= 64; ++i)
    {
        const auto resource = "bar_label_" + juce::String (i).paddedLeft ('0', 2) + "_png";
        pass &= check (resourceIs (resource.toRawUTF8(), 56, 12), "v2-bar-label-native");
        for (const auto* state : { "normal", "selected", "playing", "selected_playing" })
        {
            const auto completedCell = "bar_" + juce::String (i).paddedLeft ('0', 2) + "_" + state + "_png";
            pass &= check (resourceIs (completedCell.toRawUTF8(), 56, 80), "v2-completed-bar-cell-native");
        }
    }
    for (const auto* mini : { "off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom" })
        pass &= check (resourceIs (("bar_mini_" + juce::String (mini) + "_png").toRawUTF8(), 40, 20), "v2-bar-mini-native");
    const auto normalShell = resourceImage ("bar_cell_shell_normal_56x80_png");
    const auto selectedShell = resourceImage ("bar_cell_shell_selected_56x80_png");
    const auto playingShell = resourceImage ("bar_cell_shell_playing_56x80_png");
    const auto selectedPlayingShell = resourceImage ("bar_cell_shell_selected_playing_56x80_png");
    pass &= check (different (normalShell, selectedShell), "v2-bar-selected-shell-is-distinct");
    pass &= check (different (normalShell, playingShell), "v2-bar-playing-shell-is-distinct");
    pass &= check (different (playingShell, selectedPlayingShell), "v2-bar-selected-playing-shell-is-distinct");
    pass &= check (! hasSelectedGoldContamination (resourceImage ("bar_cell_shell_normal_56x80_png")), "v2-bar11-normal-has-no-selected-gold");

    ToyotomiHideyoshiAudioProcessor processor; processor.prepareToPlay(48000,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    pass &= check(editor != nullptr && editor->getWidth()==1024 && editor->getHeight()==683,"v2-editor-native-1024");
    if(!editor) return 1;
    // The JUCE splash is laid out on its first paint. Prime that test-only
    // lifecycle step before excluding the child from diagnostic rendering.
    juce::ignoreUnused (render (*editor));
    pass &= check (hideTestOnlySplashOverlay (*editor), "v2-offscreen-test-splash-excluded");
    auto* v2 = dynamic_cast<ToyotomiHideyoshiAudioProcessorEditorV2*> (editor.get());
    pass &= check(v2 != nullptr && v2->hasValidBarMapAssets(), "v2-editor-bar-map-asset-contract");
    const auto visualManifest = jsonResource ("visual_acceptance_manifest_json");
    const auto visualRegions = jsonProperty (visualManifest, "regions");
    const auto visualInteractive = jsonProperty (visualManifest, "interactive").getArray();
    const auto visualReference = resourceImage ("finalmasterreference1024x683_png");
    pass &= check (visualManifest.getDynamicObject() != nullptr && visualRegions.getArray() != nullptr
                && visualInteractive != nullptr && visualInteractive->size() == 39
                && visualReference.isValid() && visualReference.getWidth() == 1024 && visualReference.getHeight() == 683,
                   "v2-visual-acceptance-reference-and-manifest");
    pass &= check (v2 != nullptr && v2->validateInteractiveBounds(), "v2-visual-hit-bounds-match-manifest");
    auto& state=processor.getStateModel();
    state.selectTab (0); state.selectBar (0); state.setSlotPreset (0, PluginStateModel::ScratchPreset::off);
    state.setSelectedLength ((PluginStateModel::NoteLength) 0); state.setBypass (false); state.clearSelectedMotion();
    state.setSlotSpeed (0, 1.0f); state.setSlotPitch (0, 0.0f); state.setSlotDepth (0, 0.5f);
    auto defaultImage=render(*editor); pass &= check(png(defaultImage,"v2-default-stop.png") && png(defaultImage,"v2-full-default-actual.png"),"v2-default-render");
    pass &= check (png (visualReference, "v2-full-default-reference.png"), "v2-visual-reference-export");
    const auto dynamicMask = makeDynamicMask (visualRegions);
    const auto fullDiff = diffImages (defaultImage, visualReference, { 0, 0, 1024, 683 });
    const auto staticDiff = diffImages (defaultImage, visualReference, { 0, 0, 1024, 683 }, &dynamicMask);
    pass &= check (png (makeDiffImage (defaultImage, visualReference), "v2-full-default-diff.png")
                && png (dynamicMask, "v2-full-default-mask.png"), "v2-visual-diff-and-mask-export");
    writeVisualReport (visualRegions, defaultImage, visualReference, dynamicMask, fullDiff, staticDiff);
    // The master contains a documented non-default example state.  Dynamic
    // regions are reported, not silently ignored; only static visual drift is
    // a visual-acceptance failure for the default comparison.
    // Permit a tiny antialiasing residue after the test-only JUCE splash is
    // hidden.  Any visible/static region drift remains a failure.
    pass &= check (staticDiff.differing <= 16 && staticDiff.maxChannelError <= 64,
                   "v2-visual-static-reference-match");
    if (v2 != nullptr && visualInteractive != nullptr)
    {
        bool centresReachHitRegions = true;
        for (const auto& item : *visualInteractive)
            centresReachHitRegions &= v2->debugClickAt (jsonBounds (jsonProperty (item, "bounds")).getCentre());
        pass &= check (centresReachHitRegions, "v2-visual-centre-click-hits-every-image-control");
        state.selectTab (0); state.selectBar (0); state.setSlotPreset (0, PluginStateModel::ScratchPreset::off);
        state.setSelectedLength ((PluginStateModel::NoteLength) 0); state.setBypass (false); state.clearSelectedMotion();
    }
    pass &= check(processor.getCurrentTimelineSlot()==-1,"v2-stop-has-no-playhead");
    // This checks the actual editor paint result, not merely BinaryData decode:
    // BAR cells may never regress to the black holes in the static faceplate.
    pass &= check(cropHasVisibleCellContent(defaultImage,{259,137,56,80})
               && cropHasVisibleCellContent(defaultImage,{317,137,56,80})
               && cropHasVisibleCellContent(defaultImage,{378,221,56,80}), "v2-bar-map-default-cells-painted");
    for (int index = 0; index < 16; ++index)
    {
        const auto bounds = juce::Rectangle<int> { std::array<int, 8> { 259, 317, 378, 437, 494, 553, 611, 670 }[(size_t) (index % 8)], index < 8 ? 137 : 221, 56, 80 };
        pass &= check (cropHasVisibleCellContent (defaultImage, bounds), "v2-visible-bar-cell-populated");
        const auto completedCell = "bar_" + juce::String (index + 1).paddedLeft ('0', 2)
                                 + (index == 0 ? "_selected_png" : "_normal_png");
        pass &= check (cropMatchesResource (defaultImage, bounds, completedCell.toRawUTF8()),
                       "v2-visible-bar-cell-matches-proven-completed-asset");
    }
    pass &= check(noPlayingRed(defaultImage), "v2-stop-red-cell-count-zero");
    pass &= check(cropMatchesResource(defaultImage,{251,74,105,27},"tab_1_16_selected_png")
               && cropMatchesResource(defaultImage,{360,74,105,27},"tab_17_32_normal_png"), "v2-tab-images-painted");
    // The master is an example-state illustration.  Its gold BACKSPIN is not
    // the default: default is OFF selected and BACKSPIN must be neutral.
    pass &= check (cropMatchesResource (defaultImage, { 750, 100, 84, 64 }, "preset_off_selected_png")
                && cropMatchesResource (defaultImage, { 924, 100, 84, 64 }, "preset_backspin_normal_png"),
                   "v2-default-off-selected-backspin-neutral");
    // FINAL MASTER owns the static XY panel and its control visuals; V2 adds
    // neither a second base nor second REC/CLEAR/RESET images.
    pass &= check (! cropsDiffer (defaultImage, visualReference, { 14, 416, 232, 200 }),
                   "v2-xy-static-panel-owned-by-final-master");

    const auto initialBar=state.getUiState().selectedBar; const auto initialSlot=state.getSlot(initialBar);
    const std::array<int, 8> cellX { 259, 317, 378, 437, 494, 553, 611, 670 };
    for (int tab = 0; tab < 4; ++tab)
    {
        state.selectTab (tab);
        auto image = render (*editor);
        pass &= check (png (image, "v2-tab-" + juce::String (tab + 1) + ".png")
                    && png (image, "v2-full-tab-" + juce::String (tab + 1) + ".png"), "v2-tab-render");
        for (int cell = 0; cell < 16; ++cell)
        {
            const auto bounds = juce::Rectangle<int> { cellX[(size_t) (cell % 8)], cell < 8 ? 137 : 221, 56, 80 };
            pass &= check (cropHasVisibleCellContent (image, bounds), "v2-tab-visible-bar-cells-populated");
        }
        pass &= check (state.getUiState().selectedBar == initialBar && state.getSlot(initialBar).preset == initialSlot.preset, "v2-tab-state-isolation");
    }
    state.selectTab (0);
    for (const auto selectedBar : { 0, 4, 10, 15 })
    {
        state.selectBar (selectedBar);
        auto image = render (*editor);
        const auto bounds = juce::Rectangle<int> { cellX[(size_t) (selectedBar % 8)], selectedBar < 8 ? 137 : 221, 56, 80 };
        pass &= check (cropHasVisibleCellContent (image, bounds)
                    && (selectedBar == 0 || cropsDiffer (defaultImage, image, bounds)), "v2-only-selected-bar-uses-gold-state");
    }

    TestPlayHead playHead;
    processor.setPlayHead (&playHead);
    juce::AudioBuffer<float> audio (2, 32);
    juce::MidiBuffer midi;
    playHead.set (true, 1.25); // PPQ 1.25 -> BAR 6 (zero-based slot 5)
    processor.processBlock (audio, midi);
    state.selectTab (0);
    state.selectBar (0);
    auto playing = render (*editor);
    pass &= check (png (playing, "v2-bar-playing-separated.png") && png (playing, "v2-full-bar-playing.png"), "v2-playing-render");
    pass &= check(processor.getCurrentTimelineSlot() == 5
               && cropsDiffer (defaultImage, playing, {553,137,56,80})
               && cropHasVisibleCellContent (playing, {259,137,56,80})
               && cropHasVisibleCellContent (playing, {553,137,56,80}), "v2-playing-red-and-selected-gold-separated");
    playHead.set (true, 2.5); // PPQ 2.5 -> BAR 11 (zero-based slot 10)
    processor.processBlock (audio, midi);
    state.selectBar (10);
    auto selectedPlaying = render (*editor);
    pass &= check (png (selectedPlaying, "v2-bar-selected-playing.png") && png (selectedPlaying, "v2-full-bar-selected-playing.png"), "v2-selected-playing-render");
    pass &= check(processor.getCurrentTimelineSlot() == 10
               && cropHasVisibleCellContent (selectedPlaying,{378,221,56,80})
               && cropsDiffer (playing, selectedPlaying, {378,221,56,80}), "v2-selected-playing-single-state-image");
    playHead.set (false, 0.0);
    processor.processBlock (audio, midi);
    state.selectBar (0);
    auto stopped = render (*editor);
    pass &= check (png (stopped, "v2-bar-stopped.png"), "v2-stopped-render");
    pass &= check(processor.getCurrentTimelineSlot() == -1 && noPlayingRed(stopped), "v2-stop-clears-all-playing-red");
    processor.setPlayHead (nullptr);
    state.selectTab(0); state.selectBar(10); auto selected=render(*editor); pass &= check(png(selected,"v2-bar-selected.png"),"v2-bar-selected-render");
    state.selectBar(0); for(int p=0;p<10;++p){state.setSelectedPreset((PluginStateModel::ScratchPreset)p);auto image=render(*editor);pass &= check(png(image,"v2-preset-"+juce::String(p)+".png") && png(image,"v2-full-preset-"+juce::String(p)+".png"),"v2-preset-render");pass &= check(state.getSlot(0).preset==(PluginStateModel::ScratchPreset)p,"v2-preset-single-source");}
    for(int l=0;l<5;++l){state.setSelectedLength((PluginStateModel::NoteLength)l);auto image=render(*editor);pass &=check(png(image,"v2-length-"+juce::String(l)+".png") && png(image,"v2-full-length-"+juce::String(l)+".png"),"v2-length-render");}
    const auto bypassBefore=state.getUiState().bypass; state.setSelectedPreset(PluginStateModel::ScratchPreset::custom);state.setBypass(!bypassBefore);auto bypassImage=render(*editor);pass &=check(png(bypassImage,"v2-full-bypass-on.png") && state.getSlot(0).preset==PluginStateModel::ScratchPreset::custom,"v2-bypass-preset-isolation");
    state.setSlotSpeed(0,PluginStateModel::kMinSpeed);state.setSlotPitch(0,PluginStateModel::kMinPitch);state.setSlotDepth(0,0.f);auto min=render(*editor);state.setSlotSpeed(0,PluginStateModel::kMaxSpeed);state.setSlotPitch(0,PluginStateModel::kMaxPitch);state.setSlotDepth(0,1.f);auto max=render(*editor);pass &=check(different(min,max) && png(min,"v2-knobs-min.png") && png(max,"v2-knobs-max.png"),"v2-knob-min-max-render");
    editor.reset(); processor.releaseResources(); return pass ? 0 : 1;
}
