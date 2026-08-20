#!/usr/bin/env python3
"""Recover only previously approved standalone V2 assets.

This is an audit/copy operation.  It does not crop, resize, regenerate, or
modify any UI implementation input.  Every adopted file is copied verbatim
and recorded with provenance, dimensions, alpha range, and SHA-256.
"""
from __future__ import annotations

import hashlib
import json
import shutil
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "Resources/ui-v2/source/approved-standalone"
REPORT = ROOT / "ui/v2/approved-standalone-audit.json"

ADOPT = [
    ("knob_base", "Resources/knob_ring_60.png", "knob_ring_60.png", "approved-standalone", "approved 60px base/ring; transparent canvas; visual review PASS"),
    ("knob_pointer", "Resources/knob_pointer_60.png", "knob_pointer_60.png", "approved-standalone", "approved 60px pointer; transparent canvas; pivot remains (30,30)"),
    ("xy_neutral", "Resources/ui-master-default-preview-20260808/xy-buttons/xy_neutral_base_288x256.png", "xy_neutral_base_288x256.png", "approved-standalone", "previously approved neutral XY asset; normal controls are baked as static pixels"),
    ("bypass_off", "Resources/ui-user-exact-preset-bypass-20260809/bypass_off.png", "bypass_off.png", "approved-standalone", "user-provided exact asset"),
    ("bypass_on", "Resources/ui-user-exact-preset-bypass-20260809/bypass_on.png", "bypass_on.png", "approved-standalone", "user-provided exact asset"),
]

REJECTED = [
    ("rec_normal", "Resources/ui-v2/assets-1024/xy-buttons/rec-normal.png", "found-but-not-adopted", "prior V2 generated output; provenance is not an approved standalone source"),
    ("clear_normal", "Resources/ui-v2/assets-1024/xy-buttons/clear-normal.png", "found-but-not-adopted", "prior V2 generated output; provenance is not an approved standalone source"),
    ("reset_view_normal", "Resources/ui-v2/assets-1024/xy-buttons/reset-view-normal.png", "found-but-not-adopted", "prior V2 generated output; provenance is not an approved standalone source"),
    ("bar_labels_17_64", "Resources/ui-runtime-1024-20260818/bar-labels/bar_label_17.png … bar_label_64.png", "found-but-not-adopted", "legacy deterministic glyph-composition set; rejected because it is the previously problematic auto-generated label path"),
    ("runtime_background", "Resources/ui-runtime-1024-20260818/background/master_default_no_count_grid_title_1024x683.png", "found-but-not-adopted", "old runtime background may contain legacy mutable-state pixels; not a V2 neutral static source"),
]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def inspect(path: Path) -> dict:
    with Image.open(path) as image:
        alpha = {"present": "A" in image.getbands()}
        if alpha["present"]:
            alpha["min"], alpha["max"] = image.getchannel("A").getextrema()
        return {"pixelSize": [image.width, image.height], "mode": image.mode, "alpha": alpha}


def main() -> None:
    DEST.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    adopted = []
    for asset_id, source_rel, filename, provenance, visual_status in ADOPT:
        source = ROOT / source_rel
        if not source.is_file():
            raise SystemExit(f"approved source missing: {source}")
        destination = DEST / filename
        shutil.copy2(source, destination)
        record = {
            "id": asset_id,
            "sourceOriginalPath": str(source),
            "copiedV2Path": str(destination.relative_to(ROOT)).replace("\\", "/"),
            "provenance": provenance,
            "visualStatus": visual_status,
            **inspect(destination),
            "sourceSha256": sha(source),
            "copiedSha256": sha(destination),
        }
        record["copyMatch"] = record["sourceSha256"] == record["copiedSha256"]
        adopted.append(record)
    report = {
        "schemaVersion": 1,
        "scope": "V2 pre-implementation standalone asset recovery audit",
        "rules": {
            "finalMasterRecrop": "FORBIDDEN",
            "image2DirectCrop": "FORBIDDEN",
            "newAssetGeneration": "FORBIDDEN",
            "runtimeOrCandidateChanged": False,
        },
        "adopted": adopted,
        "rawSourceCoverage": {
            "recClearReset": "RAW xy_panel.png contains the three normal controls as part of its supplied panel crop; no direct standalone RAW files exist.",
            "staticPanels": ["header.png", "footer.png", "quote_panel.png", "xy_panel.png", "preset_panel.png", "parameter_panel.png", "output_panel.png"],
        },
        "foundButNotAdopted": [
            {"id": asset_id, "path": path, "status": status, "reason": reason}
            for asset_id, path, status, reason in REJECTED
        ],
        "trulyMissing": [
            "neutral static background containing left artwork and main frame, without mutable UI pixels",
            "independent REC / CLEAR / RESET VIEW normal PNGs that are RAW-source or explicitly approved standalone assets",
            "BAR 17–64 visual cell/label source assets (the only existing set is rejected legacy auto-composed labels)",
        ],
    }
    REPORT.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"adopted={len(adopted)} report={REPORT}")
    for row in adopted:
        print(f"{row['id']} {row['pixelSize']} alpha={row['alpha']} copyMatch={row['copyMatch']}")


if __name__ == "__main__":
    main()
