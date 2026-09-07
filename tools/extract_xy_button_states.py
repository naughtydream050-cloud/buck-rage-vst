"""Deterministically prepare native XY button state sprites from the approved 1024 reference.

No artwork is generated: normal sprites are exact supplied crops and pressed sprites are
the same native reference crop sampled one pixel lower, providing the approved physical
down-state without scaling or JUCE-drawn overlays.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "Resources/ui-v2/reference/final-master-reference-1024x683.png"
SOURCE = ROOT / "Resources/ui-v2/assets-1024/xy-buttons"
OUT = ROOT / "Resources/ui-v2/runtime-1024/xy-buttons/dynamic"

# Native reference rectangles. Reset and VIEW are independently clickable halves
# of their shared printed plate; their images tile exactly to the original 81 px plate.
BUTTONS = {
    "rec": (26, 596, 60, 30, "rec-normal.png"),
    "clear": (94, 596, 60, 30, "clear-normal.png"),
    "reset": (158, 596, 46, 30, "reset-view-normal.png"),
    "view": (204, 596, 35, 30, "reset-view-normal.png"),
}


def crop(image: Image.Image, x: int, y: int, width: int, height: int) -> Image.Image:
    return image.crop((x, y, x + width, y + height)).convert("RGBA")


def main() -> None:
    reference = Image.open(REFERENCE).convert("RGBA")
    OUT.mkdir(parents=True, exist_ok=True)
    for name, (x, y, width, height, source_name) in BUTTONS.items():
        # The supplied direct crop is the canonical normal artwork for REC/CLEAR;
        # RESET/VIEW are exact native sub-crops of their supplied shared plate.
        source = Image.open(SOURCE / source_name).convert("RGBA")
        if name == "reset":
            normal = source.crop((0, 0, width, height))
        elif name == "view":
            normal = source.crop((46, 0, 46 + width, height))
        else:
            if source.size != (width, height):
                raise ValueError(f"{source_name}: expected {(width, height)}, got {source.size}")
            normal = source
        normal.save(OUT / f"xy_{name}_normal.png")

        # A press is a native one-pixel-down sample from the same approved faceplate,
        # not a tinted frame, transform, or newly designed image.
        pressed = crop(reference, x, y + 1, width, height)
        pressed.save(OUT / f"xy_{name}_pressed.png")
        # REC and VIEW retain the physical down-state while toggled. CLEAR and RESET
        # are momentary controls and return to their exact normal image after release.
        (pressed if name in {"rec", "view"} else normal).save(OUT / f"xy_{name}_active.png")


if __name__ == "__main__":
    main()
