"""Read-only coverage audit of visible TypeIds in Fusion32 .sec sectors."""

import argparse
import json
import re
from collections import Counter
from pathlib import Path

SECTOR_NAME = re.compile(r"^(\d+)-(\d+)-(\d+)\.sec$")
CONTENT = re.compile(r"\bContent=\{([^}]*)\}", re.IGNORECASE | re.DOTALL)
ITEM = re.compile(r"(?:\{|,)\s*(\d+)(?=\s*(?:[,}]|[A-Za-z]))")
TYPE = re.compile(r"\bTypeID\s*=\s*(\d+)")
FLAGS = re.compile(r"\bFlags\s*=\s*\{([^}]*)\}")
DISGUISE = re.compile(r"\bDisguiseTarget\s*=\s*(\d+)")


def load_disguises(path):
    text = path.read_text(encoding="latin-1")
    declarations = list(TYPE.finditer(text))
    result = {}
    for index, match in enumerate(declarations):
        end = declarations[index + 1].start() if index + 1 < len(declarations) else len(text)
        record = text[match.end():end]
        flags = FLAGS.search(record)
        target = DISGUISE.search(record)
        if flags and re.search(r"\bDisguise\b", flags.group(1)) and target:
            result[int(match.group(1))] = int(target.group(1))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--origmap", type=Path, required=True)
    parser.add_argument("--objects", type=Path, required=True)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--center-x", type=int, default=32330)
    parser.add_argument("--center-y", type=int, default=32226)
    parser.add_argument("--radius", type=int, default=128)
    parser.add_argument("--z-min", type=int, default=0)
    parser.add_argument("--z-max", type=int, default=7)
    args = parser.parse_args()

    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    mapped = {int(entry["item_id"]) for entry in catalog["entries"]}
    disguises = load_disguises(args.objects)
    raw = Counter()
    visible = Counter()
    missing_samples = []
    sectors = 0
    tiles = 0
    nested = 0
    for path in args.origmap.glob("*.sec"):
        name = SECTOR_NAME.match(path.name)
        if not name:
            continue
        sx, sy, z = map(int, name.groups())
        if not args.z_min <= z <= args.z_max:
            continue
        if sx * 32 > args.center_x + args.radius or (sx + 1) * 32 - 1 < args.center_x - args.radius:
            continue
        if sy * 32 > args.center_y + args.radius or (sy + 1) * 32 - 1 < args.center_y - args.radius:
            continue
        sectors += 1
        source = path.read_text(encoding="latin-1")
        for content in CONTENT.finditer(source):
            tiles += 1
            if "Content={" in content.group(1):
                nested += 1
            line_start = source.rfind("\n", 0, content.start()) + 1
            offset = re.match(r"\s*(\d+)-(\d+):", source[line_start:content.start()])
            for item in ITEM.finditer("{" + content.group(1) + "}"):
                raw_id = int(item.group(1))
                display_id = disguises.get(raw_id, raw_id)
                raw[raw_id] += 1
                visible[display_id] += 1
                if display_id not in mapped and len(missing_samples) < 16:
                    sample = {"sector": path.name, "source_type": raw_id, "display_type": display_id}
                    if offset:
                        sample["position"] = [sx * 32 + int(offset.group(1)),
                                              sy * 32 + int(offset.group(2)), z]
                    missing_samples.append(sample)

    missing = {item: count for item, count in visible.items() if item not in mapped}
    print(json.dumps({
        "center": [args.center_x, args.center_y],
        "radius_tiles": args.radius,
        "floor_range": [args.z_min, args.z_max],
        "sectors_scanned": sectors,
        "tiles_with_content_scanned": tiles,
        "unique_source_type_ids": len(raw),
        "unique_visible_type_ids": len(visible),
        "missing_visible_type_ids": missing,
        "missing_visible_occurrences": sum(missing.values()),
        "missing_samples": missing_samples,
        "nested_content_detected": nested,
        "scope": "Visual TypeId coverage only; not a sector parser or semantic classification."
    }, indent=2))


if __name__ == "__main__":
    main()