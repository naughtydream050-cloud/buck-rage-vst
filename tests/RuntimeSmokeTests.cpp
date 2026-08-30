#include "PluginProcessor.h"
#include "PluginEditorV2.h"
#include "GeneratedLayout.h"
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

bool usesNeutralBarBase (const juce::Image& target, const juce::Image& neutral)
{
    if (! target.isValid() || ! neutral.isValid()
        || target.getWidth() != 56 || target.getHeight() != 80
        || neutral.getWidth() != 56 || neutral.getHeight() != 80)
        return false;

    const std::array<juce::Rectangle<int>, 3> content {{ { 5, 6, 46, 24 }, { 7, 32, 42, 25 }, { 18, 57, 20, 18 } }};
    for (int y = 0; y < 80; ++y)
        for (int x = 0; x < 56; ++x)
            if (! std::any_of (content.begin(), content.end(), [x, y] (auto r) { return r.contains (x, y); })
                && target.getPixelAt (x, y) != neutral.getPixelAt (x, y))
                return false;
    return true;
}

bool fullyTransparent (const juce::Image& image, juce::Rectangle<int> bounds)
{
    if (! image.isValid() || ! image.getBounds().contains (bounds)) return false;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
            if (image.getPixelAt (bounds.getX() + x, bounds.getY() + y).getAlpha() != 0)
                return false;
    return true;
}

bool opaqueRgbMatches (const juce::Image& actual, const juce::Image& reference, juce::Rectangle<int> bounds)
{
    if (! actual.isValid() || ! reference.isValid() || ! actual.getBounds().contains (bounds)
        || ! reference.getBounds().contains (bounds)) return false;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            const auto a = actual.getPixelAt (bounds.getX() + x, bounds.getY() + y);
            const auto b = reference.getPixelAt (bounds.getX() + x, bounds.getY() + y);
            if (a.getAlpha() != 255 || a.getRed() != b.getRed() || a.getGreen() != b.getGreen() || a.getBlue() != b.getBlue())
                return false;
        }
    return true;
}

bool hasNoDynamicGoldTrace (const juce::Image& image, juce::Rectangle<int> bounds)
{
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            const auto c = image.getPixelAt (bounds.getX() + x, bounds.getY() + y);
            if (c.getRed() > 150 && c.getGreen() > 80 && c.getBlue() < 145 && c.getRed() - c.getGreen() > 25)
                return false;
        }
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

juce::var jsonProperty (const juce::var& object, const juce::String& key)
{
    return jsonProperty (object, key.toRawUTF8());
}

juce::String byteFingerprint (const void* data, size_t size)
{
    auto hash = uint64_t { 1469598103934665603ull };
    const auto* bytes = static_cast<const uint8_t*> (data);
    for (size_t i = 0; i < size; ++i)
        hash = (hash ^ bytes[i]) * 1099511628211ull;
    return juce::String::toHexString ((juce::int64) hash);
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
                // State artwork owns its one-pixel exterior rim as well.  The
                // faceplate clears that rim so a neutral state cannot retain
                // the master image's gold selection edge.
                g.fillRect (jsonBounds (jsonProperty (region, "bounds")).expanded (1));
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

constexpr int kGateCellWidth = 56, kGateCellHeight = 80;
const std::array<int, 8> kGateCellX { 259, 317, 378, 437, 494, 553, 611, 670 };
const std::array<const char*, 10> kGatePresetNames {{ "off", "forward_cut", "backspin", "chirp", "baby",
                                                       "transform", "drag", "zigzag", "tape_brake", "custom" }};
const std::array<const char*, 5> kGateLengthNames {{ "1_16", "1_8", "1_4", "1_2", "1_bar" }};
const juce::Rectangle<int> kGateBarMapBounds { 259, 137, 467, 164 };
const juce::Rectangle<int> kGateBypassBounds { 931, 14, 80, 31 };

juce::Rectangle<int> gateCellBounds (int index)
{
    return { kGateCellX[(size_t) (index % 8)], index < 8 ? 137 : 221, kGateCellWidth, kGateCellHeight };
}

int gateCropMismatchPixels (const juce::Image& rendered, juce::Rectangle<int> bounds, const juce::String& resource)
{
    const auto expected = resourceImage (resource.toRawUTF8());
    if (! expected.isValid() || expected.getWidth() != bounds.getWidth() || expected.getHeight() != bounds.getHeight())
        return -1;
    int differing = 0;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
            if (rendered.getPixelAt (bounds.getX() + x, bounds.getY() + y) != expected.getPixelAt (x, y))
                ++differing;
    return differing;
}

// LENGTH's locked manifest has intentional 1–2px shared edges.  Surface::paint
// draws the later button last, so test the exact visible ownership of every
// PNG rather than incorrectly expecting a covered edge to remain visible.
int gateVisibleCropMismatchPixels (const juce::Image& rendered, juce::Rectangle<int> bounds,
                                   const juce::String& resource,
                                   const juce::Array<juce::Rectangle<int>>& laterBounds)
{
    const auto expected = resourceImage (resource.toRawUTF8());
    if (! expected.isValid() || expected.getWidth() != bounds.getWidth() || expected.getHeight() != bounds.getHeight())
        return -1;
    int differing = 0;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            const auto point = juce::Point<int> { bounds.getX() + x, bounds.getY() + y };
            bool covered = false;
            for (const auto& later : laterBounds)
                if (later.contains (point)) { covered = true; break; }
            if (! covered && rendered.getPixelAt (point.x, point.y) != expected.getPixelAt (x, y))
                ++differing;
        }
    return differing;
}

int gateCountSelectionGold (const juce::Image& image, juce::Rectangle<int> bounds)
{
    int count = 0;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            const auto c = image.getPixelAt (bounds.getX() + x, bounds.getY() + y);
            if (c.getRed() > 120 && c.getGreen() > 75 && c.getBlue() < 70 && c.getRed() > c.getGreen() * 1.15f)
                ++count;
        }
    return count;
}

int gateCountPlayingRed (const juce::Image& image, juce::Rectangle<int> bounds)
{
    int count = 0;
    for (int y = 0; y < bounds.getHeight(); ++y)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            const auto c = image.getPixelAt (bounds.getX() + x, bounds.getY() + y);
            if (c.getRed() > 135 && c.getGreen() < 100 && c.getBlue() < 100)
                ++count;
        }
    return count;
}

struct GateRegionDiffStats
{
    int differingInsideAllowed = 0, differingOutsideAllowed = 0;
    juce::Rectangle<int> outsideMismatchBounds;
};

GateRegionDiffStats gateCompareAgainstAllowedRegions (const juce::Image& a, const juce::Image& b,
                                                      const juce::Array<juce::Rectangle<int>>& allowed,
                                                      int* differingInsideTotal = nullptr)
{
    GateRegionDiffStats stats;
    bool firstOutside = true;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
        {
            if (a.getPixelAt (x, y) == b.getPixelAt (x, y)) continue;
            bool inside = false;
            for (const auto& region : allowed)
                if ((inside = region.contains (x, y)) == true) break;
            if (inside)
                ++stats.differingInsideAllowed;
            else
            {
                ++stats.differingOutsideAllowed;
                const juce::Rectangle<int> pixel { x, y, 1, 1 };
                stats.outsideMismatchBounds = firstOutside ? pixel : stats.outsideMismatchBounds.getUnion (pixel);
                firstOutside = false;
            }
        }
    if (differingInsideTotal != nullptr) *differingInsideTotal = stats.differingInsideAllowed;
    return stats;
}

class DynamicStateVisualReporter final
{
public:
    void beginCase (const juce::String& name, const juce::String& description, const juce::var& stateSnapshot)
    {
        currentObject = new juce::DynamicObject();
        currentObject->setProperty ("name", name);
        currentObject->setProperty ("description", description);
        currentObject->setProperty ("state", stateSnapshot);
        currentChecks = juce::Array<juce::var>();
        currentPass = true;
        currentSaved = {};
    }

    void checkValue (const char* expectation, bool ok, int value)
    {
        auto* entry = new juce::DynamicObject();
        entry->setProperty ("expectation", expectation);
        entry->setProperty ("status", ok ? "PASS" : "FAIL");
        entry->setProperty ("value", value);
        currentChecks.add (juce::var (entry));
        currentPass = currentPass && ok;
        if (! ok) std::cout << "FAIL " << currentName() << ':' << expectation << '\n';
    }

    void check (const char* expectation, bool ok) { checkValue (expectation, ok, ok ? 1 : 0); }

    void screenshot (const juce::Image& image, const juce::String& file)
    {
        png (image, file);
        currentSaved.add (file);
    }

    bool endCase()
    {
        currentObject->setProperty ("checks", currentChecks);
        currentObject->setProperty ("saved_images", currentSaved);
        currentObject->setProperty ("status", currentPass ? "PASS" : "FAIL");
        cases.add (juce::var (currentObject));
        std::cout << (currentPass ? "PASS " : "FAIL ") << "[DYNAMIC_STATE_VISUAL_GATE] "
                  << currentName() << '\n';
        overall &= currentPass;
        if (currentPass) ++passedCases_;
        return currentPass;
    }

    bool finishReport() const
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("gate", "DYNAMIC_STATE_VISUAL_GATE");
        root->setProperty ("canvas", juce::Array<juce::var> { 1024, 683 });
        root->setProperty ("cases", cases);
        root->setProperty ("passed_cases", cases.size() - failedCases());
        root->setProperty ("failed_cases", failedCases());
        root->setProperty ("status", overall ? "PASS" : "FAIL");
        const auto text = juce::JSON::toString (juce::var (root), true);
        juce::File::getCurrentWorkingDirectory().getChildFile ("dynamic-state-report.json").replaceWithText (text);
        juce::File::getCurrentWorkingDirectory().getChildFile ("v2-dynamic-state-report.json").replaceWithText (text);
        return overall;
    }

private:
    int failedCases() const { return cases.size() - passedCases_; }
    juce::String currentName() const
    {
        return currentObject != nullptr ? currentObject->getProperty ("name").toString() : juce::String();
    }

    juce::Array<juce::var> cases, currentChecks, currentSaved;
    juce::DynamicObject* currentObject = nullptr;
    bool currentPass = true, overall = true;
    mutable int passedCases_ = 0;
};

bool verifyGateBarPage (const juce::Image& image, int tab, DynamicStateVisualReporter& reporter,
                        int selectedAbsoluteBar, const char* selectedStateSuffix,
                        bool requireZeroSelectionGoldAnywhere)
{
    int mismatchedCells = 0;
    for (int cell = 0; cell < 16; ++cell)
    {
        const auto absolute = tab * 16 + cell;
        const auto suffix = absolute == selectedAbsoluteBar ? selectedStateSuffix : "normal";
        const auto resource = "bar_" + juce::String (absolute + 1).paddedLeft ('0', 2) + "_" + suffix + "_png";
        const auto mismatches = gateCropMismatchPixels (image, gateCellBounds (cell), resource);
        mismatchedCells += mismatches == 0 ? 0 : 1;
        reporter.checkValue (("page-cells-match-completed-assets:" + resource).toRawUTF8(),
                             mismatches == 0, mismatches);
    }
    const auto selectedVisible = selectedAbsoluteBar >= tab * 16 && selectedAbsoluteBar < (tab + 1) * 16;
    const auto totalGold = gateCountSelectionGold (image, kGateBarMapBounds);
    const auto selectedBounds = selectedVisible ? gateCellBounds (selectedAbsoluteBar - tab * 16)
                                                : juce::Rectangle<int> {};
    const auto goldInSelectedCell = selectedVisible ? gateCountSelectionGold (image, selectedBounds) : 0;
    const auto totalRed = gateCountPlayingRed (image, kGateBarMapBounds);
    reporter.checkValue ("selection-gold-confined-to-selected-cell-or-zero",
                         requireZeroSelectionGoldAnywhere ? ! selectedVisible : totalGold == goldInSelectedCell, totalGold);
    reporter.checkValue ("playing-red-absent-without-playhead", totalRed == 0, totalRed);
    return mismatchedCells == 0;
}

juce::var gateStateSnapshot (int tab, int selectedBar, int playingSlot, int presetIndex, int lengthIndex,
                             bool bypass)
{
    auto* object = new juce::DynamicObject();
    object->setProperty ("tab", tab);
    object->setProperty ("selected_bar_1based", selectedBar + 1);
    object->setProperty ("playing_slot", playingSlot);
    object->setProperty ("playing_bar_1based", playingSlot < 0 ? 0 : playingSlot + 1);
    object->setProperty ("visible_page", tab + 1);
    object->setProperty ("expected_selected_cell_count", selectedBar >= tab * 16 && selectedBar < (tab + 1) * 16 ? 1 : 0);
    object->setProperty ("expected_red_cell_count", playingSlot >= tab * 16 && playingSlot < (tab + 1) * 16 ? 1 : 0);
    object->setProperty ("preset_index", presetIndex);
    object->setProperty ("length_index", lengthIndex);
    object->setProperty ("bypass", bypass);
    return juce::var (object);
}

struct GateGeometry
{
    juce::Array<juce::Rectangle<int>> presetBounds, lengthBounds;
    juce::Rectangle<int> bypassBounds, xyRegionBounds, presetRegionBounds,
                         speedRegionBounds, pitchRegionBounds, depthRegionBounds;
    bool resolved = false;
};

GateGeometry resolveGateGeometry (const juce::var& regions, const juce::var& interactive)
{
    GateGeometry geometry;
    auto findById = [interactive] (const juce::String& id) -> juce::Rectangle<int>
    {
        if (const auto* array = interactive.getArray())
            for (const auto& item : *array)
                if (jsonProperty (item, "id").toString() == id)
                    return jsonBounds (jsonProperty (item, "bounds"));
        return {};
    };
    auto findRegionByName = [regions] (const char* name) -> juce::Rectangle<int>
    {
        if (const auto* array = regions.getArray())
            for (const auto& item : *array)
                if (jsonProperty (item, "name").toString() == name)
                    return jsonBounds (jsonProperty (item, "bounds"));
        return {};
    };
    for (const auto* preset : kGatePresetNames)
        geometry.presetBounds.add (findById ("preset_" + juce::String (preset)));
    for (const auto* length : kGateLengthNames)
        geometry.lengthBounds.add (findById ("length_" + juce::String (length)));
    geometry.bypassBounds = findById ("bypass");
    geometry.xyRegionBounds = findRegionByName ("XY");
    geometry.presetRegionBounds = findRegionByName ("PRESET");
    geometry.speedRegionBounds = findRegionByName ("SPEED");
    geometry.pitchRegionBounds = findRegionByName ("PITCH");
    geometry.depthRegionBounds = findRegionByName ("DEPTH");
    geometry.resolved = geometry.presetBounds.size() == 10 && geometry.lengthBounds.size() == 5
                     && ! geometry.bypassBounds.isEmpty() && ! geometry.xyRegionBounds.isEmpty()
                     && ! geometry.presetRegionBounds.isEmpty() && ! geometry.speedRegionBounds.isEmpty()
                     && ! geometry.pitchRegionBounds.isEmpty() && ! geometry.depthRegionBounds.isEmpty();
    return geometry;
}

void appendBarPixelTrace (juce::Array<juce::var>& output, const juce::var& runtimeManifest,
                          const juce::Image& rendered, int bar, juce::Rectangle<int> bounds,
                          const char* state)
{
    const auto bars = jsonProperty (jsonProperty (runtimeManifest, "barMap"), "bars").getArray();
    const auto record = bars != nullptr && bar >= 0 && bar < bars->size() ? bars->getReference (bar) : juce::var {};
    const auto asset = jsonProperty (record, juce::String (state) + "_asset");
    const auto expected = resourceImage (juce::File (jsonProperty (asset, "file").toString()).getFileName()
                                         .replaceCharacters (" .", "__").retainCharacters ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789").toRawUTF8());
    const auto actualCrop = rendered.getClippedImage (bounds);
    const auto stats = expected.isValid() && actualCrop.isValid()
        ? diffImages (actualCrop, expected, { 0, 0, 56, 80 }) : DiffStats {};
    auto* item = new juce::DynamicObject();
    item->setProperty ("bar_id", bar + 1);
    item->setProperty ("resolved_state", state);
    item->setProperty ("resolved_asset_path", jsonProperty (asset, "file"));
    item->setProperty ("resolved_asset_sha256", jsonProperty (asset, "sha256"));
    juce::MemoryOutputStream encodedActual;
    juce::PNGImageFormat().writeImageToStream (actualCrop, encodedActual);
    item->setProperty ("actual_crop_fingerprint", byteFingerprint (encodedActual.getData(), encodedActual.getDataSize()));
    item->setProperty ("differing_pixel_count", stats.differing);
    item->setProperty ("mismatch_bounds", juce::Array<juce::var> { stats.mismatchBounds.getX(), stats.mismatchBounds.getY(), stats.mismatchBounds.getWidth(), stats.mismatchBounds.getHeight() });
    output.add (juce::var (item));
}

}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui; bool pass=true;
    pass &= check(resourceIs("static_faceplate_1024x683_png",1024,683),"v2-static-faceplate-native");
    pass &= check(resourceIs("knob_ring_60_png",48,48) && resourceIs("knob_pointer_60_png",48,48),"v2-knob-assets-native");
    pass &= check(resourceIs("bypass_off_png",80,31) && resourceIs("bypass_on_png",80,31),"v2-bypass-native");
    pass &= check(resourceIs("meter_led_strip_png",12,204),"v2-output-meter-led-native");
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
    pass &= check (resourceImage ("bar_11_normal_png").isValid()
                && resourceImage ("bar_27_normal_png").isValid()
                && resourceImage ("bar_43_normal_png").isValid()
                && resourceImage ("bar_59_normal_png").isValid(),
                   "v2-user-normal-bar-replacements-decode");
    const auto neutralBarBase = resourceImage ("bar_10_normal_png");
    pass &= check (usesNeutralBarBase (resourceImage ("bar_11_normal_png"), neutralBarBase)
                && usesNeutralBarBase (resourceImage ("bar_27_normal_png"), neutralBarBase)
                && usesNeutralBarBase (resourceImage ("bar_43_normal_png"), neutralBarBase)
                && usesNeutralBarBase (resourceImage ("bar_59_normal_png"), neutralBarBase),
                   "v2-four-normal-bars-use-neutral-base");

    ToyotomiHideyoshiAudioProcessor processor; processor.prepareToPlay(48000,512);
    // The meter tap is the final buffer emitted by processBlock. Verify L/R
    // transport independently before the UI Timer consumes either peak.
    juce::AudioBuffer<float> meterProbe (2, 32); meterProbe.clear();
    meterProbe.setSample (0, 0, 0.75f); meterProbe.setSample (1, 0, 0.25f);
    juce::MidiBuffer meterMidi; processor.processBlock (meterProbe, meterMidi);
    pass &= check (std::abs (processor.consumeOutputPeak (0) - 0.75f) < 0.0001f
                && std::abs (processor.consumeOutputPeak (1) - 0.25f) < 0.0001f,
                   "v2-output-meter-post-dsp-lr-peak-transport");
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
    const auto staticFaceplate = resourceImage ("static_faceplate_1024x683_png");
    const auto visualReference = resourceImage ("mastertimelinereference1024x683_png");
    const auto runtimeManifest = jsonResource ("runtimemanifest_json");
    juce::Array<juce::var> barPixelTrace;
    pass &= check (visualManifest.getDynamicObject() != nullptr && visualRegions.getArray() != nullptr
                && visualInteractive != nullptr && visualInteractive->size() == 39
                && visualReference.isValid() && visualReference.getWidth() == 1024 && visualReference.getHeight() == 683,
                   "v2-visual-acceptance-reference-and-manifest");
    bool faceplateClean = staticFaceplate.isValid();
    if (visualInteractive != nullptr)
        for (const auto& item : *visualInteractive)
        {
            const auto id = jsonProperty (item, "id").toString();
            if (id.startsWith ("tab_") || id.startsWith ("bar_") || id.startsWith ("preset_")
                || id.startsWith ("length_") || id == "bypass")
                faceplateClean = faceplateClean && fullyTransparent (staticFaceplate, jsonBounds (jsonProperty (item, "bounds")));
        }
    for (const auto& bounds : std::array<juce::Rectangle<int>, 10> {{
        GeneratedLayout::speedKnobBounds(), GeneratedLayout::pitchKnobBounds(), GeneratedLayout::depthKnobBounds(),
        GeneratedLayout::speedReadoutBounds(), GeneratedLayout::pitchReadoutBounds(), GeneratedLayout::depthReadoutBounds(),
        GeneratedLayout::outputLFaceplateMeterHoleBounds(), GeneratedLayout::outputRFaceplateMeterHoleBounds(),
        GeneratedLayout::outputLFaceplateHoleBounds(), GeneratedLayout::outputRFaceplateHoleBounds()
    }})
        faceplateClean = faceplateClean && fullyTransparent (staticFaceplate, bounds);
    faceplateClean = faceplateClean && hasNoDynamicGoldTrace (staticFaceplate, { 56, 450, 157, 120 });
    pass &= check (faceplateClean, "v2-static-background-clean-gate");
    pass &= check (opaqueRgbMatches (staticFaceplate, visualReference, { 933, 409, 4, 192 })
                && opaqueRgbMatches (staticFaceplate, visualReference, { 968, 409, 6, 192 }),
                   "v2-output-meter-old-hole-restored");
    pass &= check (v2 != nullptr && v2->validateInteractiveBounds(), "v2-visual-hit-bounds-match-manifest");
    auto& state=processor.getStateModel();
    // A fresh instance owns no selected timeline slot. Defaults remain valid
    // values, but no BAR/PRESET/LENGTH state image may be gold.
    state.selectTab (0); state.setBypass (false);
    const auto freshImage = render (*editor);
    const auto centerX2 = [] (juce::Rectangle<int> bounds) { return 2 * bounds.getX() + bounds.getWidth(); };
    const auto outputCentersAligned = [&]
    {
        const auto labelL = 2 * GeneratedLayout::outputLLabelCenterX();
        const auto labelR = 2 * GeneratedLayout::outputRLabelCenterX();
        return std::abs (labelL - centerX2 (GeneratedLayout::outputLBounds())) <= 2
            && std::abs (labelL - centerX2 (GeneratedLayout::outputLReadoutBounds())) <= 2
            && std::abs (labelR - centerX2 (GeneratedLayout::outputRBounds())) <= 2
            && std::abs (labelR - centerX2 (GeneratedLayout::outputRReadoutBounds())) <= 2;
    };
    pass &= check (freshImage.isValid() && outputCentersAligned(), "v2-output-final-composite-center-alignment");
    int freshGoldBars = 0, freshGoldPresets = 0, freshGoldLengths = 0;
    for (int index = 0; index < 16; ++index)
    {
        const auto bounds = juce::Rectangle<int> { std::array<int, 8> { 259, 317, 378, 437, 494, 553, 611, 670 }[(size_t) (index % 8)], index < 8 ? 137 : 221, 56, 80 };
        freshGoldBars += cropMatchesResource (freshImage, bounds,
            ("bar_" + juce::String (index + 1).paddedLeft ('0', 2) + "_selected_png").toRawUTF8()) ? 1 : 0;
    }
    const std::array<juce::Rectangle<int>, 10> freshPresetBounds {{{750,100,84,64},{836,100,84,64},{924,100,84,64},{750,166,84,64},{836,166,84,64},{924,166,84,64},{750,232,84,64},{836,232,84,64},{924,232,84,64},{750,296,84,64}}};
    const std::array<juce::Rectangle<int>, 5> freshLengthBounds {{{742,425,32,26},{773,425,32,26},{803,425,32,26},{834,425,32,26},{864,425,32,26}}};
    for (int index = 0; index < 10; ++index)
        freshGoldPresets += cropMatchesResource (freshImage, freshPresetBounds[(size_t) index],
            ("preset_" + juce::String (kGatePresetNames[(size_t) index]) + "_selected_png").toRawUTF8()) ? 1 : 0;
    for (int index = 0; index < 5; ++index)
        freshGoldLengths += cropMatchesResource (freshImage, freshLengthBounds[(size_t) index],
            ("length_" + juce::String (kGateLengthNames[(size_t) index]) + "_selected_png").toRawUTF8()) ? 1 : 0;
    pass &= check (state.getUiState().selectedBar == PluginStateModel::kNoSelectedBar && freshGoldBars == 0,
                   "fresh-default-no-selected-bar");
    pass &= check (freshGoldPresets == 0, "fresh-default-no-selected-preset");
    pass &= check (freshGoldLengths == 0, "fresh-default-no-selected-length");
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
        appendBarPixelTrace (barPixelTrace, runtimeManifest, defaultImage, index, bounds, index == 0 ? "selected" : "normal");
    }
    juce::File::getCurrentWorkingDirectory().getChildFile ("v2-bar-pixel-trace.json")
        .replaceWithText (juce::JSON::toString (juce::var (barPixelTrace), true));
    pass &= check(noPlayingRed(defaultImage), "v2-stop-red-cell-count-zero");
    pass &= check(cropMatchesResource(defaultImage,{251,74,105,27},"tab_1_16_selected_png")
               && cropMatchesResource(defaultImage,{360,74,105,27},"tab_17_32_normal_png"), "v2-tab-images-painted");
    // The master is an example-state illustration.  Its gold BACKSPIN is not
    // the default: default is OFF selected and BACKSPIN must be neutral.
    pass &= check (cropMatchesResource (defaultImage, { 750, 100, 84, 64 }, "preset_off_selected_png")
                && cropMatchesResource (defaultImage, { 924, 100, 84, 64 }, "preset_backspin_normal_png"),
                   "v2-default-off-selected-backspin-neutral");
    pass &= check (hasNoDynamicGoldTrace (defaultImage, { 56, 450, 157, 120 }),
                   "v2-xy-static-panel-owned-by-faceplate");

    const auto initialBar=state.getUiState().selectedBar; const auto initialSlot=state.getSlot(initialBar);
    const std::array<int, 8> cellX { 259, 317, 378, 437, 494, 553, 611, 670 };
    const std::array<const char*, 4> expectedUserNormalBars {
        "bar_11_normal_png", "bar_27_normal_png", "bar_43_normal_png", "bar_59_normal_png"
    };
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
        pass &= check (cropMatchesResource (image, { 378, 221, 56, 80 }, expectedUserNormalBars[(size_t) tab]),
                       "v2-user-normal-bar-replacement-render");
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

    // DYNAMIC_STATE_VISUAL_GATE: each check is made against the real V2
    // offscreen renderer and its single selected state PNG, never a cache or
    // an asset-sheet expectation.
    DynamicStateVisualReporter dynamicReport;
    const auto geometry = resolveGateGeometry (visualRegions, jsonProperty (visualManifest, "interactive"));
    pass &= check (geometry.resolved, "v2-dynamic-state-gate-geometry");
    auto barCase = [&] (const juce::String& name, int tab, int selectedBar, const juce::String& file)
    {
        state.selectTab (tab); state.selectBar (selectedBar);
        const auto image = render (*editor);
        dynamicReport.beginCase (name, "absolute BAR state resolution", gateStateSnapshot (tab, selectedBar, -1, (int) state.getSlot (selectedBar).preset, (int) state.getSlot (selectedBar).length, state.getUiState().bypass));
        dynamicReport.screenshot (image, file);
        verifyGateBarPage (image, tab, dynamicReport, selectedBar, "selected", selectedBar / 16 != tab);
        dynamicReport.endCase();
    };
    state.setBypass (false);
    barCase ("bar-page-1-selected-11", 0, 10, "dynamic-bar-page-1.png");
    barCase ("bar-page-2-selected-11-not-propagated", 1, 10, "dynamic-bar-page-2.png");
    barCase ("bar-page-2-selected-27", 1, 26, "dynamic-bar-page-2-selected-27.png");
    barCase ("bar-page-3-selected-35", 2, 34, "dynamic-bar-page-3.png");
    barCase ("bar-page-4-selected-49", 3, 48, "dynamic-bar-page-4.png");

    TestPlayHead dynamicPlayHead;
    processor.setPlayHead (&dynamicPlayHead);
    dynamicPlayHead.set (true, 1.25); // BAR 6
    processor.processBlock (audio, midi);
    state.selectTab (0); state.selectBar (10);
    auto separated = render (*editor);
    dynamicReport.beginCase ("bar-selected-11-playing-6", "selected GOLD and playing RED use separate absolute BARs", gateStateSnapshot (0, 10, 5, (int) state.getSlot (10).preset, (int) state.getSlot (10).length, state.getUiState().bypass));
    dynamicReport.screenshot (separated, "dynamic-bar-selected-11-playing-6.png");
    for (int cell = 0; cell < 16; ++cell)
    {
        const auto bar = cell;
        const auto suffix = bar == 10 ? "selected" : bar == 5 ? "playing" : "normal";
        const auto resource = "bar_" + juce::String (bar + 1).paddedLeft ('0', 2) + "_" + suffix + "_png";
        dynamicReport.checkValue ("separate-selected-playing-cell", gateCropMismatchPixels (separated, gateCellBounds (cell), resource) == 0, gateCropMismatchPixels (separated, gateCellBounds (cell), resource));
    }
    dynamicReport.endCase();
    dynamicPlayHead.set (true, 2.5); // BAR 11
    processor.processBlock (audio, midi);
    auto combined = render (*editor);
    dynamicReport.beginCase ("bar-selected-playing-11", "selected+playing resolves to one completed PNG", gateStateSnapshot (0, 10, 10, (int) state.getSlot (10).preset, (int) state.getSlot (10).length, state.getUiState().bypass));
    dynamicReport.screenshot (combined, "dynamic-bar-selected-playing-11.png");
    dynamicReport.checkValue ("selected-playing-single-completed-cell", gateCropMismatchPixels (combined, gateCellBounds (10), "bar_11_selected_playing_png") == 0, gateCropMismatchPixels (combined, gateCellBounds (10), "bar_11_selected_playing_png"));
    dynamicReport.endCase();
    dynamicPlayHead.set (false, 0.0); processor.processBlock (audio, midi); processor.setPlayHead (nullptr);

    state.selectTab (0); state.selectBar (0);
    for (int preset = 0; preset < 10; ++preset)
    {
        state.setSelectedPreset ((PluginStateModel::ScratchPreset) preset);
        const auto image = render (*editor);
        dynamicReport.beginCase ("preset-" + juce::String (kGatePresetNames[(size_t) preset]), "exactly one preset selected", gateStateSnapshot (0, 0, -1, preset, (int) state.getSlot (0).length, state.getUiState().bypass));
        if (preset == 0) dynamicReport.screenshot (image, "dynamic-preset-off.png");
        if (preset == 2) dynamicReport.screenshot (image, "dynamic-preset-backspin.png");
        if (preset == 3) dynamicReport.screenshot (image, "dynamic-preset-chirp.png");
        int selectedCount = 0;
        for (int index = 0; index < 10; ++index)
        {
            const auto resource = "preset_" + juce::String (kGatePresetNames[(size_t) index]) + (index == preset ? "_selected_png" : "_normal_png");
            const auto mismatch = gateCropMismatchPixels (image, geometry.presetBounds.getReference (index), resource);
            dynamicReport.checkValue ("preset-image-state", mismatch == 0, mismatch);
            selectedCount += gateCropMismatchPixels (image, geometry.presetBounds.getReference (index), "preset_" + juce::String (kGatePresetNames[(size_t) index]) + "_selected_png") == 0 ? 1 : 0;
        }
        dynamicReport.checkValue ("preset-selected-count", selectedCount == 1, selectedCount);
        dynamicReport.endCase();
    }
    for (int length = 0; length < 5; ++length)
    {
        state.setSelectedLength ((PluginStateModel::NoteLength) length);
        const auto image = render (*editor);
        dynamicReport.beginCase ("length-" + juce::String (kGateLengthNames[(size_t) length]), "exactly one length selected", gateStateSnapshot (0, 0, -1, (int) state.getSlot (0).preset, length, state.getUiState().bypass));
        if (length == 0) dynamicReport.screenshot (image, "dynamic-length-1-16.png");
        if (length == 2) dynamicReport.screenshot (image, "dynamic-length-1-4.png");
        if (length == 4) dynamicReport.screenshot (image, "dynamic-length-1bar.png");
        int selectedCount = 0;
        for (int index = 0; index < 5; ++index)
        {
            const auto resource = "length_" + juce::String (kGateLengthNames[(size_t) index]) + (index == length ? "_selected_png" : "_normal_png");
            juce::Array<juce::Rectangle<int>> laterBounds;
            for (int later = index + 1; later < 5; ++later)
                laterBounds.add (geometry.lengthBounds.getReference (later));
            const auto mismatch = gateVisibleCropMismatchPixels (image, geometry.lengthBounds.getReference (index), resource, laterBounds);
            dynamicReport.checkValue ("length-image-state", mismatch == 0, mismatch);
            selectedCount += gateVisibleCropMismatchPixels (image, geometry.lengthBounds.getReference (index), "length_" + juce::String (kGateLengthNames[(size_t) index]) + "_selected_png", laterBounds) == 0 ? 1 : 0;
        }
        dynamicReport.checkValue ("length-selected-count", selectedCount == 1, selectedCount);
        dynamicReport.endCase();
    }
    const auto fixedPreset = state.getSlot (0).preset;
    for (const auto bypass : { false, true })
    {
        state.setBypass (bypass);
        const auto image = render (*editor);
        dynamicReport.beginCase (bypass ? "bypass-on" : "bypass-off", "bypass is independent from preset", gateStateSnapshot (0, 0, -1, (int) fixedPreset, (int) state.getSlot (0).length, bypass));
        dynamicReport.screenshot (image, bypass ? "dynamic-bypass-on.png" : "dynamic-bypass-off.png");
        const auto mismatch = gateCropMismatchPixels (image, kGateBypassBounds, bypass ? "bypass_on_png" : "bypass_off_png");
        dynamicReport.checkValue ("bypass-single-image", mismatch == 0, mismatch);
        dynamicReport.check ("bypass-does-not-change-preset", state.getSlot (0).preset == fixedPreset);
        dynamicReport.endCase();
    }
    pass &= check (dynamicReport.finishReport(), "DYNAMIC_STATE_VISUAL_GATE");
    editor.reset(); processor.releaseResources(); return pass ? 0 : 1;
}
