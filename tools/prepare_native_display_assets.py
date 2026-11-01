"""Build only source-derived dynamic display sprites for Toyotomi's 1024 UI.

No fonts, vector shapes, AI generation, or coordinate changes are involved.
The script uses the approved master reference and emits uniquely-prefixed
BinaryData filenames.
"""
from hashlib import sha256
from pathlib import Path
import json
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "Resources/ui-v2/reference/master-timeline-reference-1024x683.png"
OUT = ROOT / "Resources/ui-v2/runtime-1024/native-display"
REPORT = ROOT / "reports/latest/native-display-assets.json"


def crop(image, bounds):
    x, y, w, h = bounds
    return image.crop((x, y, x + w, y + h))


def foreground_sprite(image, bounds, threshold=70):
    """Preserve source pixels only where their luminance proves glyph ink."""
    source = crop(image, bounds).convert("RGBA")
    pixels = source.load()
    for y in range(source.height):
        for x in range(source.width):
            r, g, b, _ = pixels[x, y]
            pixels[x, y] = (r, g, b, 255 if max(r, g, b) >= threshold else 0)
    bbox = source.getbbox()
    if bbox is None:
        raise RuntimeError(f"no native glyph pixels in {bounds}")
    return source.crop(bbox)


def neutral_backing(image, bounds):
    """Return a text-free substrate made only from local dark source pixels."""
    source = crop(image, bounds).convert("RGBA")
    pixels = source.load()
    original = source.copy().load()
    for y in range(source.height):
        for x in range(source.width):
            # The entire rectangle is dynamic text space.  Repeating only
            # dark pixels sampled from its own 1px perimeter guarantees that
            # no dim antialiased source glyph survives while retaining the
            # reference's native substrate/noise rather than a JUCE fillRect.
            perimeter = []
            for nx, ny in ((x, 0), (x, source.height - 1), (0, y), (source.width - 1, y)):
                candidate = original[nx, ny]
                if max(candidate[0], candidate[1], candidate[2]) < 90:
                    perimeter.append(candidate)
            if not perimeter:
                perimeter = [(18, 20, 19, 255)]
            pixels[x, y] = perimeter[(x * 7 + y * 11) % len(perimeter)]
    return source


def write(image, filename, records, source):
    path = OUT / filename
    image.save(path)
    records.append({"file": str(path.relative_to(ROOT)).replace("\\", "/"), "source": source,
                    "size": [image.width, image.height], "sha256": sha256(path.read_bytes()).hexdigest()})


def main():
    reference = Image.open(REFERENCE).convert("RGBA")
    OUT.mkdir(parents=True, exist_ok=True)
    records = []

    # Backings use precisely the existing GeneratedLayout value rectangles.
    backings = {
        "toy_display_header_bpm_backing.png": (472, 31, 48, 21),
        "toy_display_header_timesig_backing.png": (543, 31, 43, 21),
        "toy_display_header_preset_backing.png": (701, 26, 128, 20),
        "toy_display_speed_readout_backing.png": (745, 571, 43, 17),
        "toy_display_pitch_readout_backing.png": (797, 571, 45, 17),
        "toy_display_depth_readout_backing.png": (849, 571, 43, 17),
        "toy_display_output_l_readout_backing.png": (923, 601, 39, 21),
        "toy_display_output_r_readout_backing.png": (963, 601, 39, 21),
    }
    for name, bounds in backings.items():
        write(neutral_backing(reference, bounds), name, records, "master-reference local dynamic bounds")

    lamp = crop(reference, (433, 30, 11, 12))
    write(lamp, "toy_display_host_sync_lamp_on.png", records, "master-reference native lamp")
    write(neutral_backing(reference, (433, 30, 11, 12)), "toy_display_host_sync_lamp_off.png", records,
          "master-reference lamp substrate")

    # Each tight crop contains only the approved master-reference BAR numeral.
    # foreground_sprite restores transparent background pixels before polish.
    reference_digits = {
        0: (356, 232, 7, 10), 1: (296, 146, 7, 11), 2: (353, 146, 7, 11),
        3: (412, 146, 7, 11), 4: (470, 146, 7, 11), 5: (529, 146, 7, 11),
        6: (587, 146, 8, 11), 7: (647, 146, 7, 11), 8: (706, 146, 7, 11),
        9: (296, 232, 8, 10),
    }
    for digit, bounds in reference_digits.items():
        write(foreground_sprite(reference, bounds), f"toy_display_digit_{digit}.png", records,
              "master-reference BAR-number glyph")

    # Symbols and complete word assets come only from their approved source
    # instances; no missing character is synthesized.
    sprites = {
        "toy_display_dot.png": ((497, 46, 4, 5), "master-reference BPM decimal"),
        "toy_display_minus.png": ((799, 578, 5, 6), "master-reference pitch minus"),
        "toy_display_slash.png": ((562, 37, 7, 14), "master-reference time-signature slash"),
        "toy_display_s.png": ((831, 576, 5, 9), "master-reference pitch unit s"),
        "toy_display_t.png": ((836, 576, 5, 9), "master-reference pitch unit t"),
        "toy_display_percent.png": ((874, 575, 9, 10), "master-reference depth percent"),
        "toy_display_init.png": ((702, 29, 21, 14), "master-reference Init word"),
        "toy_display_minus_inf.png": ((933, 608, 21, 11), "master-reference output -Inf"),
    }
    for name, (bounds, source) in sprites.items():
        write(foreground_sprite(reference, bounds), name, records, source)

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({"reference": str(REFERENCE.relative_to(ROOT)).replace("\\", "/"),
                                  "assets": records}, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
