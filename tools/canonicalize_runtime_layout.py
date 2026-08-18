"""Canonicalize the audited logical layout without touching the runtime code.

The input is the code-derived actual 1280 audit.  The only visual correction
in this pass is the PRESET parent: the supplied completed UI keeps the CUSTOM
button at its present position, so its surrounding panel is extended to
contain it rather than moving CUSTOM or changing any button.
"""
from __future__ import annotations

import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
AUDIT = ROOT / "reports" / "latest" / "runtime-layout-audit-20260818"
ACTUAL = AUDIT / "runtime-1280-actual-layout.json"
CANONICAL_1280 = ROOT / "ui" / "spec" / "runtime-1280-canonical-layout.json"
CANONICAL_1024 = ROOT / "ui" / "spec" / "runtime-1024-layout.json"
VISUAL_1280 = ROOT / "Resources" / "ui-user-timeline-20260809" / "master-timeline-1280x853.png"
RUNTIME_ASSETS = ROOT / "Resources" / "ui-runtime-1024-20260818"

X_NUM, X_DEN = 1024, 1280
Y_NUM, Y_DEN = 683, 853


def edge(value: int, numerator: int, denominator: int) -> int:
    return round(value * numerator / denominator)


def scale_rectangle(item):
    if item["parent"] == "root":
        result = dict(item)
        result.update({"x": 0, "y": 0, "w": 1024, "h": 683, "visualBounds": [0, 0, 1024, 683],
                       "hitBounds": [0, 0, 1024, 683]})
        return result
    l, t = edge(item["x"], X_NUM, X_DEN), edge(item["y"], Y_NUM, Y_DEN)
    r = edge(item["x"] + item["w"], X_NUM, X_DEN)
    b = edge(item["y"] + item["h"], Y_NUM, Y_DEN)
    result = dict(item)
    result.update({"x": l, "y": t, "w": r-l, "h": b-t, "visualBounds": [l, t, r-l, b-t]})
    if item["hitBounds"] is not None:
        hx, hy, hw, hh = item["hitBounds"]
        hl, ht = edge(hx, X_NUM, X_DEN), edge(hy, Y_NUM, Y_DEN)
        hr, hb = edge(hx+hw, X_NUM, X_DEN), edge(hy+hh, Y_NUM, Y_DEN)
        result["hitBounds"] = [hl, ht, hr-hl, hb-ht]
    return result


def containment(components):
    errors = []
    for name, item in components.items():
        parent_name = item["parent"]
        if parent_name == "root" or parent_name not in components:
            continue
        p = components[parent_name]
        if not (p["x"] <= item["x"] and p["y"] <= item["y"]
                and item["x"] + item["w"] <= p["x"] + p["w"]
                and item["y"] + item["h"] <= p["y"] + p["h"]):
            errors.append({"component": name, "parent": parent_name,
                           "bounds": item["visualBounds"], "parentBounds": p["visualBounds"]})
    return errors


def draw_overlay(source, components, output, detailed=False):
    image = Image.open(source).convert("RGBA")
    d = ImageDraw.Draw(image, "RGBA")
    colors = {"Header": (100, 150, 255, 255), "BarTabs": (245, 200, 55, 255),
              "BarMap": (255, 75, 75, 255), "PresetPanel": (75, 225, 255, 255),
              "XYPad": (100, 250, 155, 255), "QuotePanel": (255, 180, 65, 255),
              "CountParameters": (255, 100, 220, 255), "Output": (175, 130, 255, 255),
              "Footer": (100, 150, 255, 255)}
    for name, item in components.items():
        if item["parent"] not in ("content", "BarTabs", "PresetPanel", "XYPad", "CountParameters"):
            continue
        if not detailed and (".Cell." in name or ".Label" in name or ".Preview" in name):
            continue
        color = colors.get(item["parent"], colors.get(name, (255, 255, 255, 190)))
        x, y, w, h = item["x"], item["y"], item["w"], item["h"]
        d.rectangle((x, y, x+w-1, y+h-1), outline=color, width=1 if detailed else 2)
        if item["role"] == "visual+hit":
            d.rectangle((x+2, y+2, x+w-3, y+h-3), outline=(color[0], color[1], color[2], 100), width=1)
    image.save(output)


def image_size(relative):
    file = RUNTIME_ASSETS / relative
    return Image.open(file).size if file.exists() else None


def asset_checks(native):
    checks = []
    def check(asset, expected, component):
        actual = image_size(asset)
        checks.append({"asset": str(asset).replace('\\\\', '/'), "component": component,
                       "expectedNativeBounds": expected, "actualImageSize": list(actual) if actual else None,
                       "match": actual == tuple(expected)})
    check(Path("background/master_default_no_count_grid_title_1024x683.png"), [1024, 683], "Editor")
    check(Path("quote/quote_panel_user_409x288.png"), [native["QuotePanel"]["w"], native["QuotePanel"]["h"]], "QuotePanel")
    for file in sorted((RUNTIME_ASSETS / "length").glob("*.png")):
        check(file.relative_to(RUNTIME_ASSETS), [native["Length.1_16"]["w"], native["Length.1_16"]["h"]], "Length.*")
    check(Path("knobs/knob_ring_48.png"), [native["Knob.Speed"]["w"], native["Knob.Speed"]["h"]], "Knob.*")
    check(Path("knobs/knob_pointer_48.png"), [native["Knob.Speed"]["w"], native["Knob.Speed"]["h"]], "Knob.*")
    check(Path("tabs/tab_strip_selected_1_16.png"), [native["BarTabs"]["w"], native["BarTabs"]["h"]], "BarTabs")
    check(Path("bar-cells/bar_cell_shell_normal_clean_72x94.png"), [native["BarMap.Cell.01"]["w"], native["BarMap.Cell.01"]["h"]], "BarMap.Cell.*")
    check(Path("xy-buttons/xy_neutral_base_288x256.png"), [native["XYPad"]["w"], native["XYPad"]["h"]], "XYPad")
    check(Path("xy-buttons/clear_normal_73x30.png"), [native["XYPad.Clear"]["w"], native["XYPad.Clear"]["h"]], "XYPad.Clear")
    check(Path("xy-buttons/reset_normal_102x30.png"), [native["XYPad.Reset"]["w"], native["XYPad.Reset"]["h"]], "XYPad.Reset")
    check(Path("output/output_neutral_base_140x343.png"), [native["Output"]["w"], native["Output"]["h"]], "Output")
    check(Path("preset-bypass/preset_off_normal_102x79.png"), [native["Preset.OFF"]["w"], native["Preset.OFF"]["h"]], "Preset.*")
    check(Path("preset-bypass/bypass_off.png"), [native["Bypass"]["w"], native["Bypass"]["h"]], "Bypass")
    return checks


def main():
    actual = json.loads(ACTUAL.read_text(encoding="utf-8"))["components"]
    canonical = {name: dict(value) for name, value in actual.items()}
    # Visual correction from the completed UI: preserve all ten button bounds;
    # restore the complete surrounding panel (left outer frame at x=932) so
    # CUSTOM remains where authored and is fully contained.
    canonical["PresetPanel"] = dict(canonical["PresetPanel"])
    canonical["PresetPanel"].update({"x": 932, "y": 87, "w": 343, "h": 368,
                                      "visualBounds": [932, 87, 343, 368]})

    assert not containment(canonical), containment(canonical)
    canonical_document = {
        "schemaVersion": 1,
        "kind": "canonical-1280-layout",
        "canvas": {"width": 1280, "height": 853},
        "visualTruth": str(VISUAL_1280.relative_to(ROOT)).replace('\\\\', '/'),
        "corrections": [{"component": "PresetPanel", "actual": [942, 87, 333, 350],
                         "canonical": [932, 87, 343, 368],
                         "reason": "Keep CUSTOM [949,376,102,79] unchanged and fully contained."}],
        "components": canonical
    }
    CANONICAL_1280.write_text(json.dumps(canonical_document, indent=2), encoding="utf-8")
    AUDIT.mkdir(parents=True, exist_ok=True)
    (AUDIT / "runtime-1280-canonical-layout.json").write_text(json.dumps(canonical_document, indent=2), encoding="utf-8")
    draw_overlay(VISUAL_1280, canonical, AUDIT / "runtime-1280-canonical-layout-overlay.png")
    draw_overlay(VISUAL_1280, canonical, AUDIT / "runtime-1280-canonical-layout-overlay-all.png", detailed=True)

    native = {name: scale_rectangle(value) for name, value in canonical.items()}
    containment_errors = containment(native)
    outside = [{"component": name, "bounds": item["visualBounds"]} for name, item in native.items()
               if item["x"] < 0 or item["y"] < 0 or item["x"]+item["w"] > 1024 or item["y"]+item["h"] > 683]
    runtime_document = {
        "schemaVersion": 1,
        "kind": "canonical-1024-native-layout-draft",
        "canvas": {"width": 1024, "height": 683, "resizable": False},
        "conversion": {"x": "round(edge * 1024 / 1280)", "y": "round(edge * 683 / 853)",
                       "runtimeScaling": "none; this file is layout-only until runtime migration is approved"},
        "components": native,
        "validation": {"integerBounds": True, "parentContainment": not containment_errors,
                       "canvasContainment": not outside, "parentContainmentFailures": containment_errors,
                       "canvasFailures": outside}
    }
    CANONICAL_1024.write_text(json.dumps(runtime_document, indent=2), encoding="utf-8")
    (AUDIT / "runtime-1024-canonical-layout.json").write_text(json.dumps(runtime_document, indent=2), encoding="utf-8")
    reference_1024 = Image.open(VISUAL_1280).convert("RGBA").resize((1024, 683), Image.Resampling.LANCZOS)
    reference_file = AUDIT / "_canonical-reference-1024.png"
    reference_1024.save(reference_file)
    draw_overlay(reference_file, native, AUDIT / "runtime-1024-canonical-layout-overlay.png")
    draw_overlay(reference_file, native, AUDIT / "runtime-1024-canonical-layout-overlay-all.png", detailed=True)
    reference_file.unlink()

    checks = asset_checks(native)
    (AUDIT / "runtime-1024-asset-mismatches.json").write_text(json.dumps({
        "checked": checks, "mismatches": [entry for entry in checks if not entry["match"]]
    }, indent=2), encoding="utf-8")
    print("canonical1280=PASS")
    print(f"canonical1024=PASS containment={not containment_errors} canvas={not outside}")
    print(f"asset_mismatches={sum(not item['match'] for item in checks)}")


if __name__ == "__main__":
    main()
