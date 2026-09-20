#!/usr/bin/env python3
"""Deterministic checks for the generated V08 experimental manifest."""

import json
from collections import Counter
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[1]
path = ROOT / "visual/qa/full_catalog_v08/full_catalog_manifest.json"
data = json.loads(path.read_text(encoding="utf-8"))
items = data["items"]

assert data["schema"] == "real33d.experimental-v08-test-catalog.v1"
assert data["source"]["commit"] == "21fafb57dd86b594223bab7dbe9076d1fa380640"
assert data["test_only"] is True
assert data["approved"] is False
assert data["ready"] is False
assert data["production_integrated"] is False
assert len(items) == 4913
assert len({item["item_id"] for item in items}) == 4913
assert all(item["model_status"] == "MODEL_RESOLVED" for item in items)
assert all(item["package_manifest_match"] is True for item in items)
assert all(item["art_approval"] == "NO" and item["test_imported"] is False for item in items)
assert all(
    item["glb_path"]
    and not PurePosixPath(item["glb_path"]).is_absolute()
    and ".." not in PurePosixPath(item["glb_path"]).parts
    for item in items
)
assert data["by_refinement_status"] == {
    "IN_REVIEW": 599,
    "NEEDS_ASSEMBLY": 12,
    "PENDING": 4126,
    "REFINED": 54,
    "RETAINED_REFERENCE": 122,
}
assert data["by_geometry_quality"] == {
    "APPROVED_PILOT_REFERENCE": 60,
    "FAMILY_RECIPE": 2399,
    "PROVISIONAL_SPRITE_RELIEF": 2454,
}
technical = Counter(warning for item in items for warning in item["technical_warnings"])
assert technical == {
    "DEGENERATE_UV_TRIANGLES": 4397,
    "ZERO_AREA_TRIANGLES": 2272,
    "PIVOT_NOT_AT_FLOOR": 1253,
    "VERY_FLAT": 19,
}
assert sum(item["model_bytes"] for item in items) == 485747980
print("V08 catalog manifest PASS: 4913 unique, structurally valid, provenance-matched GLBs")
