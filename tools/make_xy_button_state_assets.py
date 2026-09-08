"""Build deterministic native-size XY control state sprites from approved crops.

No canvas is translated: outer borders remain at their original pixels. Pressed
states move only the inner face one pixel down, so their edge geometry cannot
lean left or reveal a stale faceplate. REC idle desaturates the existing lamp;
REC active retains the approved red lamp.
"""
from pathlib import Path
from PIL import Image, ImageEnhance


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Resources" / "ui-v2" / "assets-1024" / "xy-buttons"
DEST = ROOT / "Resources" / "ui-v2" / "runtime-1024" / "xy-buttons" / "dynamic"


def pressed(source: Image.Image) -> Image.Image:
    """Keep the native outer border fixed; lower the interior face by one pixel."""
    result = source.copy().convert("RGBA")
    width, height = result.size
    # The border is deliberately excluded.  This preserves horizontal alignment.
    interior = source.convert("RGBA").crop((2, 4, width - 2, height - 3))
    result.paste(interior, (2, 5))
    return ImageEnhance.Brightness(result).enhance(0.92)


def rec_idle(source: Image.Image) -> Image.Image:
    """Turn only the existing lamp's red pixels into the inactive dark lamp."""
    result = source.convert("RGBA").copy()
    pixels = result.load()
    for y in range(8, min(23, result.height)):
        for x in range(8, min(22, result.width)):
            red, green, blue, alpha = pixels[x, y]
            if alpha and red > green * 1.35 and red > blue * 1.35:
                value = max(32, int((green + blue) / 2))
                pixels[x, y] = (value, value, value, alpha)
    return result


def save(image: Image.Image, name: str) -> None:
    image.save(DEST / name, "PNG", optimize=False)


def main() -> None:
    DEST.mkdir(parents=True, exist_ok=True)
    rec = Image.open(SOURCE / "rec-normal.png").convert("RGBA")
    clear = Image.open(SOURCE / "clear-normal.png").convert("RGBA")
    reset_view = Image.open(SOURCE / "reset-view-normal.png").convert("RGBA")

    idle = rec_idle(rec)
    save(idle, "xy_rec_normal.png")
    save(pressed(rec), "xy_rec_pressed.png")
    save(rec, "xy_rec_active.png")
    save(clear, "xy_clear_normal.png")
    save(pressed(clear), "xy_clear_pressed.png")
    save(clear, "xy_clear_active.png")
    save(reset_view, "xy_reset_view_normal.png")
    save(pressed(reset_view), "xy_reset_view_pressed.png")
    save(reset_view, "xy_reset_view_active.png")


if __name__ == "__main__":
    main()
