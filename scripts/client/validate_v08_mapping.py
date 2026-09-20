#!/usr/bin/env python3
"""Validate V08 source, import, and runtime identities without starting Unreal."""

import argparse
import hashlib
import json
import os
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QA = ROOT / "visual/qa/full_catalog_v08"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", type=Path, default=Path(os.environ.get("V08_SOURCE_ROOT", ROOT.parents[1] / "3DTIBIA_leo")), help="Root of the source 3DTIBIA checkout")
SOURCE = parser.parse_args().source


def read_json(name):
    return json.loads((QA / name).read_text(encoding="utf-8"))


manifest = read_json("full_catalog_manifest.json")
runtime = read_json("experimental_catalog_runtime.json")
imports = {}
for line in (QA / "import_results.jsonl").read_text(encoding="utf-8").splitlines():
    row = json.loads(line)
    imports[row["item_id"]] = row

items = {row["item_id"]: row for row in manifest["items"]}
entries = {row["item_id"]: row for row in runtime["entries"]}
assert len(items) == len(entries) == len(imports) == 4913
assert all(0 <= item_id <= 65535 for item_id in items)
assert set(items) == set(entries) == set(imports)

errors = []
for item_id, item in sorted(items.items()):
    imported = imports[item_id]
    entry = entries[item_id]
    mesh = f"SM_V08_{item_id:05d}"
    directory = f"ID_{item_id:05d}"
    expected_path = f"/Game/Experimental/V08/{directory}/{mesh}.{mesh}"
    asset = ROOT / "unreal/REAL33D/Content/Experimental/V08" / directory / f"{mesh}.uasset"
    if imported["source"] != item["glb_path"]:
        errors.append(f"{item_id}: imported GLB differs from manifest")
    if imported["mesh_path"] != expected_path or entry["mesh_path"] != expected_path:
        errors.append(f"{item_id}: imported/runtime mesh path differs from TypeId")
    if imported["status"] != "IMPORTED_OK" or entry["import_status"] != "IMPORTED_OK":
        errors.append(f"{item_id}: import status differs")
    if entry["name"] != item["name"] or entry["refinement_status"] != item["refinement_status"]:
        errors.append(f"{item_id}: catalog identity metadata differs")
    if not asset.is_file():
        errors.append(f"{item_id}: imported .uasset is missing")

sample = {408, 1270, 1294, 1295, 1301, 1303, 1626, 1627, 1735, 2173,
          2174, 2328, 3497, 3498, 3499, 3500, 3501, 3502, 3508}
for status in ("REFINED", "RETAINED_REFERENCE", "IN_REVIEW", "PENDING", "NEEDS_ASSEMBLY"):
    sample.add(min(item_id for item_id, row in items.items() if row["refinement_status"] == status))
for warning in ("PIVOT_NOT_AT_FLOOR", "DEGENERATE_UV_TRIANGLES", "ZERO_AREA_TRIANGLES", "VERY_FLAT"):
    sample.add(min(item_id for item_id, row in items.items() if warning in row["technical_warnings"]))

lines = [
    "# V08 deterministic identity check",
    "",
    "This is a static data check. It does not prove a live Fusion32 TypeId or a visible Unreal actor.",
    "",
    f"Checked rows: {len(items)}; mismatches: {len(errors)}.",
    "",
    "| V08 ID / intended Fusion32 TypeId | Name | State | Source GLB | Imported Unreal asset | GLB hash | Pending warnings |",
    "|---:|---|---|---|---|---|---|",
]
for item_id in sorted(sample):
    item = items[item_id]
    source = SOURCE / item["glb_path"]
    digest = hashlib.sha256(source.read_bytes()).hexdigest() if source.is_file() else "MISSING"
    if digest != item["model_sha256"]:
        errors.append(f"{item_id}: source GLB SHA-256 differs from manifest")
    lines.append(
        f"| {item_id} | {item['name']} | {item['refinement_status']} | "
        f"`{item['glb_path']}` | `{entries[item_id]['mesh_path']}` | "
        f"{'MATCH' if digest == item['model_sha256'] else 'MISMATCH'} | "
        f"{', '.join(item['technical_warnings']) or '?'} |"
    )

lines[4] = f"Checked rows: {len(items)}; mismatches: {len(errors)}."
lines += [
    "", "## Intentional depot appearance in V08 QA", "",
    "Fusion32 places locker TypeIds 3497 through 3500 in cities. They keep those logical IDs and container behavior; the Unreal experimental visual resolver displays the imported 3502 depot-chest mesh for each. The source/import/runtime catalog mapping for every item still points to its own imported asset. This is an explicit presentation alias, not an identity substitution in WorldState.",
    "", "| Fusion32 TypeId | Catalog asset for that ID | Displayed QA mesh |",
    "|---:|---|---|",
]
for locker_id in (3497, 3498, 3499, 3500):
    lines.append(f"| {locker_id} | `{entries[locker_id]['mesh_path']}` | `{entries[3502]['mesh_path']}` |")
for wall_id in (1295, 1301, 1303):
    lines.append(f"| {wall_id} | `{entries[wall_id]['mesh_path']}` | `{entries[1294]['mesh_path']}` |")
lines += ["", "Wall TypeIds 1295, 1301, and 1303 use the reviewed 1294 appearance in this experimental presentation. TypeId 429 remains a stone tile and is not replaced by a wall mesh. Every alias preserves its logical WorldState TypeId.", "", "## QA warnings pending visual evaluation", "", "| Warning | Catalog entries |", "|---|---:|"]
for warning in ("DEGENERATE_UV_TRIANGLES", "ZERO_AREA_TRIANGLES", "PIVOT_NOT_AT_FLOOR", "VERY_FLAT"):
    lines.append(f"| {warning} | {sum(warning in row['technical_warnings'] for row in items.values())} |")
lines += ["", "These counts remain warnings; no visual severity conclusion is assigned.", "", "## Mismatches", ""]
lines += [f"- {error}" for error in errors] if errors else ["None."]
lines += ["", "The registry derives `TypeId` directly from each runtime `item_id`; live WorldState events require a separate test.", ""]
report = QA / "MAPPING_VALIDATION.md"
report.write_text("\n".join(lines), encoding="utf-8")
print(f"V08 mapping: {len(items)} rows, {len(sample)} sampled GLB hashes, {len(errors)} mismatches; {report}")
if errors:
    raise SystemExit(1)
