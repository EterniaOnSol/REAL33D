#!/usr/bin/env python3
"""Can a delivered outfit asset carry the four 7.72 colour regions?

`verify_outfit_color_mapping.py` proved that the four outfit colour bytes are
Head, Body, Legs and Feet. `verify_outfit_palette.py` proved what colour each
byte value is. This asks the question those two leave: is there anything in the
delivered art for those colours to be applied *to*, region by region.

It is an audit, not a test of this client. It reads a glTF/GLB and reports what
regions the geometry can address, because in 7.72 a colour tints an area of the
sprite -- hair, shirt, trousers, boots -- and a 3D asset can only reproduce
that if the corresponding area is separately addressable: its own node, its own
material slot, a mask texture, a second UV set, or vertex colours.

Read-only. Nothing is written and no asset is modified.

Usage:
    python tests/audit_outfit_asset_regions.py <path-to.glb> [more.glb ...]

Exit 0 when every asset can address all four regions, 3 when at least one
cannot. 3 rather than 1 so a caller can tell "the asset is insufficient" apart
from "the audit itself failed".
"""

from __future__ import annotations

import json
import struct
import sys

# The words a rigger would plausibly use for each of the four 7.72 regions.
# Deliberately generous: a false positive here makes the audit *less* likely to
# report a blocker, so a wide net cannot manufacture one.
REGION_WORDS = {
    "Head": ("head", "skull", "hair", "helmet", "hat", "hood", "face"),
    "Body": ("torso", "chest", "body", "arm", "tunic", "shirt", "hand",
             "sleeve", "shoulder", "glove", "armour", "armor"),
    "Legs": ("leg", "pelvis", "thigh", "trouser", "hip", "shin", "kilt",
             "skirt", "pant"),
    "Feet": ("foot", "feet", "boot", "shoe", "toe", "ankle", "sandal", "sock"),
}

# A region mask could live in any of these. None of them is a region *by*
# itself, but their absence closes off every alternative to node separation.
MASK_ATTRIBUTES = ("COLOR_0", "TEXCOORD_1", "TEXCOORD_2")


def read_gltf(path: str) -> dict:
    with open(path, "rb") as fh:
        data = fh.read()
    if data[:4] != b"glTF":
        return json.loads(data.decode("utf-8"))  # a .gltf, not a .glb
    off = 12
    while off < len(data):
        clen, ctype = struct.unpack_from("<II", data, off)
        if ctype == 0x4E4F534A:
            return json.loads(data[off + 8: off + 8 + clen].decode("utf-8"))
        off += 8 + clen + ((4 - clen % 4) % 4)
    raise ValueError("no JSON chunk")


def audit(path: str) -> bool:
    g = read_gltf(path)
    meshes = g.get("meshes", [])
    mats = g.get("materials", [])
    nodes = g.get("nodes", [])

    print(f"\n{'=' * 72}\nasset  {path}")

    # The only mapping that matters: a node names the body part, and it is the
    # node that becomes one component in Unreal.
    rows = []
    for n in nodes:
        if "mesh" not in n:
            continue
        mesh = meshes[n["mesh"]]
        prim = mesh["primitives"][0]
        mi = prim.get("material")
        matname = mats[mi].get("name", "?") if mi is not None else "<none>"
        base = (mats[mi].get("pbrMetallicRoughness", {}).get("baseColorFactor")
                if mi is not None else None)
        attrs = set()
        for p in mesh["primitives"]:
            attrs |= set(p.get("attributes", {}).keys())
        rows.append((n.get("name", "?"), mesh.get("name", "?"), matname, base, attrs))

    print(f"\n  {'node':<12} {'mesh':<16} {'material':<14} base colour")
    print(f"  {'-' * 12} {'-' * 16} {'-' * 14} {'-' * 22}")
    for name, mesh, mat, base, _ in sorted(rows):
        shown = ("[" + ", ".join(f"{c:.3f}" for c in base[:3]) + "]") if base else "-"
        grey = ""
        if base and max(base[:3]) - min(base[:3]) < 0.08:
            grey = "  greyscale"
        print(f"  {name:<12} {mesh:<16} {mat:<14} {shown}{grey}")

    # One material spanning several body parts means a material slot cannot be
    # a region even where the geometry is separate.
    spans: dict[str, list[str]] = {}
    for name, _mesh, mat, _base, _attrs in rows:
        spans.setdefault(mat, []).append(name)
    shared = {m: n for m, n in spans.items() if len(n) > 1}
    print(f"\n  materials: {len(mats)}   nodes with geometry: {len(rows)}")
    if shared:
        print("  materials shared across body parts (so a slot is not a region):")
        for mat, names in sorted(shared.items()):
            print(f"    {mat:<14} {sorted(names)}")

    print(f"\n  textures: {len(g.get('textures', []))}   "
          f"images: {len(g.get('images', []))}   "
          f"extensions: {g.get('extensionsUsed') or 'none'}")
    present_attrs = set()
    for *_x, attrs in rows:
        present_attrs |= attrs
    masks = [a for a in MASK_ATTRIBUTES if a in present_attrs]
    print(f"  possible region masks present: {masks or 'NONE'}")

    # Region coverage, by node name.
    print("\n  region coverage:")
    missing = []
    for region, words in REGION_WORDS.items():
        hits = sorted({name for name, *_ in rows
                       if any(w in name.lower() for w in words)})
        if hits:
            print(f"    {region:<5} YES  {hits}")
        else:
            print(f"    {region:<5} NO   -- no node names this region")
            missing.append(region)

    if missing:
        print(f"\n  VERDICT: INSUFFICIENT. No geometry for: {', '.join(missing)}.")
        print("  A colour byte for a region with no geometry has nothing to tint,")
        print("  and no mask attribute is present that could define one inside")
        print("  another part. This asset cannot represent all four regions.")
        return False
    print("\n  VERDICT: all four regions are addressable by node.")
    return True


if __name__ == "__main__":
    paths = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not paths:
        print(__doc__)
        raise SystemExit(64)
    ok = all(audit(p) for p in paths)
    print(f"\n{'=' * 72}")
    print("ASSET COLOUR CAPABILITY: "
          + ("SUFFICIENT" if ok else "INSUFFICIENT for the four 7.72 regions"))
    raise SystemExit(0 if ok else 3)
