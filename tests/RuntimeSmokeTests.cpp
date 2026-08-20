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
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui; bool pass=true;
    pass &= check(resourceIs("neutral_static_background_1024x683_png",1024,683),"v2-static-background-native");
    pass &= check(resourceIs("knob_ring_60_png",48,48) && resourceIs("knob_pointer_60_png",48,48),"v2-knob-assets-native");
    pass &= check(resourceIs("xy_neutral_base_288x256_png",192,174),"v2-xy-native");
    pass &= check(resourceIs("bypass_off_png",80,31) && resourceIs("bypass_on_png",80,31),"v2-bypass-native");
    pass &= check(resourceIs("rec_normal_png",59,23) && resourceIs("clear_normal_png",59,23) && resourceIs("reset_view_normal_png",82,23),"v2-xy-buttons-native");
    for (int i=1;i<=64;++i) pass &= check(resourceIs(juce::String::formatted("bar_%02d_normal_png",i).toRawUTF8(),56,80),"v2-bar-cell-native");

    ToyotomiHideyoshiAudioProcessor processor; processor.prepareToPlay(48000,512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    pass &= check(editor != nullptr && editor->getWidth()==1024 && editor->getHeight()==683,"v2-editor-native-1024");
    if(!editor) return 1;
    auto& state=processor.getStateModel(); auto defaultImage=render(*editor); pass &= check(png(defaultImage,"v2-default-stop.png"),"v2-default-render");
    pass &= check(processor.getCurrentTimelineSlot()==-1,"v2-stop-has-no-playhead");

    const auto initialBar=state.getUiState().selectedBar; const auto initialSlot=state.getSlot(initialBar);
    for(int tab=0;tab<4;++tab){state.selectTab(tab);auto image=render(*editor);pass &= check(png(image,"v2-tab-"+juce::String(tab+1)+".png"),"v2-tab-render");pass &= check(state.getUiState().selectedBar==initialBar && state.getSlot(initialBar).preset==initialSlot.preset,"v2-tab-state-isolation");}
    state.selectTab(0); state.selectBar(10); auto selected=render(*editor); pass &= check(png(selected,"v2-bar-selected.png"),"v2-bar-selected-render");
    state.selectBar(0); for(int p=0;p<10;++p){state.setSelectedPreset((PluginStateModel::ScratchPreset)p);auto image=render(*editor);pass &= check(png(image,"v2-preset-"+juce::String(p)+".png"),"v2-preset-render");pass &= check(state.getSlot(0).preset==(PluginStateModel::ScratchPreset)p,"v2-preset-single-source");}
    for(int l=0;l<5;++l){state.setSelectedLength((PluginStateModel::NoteLength)l);auto image=render(*editor);pass &=check(png(image,"v2-length-"+juce::String(l)+".png"),"v2-length-render");}
    const auto bypassBefore=state.getUiState().bypass; state.setSelectedPreset(PluginStateModel::ScratchPreset::custom);state.setBypass(!bypassBefore);pass &=check(state.getSlot(0).preset==PluginStateModel::ScratchPreset::custom,"v2-bypass-preset-isolation");
    state.setSlotSpeed(0,PluginStateModel::kMinSpeed);state.setSlotPitch(0,PluginStateModel::kMinPitch);state.setSlotDepth(0,0.f);auto min=render(*editor);state.setSlotSpeed(0,PluginStateModel::kMaxSpeed);state.setSlotPitch(0,PluginStateModel::kMaxPitch);state.setSlotDepth(0,1.f);auto max=render(*editor);pass &=check(different(min,max) && png(min,"v2-knobs-min.png") && png(max,"v2-knobs-max.png"),"v2-knob-min-max-render");
    editor.reset(); processor.releaseResources(); return pass ? 0 : 1;
}
