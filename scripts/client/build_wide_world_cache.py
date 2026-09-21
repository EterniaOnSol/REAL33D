#!/usr/bin/env python3
"""Compile clean Fusion32 .sec files into a local WideWorld streaming cache."""
import argparse
import json
import re
from collections import Counter
from pathlib import Path
from audit_wide_world_sources import load_disguises

SECTOR = re.compile(r"^(\d+)-(\d+)-(\d+)\.sec$")
TOKEN = re.compile(r'\s*(?:\#[^\n]*|("(?:\\.|[^"\\])*")|(\d+)|([A-Za-z_][A-Za-z_0-9]*)|([{}:,=\-]))', re.S)

def tokenize(source):
    tokens = []
    at = 0
    while at < len(source):
        if not source[at:].strip():
            break
        match = TOKEN.match(source, at)
        if not match:
            raise ValueError(f"unknown syntax at byte {at}: {source[at:at+40]!r}")
        token = match.group(1) or match.group(2) or match.group(3) or match.group(4)
        if token is not None:
            tokens.append(token)
        at = match.end()
    return tokens

class Cursor:
    def __init__(self, tokens):
        self.tokens, self.at = tokens, 0
    def peek(self, offset=0):
        at = self.at + offset
        return self.tokens[at] if at < len(self.tokens) else None
    def pop(self):
        value = self.peek()
        if value is None:
            raise ValueError("unexpected end of sector")
        self.at += 1
        return value
    def expect(self, value):
        actual = self.pop()
        if actual != value:
            raise ValueError(f"expected {value!r}, found {actual!r} at token {self.at}")
    def number(self):
        token = self.pop()
        if not token.isdecimal():
            raise ValueError(f"expected number, found {token!r}")
        return int(token)

def objects(cursor):
    cursor.expect("{")
    result = []
    while cursor.peek() != "}":
        result.append(cursor.number())
        while cursor.peek() not in (",", "}"):
            attribute = cursor.pop().lower()
            cursor.expect("=")
            if attribute == "content":
                objects(cursor)  # nested inventory is not a map object
            else:
                value = cursor.pop()
                if not (value.isdecimal() or value.startswith('"')):
                    raise ValueError(f"invalid {attribute} value {value!r}")
        if cursor.peek() == ",":
            cursor.pop()
    cursor.expect("}")
    return result

def parse_sector(source, sx, sy, z, disguises):
    cursor = Cursor(tokenize(source))
    rows = []
    while cursor.peek() is not None:
        if cursor.peek() == ",":
            cursor.pop()
            continue
        ox = cursor.number()
        cursor.expect("-")
        oy = cursor.number()
        cursor.expect(":")
        if not 0 <= ox < 32 or not 0 <= oy < 32:
            raise ValueError(f"invalid tile offset {ox},{oy}")
        content_seen = False
        while cursor.peek() is not None and not (cursor.peek().isdecimal() and cursor.peek(1) == "-" and cursor.peek(2) is not None and cursor.peek(2).isdecimal() and cursor.peek(3) == ":"):
            if cursor.peek() == ",":
                cursor.pop()
                continue
            field = cursor.pop().lower()
            if field in ("refresh", "nologout", "protectionzone"):
                continue
            if field != "content" or content_seen:
                raise ValueError(f"unknown or repeated tile field {field!r}")
            cursor.expect("=")
            content_seen = True
            for stack, raw in enumerate(objects(cursor)):
                rows.append((sx * 32 + ox, sy * 32 + oy, z, stack, disguises.get(raw, raw), raw))
        if cursor.peek() == ",":
            cursor.pop()
    return rows

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--origmap", type=Path, required=True)
    ap.add_argument("--objects", type=Path, required=True)
    ap.add_argument("--reference-pack", type=Path, required=True)
    ap.add_argument("--catalog", type=Path, required=True)
    ap.add_argument("--assets", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--center-x", type=int, required=True)
    ap.add_argument("--center-y", type=int, required=True)
    ap.add_argument("--radius", type=int, default=128)
    ap.add_argument("--z-min", type=int, default=0)
    ap.add_argument("--z-max", type=int, default=7)
    args = ap.parse_args()
    disguises = load_disguises(args.objects)
    catalogue = json.loads(args.catalog.read_text(encoding="utf-8"))
    v08 = {int(e["item_id"]): e for e in catalogue["entries"]}
    references = args.reference_pack / "previews" / "item"
    args.out.mkdir(parents=True, exist_ok=True)
    report = {str(r): Counter() for r in (32, 64, 96, 128)}
    id_report = Counter()
    classification_by_id = {}
    sector_count = 0
    for path in sorted(args.origmap.glob("*.sec")):
        match = SECTOR.match(path.name)
        if not match:
            continue
        sx, sy, z = map(int, match.groups())
        if not args.z_min <= z <= args.z_max:
            continue
        if sx * 32 > args.center_x + args.radius or (sx + 1) * 32 - 1 < args.center_x - args.radius:
            continue
        if sy * 32 > args.center_y + args.radius or (sy + 1) * 32 - 1 < args.center_y - args.radius:
            continue
        try:
            rows = parse_sector(path.read_text(encoding="latin-1"), sx, sy, z, disguises)
        except ValueError as exc:
            raise ValueError(f"{path}: {exc}") from exc
        if not rows:
            continue
        with (args.out / f"{sx}-{sy}-{z}.wws").open("w", encoding="ascii") as handle:
            for x, y, floor, stack, visible, raw in rows:
                handle.write(f"{x} {y} {floor} {stack} {visible} {raw}\n")
                category = classification_by_id.get(visible)
                if category is None:
                    entry = v08.get(visible)
                    mesh = entry.get("mesh_path", "") if entry else ""
                    asset_file = args.assets / (mesh.split("/V08/", 1)[-1].split(".", 1)[0] + ".uasset")
                    if entry and entry.get("import_status") == "IMPORTED_OK" and mesh and asset_file.is_file():
                        category = "V08_RESOLVED"
                    elif (references / f"{visible}.png").is_file():
                        category = "CLASSIC_SPRITE_FALLBACK"
                    else:
                        category = "MISSING_PHYSICAL_ASSET"
                    classification_by_id[visible] = category
                id_report[(visible, category)] += 1
                for radius, counts in report.items():
                    radius = int(radius)
                    if abs(x - args.center_x) <= radius and abs(y - args.center_y) <= radius:
                        counts[category] += 1
        sector_count += 1
    categories = ("V08_RESOLVED", "CLASSIC_SPRITE_FALLBACK",
                  "MISSING_PHYSICAL_ASSET")
    result = {"schema": "real33d.wide-world-cache.v1", "sector_size": 32,
              "sectors": sector_count, "center": [args.center_x, args.center_y],
              "radii": {radius: {category: counts[category]
                                  for category in categories}
                        for radius, counts in report.items()},
              "type_469": {
                  "IDENTITY": "KNOWN",
                  "V08_PHYSICAL_ASSET": "MISSING",
                  "WIDE_WORLD_RENDER": "CLASSIC_SPRITE_FALLBACK",
                  "occurrences": {category: id_report[(469, category)]
                                  for category in categories}},
              "classification": "static object occurrences; all floors; exact XY square radius",
              "source": "local clean Fusion32 origmap; visible ID from objects.srv DisguiseTarget",
              "reference_pack": "local gitignored classic 7.72 previews"}
    (args.out / "report.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()
