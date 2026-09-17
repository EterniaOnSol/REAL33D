#!/usr/bin/env python3
"""Builds the REAL33D artist reference pack from the 7.72 client visual data.

Reads the appearance data the operator supplies locally, crosses it with the
manifests produced by VISUAL-ASSET-MASTER-INVENTORY-001, revalidates the visual
groups against real sprite evidence, and writes a navigable catalogue with
previews.

Everything it writes is derived from a client artifact whose provenance is
recorded as UNKNOWN, so the output directory is gitignored. Rebuild it after a
clone rather than expecting it in the repository.

Usage:
  build_reference_pack.py --dat <Tibia.dat> --spr <Tibia.spr> \
                          --manifests visual/manifests \
                          --out visual/reference_pack [--only-p0] [--scale 3]
"""

import argparse
import csv
import html
import json
import os
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import pngwrite                       # noqa: E402
import tibia772 as t                  # noqa: E402

DEMONSTRATED = "DEMONSTRATED"
INFERRED = "INFERRED"
UNRESOLVED = "UNRESOLVED"

# Outfits a player can choose. Source: reference/game/src/sending.cc::SendOutfit,
# which derives the range from the player's sex and premium right: 128..131 for
# male and 136..139 for female, each extended by three with premium.
PLAYER_OUTFITS = set(range(128, 135)) | set(range(136, 143))


def read_csv(path):
    if not os.path.exists(path):
        return []
    with open(path, newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


# --------------------------------------------------------------------- render

def compose(appearance, sprites, frame, layer, px, py, pz):
    """Composes one w x h tile block into a pixel buffer.

    Sprite order follows the file's own indexing: the innermost axis is width,
    then height, then layer, then patternX, patternY, patternZ, then frame.
    A block is drawn with its origin at the bottom right, which is how the
    client stacks multi-field things upward and leftward from their anchor.
    """
    width = appearance.width * t.SpriteFile.WIDTH
    height = appearance.height * t.SpriteFile.HEIGHT
    canvas = [(0, 0, 0, 0)] * (width * height)

    for tile_y in range(appearance.height):
        for tile_x in range(appearance.width):
            index = (((((frame * appearance.pattern_z + pz)
                        * appearance.pattern_y + py)
                       * appearance.pattern_x + px)
                      * appearance.layers + layer)
                     * appearance.height + tile_y) * appearance.width + tile_x
            if index >= len(appearance.sprite_ids):
                continue
            sprite_id = appearance.sprite_ids[index]
            if not sprite_id:
                continue
            pixels = sprites.decode(sprite_id)
            if pixels is None:
                continue
            # Tile (0,0) is the far corner; the anchor field is the last one.
            origin_x = (appearance.width - 1 - tile_x) * t.SpriteFile.WIDTH
            origin_y = (appearance.height - 1 - tile_y) * t.SpriteFile.HEIGHT
            for row in range(t.SpriteFile.HEIGHT):
                base = (origin_y + row) * width + origin_x
                source = row * t.SpriteFile.WIDTH
                for column in range(t.SpriteFile.WIDTH):
                    pixel = pixels[source + column]
                    if pixel[3]:
                        canvas[base + column] = pixel
    return canvas, width, height


def sheet(appearance, sprites, scale):
    """Lays every direction, layer and frame out as a grid."""
    columns = []
    for pz in range(appearance.pattern_z):
        for py in range(appearance.pattern_y):
            for px in range(appearance.pattern_x):
                for layer in range(appearance.layers):
                    columns.append((px, py, pz, layer))
    rows = appearance.frames
    if not columns or not rows:
        return None

    cell_w = appearance.width * t.SpriteFile.WIDTH
    cell_h = appearance.height * t.SpriteFile.HEIGHT
    pad = 2
    total_w = len(columns) * (cell_w + pad) + pad
    total_h = rows * (cell_h + pad) + pad
    canvas = [(24, 24, 28, 255)] * (total_w * total_h)

    for row_index in range(rows):
        for column_index, (px, py, pz, layer) in enumerate(columns):
            cell, width, height = compose(appearance, sprites, row_index,
                                          layer, px, py, pz)
            ox = pad + column_index * (cell_w + pad)
            oy = pad + row_index * (cell_h + pad)
            for y in range(height):
                base = (oy + y) * total_w + ox
                source = y * width
                for x in range(width):
                    pixel = cell[source + x]
                    if pixel[3]:
                        canvas[base + x] = pixel
    return pngwrite.scale_nearest(canvas, total_w, total_h, scale)


# ------------------------------------------------------------------- families

def revalidate_families(appearances):
    """Groups client ids by the sprites they actually use.

    Identical sprite sets are the same picture and are DEMONSTRATED. Sets that
    overlap without matching share artwork, which is what modular pieces and
    recolour families look like.
    """
    by_signature = defaultdict(list)
    for appearance in appearances:
        used = tuple(sorted({s for s in appearance.sprite_ids if s}))
        if not used:
            continue
        by_signature[used].append(appearance)

    sprite_owners = defaultdict(set)
    for signature, members in by_signature.items():
        for sprite_id in signature:
            sprite_owners[sprite_id].add(signature)

    families = []
    for signature, members in by_signature.items():
        shares_with = set()
        for sprite_id in signature:
            shares_with |= sprite_owners[sprite_id]
        shares_with.discard(signature)
        families.append({
            "signature_size": len(signature),
            "members": members,
            "identical_count": len(members),
            "shares_sprites_with": len(shares_with),
        })
    return families, by_signature


# ----------------------------------------------------------------------- main

def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dat", required=True)
    parser.add_argument("--spr", required=True)
    parser.add_argument("--manifests", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--scale", type=int, default=3)
    parser.add_argument("--only-p0", action="store_true")
    parser.add_argument("--no-previews", action="store_true")
    args = parser.parse_args()

    for path in (args.dat, args.spr):
        if not os.path.exists(path):
            print("MISSING VISUAL INPUT: %s" % path, file=sys.stderr)
            print("Place the authorised Tibia 7.72 client data there and rerun.",
                  file=sys.stderr)
            return 2

    appearance_file = t.AppearanceFile.read(args.dat)
    sprites = t.SpriteFile.read(args.spr)

    objects = {int(row["id"]): row for row in
               read_csv(os.path.join(args.manifests, "objects.csv"))}
    creatures = read_csv(os.path.join(args.manifests, "creatures.csv"))
    npcs = read_csv(os.path.join(args.manifests, "npcs.csv"))
    effects = {int(row["effect_id"]): row for row in
               read_csv(os.path.join(args.manifests, "effects.csv"))}

    server_flags = {cid: set(f for f in row["flags"].split(",") if f)
                    for cid, row in objects.items()}
    report = appearance_file.validate(sprite_count=sprites.count,
                                      server_flags=server_flags)

    os.makedirs(args.out, exist_ok=True)
    manifests_out = os.path.join(args.out, "manifests")
    previews_out = os.path.join(args.out, "previews")
    os.makedirs(manifests_out, exist_ok=True)

    # ---- appearance manifest, crossed with the master inventory -----------
    outfit_users = defaultdict(list)
    for row in creatures:
        if row["outfit_id"] and row["outfit_id"] != "0":
            outfit_users[int(row["outfit_id"])].append("mon:%s" % row["name"])
    for row in npcs:
        if row["outfit_id"] and row["outfit_id"] != "0":
            outfit_users[int(row["outfit_id"])].append("npc:%s" % row["name"])

    rows = []
    unresolved = []
    for appearance in appearance_file.appearances:
        kind, cid = appearance.kind, appearance.client_id
        name = category = subcategory = priority = group = ""
        tracker_id = ""
        provenance = ""
        if kind == t.KIND_ITEM:
            tracker_id = "obj:%d" % cid
            row = objects.get(cid)
            if row:
                name, category = row["name"], row["category"]
                subcategory = row["subcategory"] or row["art_class"]
                priority, group = row["priority"], row["visual_group"]
                provenance = row["source"]
            else:
                unresolved.append(("item", cid, "no server object type"))
        elif kind == t.KIND_OUTFIT:
            tracker_id = "outfit:%d" % cid
            users = outfit_users.get(cid, [])
            name = "outfit %d" % cid
            category = "outfit"
            priority = "P2"
            if cid in PLAYER_OUTFITS:
                subcategory = "player_selectable"
                provenance = ("reference/game/src/sending.cc::SendOutfit "
                              "selectable range")
            elif users:
                subcategory = "creature_or_npc"
                provenance = "./mon/*.mon and ./npc/*.npc Outfit fields"
            else:
                subcategory = "unreferenced"
                provenance = ""
                unresolved.append(("outfit", cid,
                                   "shipped by the client but referenced by no "
                                   "monster, npc or player-selectable range"))
        elif kind == t.KIND_EFFECT:
            tracker_id = "fx:%d" % cid
            row = effects.get(cid)
            name = row["name"] if row else ""
            category, subcategory, priority = "effect", "graphical_effect", "P2"
            provenance = row["source"] if row else ""
            if not row:
                unresolved.append(("effect", cid, "no EffectType enumerator"))
        else:
            tracker_id = "msl:%d" % cid
            name = "missile %d" % cid
            category, subcategory, priority = "projectile", "missile", "P2"
            provenance = "./dat/objects.srv attributes"

        used = sorted({s for s in appearance.sprite_ids if s})
        missing = [s for s in used if not sprites.has(s)]
        rows.append({
            "tracker_id": tracker_id,
            "kind": kind,
            "client_id": cid,
            "name": name,
            "category": category,
            "subcategory": subcategory,
            "priority": priority,
            "visual_group": group,
            "width": appearance.width,
            "height": appearance.height,
            "exact_size": appearance.exact_size if appearance.exact_size is not None else "",
            "layers": appearance.layers,
            "pattern_x": appearance.pattern_x,
            "pattern_y": appearance.pattern_y,
            "pattern_z": appearance.pattern_z,
            "frames": appearance.frames,
            "sprite_count": len(appearance.sprite_ids),
            "distinct_sprites": len(used),
            "sprite_ids": ";".join(str(s) for s in used[:64]),
            "sprite_ids_truncated": "yes" if len(used) > 64 else "no",
            "missing_sprites": ";".join(str(s) for s in missing),
            "options": ";".join("0x%02X" % o for o in sorted(appearance.options)),
            "appearance_confidence": DEMONSTRATED,
            "appearance_source": "client Tibia.dat, validated against Tibia.spr",
            "inventory_source": provenance,
        })

    with open(os.path.join(manifests_out, "appearances.csv"), "w", newline="",
              encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    # ---- families ----------------------------------------------------------
    families, by_signature = revalidate_families(appearance_file.appearances)
    family_rows = []
    for family in sorted(families, key=lambda f: -f["identical_count"]):
        members = family["members"]
        family_rows.append({
            "member_count": family["identical_count"],
            "members": ";".join("%s:%d" % (m.kind, m.client_id) for m in members[:40]),
            "truncated": "yes" if len(members) > 40 else "no",
            "distinct_sprites": family["signature_size"],
            "shares_sprites_with_families": family["shares_sprites_with"],
            "relationship": (DEMONSTRATED + ":identical_sprite_set"
                             if family["identical_count"] > 1 else "unique"),
        })
    with open(os.path.join(manifests_out, "visual_families.csv"), "w", newline="",
              encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(family_rows[0].keys()))
        writer.writeheader()
        writer.writerows(family_rows)

    # ---- previews ----------------------------------------------------------
    selected = rows
    if args.only_p0:
        selected = [row for row in rows if row["priority"] == "P0"]
    generated = 0
    failed = 0
    if not args.no_previews:
        for row in selected:
            appearance = appearance_file.by_kind[row["kind"]][row["client_id"]]
            directory = os.path.join(previews_out, row["kind"])
            os.makedirs(directory, exist_ok=True)
            path = os.path.join(directory, "%d.png" % row["client_id"])
            try:
                result = sheet(appearance, sprites, args.scale)
                if result is None:
                    failed += 1
                    continue
                pixels, width, height = result
                pngwrite.write_rgba(path, width, height, pixels)
                generated += 1
            except Exception as error:               # noqa: BLE001
                failed += 1
                if failed <= 3:
                    print("preview failed for %s %d: %s"
                          % (row["kind"], row["client_id"], error), file=sys.stderr)

    # ---- catalogue ---------------------------------------------------------
    write_catalogue(os.path.join(args.out, "catalogue.html"), rows, "REAL33D reference pack")
    p0_rows = [row for row in rows if row["priority"] == "P0"]
    write_catalogue(os.path.join(args.out, "p0_rookgaard.html"), p0_rows,
                    "REAL33D Rookgaard P0 queue")

    # ---- summary -----------------------------------------------------------
    by_kind = Counter(row["kind"] for row in rows)
    resolved_names = sum(1 for row in rows if row["name"])
    multi = sum(1 for f in families if f["identical_count"] > 1)
    summary = {
        "dat_signature": "0x%08X" % appearance_file.signature,
        "spr_signature": "0x%08X" % sprites.signature,
        "header_counts": appearance_file.counts,
        "validation": report,
        "appearances_decoded": len(rows),
        "by_kind": dict(by_kind),
        "sprites_in_file": sprites.count,
        "sprites_decodable": sum(1 for i in range(1, sprites.count + 1)
                                 if sprites.has(i)),
        "appearances_with_inventory_name": resolved_names,
        "appearances_without_inventory_name": len(rows) - resolved_names,
        "unresolved": [{"kind": k, "client_id": c, "reason": r}
                       for k, c, r in unresolved],
        "visual_families": {
            "total": len(families),
            "identical_sprite_set_families": multi,
            "members_in_identical_families": sum(
                f["identical_count"] for f in families if f["identical_count"] > 1),
        },
        "p0_rookgaard_appearances": len(p0_rows),
        "previews_generated": generated,
        "previews_failed": failed,
    }
    with open(os.path.join(args.out, "summary.json"), "w", encoding="utf-8") as handle:
        json.dump(summary, handle, indent=2)
        handle.write("\n")

    print("appearances decoded   %d  %s" % (len(rows), dict(by_kind)))
    print("sprites decodable     %d of %d" % (summary["sprites_decodable"], sprites.count))
    print("named from inventory  %d (unnamed %d)"
          % (resolved_names, len(rows) - resolved_names))
    print("visual families       %d (identical-sprite families %d covering %d ids)"
          % (len(families), multi, summary["visual_families"]["members_in_identical_families"]))
    print("p0 rookgaard          %d" % len(p0_rows))
    print("previews              %d generated, %d failed" % (generated, failed))
    print("unresolved            %d" % len(unresolved))
    print("catalogue             %s" % os.path.join(args.out, "catalogue.html"))
    return 0


def write_catalogue(path, rows, title):
    order = {"P0": 0, "P1": 1, "P2": 2, "UNPRIORITIZED": 3, "": 4}
    rows = sorted(rows, key=lambda r: (order.get(r["priority"], 4), r["kind"],
                                       r["category"], r["client_id"]))
    parts = ["""<!doctype html><meta charset="utf-8"><title>%s</title>
<style>
 body{background:#15161a;color:#e7e7ea;font:14px/1.5 system-ui,sans-serif;margin:0}
 header{position:sticky;top:0;background:#15161a;border-bottom:1px solid #303038;padding:12px 16px;z-index:2}
 h1{font-size:16px;margin:0 0 8px}
 input,select{background:#232430;color:#e7e7ea;border:1px solid #3a3b48;border-radius:6px;padding:6px 8px;font:inherit}
 .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:12px;padding:16px}
 .card{background:#1d1e26;border:1px solid #2e2f3a;border-radius:10px;padding:10px;overflow:hidden}
 .card h2{font-size:13px;margin:0 0 6px;word-break:break-word}
 .img{background:#0e0f13;border-radius:6px;padding:6px;overflow:auto;max-height:230px}
 img{image-rendering:pixelated;display:block}
 dl{display:grid;grid-template-columns:auto 1fr;gap:2px 8px;margin:8px 0 0;font-size:12px}
 dt{color:#8b8c99}dd{margin:0;word-break:break-word}
 .p0{border-left:3px solid #4ade80}.p1{border-left:3px solid #facc15}
 .p2{border-left:3px solid #64748b}.pu{border-left:3px solid #3f3f46}
 .muted{color:#8b8c99}
</style>
<header><h1>%s</h1>
<input id="q" placeholder="filter by name, id, category, group" size="46">
<select id="p"><option value="">all priorities</option><option>P0</option>
<option>P1</option><option>P2</option><option>UNPRIORITIZED</option></select>
<span class="muted" id="n"></span></header><div class="grid" id="g">""" % (
        html.escape(title), html.escape(title))]

    klass = {"P0": "p0", "P1": "p1", "P2": "p2"}
    for row in rows:
        label = row["name"] or "%s %d" % (row["kind"], row["client_id"])
        preview = "previews/%s/%d.png" % (row["kind"], row["client_id"])
        search = " ".join([label, row["tracker_id"], row["category"],
                           row["subcategory"], row["visual_group"]]).lower()
        parts.append(
            '<div class="card %s" data-s="%s" data-p="%s">'
            '<h2>%s</h2><div class="img"><img loading="lazy" src="%s" alt=""></div>'
            '<dl>'
            '<dt>id</dt><dd>%s</dd>'
            '<dt>category</dt><dd>%s / %s</dd>'
            '<dt>priority</dt><dd>%s</dd>'
            '<dt>group</dt><dd>%s</dd>'
            '<dt>geometry</dt><dd>%sx%s, %s layer(s), pattern %sx%sx%s, %s frame(s)</dd>'
            '<dt>sprites</dt><dd>%s distinct</dd>'
            '<dt>provenance</dt><dd class="muted">%s</dd>'
            '</dl></div>' % (
                klass.get(row["priority"], "pu"), html.escape(search),
                html.escape(row["priority"]), html.escape(label), html.escape(preview),
                html.escape(row["tracker_id"]),
                html.escape(row["category"]), html.escape(row["subcategory"]),
                html.escape(row["priority"]), html.escape(row["visual_group"] or "-"),
                row["width"], row["height"], row["layers"],
                row["pattern_x"], row["pattern_y"], row["pattern_z"], row["frames"],
                row["distinct_sprites"],
                html.escape(row["inventory_source"] or row["appearance_source"])))

    parts.append("""</div><script>
const g=document.getElementById('g'),q=document.getElementById('q'),
      p=document.getElementById('p'),n=document.getElementById('n');
const cards=[...g.children];
function f(){const s=q.value.toLowerCase().trim(),pr=p.value;let c=0;
 for(const el of cards){const ok=(!s||el.dataset.s.includes(s))&&(!pr||el.dataset.p===pr);
  el.style.display=ok?'':'none';if(ok)c++;}
 n.textContent=c+' of '+cards.length;}
q.addEventListener('input',f);p.addEventListener('change',f);f();
</script>""")
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("".join(parts))


if __name__ == "__main__":
    sys.exit(main())
