#!/usr/bin/env python3
"""Regenerates the REAL33D visual master inventory from Fusion32 source truth.

Every row this tool emits carries the dataset it came from. Nothing here
invents an id, a name or a category: categories are derived from the object
type flags declared in reference/game/src/enums.hh, and any classification that
leans on an object's own Name string is marked INFERRED rather than DEMONSTRATED.

Datasets consumed, all from the historical runtime archive unless noted:

  ./dat/objects.srv   every declared object type, its name, flags and attributes
  ./dat/map.dat       map bounds, named marks, newbie and veteran start
  ./dat/monster.db    monster spawn points
  ./mon/*.mon         monster races: name, outfit, corpse, blood
  ./npc/*.npc         npc identities: name, sex, race, outfit, home
  ./origmap/*.sec     which type ids actually occur in the world, and where
  reference/game/src/enums.hh   graphical effect ids and the FLAG table

Usage:
  extract_visual_inventory.py --archive tibia-game.tarball.tar.gz \
                              --source reference/game/src \
                              --out visual/manifests
"""

import argparse
import csv
import json
import os
import re
import sys
import tarfile
from collections import Counter, defaultdict

SCHEMA_VERSION = "1.0.0"

# --------------------------------------------------------------------------
# Confidence vocabulary, used verbatim in every manifest.
DEMONSTRATED = "DEMONSTRATED"   # read directly out of a dataset field
INFERRED = "INFERRED"           # derived from dataset evidence, evidence recorded
UNRESOLVED = "UNRESOLVED"       # the dataset needed is not available here

BRACES = re.compile(r"\{(.*)\}")
COORD = re.compile(r"\[\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\]")
MARK = re.compile(r'Mark\s*=\s*\(\s*"([^"]+)"\s*,\s*\[(\d+),(\d+),(\d+)\]\s*\)')
OUTFIT = re.compile(r"Outfit\s*=\s*\(\s*(\d+)\s*(?:,\s*([0-9\-]+))?\s*\)")
SECTOR_NAME = re.compile(r"^(\d+)-(\d+)-(\d+)\.sec$")
CONTENT = re.compile(r"Content=\{([^}]*)\}")

SECTOR_SIZE = 32


def strip_comment(line):
    out = []
    in_string = False
    for ch in line:
        if ch == '"':
            in_string = not in_string
        elif ch == "#" and not in_string:
            break
        out.append(ch)
    return "".join(out)


# --------------------------------------------------------------------------
# objects.srv

def parse_objects(text):
    """TypeID / Name / Description / Flags / Attributes records."""
    types = {}
    current = None
    for raw in text.splitlines():
        line = strip_comment(raw).strip()
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        if key == "TypeID":
            current = int(value)
            types[current] = {"id": current, "name": "", "description": "",
                              "flags": set(), "attrs": {}}
        elif current is None:
            continue
        elif key == "Name":
            types[current]["name"] = value.strip('"')
        elif key == "Description":
            types[current]["description"] = value.strip('"')
        elif key == "Flags":
            match = BRACES.search(value)
            if match:
                types[current]["flags"] = {
                    item.strip() for item in match.group(1).split(",") if item.strip()
                }
        elif key == "Attributes":
            match = BRACES.search(value)
            if match:
                for pair in match.group(1).split(","):
                    if "=" in pair:
                        name, raw_value = pair.split("=", 1)
                        types[current]["attrs"][name.strip()] = raw_value.strip()
    return types


# --------------------------------------------------------------------------
# Category derivation. Primary category comes from flags only, so it is
# DEMONSTRATED. Subcategory may lean on the object's own Name, which is still
# dataset content but is a weaker signal, so those rows are marked INFERRED.

# Flag names are written in objects.srv exactly as spelled here. They are the
# same set as enum FLAG in reference/game/src/enums.hh, which the loader in
# reference/game/src/objects.cc::LoadObjects resolves by name.
#
# Ordered: the first matching rule wins, most specific first, mirroring how the
# server itself tests priority flags in map.cc::GetObjectPriority and the
# wire-relevant ones in sending.cc::SendItem.
FLAG_CATEGORY_RULES = [
    ("Corpse", "creature_remains", "corpse"),
    ("MagicField", "effect_object", "magic_field"),
    ("KeyDoor", "structure", "door"),
    ("NameDoor", "structure", "door"),
    ("LevelDoor", "structure", "door"),
    ("QuestDoor", "structure", "door"),
    ("Bed", "furniture", "bed"),
    ("Chest", "container", "chest"),
    ("Weapon", "equipment", "weapon"),
    ("Bow", "equipment", "weapon_bow"),
    ("Throw", "equipment", "weapon_throw"),
    ("Wand", "equipment", "weapon_wand"),
    ("Ammo", "equipment", "ammunition"),
    ("Shield", "equipment", "shield"),
    ("Armor", "equipment", "armor"),
    ("Clothes", "equipment", "clothing"),
    ("Rune", "usable", "rune"),
    ("Food", "usable", "food"),
    ("Key", "usable", "key"),
    ("LiquidContainer", "usable", "liquid_container"),
    ("LiquidPool", "effect_object", "liquid_pool"),
    ("LiquidSource", "usable", "liquid_source"),
    ("RopeSpot", "traversal", "rope_spot"),
    ("TeleportAbsolute", "traversal", "teleport"),
    ("TeleportRelative", "traversal", "level_change"),
    ("Text", "readable", "text"),
    ("Write", "readable", "writable"),
    ("WriteOnce", "readable", "writable_once"),
    ("Information", "readable", "information"),
    # Structural priority classes come last: a great many objects also carry
    # one of these, so a more specific flag above should win first.
    ("Bank", "terrain", "ground"),
    ("Clip", "terrain", "border"),
    ("Container", "container", "container"),
    ("Light", "decoration", "light_source"),
    ("Top", "structure", "top_overlay"),
    ("Bottom", "decoration", "bottom_overlay"),
]

USABLE_FLAGS = {"UseEvent", "MultiUse", "DistUse", "ChangeUse", "ForceUse"}
HANG_FLAGS = {"Hang", "HookSouth", "HookEast"}

# Name keywords only refine a subcategory that flags cannot separate, such as
# telling a wall from a tree when both are merely unpassable and unmovable.
NAME_SUBCATEGORY = [
    ("vegetation", ("tree", "bush", "shrub", "flower", "grass", "fern", "cactus",
                    "mushroom", "palm", "reed", "vine", "wheat", "corn", "seaweed",
                    "lily", "rose", "branch", "leaf", "leaves", "log", "stump")),
    ("wall", ("wall", "fence", "pillar", "column", "railing", "banister", "gate")),
    ("roof", ("roof", "shingle", "thatch")),
    ("stairs", ("stair", "ladder", "ramp", "step")),
    ("water", ("water", "sea", "lake", "river", "swamp", "puddle")),
    ("furniture", ("table", "chair", "stool", "bench", "shelf", "cabinet", "counter",
                   "desk", "throne", "sofa", "bed", "cupboard", "drawer", "barrel",
                   "crate", "box", "basket", "bucket", "vase", "pot", "urn")),
    ("sign", ("sign", "signpost", "plaque")),
    ("statue", ("statue", "sculpture", "monument", "idol", "obelisk")),
    ("rock", ("rock", "stone", "boulder", "pebble", "ore", "crystal")),
    ("bone", ("bone", "skull", "skeleton", "rib", "spine")),
]


def categorize(entry):
    """Behaviour class, taken from flags only, so always DEMONSTRATED when a
    flag matches. Returns (category, subcategory, confidence, evidence)."""
    flags = entry["flags"]
    for flag, category, subcategory in FLAG_CATEGORY_RULES:
        if flag in flags:
            return category, subcategory, DEMONSTRATED, "flag:" + flag
    if flags & USABLE_FLAGS:
        return "usable", "interactable", DEMONSTRATED, "flag:use"
    if flags & HANG_FLAGS:
        return "decoration", "hangable", DEMONSTRATED, "flag:hang"
    if "Unpass" in flags and "Unmove" in flags:
        return "structure", "obstacle", DEMONSTRATED, "flag:Unpass+Unmove"
    if "Take" in flags:
        return "item", "portable", DEMONSTRATED, "flag:Take"
    if not flags and not entry["name"]:
        return "unclassified", "", UNRESOLVED, "no flags, no name"
    return "item", "unspecified", INFERRED, "no categorising flag"


def art_class(entry):
    """What the thing depicts, which is what decides whether it needs its own
    mesh. Flags cannot separate a tree from a wall when both are merely
    unpassable, so this axis reads the object's own Name and is always
    INFERRED. Kept independent of categorize() so neither signal is lost.

    Returns (art_class, confidence, evidence)."""
    name = entry["name"].lower()
    if name:
        for label, keywords in NAME_SUBCATEGORY:
            for keyword in keywords:
                if keyword in name:
                    return label, INFERRED, "name:" + keyword
        return "unclassified_named", UNRESOLVED, "name matched no art keyword"
    return "unnamed", UNRESOLVED, "object type has no name"


# --------------------------------------------------------------------------
# Visual grouping. Three identities are kept apart on purpose:
#   logical_id     the Fusion32 type id
#   visual_group   ids expected to share one produced asset
#   asset_id       filled in by production, never by this tool

def visual_group_key(entry, category, subcategory, types):
    """Returns (group_key, reason, confidence)."""
    # A DISGUISE object is rendered by the client as its target, which is a
    # demonstrated visual alias rather than a guess.
    if "Disguise" in entry["flags"] and "DisguiseTarget" in entry["attrs"]:
        target = entry["attrs"]["DisguiseTarget"]
        return "alias:%s" % target, "DisguiseTarget=%s" % target, DEMONSTRATED

    name = entry["name"].strip().lower()
    if name:
        return ("name:%s|%s" % (name, category), "shared name and category", INFERRED)
    return ("id:%d" % entry["id"], "no name to group by", UNRESOLVED)


# --------------------------------------------------------------------------
# Creatures, NPCs, spawns

def parse_monster(text, filename):
    entry = {"source_file": filename, "race": "", "name": "", "article": "",
             "outfit_id": "", "outfit_colors": "", "outfit_object": "",
             "corpse": "", "blood": ""}
    for raw in text.splitlines():
        line = strip_comment(raw).strip()
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        key = key.lower()
        if key == "racenumber":
            entry["race"] = value
        elif key == "name":
            entry["name"] = value.strip('"')
        elif key == "article":
            entry["article"] = value.strip('"')
        elif key == "corpse":
            entry["corpse"] = value
        elif key == "blood":
            entry["blood"] = value
        elif key == "outfit":
            match = OUTFIT.search("Outfit = " + value)
            if match:
                outfit_id = match.group(1)
                rest = match.group(2) or ""
                # SendOutfit: an outfit id of zero means the creature is drawn
                # as an object type instead of a character.
                if outfit_id == "0":
                    entry["outfit_id"] = "0"
                    entry["outfit_object"] = rest
                else:
                    entry["outfit_id"] = outfit_id
                    entry["outfit_colors"] = rest
    return entry


def parse_npc(text, filename):
    entry = {"source_file": filename, "name": "", "sex": "", "race": "",
             "outfit_id": "", "outfit_colors": "", "outfit_object": "",
             "home_x": "", "home_y": "", "home_z": ""}
    for raw in text.splitlines():
        line = strip_comment(raw).strip()
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        key = key.lower()
        if key == "name":
            entry["name"] = value.strip('"')
        elif key == "sex":
            entry["sex"] = value
        elif key == "race":
            entry["race"] = value
        elif key == "home":
            match = COORD.search(value)
            if match:
                entry["home_x"], entry["home_y"], entry["home_z"] = match.groups()
        elif key == "outfit":
            match = OUTFIT.search("Outfit = " + value)
            if match:
                outfit_id = match.group(1)
                rest = match.group(2) or ""
                if outfit_id == "0":
                    entry["outfit_id"] = "0"
                    entry["outfit_object"] = rest
                else:
                    entry["outfit_id"] = outfit_id
                    entry["outfit_colors"] = rest
        if entry["name"] and entry["outfit_id"] and entry["home_x"]:
            # Behaviour blocks below can contain arbitrary text; stop early.
            continue
    return entry


def parse_spawns(text):
    spawns = []
    for raw in text.splitlines():
        line = strip_comment(raw).strip()
        if not line:
            continue
        parts = line.split()
        if len(parts) < 7:
            continue
        try:
            spawns.append({
                "race": int(parts[0]), "x": int(parts[1]), "y": int(parts[2]),
                "z": int(parts[3]), "radius": int(parts[4]),
                "amount": int(parts[5]), "regen": int(parts[6]),
            })
        except ValueError:
            continue
    return spawns


# --------------------------------------------------------------------------
# map.dat

def parse_map_dat(text):
    info = {"marks": [], "bounds": {}, "newbie_start": None, "veteran_start": None}
    for raw in text.splitlines():
        line = strip_comment(raw).strip()
        mark = MARK.search(line)
        if mark:
            info["marks"].append({
                "name": mark.group(1), "x": int(mark.group(2)),
                "y": int(mark.group(3)), "z": int(mark.group(4)),
            })
            continue
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        low = key.lower()
        if low.startswith("sector"):
            try:
                info["bounds"][low] = int(value)
            except ValueError:
                pass
        elif low in ("newbiestart", "veteranstart"):
            coord = COORD.search(value)
            if coord:
                info["newbie_start" if low == "newbiestart" else "veteran_start"] = (
                    int(coord.group(1)), int(coord.group(2)), int(coord.group(3)))
    return info


# --------------------------------------------------------------------------
# Regions, from the map's own named marks. Every sector is attributed to the
# nearest mark, which partitions the world using only shipped data. It is an
# approximation of a region boundary, so it is reported as INFERRED.

def nearest_mark(marks, x, y):
    best = None
    best_distance = None
    for mark in marks:
        distance = (mark["x"] - x) ** 2 + (mark["y"] - y) ** 2
        if best_distance is None or distance < best_distance:
            best_distance = distance
            best = mark["name"]
    return best


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--archive", required=True, help="historical runtime archive")
    parser.add_argument("--source", required=True, help="reference/game/src directory")
    parser.add_argument("--out", required=True, help="manifest output directory")
    parser.add_argument("--p0-sector-radius", type=int, default=1,
                        help="sector rings around the newbie start sector for P0")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    sources_used = []
    missing = []

    # ---- effects, straight out of the server enum -------------------------
    effects = []
    enums_path = os.path.join(args.source, "enums.hh")
    try:
        with open(enums_path, "r", encoding="latin-1") as handle:
            enums_text = handle.read()
        block = re.search(r"enum EffectType: int \{(.*?)\};", enums_text, re.S)
        if block:
            for name, value in re.findall(r"(EFFECT_[A-Z_]+)\s*=\s*(\d+)", block.group(1)):
                effects.append({"effect_id": int(value), "name": name,
                                "source": "reference/game/src/enums.hh::EffectType",
                                "confidence": DEMONSTRATED})
        sources_used.append(enums_path)
    except OSError as error:
        missing.append("enums.hh (%s)" % error)

    # ---- everything inside the archive ------------------------------------
    objects = {}
    monsters = []
    npcs = []
    spawns = []
    map_info = {"marks": [], "bounds": {}, "newbie_start": None, "veteran_start": None}
    sector_usage = defaultdict(Counter)   # type id -> Counter(region)
    sector_count = 0
    tile_count = 0
    sector_types = defaultdict(set)       # (sx, sy, sz) -> set of type ids

    with tarfile.open(args.archive, "r:gz") as archive:
        for member in archive:
            name = member.name
            if not member.isfile():
                continue
            if name == "./dat/objects.srv":
                objects = parse_objects(archive.extractfile(member).read().decode("latin-1"))
                sources_used.append(name)
            elif name == "./dat/map.dat":
                map_info = parse_map_dat(archive.extractfile(member).read().decode("latin-1"))
                sources_used.append(name)
            elif name == "./dat/monster.db":
                spawns = parse_spawns(archive.extractfile(member).read().decode("latin-1"))
                sources_used.append(name)
            elif name.startswith("./mon/") and name.endswith(".mon"):
                monsters.append(parse_monster(
                    archive.extractfile(member).read().decode("latin-1"), name))
            elif name.startswith("./npc/") and name.endswith(".npc"):
                npcs.append(parse_npc(
                    archive.extractfile(member).read().decode("latin-1"), name))
            elif name.startswith("./origmap/") and name.endswith(".sec"):
                base = os.path.basename(name)
                match = SECTOR_NAME.match(base)
                if not match:
                    continue
                sx, sy, sz = (int(part) for part in match.groups())
                sector_count += 1
                body = archive.extractfile(member).read().decode("latin-1")
                ids = set()
                for line in body.splitlines():
                    content = CONTENT.search(line)
                    if not content:
                        continue
                    tile_count += 1
                    for token in content.group(1).split(","):
                        token = token.strip()
                        if token.isdigit():
                            ids.add(int(token))
                sector_types[(sx, sy, sz)] = ids

    if monsters:
        sources_used.append("./mon/*.mon")
    if npcs:
        sources_used.append("./npc/*.npc")
    if sector_count:
        sources_used.append("./origmap/*.sec")

    if not objects:
        print("FATAL: ./dat/objects.srv was not found in the archive", file=sys.stderr)
        return 2

    # ---- region attribution and map usage ---------------------------------
    marks = map_info["marks"]
    region_of_sector = {}
    for (sx, sy, sz), ids in sector_types.items():
        centre_x = sx * SECTOR_SIZE + SECTOR_SIZE // 2
        centre_y = sy * SECTOR_SIZE + SECTOR_SIZE // 2
        region = nearest_mark(marks, centre_x, centre_y) if marks else "UNRESOLVED"
        region_of_sector[(sx, sy, sz)] = region
        for type_id in ids:
            sector_usage[type_id][region] += 1

    # ---- priorities --------------------------------------------------------
    newbie = map_info["newbie_start"]
    p0_ids = set()
    p0_sectors = []
    if newbie:
        nx, ny, _nz = newbie
        base_sx, base_sy = nx // SECTOR_SIZE, ny // SECTOR_SIZE
        radius = args.p0_sector_radius
        for (sx, sy, sz), ids in sector_types.items():
            if abs(sx - base_sx) <= radius and abs(sy - base_sy) <= radius:
                p0_sectors.append("%04d-%04d-%02d" % (sx, sy, sz))
                p0_ids |= ids
    else:
        missing.append("newbiestart from ./dat/map.dat, so P0 could not be derived")

    rookgaard_ids = set()
    for (key, ids) in sector_types.items():
        if region_of_sector.get(key) == "Rookgaard":
            rookgaard_ids |= ids
    p1_ids = rookgaard_ids - p0_ids

    def priority_of(type_id):
        if type_id in p0_ids:
            return "P0", "sector within %d rings of newbiestart" % args.p0_sector_radius
        if type_id in p1_ids:
            return "P1", "sector nearest mark Rookgaard"
        if type_id in sector_usage:
            regions = ",".join(sorted(sector_usage[type_id]))
            return "P2", "map regions: " + regions
        return "UNPRIORITIZED", "declared but absent from origmap"

    # ---- object manifest ---------------------------------------------------
    group_members = defaultdict(list)
    object_rows = []
    for type_id in sorted(objects):
        entry = objects[type_id]
        category, subcategory, confidence, evidence = categorize(entry)
        art, art_confidence, art_evidence = art_class(entry)
        group, group_reason, group_confidence = visual_group_key(
            entry, category, subcategory, objects)
        group_members[group].append(type_id)
        priority, priority_evidence = priority_of(type_id)
        flags = ",".join(sorted(entry["flags"]))
        wire_extra = []
        if entry["flags"] & {"LiquidContainer", "LiquidPool"}:
            wire_extra.append("liquid_colour")
        if "Cumulative" in entry["flags"]:
            wire_extra.append("amount")
        object_rows.append({
            "id": type_id,
            "name": entry["name"],
            "description": entry["description"],
            "category": category,
            "subcategory": subcategory,
            "category_confidence": confidence,
            "category_evidence": evidence,
            "art_class": art,
            "art_class_confidence": art_confidence,
            "art_class_evidence": art_evidence,
            "flags": flags,
            "wire_extra_bytes": ";".join(wire_extra),
            "visual_group": group,
            "visual_group_reason": group_reason,
            "visual_group_confidence": group_confidence,
            "map_occurrence_sectors": sum(sector_usage[type_id].values()),
            "map_regions": ";".join(sorted(sector_usage[type_id])),
            "priority": priority,
            "priority_evidence": priority_evidence,
            "source": "./dat/objects.srv",
            "sprite_geometry": UNRESOLVED,
        })

    # ---- writers -----------------------------------------------------------
    def write_csv(filename, rows, fieldnames):
        path = os.path.join(args.out, filename)
        with open(path, "w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=fieldnames)
            writer.writeheader()
            for row in rows:
                writer.writerow(row)
        return path

    write_csv("objects.csv", object_rows, list(object_rows[0].keys()))

    spawn_by_race = Counter(spawn["race"] for spawn in spawns)
    rook_races = set()
    if newbie:
        nx, ny, _ = newbie
        for spawn in spawns:
            if nearest_mark(marks, spawn["x"], spawn["y"]) == "Rookgaard":
                rook_races.add(spawn["race"])

    monster_rows = []
    for monster in sorted(monsters, key=lambda m: (m["name"] or "", m["source_file"])):
        race = monster["race"]
        race_int = int(race) if race.isdigit() else None
        in_rook = race_int in rook_races if race_int is not None else False
        monster_rows.append({
            "race": race,
            "name": monster["name"],
            "article": monster["article"],
            "outfit_id": monster["outfit_id"],
            "outfit_colors": monster["outfit_colors"],
            "outfit_object_type": monster["outfit_object"],
            "corpse_type_id": monster["corpse"],
            "blood": monster["blood"],
            "spawn_points": spawn_by_race.get(race_int, 0) if race_int is not None else 0,
            "priority": "P1" if in_rook else ("P2" if spawn_by_race.get(race_int, 0) else
                                              "UNPRIORITIZED"),
            "priority_evidence": ("spawn nearest mark Rookgaard" if in_rook else
                                  ("./dat/monster.db spawn points"
                                   if spawn_by_race.get(race_int, 0) else
                                   "declared but no spawn point")),
            "category": "creature",
            "subcategory": "monster",
            "confidence": DEMONSTRATED,
            "source": monster["source_file"],
            "sprite_geometry": UNRESOLVED,
        })
    if monster_rows:
        write_csv("creatures.csv", monster_rows, list(monster_rows[0].keys()))

    npc_rows = []
    for npc in sorted(npcs, key=lambda n: (n["name"] or "", n["source_file"])):
        region = ""
        if npc["home_x"] and marks:
            region = nearest_mark(marks, int(npc["home_x"]), int(npc["home_y"]))
        npc_rows.append({
            "name": npc["name"],
            "sex": npc["sex"],
            "race": npc["race"],
            "outfit_id": npc["outfit_id"],
            "outfit_colors": npc["outfit_colors"],
            "outfit_object_type": npc["outfit_object"],
            "home_x": npc["home_x"], "home_y": npc["home_y"], "home_z": npc["home_z"],
            "region": region,
            "priority": "P1" if region == "Rookgaard" else
                        ("P2" if region else "UNPRIORITIZED"),
            "priority_evidence": ("home nearest mark " + region) if region
                                 else "no Home coordinate",
            "category": "creature",
            "subcategory": "npc",
            "confidence": DEMONSTRATED,
            "source": npc["source_file"],
            "sprite_geometry": UNRESOLVED,
        })
    if npc_rows:
        write_csv("npcs.csv", npc_rows, list(npc_rows[0].keys()))

    if effects:
        write_csv("effects.csv", effects, list(effects[0].keys()))

    # Missiles are object attributes rather than a declared enum, so the set of
    # distinct values is itself the inventory.
    missile_rows = []
    missile_seen = defaultdict(set)
    for type_id, entry in objects.items():
        for attribute in ("ThrowMissile", "WandMissile", "AmmoMissile"):
            if attribute in entry["attrs"]:
                missile_seen[(attribute, entry["attrs"][attribute])].add(type_id)
    for (attribute, value), ids in sorted(missile_seen.items()):
        missile_rows.append({
            "missile_attribute": attribute,
            "missile_id": value,
            "used_by_type_ids": ";".join(str(i) for i in sorted(ids)),
            "category": "projectile",
            "subcategory": attribute,
            "confidence": DEMONSTRATED,
            "source": "./dat/objects.srv attributes",
            "sprite_geometry": UNRESOLVED,
        })
    if missile_rows:
        write_csv("missiles.csv", missile_rows, list(missile_rows[0].keys()))

    # Player and creature outfit identities actually referenced by the data.
    outfit_rows = []
    outfit_users = defaultdict(list)
    for monster in monsters:
        if monster["outfit_id"] and monster["outfit_id"] != "0":
            outfit_users[monster["outfit_id"]].append("mon:" + (monster["name"] or "?"))
    for npc in npcs:
        if npc["outfit_id"] and npc["outfit_id"] != "0":
            outfit_users[npc["outfit_id"]].append("npc:" + (npc["name"] or "?"))
    for outfit_id in sorted(outfit_users, key=lambda value: int(value)):
        users = outfit_users[outfit_id]
        outfit_rows.append({
            "outfit_id": outfit_id,
            "users": len(users),
            "sample_users": ";".join(sorted(users)[:6]),
            "category": "outfit",
            "subcategory": "creature_or_npc",
            "confidence": DEMONSTRATED,
            "source": "./mon/*.mon and ./npc/*.npc Outfit fields",
            "sprite_geometry": UNRESOLVED,
        })
    if outfit_rows:
        write_csv("outfits.csv", outfit_rows, list(outfit_rows[0].keys()))

    group_rows = []
    for group, members in sorted(group_members.items()):
        sample = objects[members[0]]
        category, subcategory, _c, _e = categorize(sample)
        group_rows.append({
            "visual_group": group,
            "member_count": len(members),
            "member_ids": ";".join(str(i) for i in members[:40]),
            "truncated": "yes" if len(members) > 40 else "no",
            "representative_name": sample["name"],
            "category": category,
            "subcategory": subcategory,
            "reuse_candidate": "yes" if len(members) > 1 else "no",
        })
    write_csv("visual_groups.csv", group_rows, list(group_rows[0].keys()))

    region_rows = []
    region_counter = Counter(region_of_sector.values())
    for region, count in sorted(region_counter.items(), key=lambda kv: -kv[1]):
        region_rows.append({"region": region, "sectors": count})
    if region_rows:
        write_csv("regions.csv", region_rows, ["region", "sectors"])

    # ---- summary -----------------------------------------------------------
    category_totals = Counter(row["category"] for row in object_rows)
    art_totals = Counter(row["art_class"] for row in object_rows)
    reusable = sum(len(m) for m in group_members.values() if len(m) > 1)
    summary = {
        "schema_version": SCHEMA_VERSION,
        "inventory_completeness": (
            "COMPLETE over the datasets listed in sources_used. NOT complete in "
            "the visual sense: no appearance data was available, see "
            "sources_unavailable."
        ),
        "sources_used": sorted(set(sources_used)),
        "sources_missing": missing,
        "sources_unavailable": [
            {
                "source": "client Tibia.dat / Tibia.spr (7.72)",
                "would_provide": ("per-thing sprite dimensions, layer counts, "
                                  "animation frame counts, draw offsets and the "
                                  "sprite pixels themselves"),
                "why_unavailable": ("not part of this repository, covered by "
                                    ".gitignore, and its provenance is recorded "
                                    "as UNKNOWN in docs/CLASSIC_CLIENT_772.md"),
                "affected_columns": ["sprite_geometry"],
                "extractor_hook": ("add a reader alongside parse_objects and fill "
                                   "the sprite_geometry column; no other column "
                                   "changes"),
            }
        ],
        "objects": {
            "declared_total": len(object_rows),
            "with_name": sum(1 for row in object_rows if row["name"]),
            "without_name": sum(1 for row in object_rows if not row["name"]),
            "unresolved_category": sum(1 for row in object_rows
                                       if row["category_confidence"] == UNRESOLVED),
            "inferred_category": sum(1 for row in object_rows
                                     if row["category_confidence"] == INFERRED),
            "demonstrated_category": sum(1 for row in object_rows
                                         if row["category_confidence"] == DEMONSTRATED),
            "by_category": dict(sorted(category_totals.items())),
            "by_art_class": dict(sorted(art_totals.items())),
            "art_class_unresolved": sum(1 for row in object_rows
                                        if row["art_class_confidence"] == UNRESOLVED),
            "present_on_map": sum(1 for row in object_rows
                                  if row["map_occurrence_sectors"] > 0),
            "absent_from_map": sum(1 for row in object_rows
                                   if row["map_occurrence_sectors"] == 0),
        },
        "visual_groups": {
            "total": len(group_members),
            "multi_member": sum(1 for m in group_members.values() if len(m) > 1),
            "ids_in_multi_member_groups": reusable,
            "disguise_aliases": sum(1 for g in group_members if g.startswith("alias:")),
        },
        "priorities": {
            "P0": sum(1 for row in object_rows if row["priority"] == "P0"),
            "P1": sum(1 for row in object_rows if row["priority"] == "P1"),
            "P2": sum(1 for row in object_rows if row["priority"] == "P2"),
            "UNPRIORITIZED": sum(1 for row in object_rows
                                 if row["priority"] == "UNPRIORITIZED"),
        },
        "creatures": {"monster_races": len(monster_rows),
                      "rookgaard_races": len(rook_races),
                      "spawn_points": len(spawns)},
        "npcs": {"total": len(npc_rows),
                 "rookgaard": sum(1 for row in npc_rows if row["region"] == "Rookgaard")},
        "effects": len(effects),
        "missiles": len(missile_rows),
        "outfits": len(outfit_rows),
        "map": {
            "sectors_scanned": sector_count,
            "tiles_scanned": tile_count,
            "bounds": map_info["bounds"],
            "newbie_start": newbie,
            "veteran_start": map_info["veteran_start"],
            "marks": len(marks),
            "p0_sectors": sorted(p0_sectors),
        },
        "unresolved_fields": {
            "sprite_geometry": (
                "Per-thing sprite dimensions, layer counts, animation frame counts "
                "and draw offsets live in the client Tibia.dat/Tibia.spr, which are "
                "not part of this repository and whose provenance is UNKNOWN. Every "
                "manifest carries a sprite_geometry column set to UNRESOLVED so the "
                "extractor can fill it once an artifact with verifiable provenance "
                "is available."
            ),
        },
    }
    with open(os.path.join(args.out, "summary.json"), "w", encoding="utf-8") as handle:
        json.dump(summary, handle, indent=2, sort_keys=False)
        handle.write("\n")

    print("objects            %d" % len(object_rows))
    print("  demonstrated     %d" % summary["objects"]["demonstrated_category"])
    print("  inferred         %d" % summary["objects"]["inferred_category"])
    print("  unresolved       %d" % summary["objects"]["unresolved_category"])
    print("visual groups      %d (multi-member %d)"
          % (len(group_members), summary["visual_groups"]["multi_member"]))
    print("priorities         P0=%d P1=%d P2=%d unprioritized=%d"
          % (summary["priorities"]["P0"], summary["priorities"]["P1"],
             summary["priorities"]["P2"], summary["priorities"]["UNPRIORITIZED"]))
    print("monsters %d  npcs %d  effects %d  missiles %d  outfits %d"
          % (len(monster_rows), len(npc_rows), len(effects), len(missile_rows),
             len(outfit_rows)))
    print("map sectors %d  tiles %d  marks %d" % (sector_count, tile_count, len(marks)))
    if missing:
        print("MISSING SOURCES: " + "; ".join(missing))
    for unavailable in summary["sources_unavailable"]:
        print("UNAVAILABLE SOURCE: %s -> %s"
              % (unavailable["source"], ", ".join(unavailable["affected_columns"])))
    print("completeness       %s" % summary["inventory_completeness"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
