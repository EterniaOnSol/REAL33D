#!/usr/bin/env python3
"""Normalize and validate the external V08 catalog for REAL33D QA.

The 3DTIBIA checkout is read only. Paths written to the output are relative to
that checkout, so the manifest contains no workstation specific source paths.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import math
import os
import struct
import subprocess
from collections import Counter
from pathlib import Path


CATALOG_RELATIVE = Path(
    "worklog/ORQUESTADOR/generaciones_assets/refinamiento_en_curso_v8/manifest.json"
)
PACKAGE_MANIFEST_RELATIVE = Path("worklog/ORQUESTADOR/generaciones_assets/MANIFIESTO.json")
EXPECTED_REPOSITORY = "leodavidsoto/3DTIBIA"
EXPECTED_BRANCH = "carril/ORQUESTADOR"
EXPECTED_COMMIT = "21fafb57dd86b594223bab7dbe9076d1fa380640"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def relative_to_repo(path: Path, repo: Path) -> str | None:
    try:
        return path.resolve().relative_to(repo.resolve()).as_posix()
    except ValueError:
        return None


def resolve_catalog_path(value: object, catalog_dir: Path, repo: Path) -> tuple[Path | None, str | None]:
    if not isinstance(value, str) or not value:
        return None, None
    candidate = Path(value)
    if candidate.is_absolute():
        # Published manifests retain some author's macOS paths. They are
        # provenance text, never usable paths on this checkout.
        return None, None
    resolved = (catalog_dir / candidate).resolve()
    return resolved, relative_to_repo(resolved, repo)


def inspect_glb(path: Path) -> dict:
    result = {
        "valid": False,
        "reason": None,
        "model_sha256": None,
        "bytes": path.stat().st_size if path.is_file() else 0,
        "source_transform": "glTF right-handed, +Y up, metres",
        "imported_transform": "Unreal left-handed, +Z up, centimetres",
        "axis_conversion": "Interchange glTF +Y-up to Unreal +Z-up",
        "scale_conversion": "1 source tile/metre = 100 Unreal Units",
        "bounds_min": None,
        "bounds_max": None,
        "pivot_warning": None,
        "mesh_count": 0,
        "node_count": 0,
        "material_count": 0,
        "texture_count": 0,
        "image_count": 0,
        "skin_count": 0,
        "animation_count": 0,
        "structural_triangles": 0,
        "zero_area_triangles": 0,
        "degenerate_uv_triangles": 0,
        "warnings": [],
    }
    if not path.is_file():
        result["reason"] = "NO_MODEL: source path does not exist"
        return result
    try:
        data = path.read_bytes()
        header = data[:12]
        if len(header) != 12:
            raise ValueError("truncated GLB header")
        magic, version, declared = struct.unpack("<4sII", header)
        if magic != b"glTF" or version != 2:
            raise ValueError("not a glTF 2.0 binary")
        if declared != result["bytes"]:
            raise ValueError(f"declared length {declared} differs from file size {result['bytes']}")
        json_length, json_type = struct.unpack_from("<II", data, 12)
        if json_type != 0x4E4F534A:
            raise ValueError("first GLB chunk is not JSON")
        document = json.loads(data[20 : 20 + json_length].decode("utf-8"))
        binary_header = 20 + json_length
        binary = b""
        if binary_header + 8 <= len(data):
            binary_length, binary_type = struct.unpack_from("<II", data, binary_header)
            if binary_type == 0x004E4942:
                binary = data[binary_header + 8 : binary_header + 8 + binary_length]
        if not document.get("meshes"):
            raise ValueError("GLB has no mesh")
        result["mesh_count"] = len(document.get("meshes", []))
        result["node_count"] = len(document.get("nodes", []))
        result["material_count"] = len(document.get("materials", []))
        result["texture_count"] = len(document.get("textures", []))
        result["image_count"] = len(document.get("images", []))
        result["skin_count"] = len(document.get("skins", []))
        result["animation_count"] = len(document.get("animations", []))
        for image in document.get("images", []):
            uri = image.get("uri")
            if uri and not uri.startswith("data:") and not (path.parent / uri).is_file():
                result["warnings"].append("MISSING_EXTERNAL_TEXTURE")
        for buffer in document.get("buffers", []):
            uri = buffer.get("uri")
            if uri and not uri.startswith("data:") and not (path.parent / uri).is_file():
                result["warnings"].append("MISSING_EXTERNAL_BUFFER")
        accessors = document.get("accessors", [])
        buffer_views = document.get("bufferViews", [])

        def values(accessor_index: int):
            accessor = accessors[accessor_index]
            if "bufferView" not in accessor or not binary:
                return None
            view = buffer_views[accessor["bufferView"]]
            component = accessor["componentType"]
            code, component_bytes = {
                5120: ("b", 1), 5121: ("B", 1), 5122: ("h", 2),
                5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4),
            }[component]
            width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[accessor["type"]]
            offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
            stride = view.get("byteStride", component_bytes * width)
            fmt = "<" + code * width
            return [struct.unpack_from(fmt, binary, offset + row * stride) for row in range(accessor["count"])]

        mins, maxs = [], []
        for mesh in document["meshes"]:
            for primitive in mesh.get("primitives", []):
                position_index = primitive.get("attributes", {}).get("POSITION")
                if position_index is None or position_index >= len(accessors):
                    raise ValueError("mesh primitive has no valid POSITION accessor")
                accessor = accessors[position_index]
                if accessor.get("count", 0) <= 0:
                    raise ValueError("mesh primitive has an empty POSITION accessor")
                mode = primitive.get("mode", 4)
                if mode != 4:
                    result["warnings"].append(f"NON_TRIANGLE_PRIMITIVE_MODE_{mode}")
                index = primitive.get("indices")
                count = accessors[index].get("count", 0) if index is not None else accessor.get("count", 0)
                if mode == 4:
                    result["structural_triangles"] += count // 3
                    positions = values(position_index)
                    index_values = values(index) if index is not None else None
                    indices = [row[0] for row in index_values] if index_values is not None else list(range(count))
                    texcoord_index = primitive.get("attributes", {}).get("TEXCOORD_0")
                    texcoords = values(texcoord_index) if texcoord_index is not None else None
                    if "NORMAL" not in primitive.get("attributes", {}):
                        result["warnings"].append("MISSING_NORMALS")
                    if texcoords is None:
                        result["warnings"].append("MISSING_TEXCOORD_0")
                    if positions is not None:
                        for triangle in range(0, len(indices) - 2, 3):
                            ia, ib, ic = indices[triangle : triangle + 3]
                            a, b, c = positions[ia], positions[ib], positions[ic]
                            ab = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
                            ac = (c[0] - a[0], c[1] - a[1], c[2] - a[2])
                            cross = (
                                ab[1] * ac[2] - ab[2] * ac[1],
                                ab[2] * ac[0] - ab[0] * ac[2],
                                ab[0] * ac[1] - ab[1] * ac[0],
                            )
                            if sum(value * value for value in cross) <= 1e-20:
                                result["zero_area_triangles"] += 1
                            if texcoords is not None:
                                ua, ub, uc = texcoords[ia], texcoords[ib], texcoords[ic]
                                determinant = ((ub[0] - ua[0]) * (uc[1] - ua[1])
                                               - (ub[1] - ua[1]) * (uc[0] - ua[0]))
                                if abs(determinant) <= 1e-12:
                                    result["degenerate_uv_triangles"] += 1
                if len(accessor.get("min", [])) == 3 and len(accessor.get("max", [])) == 3:
                    mins.append(accessor["min"])
                    maxs.append(accessor["max"])
        if not mins:
            raise ValueError("POSITION accessors do not publish bounds")
        bounds_min = [min(v[n] for v in mins) for n in range(3)]
        bounds_max = [max(v[n] for v in maxs) for n in range(3)]
        if not all(math.isfinite(value) for value in bounds_min + bounds_max):
            raise ValueError("non-finite mesh bounds")
        extents = [bounds_max[n] - bounds_min[n] for n in range(3)]
        result["bounds_min"] = bounds_min
        result["bounds_max"] = bounds_max
        floor = bounds_min[1]
        if abs(floor) > 0.001:
            result["pivot_warning"] = f"source floor is Y={floor:.6g}, not zero; imported asset may float or be underground"
            result["warnings"].append("PIVOT_NOT_AT_FLOOR")
        if min(extents) < 0.02:
            result["warnings"].append("VERY_FLAT")
        if max(extents) > 8.0:
            result["warnings"].append("POSSIBLY_GIGANTIC")
        if max(extents) < 0.08:
            result["warnings"].append("POSSIBLY_TINY")
        if result["zero_area_triangles"]:
            result["warnings"].append("ZERO_AREA_TRIANGLES")
        if result["degenerate_uv_triangles"]:
            result["warnings"].append("DEGENERATE_UV_TRIANGLES")
        result["valid"] = True
        result["model_sha256"] = hashlib.sha256(data).hexdigest()
    except Exception as error:  # every failure becomes inventory data
        result["reason"] = f"INVALID_GLB: {error}"
    return result


def git_value(repo: Path, *args: str) -> str:
    command = ["git", "-c", f"safe.directory={repo.as_posix()}", "-C", str(repo), *args]
    return subprocess.check_output(command, text=True, stderr=subprocess.STDOUT).strip()


def build(source_repo: Path, output: Path, jobs: int) -> dict:
    source_repo = source_repo.resolve()
    catalog_path = source_repo / CATALOG_RELATIVE
    catalog_dir = catalog_path.parent
    branch = git_value(source_repo, "branch", "--show-current")
    commit = git_value(source_repo, "rev-parse", "HEAD")
    origin = git_value(source_repo, "remote", "get-url", "origin")
    if branch != EXPECTED_BRANCH or commit != EXPECTED_COMMIT or EXPECTED_REPOSITORY not in origin:
        raise RuntimeError(
            f"source identity mismatch: branch={branch!r} commit={commit!r} origin={origin!r}"
        )
    source = json.loads(catalog_path.read_text(encoding="utf-8"))
    package_manifest_path = source_repo / PACKAGE_MANIFEST_RELATIVE
    package_manifest = json.loads(package_manifest_path.read_text(encoding="utf-8"))
    package_files = {row["ruta"]: row for row in package_manifest["archivos"]}
    items = source["items"]
    if len(items) != 4913 or source.get("count") != 4913:
        raise RuntimeError(f"expected 4913 V08 records, found {len(items)}")

    resolved_models = []
    for item in items:
        model, model_relative = resolve_catalog_path(item.get("model"), catalog_dir, source_repo)
        resolved_models.append((model, model_relative))

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        inspections = list(pool.map(
            lambda pair: inspect_glb(pair[0]) if pair[0] is not None else {
                "valid": False, "reason": "NO_MODEL: unusable published path", "model_sha256": None,
                "bytes": 0, "warnings": [], "source_transform": "UNKNOWN",
                "imported_transform": "UNKNOWN", "axis_conversion": "UNKNOWN",
                "scale_conversion": "UNKNOWN", "bounds_min": None, "bounds_max": None,
                "pivot_warning": None, "mesh_count": 0, "node_count": 0,
                "material_count": 0, "texture_count": 0, "image_count": 0,
                "skin_count": 0, "animation_count": 0, "structural_triangles": 0,
                "zero_area_triangles": 0, "degenerate_uv_triangles": 0,
            },
            resolved_models,
        ))

    normalized = []
    for item, (_, model_relative), technical in zip(items, resolved_models, inspections):
        blend, blend_relative = resolve_catalog_path(item.get("source_blend"), catalog_dir, source_repo)
        sprite, sprite_relative = resolve_catalog_path(item.get("sprite"), catalog_dir, source_repo)
        render_paths = {}
        for key, value in (item.get("renders") or {}).items():
            _, render_relative = resolve_catalog_path(value, catalog_dir, source_repo)
            render_paths[key] = render_relative
        refinement = item.get("refinement") or {}
        assembly = refinement.get("assembly") or {}
        if technical["valid"] and technical["material_count"] != int(item.get("materials") or 0):
            technical["warnings"].append("CATALOG_MATERIAL_COUNT_DIFFERS")
        if technical["valid"] and technical["structural_triangles"] != int(item.get("triangles") or 0):
            technical["warnings"].append("CATALOG_TRIANGLE_COUNT_DIFFERS")
        package_prefix = "worklog/ORQUESTADOR/generaciones_assets/"
        package_key = model_relative.removeprefix(package_prefix) if model_relative else None
        package_row = package_files.get(package_key) if package_key else None
        provenance_match = bool(
            package_row
            and package_row.get("bytes") == technical["bytes"]
            and package_row.get("sha256") == technical["model_sha256"]
        )
        if not provenance_match:
            technical["warnings"].append("PACKAGE_MANIFEST_MISSING_OR_MISMATCH")
        warnings = []
        if refinement.get("status") == "NEEDS_ASSEMBLY" or assembly.get("resolved") is False:
            warnings.append("ASSEMBLY_UNRESOLVED")
        if item.get("quality") in {"FAMILY_RECIPE", "PROVISIONAL_SPRITE_RELIEF"}:
            warnings.append(item["quality"])
        normalized.append({
            "item_id": int(item["id"]),
            "name": item.get("name", ""),
            "category": item.get("category", "UNKNOWN"),
            "refinement_status": refinement.get("status", "UNKNOWN"),
            "geometry_quality": item.get("quality", "UNKNOWN"),
            "version": item.get("version", "UNKNOWN"),
            "generation": item.get("source_batch", "UNKNOWN"),
            "glb_path": model_relative,
            "blend_path": blend_relative if blend is not None and blend.is_file() else None,
            "sprite_path": sprite_relative if sprite is not None and sprite.is_file() else None,
            "renders": render_paths,
            "dimensions": item.get("dimensions_tiles"),
            "triangles": item.get("triangles"),
            "materials": item.get("materials"),
            "source_sha256": item.get("source_sha256"),
            "model_sha256": technical["model_sha256"],
            "package_manifest_match": provenance_match,
            "model_bytes": technical["bytes"],
            "model_status": "MODEL_RESOLVED" if technical["valid"] else "NO_MODEL",
            "failure_reason": technical["reason"],
            "source_transform": technical["source_transform"],
            "imported_transform": technical["imported_transform"],
            "axis_conversion": technical["axis_conversion"],
            "scale_conversion": technical["scale_conversion"],
            "bounds_min": technical["bounds_min"],
            "bounds_max": technical["bounds_max"],
            "pivot_warning": technical["pivot_warning"],
            "mesh_count": technical["mesh_count"],
            "node_count": technical["node_count"],
            "material_count_structural": technical["material_count"],
            "texture_count": technical["texture_count"],
            "image_count": technical["image_count"],
            "skin_count": technical["skin_count"],
            "animation_count": technical["animation_count"],
            "structural_triangles": technical["structural_triangles"],
            "zero_area_triangles": technical["zero_area_triangles"],
            "degenerate_uv_triangles": technical["degenerate_uv_triangles"],
            "technical_warnings": sorted(set(technical["warnings"])),
            "catalog_warnings": sorted(set(warnings)),
            "warnings": sorted(set(warnings + technical["warnings"])),
            "assembly_assessment": assembly.get("assessment"),
            "assembly_resolved": assembly.get("resolved"),
            "art_approval": "NO",
            "test_imported": False,
            "unreal_mesh_path": None,
        })

    counts = {
        "CATALOG_TOTAL": len(normalized),
        "MODEL_RESOLVED": sum(i["model_status"] == "MODEL_RESOLVED" for i in normalized),
        "NO_MODEL": sum(i["model_status"] == "NO_MODEL" for i in normalized),
        "NEEDS_ASSEMBLY": sum(i["refinement_status"] == "NEEDS_ASSEMBLY" for i in normalized),
        "TEST_WARNING": sum(bool(i["warnings"]) for i in normalized),
        "TECHNICAL_WARNING": sum(bool(i["technical_warnings"]) for i in normalized),
        "TOTAL_SOURCE_GLB_SIZE": sum(i["model_bytes"] for i in normalized),
        "PACKAGE_MANIFEST_MATCH": sum(i["package_manifest_match"] for i in normalized),
    }
    result = {
        "schema": "real33d.experimental-v08-test-catalog.v1",
        "milestone": "VISUAL-FULL-CATALOG-INGEST-TEST-001",
        "test_only": True,
        "approved": False,
        "ready": False,
        "production_integrated": False,
        "source": {
            "repository": EXPECTED_REPOSITORY,
            "origin": origin,
            "branch": branch,
            "commit": commit,
            "catalog": CATALOG_RELATIVE.as_posix(),
            "catalog_sha256": sha256(catalog_path),
            "package_manifest": PACKAGE_MANIFEST_RELATIVE.as_posix(),
            "package_manifest_sha256": sha256(package_manifest_path),
        },
        "counts": counts,
        "by_refinement_status": dict(sorted(Counter(i["refinement_status"] for i in normalized).items())),
        "by_geometry_quality": dict(sorted(Counter(i["geometry_quality"] for i in normalized).items())),
        "items": normalized,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.write_text(json.dumps(result, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    os.replace(temporary, output)
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=Path(r"C:\Users\dell\3DTIBIA_leo"))
    parser.add_argument(
        "--output", type=Path,
        default=Path("visual/qa/full_catalog_v08/full_catalog_manifest.json"),
    )
    parser.add_argument("--jobs", type=int, default=min(16, os.cpu_count() or 4))
    args = parser.parse_args()
    result = build(args.source, args.output, args.jobs)
    print(json.dumps(result["counts"], indent=2))
    print(json.dumps(result["by_refinement_status"], indent=2))
    print(json.dumps(result["by_geometry_quality"], indent=2))


if __name__ == "__main__":
    main()
