#!/usr/bin/env python3
"""Build a small private player-level index from the already installed static world.

This reads reference data only. Output stays in the ignored knowledge_local tree.
It contains public NPC homes and monster *home regions*, never live occupants.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

ORIGIN = (32096, 32208, 7)


def fields(path):
    raw = path.read_bytes()
    text = raw.decode("latin-1")
    return text, hashlib.sha256(raw).hexdigest()


def value(text, key):
    found = re.search(r"^\s*" + re.escape(key) + r"\s*=\s*(.*)$", text, re.M | re.I)
    return found.group(1).strip() if found else None


def number(text, key):
    match = re.match(r"-?\d+", value(text, key) or "")
    return int(match.group()) if match else None


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--source", type=Path, required=True,
                   help="Existing Fusion32 game/reference directory")
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--radius", type=int, default=90)
    args = p.parse_args()
    root = args.source.resolve()
    if args.radius < 1 or args.radius > 200:
        p.error("radius must be 1..200")
    if not (root / "dat/monster.db").is_file() or not (root / "npc").is_dir():
        p.error("source needs dat/monster.db and npc/")
    records = []
    races = {}
    for path in sorted((root / "mon").glob("*.mon")):
        text, digest = fields(path)
        race, name = number(text, "RaceNumber"), value(text, "Name")
        if race is not None and name:
            races[race] = (name.strip('"'), number(text, "Experience"),
                           path.relative_to(root).as_posix(), digest)
    for path in sorted((root / "npc").glob("*.npc")):
        text, digest = fields(path)
        name, home = value(text, "Name"), value(text, "Home")
        match = re.match(r"\[(\d+),(\d+),(\d+)\]", home or "")
        if not (name and match):
            continue
        x, y, z = map(int, match.groups())
        if z != ORIGIN[2] or abs(x - ORIGIN[0]) + abs(y - ORIGIN[1]) > args.radius:
            continue
        records.append(dict(knowledge_id="npc." + path.stem, domain="NPCs",
            subject=name.strip('"'), facts={"historical_home": [x, y, z]},
            coordinates=[x, y, z], source=path.relative_to(root).as_posix(),
            source_sha256=digest, source_version="Fusion32 selected local static archive",
            target_version="7.72", compatibility="VERIFIED_772",
            confidence="static_home_only", tags=["npc", "public_home"]))
    text, digest = fields(root / "dat/monster.db")
    for lineno, line in enumerate(text.splitlines(), 1):
        line = line.split("#", 1)[0]
        match = re.fullmatch(r"\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*", line)
        if not match:
            continue
        race, x, y, z, radius, amount, regen = map(int, match.groups())
        if z not in (ORIGIN[2], ORIGIN[2] + 1) or abs(x - ORIGIN[0]) + abs(y - ORIGIN[1]) > args.radius:
            continue
        if race not in races:
            continue
        name, experience, race_file, race_hash = races[race]
        records.append(dict(knowledge_id=f"monster_home.{lineno}", domain="monster_homes",
            subject=name, facts={"historical_home": [x, y, z],
            "home_radius": radius, "experience": experience,
            "live_occupancy": "UNKNOWN"}, coordinates=[x, y, z],
            source=f"dat/monster.db:{lineno}; {race_file}",
            source_sha256=f"{digest}; {race_hash}",
            source_version="Fusion32 selected local static archive",
            target_version="7.72", compatibility="VERIFIED_772",
            confidence="static_home_only", tags=["monster_home", "not_live_state"]))
    records.sort(key=lambda r: (r["domain"], r["knowledge_id"]))
    output = {"schema": "real33d.knowledge.local_772/1", "origin": list(ORIGIN),
              "radius": args.radius, "records": records}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, ensure_ascii=True, sort_keys=True,
                                      separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"local static knowledge records={len(records)} output={args.output}")


if __name__ == "__main__":
    main()
