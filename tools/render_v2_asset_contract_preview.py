#!/usr/bin/env python3
"""Render the image-only V2 default state directly from its runtime asset contract.

This does not create UI art or change supplied assets.  It is an acceptance
diagnostic: if the result contains BAR labels/cells but the VST does not, the
problem is in BinaryData/runtime loading rather than the supplied PNGs.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "Resources/ui-v2/runtime-1024"
OUT = ROOT / "reports/latest/ui-v2-asset-contract-default.png"

tabs = ((251, 74, 105, 27), (360, 74, 105, 27),
        (470, 74, 106, 27), (580, 74, 105, 27))
cell_x = (259, 317, 378, 437, 494, 553, 611, 670)
cell_y = (137, 221)
presets = ((750,100),(836,100),(924,100),(750,166),(836,166),
           (924,166),(750,232),(836,232),(924,232),(750,296))
preset_names = ("off", "forward_cut", "backspin", "chirp", "baby",
                "transform", "drag", "zigzag", "tape_brake", "custom")
length_x = (742, 773, 803, 834, 864)
length_names = ("1_16", "1_8", "1_4", "1_2", "1_bar")
tab_names = ("1_16", "17_32", "33_48", "49_64")


def paste(canvas: Image.Image, relative: str, xy: tuple[int, int]):
    image = Image.open(ASSETS / relative).convert("RGBA")
    canvas.alpha_composite(image, xy)


def main():
    # JUCE composites the supplied faceplate over an opaque editor.  Build the
    # diagnostic the same way so RGB bytes in transparent holes are never
    # mistaken for a second, stale UI layer.
    canvas = Image.new("RGBA", (1024, 683), (0, 0, 0, 255))
    canvas.alpha_composite(Image.open(ASSETS / "static/neutral_static_background_1024x683.png").convert("RGBA"))
    for index, bounds in enumerate(tabs):
        state = "selected" if index == 0 else "normal"
        paste(canvas, f"tabs/tab_{tab_names[index]}_{state}.png", bounds[:2])
    for index in range(16):
        bar = index + 1
        state = "selected" if bar == 1 else "normal"
        paste(canvas, f"bars/bar_{bar:02d}_{state}.png", (cell_x[index % 8], cell_y[index // 8]))
    for index, (x, y) in enumerate(presets):
        state = "selected" if index == 0 else "normal"
        paste(canvas, f"presets/preset_{preset_names[index]}_{state}.png", (x, y))
    for index, x in enumerate(length_x):
        state = "selected" if index == 0 else "normal"
        paste(canvas, f"length/length_{length_names[index]}_{state}.png", (x, 425))
    paste(canvas, "standalone/bypass_off.png", (931, 14))
    paste(canvas, "standalone/xy_neutral_base_288x256.png", (40, 429))
    paste(canvas, "xy-buttons/rec_normal.png", (27, 600))
    paste(canvas, "xy-buttons/clear_normal.png", (95, 600))
    paste(canvas, "xy-buttons/reset_view_normal.png", (159, 600))
    for x in (744, 793, 848):
        paste(canvas, "standalone/knob_ring_60.png", (x, 513))
        paste(canvas, "standalone/knob_pointer_60.png", (x, 513))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    canvas.convert("RGB").save(OUT)
    print(OUT)


if __name__ == "__main__":
    main()
