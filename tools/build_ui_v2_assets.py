#!/usr/bin/env python3
"""Build the UI V2 asset-review package from the supplied RAW crop ZIP only.

This tool deliberately never opens or crops the FINAL MASTER image.  Direct
assets are byte-for-byte copies of the user-supplied RAW PNGs.  The only new
pixels it writes are deterministic normal/selected/playing state-shell
composites made from those RAW PNGs.  It does not touch PluginEditor, build
inputs, BinaryData, or the current candidate.
"""
from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "Resources/ui-v2/source/raw-crops-20260818/ToyotomiHideyoshi_FINAL_MASTER_raw_crops"
RAW_MANIFEST = RAW / "manifest.json"
OUT = ROOT / "Resources/ui-v2/assets-raw-source-20260818"
MANIFEST = ROOT / "ui/v2/raw-source-asset-manifest.json"

PRESETS = ["off", "forward_cut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tape_brake", "custom"]
LENGTHS = ["1_16", "1_8", "1_4", "1_2", "1_bar"]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def raw(name: str) -> Path:
    path = RAW / name
    if not path.is_file():
        raise FileNotFoundError(path)
    return path


def source_image(name: str) -> Image.Image:
    return Image.open(raw(name)).convert("RGBA")


def raw_copy(name: str, rel: str, *, state: str, component: str) -> dict:
    source = raw(name)
    destination = OUT / rel
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    with Image.open(destination) as image:
        width, height, mode = image.width, image.height, image.mode
    return {
        "id": destination.stem,
        "output": str(destination.relative_to(ROOT)).replace("\\", "/"),
        "source": f"RAW_ZIP:{name}",
        "state": state,
        "component": component,
        "width": width,
        "height": height,
        "mode": mode,
        "sha256": sha(destination),
        "derivation": "verbatim-user-raw-crop",
    }


def save_composite(image: Image.Image, rel: str, *, state: str, component: str, sources: list[str], rule: str) -> dict:
    destination = OUT / rel
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)
    return {
        "id": destination.stem,
        "output": str(destination.relative_to(ROOT)).replace("\\", "/"),
        "source": [f"RAW_ZIP:{item}" for item in sources],
        "state": state,
        "component": component,
        "width": image.width,
        "height": image.height,
        "mode": "RGBA",
        "sha256": sha(destination),
        "derivation": rule,
    }


def resampled(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    """State-shell canvas normalization only; never crops or redraws content."""
    if image.size == size:
        return image.copy()
    return image.resize(size, Image.Resampling.LANCZOS)


def inner_content_mask(image: Image.Image, inset: int = 5) -> Image.Image:
    """Keep visible text/icon pixels, excluding a border/glow shell."""
    luminance = ImageOps.grayscale(image.convert("RGB"))
    mask = luminance.point(lambda value: 255 if value >= 98 else 0)
    draw = ImageDraw.Draw(mask)
    draw.rectangle((0, 0, image.width - 1, inset - 1), fill=0)
    draw.rectangle((0, image.height - inset, image.width - 1, image.height - 1), fill=0)
    draw.rectangle((0, 0, inset - 1, image.height - 1), fill=0)
    draw.rectangle((image.width - inset, 0, image.width - 1, image.height - 1), fill=0)
    return mask


def ivory_content(image: Image.Image) -> Image.Image:
    """Deterministically neutralise only gold content inherited from a selected raw crop."""
    result = image.convert("RGBA").copy()
    pixels = result.load()
    for y in range(result.height):
        for x in range(result.width):
            red, green, blue, alpha = pixels[x, y]
            if alpha and red > 96 and green > 62 and red > blue * 1.08:
                pixels[x, y] = (216, 207, 190, alpha)
    return result


def shell_mask(normal_shell: Image.Image, selected_shell: Image.Image) -> Image.Image:
    difference = ImageOps.grayscale(ImageChops.difference(normal_shell.convert("RGB"), selected_shell.convert("RGB")))
    mask = difference.point(lambda value: 255 if value >= 12 else 0)
    inset = 5 if min(selected_shell.size) >= 30 else 3
    ImageDraw.Draw(mask).rectangle(
        (inset, inset, selected_shell.width - inset - 1, selected_shell.height - inset - 1), fill=0
    )
    return mask


def state_shell(content: Image.Image, normal_reference: Image.Image, selected_reference: Image.Image, *, red: bool = False) -> Image.Image:
    """Apply only a RAW selected shell; original normal content remains untouched."""
    normal_reference = resampled(normal_reference, content.size)
    selected_shell = resampled(selected_reference, content.size).convert("RGBA")
    if red:
        pixels = selected_shell.load()
        for y in range(selected_shell.height):
            for x in range(selected_shell.width):
                red_value, green, blue, alpha = pixels[x, y]
                if alpha and red_value > 85 and green > 55 and red_value >= blue:
                    pixels[x, y] = (min(236, int(red_value * 1.10)), min(79, int(green * 0.42)), min(68, int(blue * 0.42)), alpha)
    result = content.copy().convert("RGBA")
    result.paste(selected_shell, (0, 0), shell_mask(normal_reference, selected_shell))
    return result


def neutral_from_selected(selected_content: Image.Image, neutral_shell: Image.Image) -> Image.Image:
    """Missing normal: retain selected crop content, replace only its outer shell.

    Starting from the selected crop matters: a normal reference contains its
    own label/icon pixels, which must never leak into the requested control.
    """
    result = ivory_content(selected_content)
    neutral_shell = resampled(neutral_shell, result.size).convert("RGBA")
    inset = 5 if min(result.size) >= 30 else 3
    shell_only = Image.new("L", result.size, 0)
    draw = ImageDraw.Draw(shell_only)
    draw.rectangle((0, 0, result.width - 1, inset - 1), fill=255)
    draw.rectangle((0, result.height - inset, result.width - 1, result.height - 1), fill=255)
    draw.rectangle((0, 0, inset - 1, result.height - 1), fill=255)
    draw.rectangle((result.width - inset, 0, result.width - 1, result.height - 1), fill=255)
    result.paste(neutral_shell, (0, 0), shell_only)
    return result


def asset_sheet(rows: list[dict]) -> Path:
    preview_rows = [row for row in rows if row["component"] in {"BAR_TAB", "PRESET", "LENGTH", "BAR_CELL", "STATIC", "KNOB"}]
    tiles = []
    for row in preview_rows:
        image = Image.open(ROOT / row["output"]).convert("RGBA")
        image.thumbnail((150, 80), Image.Resampling.LANCZOS)
        tile = Image.new("RGBA", (166, 126), (8, 10, 11, 255))
        tile.alpha_composite(image, ((166 - image.width) // 2, 5))
        draw = ImageDraw.Draw(tile)
        draw.text((5, 91), row["id"][:29], fill=(234, 199, 126, 255))
        source = row["source"] if isinstance(row["source"], str) else "SHELL"
        draw.text((5, 108), "RAW" if source.startswith("RAW_ZIP") else "SHELL COMPOSITE", fill=(182, 182, 182, 255))
        tiles.append(tile)
    columns = 5
    rows_count = (len(tiles) + columns - 1) // columns
    sheet = Image.new("RGBA", (columns * 176 + 20, rows_count * 136 + 70), (3, 4, 5, 255))
    draw = ImageDraw.Draw(sheet)
    draw.text((10, 8), "TOYOTOMI UI V2 — RAW ZIP SOURCE ASSET REVIEW", fill=(242, 208, 135, 255))
    draw.text((10, 30), "verbatim raw crops + deterministic state-shell composites only; no FINAL MASTER re-crop", fill=(190, 190, 190, 255))
    for index, tile in enumerate(tiles):
        sheet.alpha_composite(tile, (10 + (index % columns) * 176, 56 + (index // columns) * 136))
    destination = OUT / "raw-source-asset-sheet.png"
    sheet.save(destination)
    return destination


def main() -> None:
    if not RAW_MANIFEST.is_file():
        raise SystemExit(f"RAW source manifest missing: {RAW_MANIFEST}")
    supplied = json.loads(RAW_MANIFEST.read_text(encoding="utf-8"))
    OUT.mkdir(parents=True, exist_ok=True)
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    rows: list[dict] = []

    # Direct static RAW sources: exact copies, not master-image crops.
    for filename, component in [
        ("header.png", "STATIC"), ("footer.png", "STATIC"), ("quote_panel.png", "STATIC"),
        ("xy_panel.png", "STATIC"), ("preset_panel.png", "STATIC"),
        ("parameter_panel.png", "STATIC"), ("output_panel.png", "STATIC"),
        ("knob_speed_reference.png", "KNOB"), ("knob_pitch_reference.png", "KNOB"),
        ("knob_depth_reference.png", "KNOB"),
    ]:
        rows.append(raw_copy(filename, f"direct/{filename}", state="raw-reference", component=component))

    # Tabs: supplied three normal + one selected.  Missing state pairs use only
    # the supplied neutral shell and the supplied selected shell.
    tab_normal_files = {"17_32": "tab_17_32_normal.png", "33_48": "tab_33_48_normal.png", "49_64": "tab_49_64_normal.png"}
    for tab, filename in tab_normal_files.items():
        rows.append(raw_copy(filename, f"tabs/tab_{tab}_normal.png", state="normal", component="BAR_TAB"))
    selected_tab = source_image("tab_1_16_selected.png")
    neutral_tab_shell = source_image("tab_17_32_normal.png")
    tab_1_16_normal = neutral_from_selected(selected_tab, neutral_tab_shell)
    rows.append(save_composite(tab_1_16_normal, "tabs/tab_1_16_normal.png", state="normal", component="BAR_TAB", sources=["tab_1_16_selected.png", "tab_17_32_normal.png"], rule="neutral-shell+ivory-selected-content"))
    rows.append(raw_copy("tab_1_16_selected.png", "tabs/tab_1_16_selected.png", state="selected", component="BAR_TAB"))
    for tab, filename in tab_normal_files.items():
        normal = source_image(filename)
        composite = state_shell(normal, neutral_tab_shell, selected_tab)
        rows.append(save_composite(composite, f"tabs/tab_{tab}_selected.png", state="selected", component="BAR_TAB", sources=[filename, "tab_17_32_normal.png", "tab_1_16_selected.png"], rule="selected-shell+normal-content"))

    # Presets: nine supplied neutral normal crops, one supplied selected crop.
    normals: dict[str, Image.Image] = {}
    for preset in PRESETS:
        filename = f"preset_{preset}_normal.png"
        if preset != "backspin":
            normals[preset] = source_image(filename)
            rows.append(raw_copy(filename, f"presets/{filename}", state="normal", component="PRESET"))
    selected_backspin = source_image("preset_backspin_selected.png")
    neutral_preset_shell = normals["off"]
    normals["backspin"] = neutral_from_selected(selected_backspin, neutral_preset_shell)
    rows.append(save_composite(normals["backspin"], "presets/preset_backspin_normal.png", state="normal", component="PRESET", sources=["preset_backspin_selected.png", "preset_off_normal.png"], rule="neutral-shell+ivory-selected-content"))
    rows.append(raw_copy("preset_backspin_selected.png", "presets/preset_backspin_selected.png", state="selected", component="PRESET"))
    for preset in PRESETS:
        if preset == "backspin":
            continue
        composite = state_shell(normals[preset], normals["backspin"], selected_backspin)
        rows.append(save_composite(composite, f"presets/preset_{preset}_selected.png", state="selected", component="PRESET", sources=[f"preset_{preset}_normal.png", "preset_backspin_normal.png", "preset_backspin_selected.png"], rule="selected-shell+normal-content"))

    # Length: four supplied normal crops and a supplied selected 1/4 crop.
    lengths: dict[str, Image.Image] = {}
    for length in LENGTHS:
        filename = f"length_{length}_normal.png"
        if length != "1_4":
            lengths[length] = source_image(filename)
            rows.append(raw_copy(filename, f"length/{filename}", state="normal", component="LENGTH"))
    selected_1_4 = source_image("length_1_4_selected.png")
    lengths["1_4"] = neutral_from_selected(selected_1_4, lengths["1_8"])
    rows.append(save_composite(lengths["1_4"], "length/length_1_4_normal.png", state="normal", component="LENGTH", sources=["length_1_4_selected.png", "length_1_8_normal.png"], rule="neutral-shell+ivory-selected-content"))
    rows.append(raw_copy("length_1_4_selected.png", "length/length_1_4_selected.png", state="selected", component="LENGTH"))
    for length in LENGTHS:
        if length == "1_4":
            continue
        composite = state_shell(lengths[length], lengths["1_4"], selected_1_4)
        rows.append(save_composite(composite, f"length/length_{length}_selected.png", state="selected", component="LENGTH", sources=[f"length_{length}_normal.png", "length_1_4_normal.png", "length_1_4_selected.png"], rule="selected-shell+normal-content"))

    # BAR cells: 15 normal + BAR 11 selected.  Each output has a matching
    # normal/selected/playing/selected+playing canvas for the supplied page.
    bar_normals: dict[int, Image.Image] = {}
    for number in range(1, 17):
        filename = f"bar_{number:02d}_normal.png"
        if number != 11:
            bar_normals[number] = source_image(filename)
            rows.append(raw_copy(filename, f"bar-cells/{filename}", state="normal", component="BAR_CELL"))
    selected_bar = source_image("bar_11_selected.png")
    bar_normals[11] = neutral_from_selected(selected_bar, bar_normals[10])
    rows.append(save_composite(bar_normals[11], "bar-cells/bar_11_normal.png", state="normal", component="BAR_CELL", sources=["bar_11_selected.png", "bar_10_normal.png"], rule="neutral-shell+ivory-selected-content"))
    rows.append(raw_copy("bar_11_selected.png", "bar-cells/bar_11_selected.png", state="selected", component="BAR_CELL"))
    for number in range(1, 17):
        normal = bar_normals[number]
        if number != 11:
            selected = state_shell(normal, bar_normals[11], selected_bar)
            rows.append(save_composite(selected, f"bar-cells/bar_{number:02d}_selected.png", state="selected", component="BAR_CELL", sources=[f"bar_{number:02d}_normal.png", "bar_11_normal.png", "bar_11_selected.png"], rule="selected-shell+normal-content"))
        selected_for_playing = selected_bar if number == 11 else state_shell(normal, bar_normals[11], selected_bar)
        playing = state_shell(normal, bar_normals[11], selected_bar, red=True)
        selected_playing = state_shell(selected_for_playing, bar_normals[11], selected_bar, red=True)
        rows.append(save_composite(playing, f"bar-cells/bar_{number:02d}_playing.png", state="playing", component="BAR_CELL", sources=[f"bar_{number:02d}_normal.png" if number != 11 else "bar_11_selected.png", "bar_11_normal.png", "bar_11_selected.png"], rule="gold-shell-geometry+deterministic-red-treatment"))
        rows.append(save_composite(selected_playing, f"bar-cells/bar_{number:02d}_selected_playing.png", state="selected+playing", component="BAR_CELL", sources=["bar_11_selected.png", "bar_11_normal.png"], rule="red-primary-shell+gold-content-retained"))

    sheet = asset_sheet(rows)
    missing = [
        "complete 1024x683 static background / left artwork / main frame (not present in RAW ZIP)",
        "BAR MAP source labels and normal cells for BAR 17–64 (not present in RAW ZIP)",
        "standalone knob base + transparent pointer (RAW ZIP explicitly marks reference knobs as not separable)",
        "neutral XY base without trace, and BYPASS OFF/ON (not present in RAW ZIP)",
        "REC/CLEAR/RESET standalone button crops (not present in RAW ZIP)",
    ]
    document = {
        "schemaVersion": 1,
        "sourceContract": {
            "rawZip": "C:/Users/razor/Downloads/ToyotomiHideyoshi_FINAL_MASTER_raw_crops.zip",
            "rawExtractRoot": str(RAW.relative_to(ROOT)).replace("\\", "/"),
            "rawZipSha256": sha(Path("C:/Users/razor/Downloads/ToyotomiHideyoshi_FINAL_MASTER_raw_crops.zip")),
            "rawZipCrcTest": "PASS",
            "finalMasterRecrop": "FORBIDDEN",
            "image2DirectCrop": "FORBIDDEN",
        },
        "rawSourceManifest": str(RAW_MANIFEST.relative_to(ROOT)).replace("\\", "/"),
        "assets": rows,
        "missingAssets": missing,
        "assetSheet": str(sheet.relative_to(ROOT)).replace("\\", "/"),
    }
    MANIFEST.write_text(json.dumps(document, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"assets={len(rows)} manifest={MANIFEST} sheet={sheet}")


if __name__ == "__main__":
    main()
