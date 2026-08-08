"""Create transparent, reference-derived state overlays for the Phase 1 UI.

Every visible pixel copied by this script originates in Resources/reference_ui.jpg.
The script deliberately creates only edge/status overlays: JUCE never redraws the
faceplate, cell surface, type, knob ring, or button chrome.
"""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "Resources" / "reference_ui.jpg"
OUTPUT = ROOT / "Resources" / "ui-cuts"


def edge_overlay(source: Image.Image, box: tuple[int, int, int, int], edge: int = 2,
                 status_band: tuple[int, int] | None = None) -> Image.Image:
    """Copy only source pixels used for a component's real photographic frame."""
    crop = source.crop((box[0], box[1], box[0] + box[2], box[1] + box[3])).convert("RGBA")
    overlay = Image.new("RGBA", crop.size, (0, 0, 0, 0))
    width, height = crop.size

    overlay.paste(crop.crop((0, 0, width, edge)), (0, 0))
    overlay.paste(crop.crop((0, height - edge, width, height)), (0, height - edge))
    overlay.paste(crop.crop((0, 0, edge, height)), (0, 0))
    overlay.paste(crop.crop((width - edge, 0, width, height)), (width - edge, 0))

    if status_band is not None:
        top, bottom = status_band
        overlay.paste(crop.crop((0, top, width, bottom)), (0, top))

    return overlay


def main() -> None:
    source = Image.open(REFERENCE).convert("RGBA")
    OUTPUT.mkdir(parents=True, exist_ok=True)

    # Bounds are source-pixel coordinates in the 1280 x 853 visual truth.
    cuts = {
        "bar_normal_frame.png": ((328, 160, 66, 96), 1, (78, 96)),
        "bar_selected_frame.png": ((468, 260, 66, 96), 2, (78, 96)),
        "bar_playing_frame.png": ((678, 160, 66, 96), 2, (78, 96)),
        "count_normal_frame.png": ((334, 436, 117, 72), 1, None),
        "count_selected_frame.png": ((334, 511, 117, 72), 2, None),
        "preset_normal_frame.png": ((919, 122, 109, 83), 1, None),
        "preset_selected_frame.png": ((1143, 122, 109, 83), 2, None),
        "length_normal_frame.png": ((849, 503, 47, 32), 1, None),
        "length_selected_frame.png": ((951, 503, 47, 32), 2, None),
        "tab_normal_frame.png": ((461, 91, 133, 36), 1, None),
        "tab_selected_frame.png": ((324, 91, 132, 36), 2, None),
    }

    for name, (box, edge, status_band) in cuts.items():
        edge_overlay(source, box, edge, status_band).save(OUTPUT / name)

    # The artwork-only base remains the complete reference image. This crop is
    # an explicit asset for the fixed output surround (labels and dB scale).
    source.crop((1116, 408, 1265, 768)).save(OUTPUT / "output_meter_reference.png")
    source.crop((1156, 469, 1171, 724)).save(OUTPUT / "meter_led_strip.png")
    source.crop((1172, 28, 1240, 61)).save(OUTPUT / "bypass_off.png")


if __name__ == "__main__":
    main()
