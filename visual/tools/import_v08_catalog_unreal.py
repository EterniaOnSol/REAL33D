"""Resumable Unreal 5.8 import worker for the experimental V08 catalog.

Run only through scripts/client/import_v08_catalog_unreal.ps1. Every completed
attempt is appended to JSONL before the next item starts, so an interrupted run
can resume without guessing. Source GLBs are opened read only and never copied
or rewritten.
"""

from __future__ import annotations

import json
import os
import re
import time
from collections import Counter
from pathlib import Path

import unreal


def option(name: str, default: str) -> str:
    command = unreal.SystemLibrary.get_command_line()
    match = re.search(rf"(?:^|\s)-{re.escape(name)}=(?:\"([^\"]*)\"|(\S+))", command)
    return (match.group(1) or match.group(2)) if match else default


def load_latest(log_path: Path) -> dict[int, dict]:
    latest: dict[int, dict] = {}
    if not log_path.is_file():
        return latest
    with log_path.open("r", encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            if not line.strip():
                continue
            try:
                record = json.loads(line)
                latest[int(record["item_id"])] = record
            except Exception as error:
                raise RuntimeError(f"invalid import log line {line_number}: {error}") from error
    return latest


def append_record(log_path: Path, record: dict) -> None:
    with log_path.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def write_runtime_manifest(base: dict, latest: dict[int, dict], output: Path) -> None:
    entries = []
    for item in base["items"]:
        attempt = latest.get(int(item["item_id"]), {})
        entries.append({
            "item_id": item["item_id"],
            "name": item["name"],
            "category": item["category"],
            "refinement_status": item["refinement_status"],
            "geometry_quality": item["geometry_quality"],
            "generation": item["generation"],
            "model_status": item["model_status"],
            "import_status": attempt.get("status", "NOT_ATTEMPTED"),
            "mesh_path": attempt.get("mesh_path"),
            "warnings": sorted(set(item.get("warnings", []) + attempt.get("warnings", []))),
            "failure_reason": attempt.get("reason") or item.get("failure_reason"),
        })
    statuses = Counter(item["import_status"] for item in entries)
    payload = {
        "schema": "real33d.experimental-v08-runtime.v1",
        "milestone": "VISUAL-FULL-CATALOG-INGEST-TEST-001",
        "test_only": True,
        "approved": False,
        "ready": False,
        "production_integrated": False,
        "source": base["source"],
        "counts": {
            **base["counts"],
            "IMPORTED_OK": statuses["IMPORTED_OK"],
            "IMPORT_FAILED": statuses["IMPORT_FAILED"],
            "NOT_ATTEMPTED": statuses["NOT_ATTEMPTED"],
            "UNREAL_IMPORTED_ASSET_COUNT": sum(
                int(latest.get(int(item["item_id"]), {}).get("asset_count", 0))
                for item in base["items"]
            ),
        },
        "by_refinement_status": base["by_refinement_status"],
        "by_geometry_quality": base["by_geometry_quality"],
        "entries": entries,
    }
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    os.replace(temporary, output)


def import_one(item: dict, source_repo: Path) -> dict:
    item_id = int(item["item_id"])
    started = time.time()
    record = {
        "item_id": item_id,
        "name": item["name"],
        "source": item.get("glb_path"),
        "status": "IMPORT_FAILED",
        "reason": None,
        "mesh_path": None,
        "asset_count": 0,
        "imported_bounds_min": None,
        "imported_bounds_max": None,
        "warnings": [],
        "duration_seconds": 0.0,
    }
    if item.get("model_status") != "MODEL_RESOLVED" or not item.get("glb_path"):
        record["status"] = "NO_MODEL"
        record["reason"] = item.get("failure_reason") or "catalog has no valid model"
        return record

    source = source_repo / item["glb_path"]
    destination = f"/Game/Experimental/V08/ID_{item_id:05d}"
    desired_mesh = f"{destination}/SM_V08_{item_id:05d}"
    try:
        paths = list(unreal.EditorAssetLibrary.list_assets(destination, recursive=True))
        if unreal.EditorAssetLibrary.does_asset_exist(desired_mesh):
            mesh = unreal.EditorAssetLibrary.load_asset(desired_mesh)
        else:
            task = unreal.AssetImportTask()
            task.filename = str(source)
            task.destination_path = destination
            task.automated = True
            task.async_ = False
            task.replace_existing = False
            task.save = True
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            paths = list(unreal.EditorAssetLibrary.list_assets(destination, recursive=True))
            meshes = []
            for path in paths:
                asset = unreal.EditorAssetLibrary.load_asset(path)
                if isinstance(asset, unreal.StaticMesh):
                    meshes.append((path, asset))
            if len(meshes) != 1:
                raise RuntimeError(f"expected exactly one StaticMesh, found {len(meshes)}")
            old_path, mesh = meshes[0]
            if old_path.split(".", 1)[0] != desired_mesh:
                if not unreal.EditorAssetLibrary.rename_asset(old_path, desired_mesh):
                    raise RuntimeError(f"could not rename mesh {old_path} to {desired_mesh}")
                mesh = unreal.EditorAssetLibrary.load_asset(desired_mesh)
            paths = list(unreal.EditorAssetLibrary.list_assets(destination, recursive=True))
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("deterministic mesh asset is missing or has the wrong class")
        bounds = mesh.get_bounding_box()
        record["imported_bounds_min"] = [bounds.min.x, bounds.min.y, bounds.min.z]
        record["imported_bounds_max"] = [bounds.max.x, bounds.max.y, bounds.max.z]
        if abs(bounds.min.z) > 0.1:
            record["warnings"].append("IMPORTED_PIVOT_NOT_AT_FLOOR")
        extents = [
            bounds.max.x - bounds.min.x,
            bounds.max.y - bounds.min.y,
            bounds.max.z - bounds.min.z,
        ]
        if max(extents) > 800.0:
            record["warnings"].append("IMPORTED_POSSIBLY_GIGANTIC")
        if max(extents) < 8.0:
            record["warnings"].append("IMPORTED_POSSIBLY_TINY")
        if min(extents) < 2.0:
            record["warnings"].append("IMPORTED_VERY_FLAT")
        if len(mesh.get_editor_property("static_materials")) != int(item.get("materials") or 0):
            record["warnings"].append("IMPORTED_MATERIAL_COUNT_DIFFERS")
        for path in paths:
            unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
        record["status"] = "IMPORTED_OK"
        record["mesh_path"] = mesh.get_path_name()
        record["asset_count"] = len(paths)
    except Exception as error:
        record["reason"] = str(error)
    record["warnings"] = sorted(set(record["warnings"]))
    record["duration_seconds"] = round(time.time() - started, 3)
    return record


def main() -> None:
    repo = Path(unreal.Paths.project_dir()).resolve().parents[1]
    source_repo = Path(option("fullcatalog-source", r"C:\Users\dell\3DTIBIA_leo"))
    manifest_path = Path(option(
        "fullcatalog-manifest",
        str(repo / "visual/qa/full_catalog_v08/full_catalog_manifest.json"),
    ))
    output_dir = repo / "visual/qa/full_catalog_v08"
    output_dir.mkdir(parents=True, exist_ok=True)
    log_path = output_dir / "import_results.jsonl"
    runtime_path = output_dir / "experimental_catalog_runtime.json"
    start = max(0, int(option("fullcatalog-start", "0")))
    limit = max(0, int(option("fullcatalog-limit", "0")))
    retry_failed = option("fullcatalog-retry-failed", "0") == "1"

    base = json.loads(manifest_path.read_text(encoding="utf-8"))
    if base.get("schema") != "real33d.experimental-v08-test-catalog.v1":
        raise RuntimeError("unexpected normalized catalog schema")
    latest = load_latest(log_path)
    candidates = base["items"][start : start + limit if limit else None]
    attempted = 0
    for item in candidates:
        previous = latest.get(int(item["item_id"]))
        if previous and previous.get("status") == "IMPORTED_OK":
            continue
        if previous and previous.get("status") == "IMPORT_FAILED" and not retry_failed:
            continue
        record = import_one(item, source_repo)
        append_record(log_path, record)
        latest[int(item["item_id"])] = record
        attempted += 1
        print("REAL33D_V08_IMPORT", json.dumps(record, ensure_ascii=False, separators=(",", ":")))
        if attempted % 25 == 0:
            write_runtime_manifest(base, latest, runtime_path)
            if hasattr(unreal.SystemLibrary, "collect_garbage"):
                unreal.SystemLibrary.collect_garbage()
    write_runtime_manifest(base, latest, runtime_path)
    counts = Counter(record.get("status") for record in latest.values())
    print("REAL33D_V08_BATCH_COMPLETE", json.dumps({
        "start": start,
        "limit": limit,
        "attempted_this_run": attempted,
        "IMPORTED_OK": counts["IMPORTED_OK"],
        "IMPORT_FAILED": counts["IMPORT_FAILED"],
        "NO_MODEL": counts["NO_MODEL"],
    }, separators=(",", ":")))


main()
