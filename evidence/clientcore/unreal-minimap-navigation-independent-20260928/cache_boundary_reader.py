"""Read only a legitimate local minimap cache; emit compact boundary evidence."""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("cache", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--query", action="append", default=[], help="x,y,z")
args = parser.parse_args()
if args.output.resolve() == args.cache.resolve():
    raise SystemExit("evidence output must not replace the cache")
if args.cache.stat().st_size > 32 * 1024 * 1024:
    raise SystemExit("cache too large")
raw = args.cache.read_bytes()
if len(raw) > 32 * 1024 * 1024:
    raise SystemExit("cache too large")
lines = raw.decode("utf-8-sig").splitlines()
magic, scope, declared = lines[0].split()
if magic != "REAL33D_MINIMAP_2":
    raise SystemExit("unexpected cache version")
cells = {}
for line in lines[1:]:
    x, y, z, ground, obstacle, color = map(int, line.split())
    key = (x, y, z)
    if key in cells or not (0 <= x <= 65535 and 0 <= y <= 65535 and 0 <= z <= 15):
        raise SystemExit("invalid/duplicate coordinate")
    cells[key] = (ground, obstacle, color)
if len(cells) != int(declared):
    raise SystemExit("cache count mismatch")
floors = {}
for z in sorted({p[2] for p in cells}):
    points = [p for p in cells if p[2] == z]
    floors[str(z)] = dict(count=len(points), min_x=min(p[0] for p in points),
                         max_x=max(p[0] for p in points), min_y=min(p[1] for p in points),
                         max_y=max(p[1] for p in points))
queries = []
for text in args.query:
    p = tuple(map(int, text.split(",")))
    if len(p) != 3:
        raise SystemExit("query must be x,y,z")
    queries.append(dict(position=p, known=p in cells,
                        terrain_hint=cells.get(p), current_walkability="NOT_ASSERTED"))
result = dict(schema="real33d.minimap.independent.boundary.v1", source="legitimate client-owned cache",
              cache_sha256=hashlib.sha256(raw).hexdigest(), count=len(cells),
              floors=floors, queries=queries, network_commands=0, cache_modified=False,
              limitation="Bounds summarize observed coordinates, not a filled/traversable rectangle. Cache may lag memory; compare the live known count before accepting the checkpoint.")
args.output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
print(json.dumps(result))
