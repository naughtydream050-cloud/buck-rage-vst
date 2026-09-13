"""Deterministic repair of the two approved state-image regressions.

Only state PNGs are changed: BAR playing-dot pixels and the Host Sync lamp.
No layout, faceplate, DSP or state data is read or written here.
"""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
BARS = ROOT / "Resources/ui-v2/runtime-1024/bars"
LAMP = ROOT / "Resources/ui-v2/runtime-1024/native-display/toy_display_host_sync_lamp_on.png"


def red(pixel):
    r, g, b, _ = pixel
    return r > 70 and r > g * 1.35 and r > b * 1.35


def repair_bar(bar, state, reference):
    target_path = BARS / f"bar_{bar:02d}_{state}.png"
    source_path = BARS / f"bar_{bar:02d}_{reference}.png"
    target = Image.open(target_path).convert("RGBA")
    source = Image.open(source_path).convert("RGBA")
    changed = 0
    # The dot is identified by its actual red pixels inside the bottom
    # interior.  The red perimeter is outside this region and remains intact.
    for y in range(60, 76):
        for x in range(18, 38):
            if red(target.getpixel((x, y))):
                target.putpixel((x, y), source.getpixel((x, y)))
                changed += 1
    target.save(target_path)


def repair_lamp():
    # Exact legacy active state (0xffb83229) rasterised once as a state PNG.
    # Runtime never draws a generic circle; it selects this approved state asset.
    scale = 8
    hi = Image.new("RGBA", (11 * scale, 12 * scale), (0, 0, 0, 0))
    ImageDraw.Draw(hi).ellipse((0, 0, 11 * scale - 1, 12 * scale - 1), fill=(184, 50, 41, 255))
    hi.resize((11, 12), Image.Resampling.LANCZOS).save(LAMP)


def main():
    for bar in range(1, 65):
        repair_bar(bar, "playing", "normal")
        # The selected source carries its own old red centre pixel; use the
        # normal state's neutral dot while retaining every surrounding
        # selected-playing pixel.
        repair_bar(bar, "selected_playing", "normal")
    repair_lamp()


if __name__ == "__main__":
    main()
