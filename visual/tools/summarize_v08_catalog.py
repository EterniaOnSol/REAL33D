#!/usr/bin/env python3
"""Write machine and human readable results for the local V08 QA ingest."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QA = ROOT / "visual/qa/full_catalog_v08"


def latest_results(path: Path) -> dict[int, dict]:
    result = {}
    if path.is_file():
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.strip():
                row = json.loads(line)
                result[int(row["item_id"])] = row
    return result


def main() -> None:
    manifest = json.loads((QA / "full_catalog_manifest.json").read_text(encoding="utf-8"))
    latest = latest_results(QA / "import_results.jsonl")
    statuses = Counter(row.get("status", "UNKNOWN") for row in latest.values())
    imported_files = list((ROOT / "unreal/REAL33D/Content/Experimental/V08").rglob("*.uasset"))
    imported_bytes = sum(path.stat().st_size for path in imported_files)
    all_warnings = Counter()
    technical_warnings = Counter()
    import_warnings = Counter()
    failures = []
    for item in manifest["items"]:
        for warning in item.get("warnings", []):
            all_warnings[warning] += 1
        for warning in item.get("technical_warnings", []):
            technical_warnings[warning] += 1
        attempt = latest.get(int(item["item_id"]))
        if attempt:
            for warning in attempt.get("warnings", []):
                import_warnings[warning] += 1
                all_warnings[warning] += 1
            if attempt.get("status") in {"IMPORT_FAILED", "NO_MODEL"}:
                failures.append({
                    "item_id": item["item_id"], "name": item["name"],
                    "source": item.get("glb_path"),
                    "reason": attempt.get("reason") or item.get("failure_reason"),
                })
    counts = {
        "CATALOG_TOTAL": manifest["counts"]["CATALOG_TOTAL"],
        "GLB_COUNT": manifest["counts"]["MODEL_RESOLVED"],
        "MODEL_RESOLVED": manifest["counts"]["MODEL_RESOLVED"],
        "IMPORTED_OK": statuses["IMPORTED_OK"],
        "IMPORT_FAILED": statuses["IMPORT_FAILED"],
        "NO_MODEL": manifest["counts"]["NO_MODEL"] + statuses["NO_MODEL"],
        "NOT_ATTEMPTED": manifest["counts"]["CATALOG_TOTAL"] - len(latest),
        "TEST_WARNING": sum(bool(item.get("warnings")) or bool(latest.get(int(item["item_id"]), {}).get("warnings")) for item in manifest["items"]),
        "NEEDS_ASSEMBLY": manifest["counts"]["NEEDS_ASSEMBLY"],
        "TOTAL_SOURCE_GLB_SIZE": manifest["counts"]["TOTAL_SOURCE_GLB_SIZE"],
        "UNREAL_IMPORTED_ASSET_COUNT": len(imported_files),
        "UNREAL_IMPORTED_SIZE": imported_bytes,
        "ESTIMATED_GIT_LFS_SIZE": imported_bytes,
    }
    report = {
        "schema": "real33d.experimental-v08-ingest-report.v1",
        "milestone": "VISUAL-FULL-CATALOG-INGEST-TEST-001",
        "test_only": True,
        "approved": False,
        "ready": False,
        "production_integrated": False,
        "source": manifest["source"],
        "counts": counts,
        "by_refinement_status": manifest["by_refinement_status"],
        "by_geometry_quality": manifest["by_geometry_quality"],
        "technical_warning_types": dict(technical_warnings.most_common()),
        "import_warning_types": dict(import_warnings.most_common()),
        "all_warning_types": dict(all_warnings.most_common()),
        "failures": failures,
    }
    (QA / "ingest_report.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    lines = [
        "# V08 full catalog local ingest report",
        "",
        "`TEST_IMPORTED != APPROVED != READY != production INTEGRATED`.",
        "",
        "## Counts",
        "",
        "| Metric | Value |",
        "|---|---:|",
        *[f"| {key} | {value} |" for key, value in counts.items()],
        "",
        "## Refinement state",
        "",
        "| State | Records |",
        "|---|---:|",
        *[f"| {key} | {value} |" for key, value in manifest["by_refinement_status"].items()],
        "",
        "## Geometry quality",
        "",
        "| Quality | Records |",
        "|---|---:|",
        *[f"| {key} | {value} |" for key, value in manifest["by_geometry_quality"].items()],
        "",
        "## Technical warnings",
        "",
        *([f"- `{key}`: {value}" for key, value in technical_warnings.most_common()] or ["- None"]),
        "",
        "## Import warnings",
        "",
        *([f"- `{key}`: {value}" for key, value in import_warnings.most_common()] or ["- None"]),
        "",
        "## Failures",
        "",
        *([f"- `{row['item_id']}` {row['name']}: {row['reason']} (`{row['source']}`)" for row in failures] or ["- None"]),
        "",
    ]
    (QA / "INGEST_REPORT.md").write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps(counts, indent=2))


if __name__ == "__main__":
    main()
