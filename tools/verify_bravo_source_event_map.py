#!/usr/bin/env python3
"""Structural guard for the measured BRAVO event map."""
from __future__ import annotations
import json
import sys
from pathlib import Path

VALID = {"forwardCut", "backspin", "chirp", "baby", "transform", "drag", "zigzag", "tapeBrake"}
MAP = Path(__file__).resolve().parents[1] / "reports/bravodomo-preset-analysis/source-specific-event-map.json"

def main() -> None:
    data = json.loads(MAP.read_text(encoding="utf-8"))
    assert data["schema"] == "toyotomi.bravo.source-specific-events.v1"
    assert len(data["tracks"]) == 2
    total = 0
    for track in data["tracks"]:
        assert len(track["source_sha256"]) == 64 and track["normalized_bpm"] == 120.0
        assert "adoption_threshold" in track["analysis"]
        bars = []
        for event in track["events"]:
            assert event["chosen_preset"] in VALID and event["confidence"] >= 0.70
            assert event["bar"] >= 1 and event["beat"] in (1, 2, 3, 4)
            for key in ("rms_before", "rms_after", "brightness_delta", "repeat_similarity", "boundary_score"):
                assert isinstance(event[key], (float, int))
            bars.append(event["bar"])
        if len(bars) >= 4:
            assert not all(bar % 4 in (3, 0) for bar in bars), "fixed four-BAR template detected"
        assert track["events"] or track["excluded_events"], "missing adoption explanation"
        total += len(bars)
    print(f"PASS source-specific-event-map tracks=2 adopted_events={total}")

if __name__ == "__main__":
    try:
        main()
    except (AssertionError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"FAIL source-specific-event-map: {error}", file=sys.stderr)
        sys.exit(1)
