#!/usr/bin/env python3
"""Seeds and re-syncs the REAL33D visual tracker from the generated manifests.

The manifests are derived data and may be regenerated at any time. The tracker
carries human decisions -- status, approved versions, notes -- which must never
be clobbered by a regeneration. This tool therefore merges: it adds rows for
entities that appeared, refreshes the derived columns of rows that already
exist, and reports rows whose entity no longer appears instead of deleting
them.

Column ownership:
  derived  ID, Name, Category, Subcategory, Source, Priority, Representation
  human    Status, Mockup Version, Approved Version, Production Version,
           Rig, Animation, VFX, Notes

Usage:
  sync_tracker.py --manifests visual/manifests --tracker visual/tracker/VISUAL_TRACKER.csv
  sync_tracker.py ... --p0-out visual/rookgaard_p0/P0_ASSETS.csv
"""

import argparse
import csv
import os
import sys
from collections import Counter, defaultdict

COLUMNS = [
    "ID", "Name", "Category", "Subcategory", "Source", "Priority", "Status",
    "Mockup Version", "Approved Version", "Production Version",
    "Representation", "Rig", "Animation", "VFX", "Notes",
]

DERIVED = {"ID", "Name", "Category", "Subcategory", "Source", "Priority",
           "Representation"}

# Declared in visual/docs/PIPELINE.md. Not every asset walks every stage.
STATES = ["TODO", "MOCKUP", "REVIEW", "APPROVED", "REJECTED", "MODELING",
          "TEXTURING", "RIGGING", "ANIMATION", "READY", "INTEGRATED"]


def read_csv(path):
    if not os.path.exists(path):
        return []
    with open(path, newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def build_rows(manifests):
    """Returns the derived half of every tracker row."""
    rows = []

    objects = read_csv(os.path.join(manifests, "objects.csv"))
    # A visual group's lowest id is its representative; the rest are candidates
    # to reuse that asset rather than receive one of their own.
    group_first = {}
    for row in objects:
        group = row["visual_group"]
        if group not in group_first:
            group_first[group] = row["id"]

    for row in objects:
        group = row["visual_group"]
        representative = group_first[group]
        if row["id"] == representative:
            representation = "MESH"
        elif group.startswith("alias:"):
            representation = "ALIAS_OF:obj:%s" % representative
        else:
            representation = "SHARED_CANDIDATE:obj:%s" % representative
        rows.append({
            "ID": "obj:%s" % row["id"],
            "Name": row["name"],
            "Category": row["category"],
            "Subcategory": row["subcategory"] or row["art_class"],
            "Source": row["source"],
            "Priority": row["priority"],
            "Representation": representation,
            "_rig": "NONE",
            "_animation": "NONE",
            "_vfx": "REQUIRED" if row["category"] == "effect_object" else "NONE",
        })

    for row in read_csv(os.path.join(manifests, "creatures.csv")):
        rows.append({
            "ID": "mon:%s" % row["race"],
            "Name": row["name"],
            "Category": row["category"],
            "Subcategory": row["subcategory"],
            "Source": row["source"],
            "Priority": row["priority"],
            "Representation": "SKELETAL_MESH",
            "_rig": "REQUIRED", "_animation": "REQUIRED", "_vfx": "NONE",
        })

    for row in read_csv(os.path.join(manifests, "npcs.csv")):
        rows.append({
            "ID": "npc:%s" % (row["name"] or os.path.basename(row["source"])),
            "Name": row["name"],
            "Category": row["category"],
            "Subcategory": row["subcategory"],
            "Source": row["source"],
            "Priority": row["priority"],
            # NPCs wear the same outfit identities creatures and players do, so
            # they reuse the outfit asset rather than getting a unique mesh.
            "Representation": ("SHARED_CANDIDATE:outfit:%s" % row["outfit_id"])
                              if row["outfit_id"] and row["outfit_id"] != "0"
                              else "SKELETAL_MESH",
            "_rig": "REQUIRED", "_animation": "REQUIRED", "_vfx": "NONE",
        })

    for row in read_csv(os.path.join(manifests, "outfits.csv")):
        rows.append({
            "ID": "outfit:%s" % row["outfit_id"],
            "Name": "outfit %s" % row["outfit_id"],
            "Category": row["category"],
            "Subcategory": row["subcategory"],
            "Source": row["source"],
            "Priority": "P2",
            "Representation": "SKELETAL_MESH",
            "_rig": "REQUIRED", "_animation": "REQUIRED", "_vfx": "NONE",
        })

    for row in read_csv(os.path.join(manifests, "effects.csv")):
        rows.append({
            "ID": "fx:%s" % row["effect_id"],
            "Name": row["name"],
            "Category": "effect",
            "Subcategory": "graphical_effect",
            "Source": row["source"],
            "Priority": "P2",
            "Representation": "VFX",
            "_rig": "NONE", "_animation": "NONE", "_vfx": "REQUIRED",
        })

    for row in read_csv(os.path.join(manifests, "missiles.csv")):
        rows.append({
            "ID": "msl:%s:%s" % (row["missile_attribute"], row["missile_id"]),
            "Name": "%s %s" % (row["missile_attribute"], row["missile_id"]),
            "Category": "projectile",
            "Subcategory": row["subcategory"],
            "Source": row["source"],
            "Priority": "P2",
            "Representation": "VFX",
            "_rig": "NONE", "_animation": "NONE", "_vfx": "REQUIRED",
        })

    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--manifests", required=True)
    parser.add_argument("--tracker", required=True)
    parser.add_argument("--p0-out")
    args = parser.parse_args()

    derived = build_rows(args.manifests)
    if not derived:
        print("FATAL: no manifests found under %s" % args.manifests, file=sys.stderr)
        return 2

    existing = {row["ID"]: row for row in read_csv(args.tracker)}
    seen = set()
    out_rows = []
    added = 0
    refreshed = 0

    for row in derived:
        key = row["ID"]
        seen.add(key)
        previous = existing.get(key)
        merged = {column: "" for column in COLUMNS}
        if previous:
            merged.update({c: previous.get(c, "") for c in COLUMNS})
            refreshed += 1
        else:
            merged["Status"] = "TODO"
            merged["Rig"] = row["_rig"]
            merged["Animation"] = row["_animation"]
            merged["VFX"] = row["_vfx"]
            added += 1
        for column in DERIVED:
            merged[column] = row[column]
        if merged["Status"] not in STATES:
            merged["Status"] = "TODO"
        out_rows.append(merged)

    # Rows whose entity vanished from the manifests are kept and flagged rather
    # than deleted, because they may carry approved work.
    orphaned = 0
    for key, row in existing.items():
        if key in seen:
            continue
        kept = {column: row.get(column, "") for column in COLUMNS}
        note = kept.get("Notes", "")
        marker = "ORPHANED: absent from the regenerated manifests"
        if marker not in note:
            kept["Notes"] = (note + " | " if note else "") + marker
        out_rows.append(kept)
        orphaned += 1

    out_rows.sort(key=lambda r: (r["Priority"], r["Category"], r["ID"]))

    os.makedirs(os.path.dirname(args.tracker), exist_ok=True)
    with open(args.tracker, "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS)
        writer.writeheader()
        writer.writerows(out_rows)

    if args.p0_out:
        os.makedirs(os.path.dirname(args.p0_out), exist_ok=True)
        p0 = [row for row in out_rows if row["Priority"] == "P0"]
        with open(args.p0_out, "w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=COLUMNS)
            writer.writeheader()
            writer.writerows(p0)
        print("p0 rows            %d -> %s" % (len(p0), args.p0_out))

    status_totals = Counter(row["Status"] for row in out_rows)
    representation = Counter(row["Representation"].split(":")[0] for row in out_rows)
    print("tracker rows       %d (added %d, refreshed %d, orphaned %d)"
          % (len(out_rows), added, refreshed, orphaned))
    print("by status          %s" % dict(sorted(status_totals.items())))
    print("by representation  %s" % dict(sorted(representation.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
