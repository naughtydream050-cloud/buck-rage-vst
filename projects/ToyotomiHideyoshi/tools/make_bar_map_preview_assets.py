"""Create BAR MAP approval assets from reference_ui.jpg only.

No generated imagery, fonts, or C++ UI changes are used.  Every output pixel is
either copied from the 1280x853 reference or deterministically composited from
reference pixels.
"""
from __future__ import annotations

import json
import statistics
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "Resources" / "reference_ui.jpg"
OUT = ROOT / "Resources" / "ui-bar-map-preview"
PRODUCTION = ROOT / "Resources" / "ui-bar-map"
CANVAS = (1280, 853)
CELL_W, CELL_H = 66, 96
CELL_X = (328, 398, 468, 538, 608, 678, 748, 818)
CELL_Y = (160, 260)
CELLS = [(i + 1, CELL_X[i % 8], CELL_Y[i // 8], CELL_W, CELL_H) for i in range(16)]


def rgba(image: Image.Image) -> Image.Image:
    return image.convert("RGBA")


def crop_cell(reference: Image.Image, number: int) -> Image.Image:
    _, x, y, w, h = CELLS[number - 1]
    return reference.crop((x, y, x + w, y + h))


def median_normal_background(reference: Image.Image) -> Image.Image:
    """Use the median of all normal-cell pixels: a deterministic texture-only base."""
    cells = [crop_cell(reference, n).convert("RGB") for n in (1, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13, 14, 15, 16)]
    result = Image.new("RGB", (CELL_W, CELL_H))
    out = result.load()
    inputs = [cell.load() for cell in cells]
    for y in range(CELL_H):
        for x in range(CELL_W):
            out[x, y] = tuple(int(statistics.median(p[x, y][channel] for p in inputs)) for channel in range(3))
    return rgba(result)


def texture_only_base(median: Image.Image) -> Image.Image:
    """Replace dynamic zones with a tiled strip of real, clean cell texture."""
    out = median.copy()
    src, dst = median.load(), out.load()
    for left, top, right, bottom in ((5, 8, 61, 31), (5, 33, 61, 61), (5, 62, 61, 92)):
        for y in range(top, bottom):
            for x in range(left, right):
                # The 60..66 band is an unlabelled, unframed interior strip.
                dst[x, y] = src[6 + ((x - left) % 54), 60 + ((y - top) % 7)]
    return out


def clear_dynamic_regions(state: Image.Image, normal_base: Image.Image) -> Image.Image:
    """Remove label, mini-motion, status, and dot while retaining source state borders."""
    out = state.copy()
    # All regions are inside the fixed cell frame; pixels come from normal-cell median texture.
    for box in ((5, 8, 61, 31), (5, 33, 61, 61), (5, 62, 61, 92)):
        out.paste(normal_base.crop(box), box)
    return out


def red_pixel(pixel: tuple[int, int, int, int]) -> bool:
    r, g, b, _ = pixel
    return r > 70 and r > g * 1.30 and r > b * 1.35


def selected_playing(selected: Image.Image, playing: Image.Image, normal_base: Image.Image) -> Image.Image:
    """Real playing cell as primary red state, with a real selected gold lower rim."""
    out = clear_dynamic_regions(playing, normal_base)
    sel, dst = clear_dynamic_regions(selected, normal_base).load(), out.load()
    for y in range(89, CELL_H):
        for x in range(2, CELL_W - 2):
            r, g, b, a = sel[x, y]
            if a and r > g * 1.05 and g > b * 1.12 and g > 45:
                dst[x, y] = sel[x, y]
    return out


def glyph_mask(source: Image.Image, box: tuple[int, int, int, int]) -> Image.Image:
    """Extract one reference-image glyph without adding a font or background pixels."""
    source = source.convert("RGB").crop(box)
    out = Image.new("RGBA", source.size, (0, 0, 0, 0))
    s, d = source.load(), out.load()
    for y in range(source.height):
        for x in range(source.width):
            r, g, b = s[x, y]
            # The label glyphs are the bright reference-image pixels. The low
            # threshold retains anti-aliasing while excluding the dark cell texture.
            if max(r, g, b) >= 55 and r >= g * 0.58 and g >= b * 0.50:
                d[x, y] = (r, g, b, 255)
    return out


def make_labels(reference: Image.Image) -> None:
    labels_dir = OUT / "bar-labels"
    labels_dir.mkdir(parents=True, exist_ok=True)
    # The reference uses three disconnected glyph groups for "BAR". Their
    # source positions are measured from BAR 1, then retained at their original
    # baseline in every generated label.
    prefix_source = crop_cell(reference, 1).crop((0, 7, CELL_W, 31))
    prefix_groups = ((18, 0, 24, 24), (25, 0, 32, 24), (33, 0, 39, 24))
    prefix = Image.new("RGBA", (CELL_W, 24), (0, 0, 0, 0))
    for left, top, right, bottom in prefix_groups:
        prefix.alpha_composite(glyph_mask(prefix_source, (left, top, right, bottom)), (left, top))

    # Digit sources are selected only from normal (ivory) BAR labels. BAR 6
    # and BAR 11 are red/gold state examples, so BAR 16 supplies the ivory 6.
    digit_sources = {"0": 10, "1": 1, "2": 2, "3": 3, "4": 4,
                     "5": 5, "6": 16, "7": 7, "8": 8, "9": 9}
    digit_boxes = {"0": (50, 0, 56, 24), "1": (45, 0, 48, 24),
                   "2": (47, 0, 52, 24), "3": (49, 0, 54, 24),
                   "4": (49, 0, 55, 24), "5": (49, 0, 54, 24),
                   "6": (51, 0, 56, 24), "7": (50, 0, 55, 24),
                   "8": (48, 0, 54, 24), "9": (45, 0, 51, 24)}
    glyphs: dict[str, Image.Image] = {}
    for digit, number in digit_sources.items():
        label = crop_cell(reference, number).crop((0, 7, CELL_W, 31))
        glyphs[digit] = glyph_mask(label, digit_boxes[digit])

    for number in range(1, 65):
        image = prefix.copy()
        text = str(number)
        # Every two-digit label uses the BAR 10 reference baseline. One-digit
        # labels stay optically centred using the reference's own digit pixel.
        x = 45 if len(text) == 2 else 45
        if len(text) == 1:
            x = 45 + (6 - glyphs[text].width) // 2
        for digit in text:
            image.alpha_composite(glyphs[digit], (x, 0))
            x += glyphs[digit].width + 2
        image.save(labels_dir / f"bar_label_{number:02d}.png")


def overlay_label(cell: Image.Image, label: Image.Image) -> None:
    cell.alpha_composite(label, (0, 7))


def source_motion(reference: Image.Image, state: str, local_number: int) -> Image.Image:
    if state == "selected":
        source = crop_cell(reference, 11)
    elif state in ("playing", "selected_and_playing"):
        source = crop_cell(reference, 6)
    else:
        # The reference happens to show BAR 6 playing and BAR 11 selected;
        # use adjacent normal cells when either slot is previewed as normal.
        normal_source = {6: 5, 11: 10}.get(local_number, local_number)
        source = crop_cell(reference, normal_source)
    return source.crop((4, 32, 62, 94))


def make_preview(reference: Image.Image, states: dict[int, str], base_number: int, filename: str) -> None:
    preview = reference.copy()
    for local, x, y, w, h in CELLS:
        state = states.get(local, "normal")
        cell = STATES[state].copy()
        cell.alpha_composite(source_motion(reference, state, local), (4, 32))
        absolute = base_number + local - 1
        overlay_label(cell, LABELS[absolute])
        preview.alpha_composite(cell, (x, y))
    preview.save(OUT / filename)
    # Pixel-exact guard: only the union of 16 BAR cells may differ from reference.
    diff = ImageChops.difference(preview.convert("RGB"), reference.convert("RGB"))
    outside = Image.composite(diff, Image.new("RGB", CANVAS), OUTSIDE_MASK)
    payload = outside.tobytes()
    changed_outside = sum(1 for i in range(0, len(payload), 3) if payload[i] or payload[i + 1] or payload[i + 2])
    max_delta = max(outside.getextrema()[channel][1] for channel in range(3))
    METRICS[filename] = {"outsideCellDifferencePixels": changed_outside, "outsideCellMaxRgbDelta": max_delta}


OUT.mkdir(parents=True, exist_ok=True)
ref = rgba(Image.open(REFERENCE))
if ref.size != CANVAS:
    raise RuntimeError(f"Reference size must be {CANVAS}, got {ref.size}")
base = texture_only_base(median_normal_background(ref))
inside_mask = Image.new("L", CANVAS)
draw_mask = ImageDraw.Draw(inside_mask)
for _, x, y, w, h in CELLS:
    draw_mask.rectangle((x, y, x + w - 1, y + h - 1), fill=255)
OUTSIDE_MASK = ImageChops.invert(inside_mask)
normal = clear_dynamic_regions(base, base)
selected = clear_dynamic_regions(crop_cell(ref, 11), base)
playing = clear_dynamic_regions(crop_cell(ref, 6), base)
STATES = {
    "normal": normal,
    "selected": selected,
    "playing": playing,
    "selected_and_playing": selected_playing(selected, playing, base),
}
for name, image in STATES.items():
    image.save(OUT / f"bar_cell_{name}.png")

make_labels(ref)
LABELS = {number: Image.open(OUT / "bar-labels" / f"bar_label_{number:02d}.png").convert("RGBA") for number in range(1, 65)}

# Production assets deliberately exclude labels, mini motion, and preview-only
# status content. They are kept separately from approval previews.
PRODUCTION.mkdir(parents=True, exist_ok=True)
for name, image in STATES.items():
    image.save(PRODUCTION / f"bar_cell_base_{name}.png")
production_labels = PRODUCTION / "bar-labels"
production_labels.mkdir(parents=True, exist_ok=True)
for number, image in LABELS.items():
    image.save(production_labels / f"bar_label_{number:02d}.png")
# Exact reference crop: red PLAYING lettering and its status dot, no font.
crop_cell(ref, 6).crop((5, 61, 61, 92)).save(PRODUCTION / "bar_playing_badge.png")
METRICS: dict[str, dict[str, int]] = {}
make_preview(ref, {11: "selected", 6: "playing"}, 1, "preview_01_selected_11_playing_06.png")
make_preview(ref, {6: "selected_and_playing"}, 1, "preview_02_selected_playing_06.png")
make_preview(ref, {5: "selected", 14: "playing"}, 17, "preview_03_tab_17_32_selected_21_playing_30.png")
make_preview(ref, {8: "selected"}, 33, "preview_04_tab_33_48_selected_40_playing_outside.png")
make_preview(ref, {16: "selected", 1: "playing"}, 49, "preview_05_tab_49_64_selected_64_playing_49.png")

manifest = {
    "reference": str(REFERENCE.relative_to(ROOT)).replace("\\", "/"),
    "canvas": list(CANVAS),
    "states": [f"bar_cell_{name}.png" for name in STATES],
    "labelDirectory": "Resources/ui-bar-map-preview/bar-labels",
    "cells": [
        {"cell": number, "x": x, "y": y, "width": w, "height": h, "clickBounds": [x, y, w, h]}
        for number, x, y, w, h in CELLS
    ],
    "previewOutsideCellDiff": METRICS,
}
(OUT / "bar-map-preview-manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print(json.dumps(manifest, indent=2))
