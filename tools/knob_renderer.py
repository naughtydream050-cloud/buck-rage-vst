#!/usr/bin/env python3
"""
knob_renderer.py — KnobStrip Generator
Rotates knob_base.png by 300 degrees over 100 frames → knob_strip.png (vertical filmstrip)
Usage: python tools/knob_renderer.py
"""
import os
import math
from PIL import Image

KNOB_PATH = os.path.join(os.path.dirname(__file__), "..", "Resources", "knob_base.png")
OUT_PATH  = os.path.join(os.path.dirname(__file__), "..", "Resources", "knob_strip.png")

FRAMES      = 100
TOTAL_ANGLE = 300.0
START_ANGLE = -150.0

def make_strip():
    knob = Image.open(KNOB_PATH).convert("RGBA")
    W, H = knob.size
    strip = Image.new("RGBA", (W, H * FRAMES), (0, 0, 0, 0))

    for i in range(FRAMES):
        angle = START_ANGLE + (TOTAL_ANGLE * i / (FRAMES - 1))
        rotated = knob.rotate(-angle, resample=Image.BICUBIC, expand=False)
        strip.paste(rotated, (0, H * i))
        if i % 10 == 0:
            print(f"  frame {i:3d}/100  angle={angle:+.1f}°")

    strip.save(OUT_PATH, "PNG")
    print(f"\nSaved: {OUT_PATH}  ({W}x{H*FRAMES}px, {FRAMES} frames)")

if __name__ == "__main__":
    make_strip()
