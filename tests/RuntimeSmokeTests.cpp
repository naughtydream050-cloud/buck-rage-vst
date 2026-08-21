#include "PluginProcessor.h"
#include "PluginEditorV2.h"
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

juce::String barResourceName (int oneBasedBar, const char* state)
{
    return "bar_" + juce::String (oneBasedBar).paddedLeft ('0', 2) + "_" + state + "_png";
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
            {
                const auto actual = rendered.getPixelAt (bounds.getX() + x, bounds.getY() + y);
                const auto wanted = expected.getPixelAt (x, y);
                std::cout << "BAR_PIXEL_MISMATCH resource=" << resource << " x=" << (bounds.getX() + x)
                          << " y=" << (bounds.getY() + y) << " actual=" << actual.getARGB()
                          << " expected=" << wanted.getARGB() << '\n';
                return false;
            }
    return true;
}

bool resourceIsFullyOpaque (const char* resource)
{
    const auto image = resourceImage (resource);
    if (! image.isValid())
        return false;
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
            if (image.getPixelAt (x, y).getAlpha() != 255)
                return false;
    return true;
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

bool hasGoldAccent (const juce::Image& image)
{
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto c = image.getPixelAt (x, y);
            if (c.getRed() > 105 && c.getGreen() > 75 && c.getBlue() < 90
                && c.getRed() > c.getGreen() * 1.10f)
                return true;
        }
    return false;
}

bool hasRedAccent (const juce::Image& image)
{
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto c = image.getPixelAt (x, y);
            if (c.getRed() > 135 && c.getGreen() < 100 && c.getBlue() < 100)
                return true;
        }
    return false;
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui; bool pass=true;
    pass &= check(resourceIs("neutral_static_background_1024x683_png",1024,683),"v2-static-background-native");
    pass &= check(resourceIs("knob_ring_60_png",48,48) && resourceIs("knob_pointer_60_png",48,48),"v2-knob-assets-native");
    pass &= check(resourceIs("xy_neutral_base_288x256_png",192,174),"v2-xy-native");
    pass &= check(resourceIs("bypass_off_png",80,31) && resourceIs("bypass_on_png",80,31),"v2-bypass-native");
    pass &= check(resourceIs("rec_normal_png",59,23) && resourceIs("clear_normal_png",59,23) && resourceIs("reset_view_normal_png",82,23),"v2-xy-buttons-native");
    for (int i = 1; i <= 64; ++i)
        for (const auto* state : { "normal", "selected", "playing", "selected_playing" })
        {
            const auto resource = barResourceName (i, state);
            pass &= check (resourceIs (resource.toRawUTF8(), 56, 80)
                        && resourceIsFullyOpaque (resource.toRawUTF8()), "v2-bar-cell-state-native-opaque");
        }
    for (int i = 1; i <= 64; ++i)
    {
        pass &= check (hasGoldAccent (resourceImage (barResourceName (i, "selected").toRawUTF8())), "v2-bar-selected-has-gold");
        pass &= check (hasRedAccent (resourceImage (barResourceName (i, "playing").toRawUTF8())), "v2-bar-playing-has-red");
        pass &= check (hasGoldAccent (resourceImage (barResourceName (i, "selected_playing").toRawUTF8()))
                    && hasRedAccent (resourceImage (barResourceName (i, "selected_playing").toRawUTF8())), "v2-bar-selected-playing-has-red-and-gold");
    }
    pass &= check(!hasSelectedGoldContamination(resourceImage("bar_11_normal_png")), "v2-bar11-normal-has-no-selected-gold");

    ToyotomiHideyoshiAudioProcessor processor; processor.prepareToPlay(48000,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    pass &= check(editor != nullptr && editor->getWidth()==1024 && editor->getHeight()==683,"v2-editor-native-1024");
    if(!editor) return 1;
    const auto* v2 = dynamic_cast<ToyotomiHideyoshiAudioProcessorEditorV2*> (editor.get());
    pass &= check(v2 != nullptr && v2->hasValidBarMapAssets(), "v2-editor-bar-map-asset-contract");
    auto& state=processor.getStateModel(); auto defaultImage=render(*editor); pass &= check(png(defaultImage,"v2-default-stop.png"),"v2-default-render");
    pass &= check(processor.getCurrentTimelineSlot()==-1,"v2-stop-has-no-playhead");
    // This checks the actual editor paint result, not merely BinaryData decode:
    // BAR cells may never regress to the black holes in the static faceplate.
    pass &= check(cropMatchesResource(defaultImage,{259,137,56,80},"bar_01_selected_png")
               && cropMatchesResource(defaultImage,{317,137,56,80},"bar_02_normal_png")
               && cropMatchesResource(defaultImage,{378,221,56,80},"bar_11_normal_png"), "v2-bar-map-default-cells-painted");
    for (int index = 0; index < 16; ++index)
    {
        const auto bar = index + 1;
        const auto bounds = juce::Rectangle<int> { std::array<int, 8> { 259, 317, 378, 437, 494, 553, 611, 670 }[(size_t) (index % 8)], index < 8 ? 137 : 221, 56, 80 };
        const auto resource = barResourceName (bar, bar == 1 ? "selected" : "normal");
        pass &= check (cropMatchesResource (defaultImage, bounds, resource.toRawUTF8()), "v2-visible-bar-cell-exact-paint");
    }
    pass &= check(noPlayingRed(defaultImage), "v2-stop-red-cell-count-zero");
    pass &= check(cropMatchesResource(defaultImage,{251,74,105,27},"tab_1_16_selected_png")
               && cropMatchesResource(defaultImage,{360,74,105,27},"tab_17_32_normal_png"), "v2-tab-images-painted");

    const auto initialBar=state.getUiState().selectedBar; const auto initialSlot=state.getSlot(initialBar);
    const std::array<int, 8> cellX { 259, 317, 378, 437, 494, 553, 611, 670 };
    for (int tab = 0; tab < 4; ++tab)
    {
        state.selectTab (tab);
        auto image = render (*editor);
        pass &= check (png (image, "v2-tab-" + juce::String (tab + 1) + ".png"), "v2-tab-render");
        for (int cell = 0; cell < 16; ++cell)
        {
            const auto bar = tab * 16 + cell + 1;
            const auto bounds = juce::Rectangle<int> { cellX[(size_t) (cell % 8)], cell < 8 ? 137 : 221, 56, 80 };
            const auto expectedState = bar - 1 == initialBar ? "selected" : "normal";
            const auto resource = barResourceName (bar, expectedState);
            pass &= check (cropMatchesResource (image, bounds, resource.toRawUTF8()), "v2-tab-visible-bar-cells-exact-paint");
        }
        pass &= check (state.getUiState().selectedBar == initialBar && state.getSlot(initialBar).preset == initialSlot.preset, "v2-tab-state-isolation");
    }
    state.selectTab (0);
    for (const auto selectedBar : { 0, 4, 10, 15 })
    {
        state.selectBar (selectedBar);
        auto image = render (*editor);
        for (int cell = 0; cell < 16; ++cell)
        {
            const auto bounds = juce::Rectangle<int> { cellX[(size_t) (cell % 8)], cell < 8 ? 137 : 221, 56, 80 };
            const auto resource = barResourceName (cell + 1, cell == selectedBar ? "selected" : "normal");
            pass &= check (cropMatchesResource (image, bounds, resource.toRawUTF8()), "v2-only-selected-bar-uses-gold-state");
        }
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
    pass &= check (png (playing, "v2-bar-playing-separated.png"), "v2-playing-render");
    pass &= check(processor.getCurrentTimelineSlot() == 5
               && cropMatchesResource(playing,{259,137,56,80},"bar_01_selected_png")
               && cropMatchesResource(playing,{553,137,56,80},"bar_06_playing_png"), "v2-playing-red-and-selected-gold-separated");
    for (int cell = 0; cell < 16; ++cell)
    {
        const auto bounds = juce::Rectangle<int> { cellX[(size_t) (cell % 8)], cell < 8 ? 137 : 221, 56, 80 };
        const auto stateName = cell == 0 ? "selected" : (cell == 5 ? "playing" : "normal");
        const auto resource = barResourceName (cell + 1, stateName);
        pass &= check (cropMatchesResource (playing, bounds, resource.toRawUTF8()), "v2-playing-exactly-one-red-and-one-gold");
    }
    playHead.set (true, 2.5); // PPQ 2.5 -> BAR 11 (zero-based slot 10)
    processor.processBlock (audio, midi);
    state.selectBar (10);
    auto selectedPlaying = render (*editor);
    pass &= check (png (selectedPlaying, "v2-bar-selected-playing.png"), "v2-selected-playing-render");
    pass &= check(processor.getCurrentTimelineSlot() == 10
               && cropMatchesResource(selectedPlaying,{378,221,56,80},"bar_11_selected_playing_png"), "v2-selected-playing-single-state-image");
    for (int cell = 0; cell < 16; ++cell)
    {
        const auto bounds = juce::Rectangle<int> { cellX[(size_t) (cell % 8)], cell < 8 ? 137 : 221, 56, 80 };
        const auto stateName = cell == 10 ? "selected_playing" : "normal";
        const auto resource = barResourceName (cell + 1, stateName);
        pass &= check (cropMatchesResource (selectedPlaying, bounds, resource.toRawUTF8()), "v2-selected-playing-exactly-one-state-image");
    }
    playHead.set (false, 0.0);
    processor.processBlock (audio, midi);
    state.selectBar (0);
    auto stopped = render (*editor);
    pass &= check (png (stopped, "v2-bar-stopped.png"), "v2-stopped-render");
    pass &= check(processor.getCurrentTimelineSlot() == -1 && noPlayingRed(stopped), "v2-stop-clears-all-playing-red");
    processor.setPlayHead (nullptr);
    state.selectTab(0); state.selectBar(10); auto selected=render(*editor); pass &= check(png(selected,"v2-bar-selected.png"),"v2-bar-selected-render");
    state.selectBar(0); for(int p=0;p<10;++p){state.setSelectedPreset((PluginStateModel::ScratchPreset)p);auto image=render(*editor);pass &= check(png(image,"v2-preset-"+juce::String(p)+".png"),"v2-preset-render");pass &= check(state.getSlot(0).preset==(PluginStateModel::ScratchPreset)p,"v2-preset-single-source");}
    for(int l=0;l<5;++l){state.setSelectedLength((PluginStateModel::NoteLength)l);auto image=render(*editor);pass &=check(png(image,"v2-length-"+juce::String(l)+".png"),"v2-length-render");}
    const auto bypassBefore=state.getUiState().bypass; state.setSelectedPreset(PluginStateModel::ScratchPreset::custom);state.setBypass(!bypassBefore);pass &=check(state.getSlot(0).preset==PluginStateModel::ScratchPreset::custom,"v2-bypass-preset-isolation");
    state.setSlotSpeed(0,PluginStateModel::kMinSpeed);state.setSlotPitch(0,PluginStateModel::kMinPitch);state.setSlotDepth(0,0.f);auto min=render(*editor);state.setSlotSpeed(0,PluginStateModel::kMaxSpeed);state.setSlotPitch(0,PluginStateModel::kMaxPitch);state.setSlotDepth(0,1.f);auto max=render(*editor);pass &=check(different(min,max) && png(min,"v2-knobs-min.png") && png(max,"v2-knobs-max.png"),"v2-knob-min-max-render");
    editor.reset(); processor.releaseResources(); return pass ? 0 : 1;
}
