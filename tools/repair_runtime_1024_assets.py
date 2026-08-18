"""Rebuild only the six mismatched 1024 asset families from approved sources.

This is an offline asset-preparation step.  It performs no runtime scaling and
does not alter C++ or manifests.  Each output has its canonical native canvas
before it is ever used by JUCE.
"""
from __future__ import annotations

import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "ui" / "spec" / "runtime-1024-layout.json"
RUNTIME = ROOT / "Resources" / "ui-runtime-1024-20260818"
MASTER = ROOT / "Resources" / "ui-user-timeline-20260809" / "master-timeline-1280x853.png"
MASTER_ASSETS = ROOT / "Resources" / "ui-master-default-preview-20260808"
USER_ASSETS = ROOT / "Resources" / "ui-user-exact-preset-bypass-20260809"
REPORT = ROOT / "reports" / "latest" / "runtime-layout-audit-20260818"


def rgba_resize(source: Path | Image.Image, target: Path, size: tuple[int, int]):
    image = source.convert("RGBA") if isinstance(source, Image.Image) else Image.open(source).convert("RGBA")
    # This one-time source-to-native conversion uses the canonical output
    # canvas; JUCE will subsequently only draw these pixels 1:1.
    image.resize(size, Image.Resampling.LANCZOS).save(target)


def dimensions(components, name):
    item = components[name]
    return item["w"], item["h"]


def main():
    components = json.loads(CANONICAL.read_text(encoding="utf-8"))["components"]
    provenance = []
    def record(destination, source, size, crop=None):
        provenance.append({"destination": str(destination.relative_to(ROOT)).replace('\\\\', '/'),
                           "source": str(source.relative_to(ROOT)).replace('\\\\', '/') if isinstance(source, Path) else "master-timeline-1280x853 crop",
                           "crop": crop, "nativeSize": list(size)})

    # 1. All four state strips retain their approved 1280 source state art.
    tab_size = dimensions(components, "BarTabs")
    for source in sorted((MASTER_ASSETS / "tabs").glob("tab_strip_selected_*.png")):
        target = RUNTIME / "tabs" / source.name
        rgba_resize(source, target, tab_size)
        record(target, source, tab_size)

    # 2. All BAR cell shells are supplied high-resolution source states.
    cell_size = dimensions(components, "BarMap.Cell.01")
    for source in sorted((MASTER_ASSETS / "bar-cells").glob("*.png")):
        target = RUNTIME / "bar-cells" / source.name
        rgba_resize(source, target, cell_size)
        record(target, source, cell_size)

    # 3. XY is a canonical visual rectangle on the completed master.  Crop
    # the actual frame once, then create the final 232x200 native canvas.
    xy = components["XYPad"]
    master = Image.open(MASTER).convert("RGBA")
    xy_crop = master.crop((18, 520, 18 + 289, 520 + 249))
    xy_target = RUNTIME / "xy-buttons" / "xy_neutral_base_288x256.png"
    xy_size = dimensions(components, "XYPad")
    rgba_resize(xy_crop, xy_target, xy_size)
    record(xy_target, master, xy_size, [18, 520, 289, 249])

    # 4. RESET uses the approved high-resolution image; its canonical edge
    # conversion lands at 81x24, not the previous rounded 82x24.
    reset_source = MASTER_ASSETS / "xy-buttons" / "reset_normal_102x30.png"
    reset_target = RUNTIME / "xy-buttons" / reset_source.name
    reset_size = dimensions(components, "XYPad.Reset")
    rgba_resize(reset_source, reset_target, reset_size)
    record(reset_target, reset_source, reset_size)

    # 5. Each PRESET gets its own final height from canonical vertical edges.
    names = ["off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom"]
    component_names = ["OFF", "FORWARD_CUT", "BACKSPIN", "CHIRP", "BABY", "TRANSFORM", "DRAG", "ZIGZAG", "TAPE_BRAKE", "CUSTOM"]
    for name, component_name in zip(names, component_names):
        size = dimensions(components, f"Preset.{component_name}")
        for state in ("normal", "selected"):
            source = USER_ASSETS / f"preset_{name}_{state}_102x79.png"
            target = RUNTIME / "preset-bypass" / source.name
            rgba_resize(source, target, size)
            record(target, source, size)
        if name == "backspin":
            source = USER_ASSETS / "preset_backspin_normal_neutral_102x79.png"
            target = RUNTIME / "preset-bypass" / source.name
            rgba_resize(source, target, size)
            record(target, source, size)

    # 6. BYPASS uses its approved independent ON/OFF images at 80x31.
    bypass_size = dimensions(components, "Bypass")
    for name in ("bypass_off.png", "bypass_on.png"):
        source = USER_ASSETS / name
        target = RUNTIME / "preset-bypass" / name
        rgba_resize(source, target, bypass_size)
        record(target, source, bypass_size)

    REPORT.mkdir(parents=True, exist_ok=True)
    (REPORT / "runtime-1024-asset-repair-provenance.json").write_text(json.dumps(provenance, indent=2), encoding="utf-8")
    print(f"repaired={len(provenance)}")


if __name__ == "__main__":
    main()
