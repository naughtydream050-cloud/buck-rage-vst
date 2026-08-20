#include "PluginEditorV2.h"
#include <array>
#include <cmath>

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
#endif

namespace
{
constexpr int kW = 1024, kH = 683;
const std::array<int, 8> kCellX { 259, 317, 378, 437, 494, 553, 611, 670 };
const std::array<int, 2> kCellY { 137, 221 };
const std::array<juce::Rectangle<int>, 4> kTabs {{{251,74,105,27},{360,74,105,27},{470,74,106,27},{580,74,105,27}}};
const std::array<juce::Rectangle<int>, 10> kPresets {{{750,100,84,64},{836,100,84,64},{924,100,84,64},{750,166,84,64},{836,166,84,64},{924,166,84,64},{750,232,84,64},{836,232,84,64},{924,232,84,64},{750,296,84,56}}};
const std::array<juce::Rectangle<int>, 5> kLengths {{{742,425,32,26},{773,425,32,26},{803,425,32,26},{834,425,32,26},{864,425,32,26}}};
const std::array<juce::Rectangle<int>, 3> kKnobs {{{744,513,48,48},{793,513,48,48},{848,513,48,48}}};
const std::array<juce::Rectangle<int>, 3> kReadouts {{{742,563,48,16},{793,563,48,16},{848,563,48,16}}};
const std::array<const char*, 10> kPresetNames {{"off","forward_cut","backspin","chirp","baby","transform","drag","zigzag","tape_brake","custom"}};
const std::array<const char*, 5> kLengthNames {{"1_16","1_8","1_4","1_2","1_bar"}};
const std::array<const char*, 4> kTabNames {{"1_16","17_32","33_48","49_64"}};

juce::String resourceName (juce::String filename) { return filename.replaceCharacters (".- ", "___"); }
juce::Image asset (const juce::String& filename)
{
   #if __has_include(<BinaryData.h>)
    int bytes = 0; const auto* data = BinaryData::getNamedResource (resourceName (filename).toRawUTF8(), bytes);
    return data != nullptr ? juce::ImageFileFormat::loadFrom (data, (size_t) bytes) : juce::Image {};
   #else
    juce::ignoreUnused (filename); return {};
   #endif
}
void drawNative (juce::Graphics& g, const juce::Image& im, juce::Rectangle<int> b)
{
    if (! im.isValid() || im.getWidth() != b.getWidth() || im.getHeight() != b.getHeight()) return;
    g.drawImageAt (im, b.getX(), b.getY());
}
juce::Rectangle<int> cellBounds (int i) { return { kCellX[(size_t) (i % 8)], kCellY[(size_t) (i / 8)], 56, 80 }; }
juce::Colour stateColour (bool selected, bool playing)
{ return playing ? juce::Colour (0xffdf4338) : selected ? juce::Colour (0xffd6a446) : juce::Colour (0xffe3d7c5); }
}

class ToyotomiHideyoshiAudioProcessorEditorV2::HitRegion final : public juce::Component
{
public:
    explicit HitRegion (std::function<void()> f) : fn (std::move (f)) { setOpaque (false); }
    void mouseDown (const juce::MouseEvent&) override { if (fn) fn(); }
private: std::function<void()> fn;
};

class ToyotomiHideyoshiAudioProcessorEditorV2::KnobRegion final : public juce::Component
{
public:
    KnobRegion (ToyotomiHideyoshiAudioProcessor& p, int i) : processor (p), index (i) {}
    void mouseDown (const juce::MouseEvent& e) override { previousY = e.position.y; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const auto d = previousY - e.position.y; previousY = e.position.y; auto& s=processor.getStateModel(); const auto bar=s.getUiState().selectedBar; const auto& slot=s.getSlot(bar);
        if(index==0) s.setSlotSpeed(bar,slot.speed+d*.012f); else if(index==1) s.setSlotPitch(bar,slot.pitch+d*.20f); else s.setSlotDepth(bar,slot.depth+d*.01f);
    }
private: ToyotomiHideyoshiAudioProcessor& processor; int index; float previousY=0.0f;
};

class ToyotomiHideyoshiAudioProcessorEditorV2::XYRegion final : public juce::Component
{
public:
    explicit XYRegion (ToyotomiHideyoshiAudioProcessor& p) : processor(p) {}
    void mouseDown (const juce::MouseEvent& e) override { points.clear(); add(e.position); }
    void mouseDrag (const juce::MouseEvent& e) override { add(e.position); }
    void mouseUp (const juce::MouseEvent&) override { processor.getStateModel().setSelectedMotion(points); }
private:
    void add (juce::Point<float> p) { if(points.size()<PluginStateModel::kMaxMotionPoints) points.push_back({juce::jlimit(0.f,1.f,p.x/(float)getWidth()),juce::jlimit(0.f,1.f,p.y/(float)getHeight())}); }
    ToyotomiHideyoshiAudioProcessor& processor; std::vector<PluginStateModel::MotionPoint> points;
};

class ToyotomiHideyoshiAudioProcessorEditorV2::Surface final : public juce::Component
{
public:
    explicit Surface (ToyotomiHideyoshiAudioProcessor& p) : processor(p)
    {
        background=asset("neutral_static_background_1024x683.png"); ring=asset("knob_ring_60.png"); pointer=asset("knob_pointer_60.png"); xy=asset("xy_neutral_base_288x256.png");
        bypass[0]=asset("bypass_off.png"); bypass[1]=asset("bypass_on.png"); rec=asset("rec_normal.png"); clear=asset("clear_normal.png"); reset=asset("reset_view_normal.png");
    }
    void paint (juce::Graphics& g) override
    {
        drawNative(g,background,{0,0,kW,kH}); const auto ui=processor.getStateModel().getUiState(); const int tab=ui.selectedTab, selected=ui.selectedBar, playing=processor.getCurrentTimelineSlot();
        for(int i=0;i<4;++i) drawNative(g,asset("tab_"+juce::String(kTabNames[(size_t)i])+"_"+(i==tab?"selected":"normal")+".png"),kTabs[(size_t)i]);
        for(int i=0;i<16;++i) { int bar=tab*16+i; bool isSel=bar==selected, isPlay=bar==playing; auto state=isPlay?(isSel?"selected_playing":"playing"):(isSel?"selected":"normal"); drawNative(g,asset(juce::String::formatted("bar_%02d_%s.png",bar+1,state)),cellBounds(i)); }
        const auto& slot=processor.getStateModel().getSlot(selected);
        for(int i=0;i<10;++i) drawNative(g,asset("preset_"+juce::String(kPresetNames[(size_t)i])+"_"+(i==(int)slot.preset?"selected":"normal")+".png"),kPresets[(size_t)i]);
        for(int i=0;i<5;++i) drawNative(g,asset("length_"+juce::String(kLengthNames[(size_t)i])+"_"+(i==(int)slot.length?"selected":"normal")+".png"),kLengths[(size_t)i]);
        drawNative(g,bypass[ui.bypass?1:0],{931,14,80,31}); drawNative(g,xy,{40,429,192,174}); drawNative(g,rec,{27,600,59,23}); drawNative(g,clear,{95,600,59,23}); drawNative(g,reset,{159,600,82,23});
        const std::array<float,3> value {{slot.speed,slot.pitch,slot.depth}}; const std::array<float,3> normal {{(slot.speed-.25f)/3.75f,(slot.pitch+12.f)/24.f,slot.depth}};
        const std::array<juce::String,3> text {{juce::String(slot.speed,2)+"x",juce::String(slot.pitch,1)+" st",juce::String(juce::roundToInt(slot.depth*100.f))+" %"}};
        for(int i=0;i<3;++i) { drawNative(g,ring,kKnobs[(size_t)i]); const auto b=kKnobs[(size_t)i]; g.saveState(); const auto c=b.getCentre().toFloat(); const auto angle=juce::MathConstants<float>::pi*1.25f+juce::MathConstants<float>::pi*1.5f*juce::jlimit(0.f,1.f,normal[(size_t)i]); g.addTransform(juce::AffineTransform::rotation(angle+juce::MathConstants<float>::halfPi,c.x,c.y)); drawNative(g,pointer,b); g.restoreState(); g.setColour(juce::Colour(0xffe3d7c5)); g.setFont(10.f); g.drawText(text[(size_t)i],kReadouts[(size_t)i],juce::Justification::centred); }
        // Trace only; the supplied XY base owns all fixed geometry.
        if(!slot.motion.empty()) { juce::Path trace; for(size_t i=0;i<slot.motion.size();++i){ auto p=slot.motion[i]; auto pt=juce::Point<float>(40.f+p.x*192.f,429.f+p.y*174.f); if(i==0)trace.startNewSubPath(pt);else trace.lineTo(pt);} g.saveState();g.reduceClipRegion({40,429,192,174});g.setColour(juce::Colour(0xffd6a446));g.strokePath(trace,juce::PathStrokeType(1.5f));g.restoreState(); }
    }
private:
    ToyotomiHideyoshiAudioProcessor& processor; juce::Image background,ring,pointer,xy,rec,clear,reset; std::array<juce::Image,2> bypass;
};

ToyotomiHideyoshiAudioProcessorEditorV2::ToyotomiHideyoshiAudioProcessorEditorV2 (ToyotomiHideyoshiAudioProcessor& p) : AudioProcessorEditor(&p),processor(p)
{
    surface=std::make_unique<Surface>(processor); addAndMakeVisible(*surface); surface->setInterceptsMouseClicks(false,false);
    for(int i=0;i<4;++i) addImageHit(kTabs[(size_t)i],[this,i]{processor.getStateModel().selectTab(i);});
    for(int i=0;i<16;++i) addImageHit(cellBounds(i),[this,i]{const auto page=processor.getStateModel().getUiState().selectedTab;processor.getStateModel().selectBar(page*16+i);});
    for(int i=0;i<10;++i) addImageHit(kPresets[(size_t)i],[this,i]{processor.getStateModel().setSelectedPreset((PluginStateModel::ScratchPreset)i);});
    for(int i=0;i<5;++i) addImageHit(kLengths[(size_t)i],[this,i]{processor.getStateModel().setSelectedLength((PluginStateModel::NoteLength)i);});
    addImageHit({931,14,80,31},[this]{auto&s=processor.getStateModel();s.setBypass(!s.getUiState().bypass);});
    addImageHit({27,600,59,23},[]{}); addImageHit({95,600,59,23},[this]{processor.getStateModel().clearSelectedMotion();}); addImageHit({159,600,82,23},[this]{processor.getStateModel().resetSelectedSlot();});
    for(int i=0;i<3;++i){knobs[(size_t)i]=std::make_unique<KnobRegion>(processor,i);addAndMakeVisible(*knobs[(size_t)i]);knobs[(size_t)i]->setBounds(kKnobs[(size_t)i]);}
    xyInput=std::make_unique<XYRegion>(processor);addAndMakeVisible(*xyInput);xyInput->setBounds({40,429,192,174});
    setResizable(false,false);setSize(kW,kH);startTimerHz(30);
}
void ToyotomiHideyoshiAudioProcessorEditorV2::paint(juce::Graphics& g){juce::ignoreUnused(g);}
void ToyotomiHideyoshiAudioProcessorEditorV2::resized(){surface->setBounds(getLocalBounds());}
void ToyotomiHideyoshiAudioProcessorEditorV2::timerCallback(){surface->repaint();}
void ToyotomiHideyoshiAudioProcessorEditorV2::addImageHit(juce::Rectangle<int> b,std::function<void()> f){auto*h=new HitRegion(std::move(f));h->setBounds(b);addAndMakeVisible(h);hitRegions.add(h);}
