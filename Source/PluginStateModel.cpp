#include "PluginStateModel.h"
#include <cmath>

namespace
{
constexpr int kStateVersion = 2;
const juce::Identifier rootId { "ToyotomiHideyoshiState" }, globalId { "Global" }, barsId { "Bars" },
                       barId { "Bar" }, slotId { "Slot" }, countId { "Count" }, pointId { "Point" };

void writeSlot (juce::ValueTree& node, const PluginStateModel::TimelineSlot& slot)
{
    node.setProperty ("preset", static_cast<int> (slot.preset), nullptr);
    node.setProperty ("length", static_cast<int> (slot.length), nullptr);
    node.setProperty ("speed", slot.speed, nullptr);
    node.setProperty ("pitch", slot.pitch, nullptr);
    node.setProperty ("depth", slot.depth, nullptr);
    node.setProperty ("customMotion", slot.customMotion, nullptr);
    for (const auto point : slot.motion)
    {
        juce::ValueTree child (pointId);
        child.setProperty ("x", point.x, nullptr);
        child.setProperty ("y", point.y, nullptr);
        node.addChild (child, -1, nullptr);
    }
}
}

PluginStateModel::PluginStateModel() { reset(); }
int PluginStateModel::barIndex (int value) noexcept { return juce::jlimit (0, kNumBars - 1, value); }
float PluginStateModel::finiteClamp (float value, float low, float high, float fallbackValue) noexcept
{
    return std::isfinite (value) ? juce::jlimit (low, high, value) : fallbackValue;
}
std::vector<PluginStateModel::MotionPoint> PluginStateModel::sanitiseMotion (const std::vector<MotionPoint>& input)
{
    std::vector<MotionPoint> output;
    output.reserve (juce::jmin (static_cast<int> (input.size()), kMaxMotionPoints));
    for (const auto point : input)
    {
        if (! std::isfinite (point.x) || ! std::isfinite (point.y)) continue;
        const MotionPoint clamped { juce::jlimit (0.0f, 1.0f, point.x), juce::jlimit (0.0f, 1.0f, point.y) };
        if (output.empty() || std::hypot (clamped.x - output.back().x, clamped.y - output.back().y) >= 0.004f)
            output.push_back (clamped);
        if (static_cast<int> (output.size()) == kMaxMotionPoints) break;
    }
    return output;
}
std::vector<PluginStateModel::MotionPoint> PluginStateModel::presetMotion (ScratchPreset preset)
{
    if (preset == ScratchPreset::off) return {};
    if (preset == ScratchPreset::zigzag) return {{ 0,.75f },{ .25f,.25f },{ .5f,.75f },{ .75f,.25f },{ 1,.75f }};
    if (preset == ScratchPreset::backspin) return {{ 1,.3f },{ .72f,.45f },{ .42f,.65f },{ .12f,.8f }};
    return {{ 0,.65f },{ .35f,.45f },{ .7f,.3f },{ 1,.5f }};
}
void PluginStateModel::reset() { slots = {}; ui = UiState {}; }
const PluginStateModel::TimelineSlot& PluginStateModel::getSlot (int bar) const noexcept { return slots[static_cast<size_t> (barIndex (bar))]; }
PluginStateModel::TimelineSlot& PluginStateModel::mutableSlot (int bar) noexcept { return slots[static_cast<size_t> (barIndex (bar))]; }
void PluginStateModel::selectTab (int tab) { ui.selectedTab = juce::jlimit (0, 3, tab); }
void PluginStateModel::selectBar (int bar) { ui.selectedBar = bar == kNoSelectedBar ? kNoSelectedBar : barIndex (bar); }
void PluginStateModel::setBypass (bool enabled) { ui.bypass = enabled; }
void PluginStateModel::setSlotPreset (int bar, ScratchPreset preset) { auto& slot = mutableSlot (bar); slot.preset = preset; slot.customMotion = false; slot.motion = presetMotion (preset); }
void PluginStateModel::setSlotLength (int bar, NoteLength value) { mutableSlot (bar).length = static_cast<NoteLength> (juce::jlimit (0, 4, static_cast<int> (value))); }
void PluginStateModel::setSlotSpeed (int bar, float value) { mutableSlot (bar).speed = finiteClamp (value, kMinSpeed, kMaxSpeed, 1.0f); }
void PluginStateModel::setSlotPitch (int bar, float value) { mutableSlot (bar).pitch = finiteClamp (value, kMinPitch, kMaxPitch, 0.0f); }
void PluginStateModel::setSlotDepth (int bar, float value) { mutableSlot (bar).depth = finiteClamp (value, 0.0f, 1.0f, 0.5f); }
void PluginStateModel::setSlotMotion (int bar, const std::vector<MotionPoint>& motion) { auto& slot = mutableSlot (bar); slot.motion = sanitiseMotion (motion); slot.customMotion = true; slot.preset = ScratchPreset::custom; }
void PluginStateModel::clearSlotMotion (int bar) { auto& slot = mutableSlot (bar); slot.motion.clear(); slot.customMotion = false; if (slot.preset == ScratchPreset::custom) slot.preset = ScratchPreset::off; }
void PluginStateModel::resetSlot (int bar) { mutableSlot (bar) = TimelineSlot {}; }
void PluginStateModel::setSelectedPreset (ScratchPreset preset) { if (hasSelectedBar (ui.selectedBar)) setSlotPreset (ui.selectedBar, preset); }
void PluginStateModel::setSelectedLength (NoteLength value) { if (hasSelectedBar (ui.selectedBar)) setSlotLength (ui.selectedBar, value); }
void PluginStateModel::setSelectedSpeed (float value) { if (hasSelectedBar (ui.selectedBar)) setSlotSpeed (ui.selectedBar, value); }
void PluginStateModel::setSelectedPitch (float value) { if (hasSelectedBar (ui.selectedBar)) setSlotPitch (ui.selectedBar, value); }
void PluginStateModel::setSelectedDepth (float value) { if (hasSelectedBar (ui.selectedBar)) setSlotDepth (ui.selectedBar, value); }
void PluginStateModel::setSelectedMotion (const std::vector<MotionPoint>& motion) { if (hasSelectedBar (ui.selectedBar)) setSlotMotion (ui.selectedBar, motion); }
void PluginStateModel::clearSelectedMotion() { if (hasSelectedBar (ui.selectedBar)) clearSlotMotion (ui.selectedBar); }
void PluginStateModel::resetSelectedSlot() { if (hasSelectedBar (ui.selectedBar)) resetSlot (ui.selectedBar); }

juce::ValueTree PluginStateModel::toValueTree() const
{
    juce::ValueTree root (rootId);
    root.setProperty ("stateVersion", kStateVersion, nullptr);
    juce::ValueTree global (globalId);
    global.setProperty ("selectedTab", ui.selectedTab, nullptr);
    global.setProperty ("selectedBar", ui.selectedBar, nullptr);
    global.setProperty ("bypass", ui.bypass, nullptr);
    root.addChild (global, -1, nullptr);
    juce::ValueTree timeline (barsId);
    for (int bar = 0; bar < kNumBars; ++bar)
    {
        juce::ValueTree node (slotId);
        node.setProperty ("index", bar, nullptr);
        writeSlot (node, slots[static_cast<size_t> (bar)]);
        timeline.addChild (node, -1, nullptr);
    }
    root.addChild (timeline, -1, nullptr);
    return root;
}

bool PluginStateModel::fromValueTree (const juce::ValueTree& root)
{
    if (! root.hasType (rootId)) return false;
    const auto version = static_cast<int> (root.getProperty ("stateVersion", 0));
    if (version < 1 || version > kStateVersion) return false;
    PluginStateModel parsed;
    const auto global = root.getChildWithName (globalId);
    if (global.isValid())
    {
        parsed.ui.selectedTab = juce::jlimit (0, 3, static_cast<int> (global.getProperty ("selectedTab", 0)));
        const auto restoredBar = static_cast<int> (global.getProperty ("selectedBar", 0));
        parsed.ui.selectedBar = restoredBar == kNoSelectedBar ? kNoSelectedBar : barIndex (restoredBar);
        parsed.ui.bypass = static_cast<bool> (global.getProperty ("bypass", false));
    }
    const auto timeline = root.getChildWithName (barsId);
    for (int i = 0; i < timeline.getNumChildren(); ++i)
    {
        const auto node = timeline.getChild (i);
        const auto bar = static_cast<int> (node.getProperty ("index", -1));
        if (bar < 0 || bar >= kNumBars) continue;
        // v1 stored 16 Counts beneath each Bar. Migrate its first Count into
        // the single v2 timeline slot, preserving a deterministic legacy value.
        const auto legacyCount = version == 1 ? node.getChildWithName (countId) : juce::ValueTree {};
        const auto source = version == 1 ? legacyCount : node;
        if (! source.isValid()) continue;
        auto& slot = parsed.mutableSlot (bar);
        slot.preset = static_cast<ScratchPreset> (juce::jlimit (0, 9, static_cast<int> (source.getProperty ("preset", 0))));
        slot.length = static_cast<NoteLength> (juce::jlimit (0, 4, static_cast<int> (source.getProperty ("length", 0))));
        slot.speed = finiteClamp (static_cast<float> (source.getProperty ("speed", 1.0)), kMinSpeed, kMaxSpeed, 1.0f);
        slot.pitch = finiteClamp (static_cast<float> (source.getProperty ("pitch", 0.0)), kMinPitch, kMaxPitch, 0.0f);
        slot.depth = finiteClamp (static_cast<float> (source.getProperty ("depth", 0.5)), 0.0f, 1.0f, 0.5f);
        slot.customMotion = static_cast<bool> (source.getProperty ("customMotion", false));
        std::vector<MotionPoint> motion;
        for (int p = 0; p < source.getNumChildren(); ++p)
        {
            const auto point = source.getChild (p);
            if (point.hasType (pointId)) motion.push_back ({ static_cast<float> (point.getProperty ("x", NAN)), static_cast<float> (point.getProperty ("y", NAN)) });
        }
        slot.motion = sanitiseMotion (motion);
        if (slot.customMotion) slot.preset = ScratchPreset::custom;
    }
    *this = std::move (parsed);
    return true;
}
