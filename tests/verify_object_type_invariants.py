#!/usr/bin/env python3
"""Checks the object-type invariants the FULLSCREEN decoder relies on.

The 7.72 map encoding is not self-describing: reference/game/src/sending.cc::
SendItem emits an extra byte only when the object type carries LIQUIDCONTAINER,
LIQUIDPOOL or CUMULATIVE, and reference/game/src/sending.cc::SendMapObject
reuses the low words 97/98/99 for creature descriptors. This script proves,
against the shipped dat/objects.srv, that those rules can be applied from the
type id that actually travels on the wire.

Usage:
  verify_object_type_invariants.py /path/to/tibia-game.tarball.tar.gz
  verify_object_type_invariants.py --file /path/to/objects.srv
"""

import argparse
import re
import sys
import tarfile

WIRE_FLAGS = frozenset({"LiquidContainer", "LiquidPool", "Cumulative"})
CREATURE_MARKERS = (97, 98, 99)
SKIP_MARKER_BASE = 0xFF00
RESERVED_CONTAINER_IDS = frozenset(range(0, 11))
TYPEID_CREATURE_CONTAINER = 99
FIRST_MAP_OBJECT_TYPE_ID = 100

BRACES = re.compile(r"\{(.*)\}")


def strip_comment(line):
    out = []
    in_string = False
    for character in line:
        if character == '"':
            in_string = not in_string
        elif character == "#" and not in_string:
            break
        out.append(character)
    return "".join(out)


def parse(text):
    types = {}
    current = None
    for raw in text.splitlines():
        line = strip_comment(raw).strip()
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        if key == "TypeID":
            current = int(value)
            if current in types:
                raise SystemExit("duplicate TypeID %d" % current)
            types[current] = {"flags": set(), "attrs": {}}
        elif current is None:
            continue
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


def read_objects_srv(args):
    if args.file:
        with open(args.file, "r", encoding="latin-1") as handle:
            return handle.read()
    with tarfile.open(args.tarball, "r:gz") as archive:
        member = archive.extractfile("./dat/objects.srv")
        if member is None:
            raise SystemExit("./dat/objects.srv not found in the archive")
        return member.read().decode("latin-1")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tarball", nargs="?", help="historical runtime archive")
    parser.add_argument("--file", help="an already extracted objects.srv")
    args = parser.parse_args()
    if not args.tarball and not args.file:
        parser.error("pass a tarball path or --file")

    types = parse(read_objects_srv(args))
    ids = sorted(types)
    failures = []

    print("declared object types: %d (min %d, max %d)" % (len(ids), ids[0], ids[-1]))

    # 1. The creature markers must not collide with any real map object type.
    for marker in CREATURE_MARKERS:
        if marker in types and marker != TYPEID_CREATURE_CONTAINER:
            failures.append("creature marker %d is also a declared object type" % marker)
    if TYPEID_CREATURE_CONTAINER not in types:
        failures.append("TYPEID_CREATURE_CONTAINER is absent from the data")

    # 2. Nothing is declared between the reserved containers and the first real
    #    map object type, so 11..98 can be rejected as unknown.
    between = [i for i in ids if 10 < i < FIRST_MAP_OBJECT_TYPE_ID and i != TYPEID_CREATURE_CONTAINER]
    if between:
        failures.append("unexpected type ids between the reserved ranges: %s" % between[:10])
    reserved = sorted(set(ids) & RESERVED_CONTAINER_IDS)
    print("reserved container ids present: %s" % reserved)

    # 3. No type id may reach the skip-marker page.
    if ids[-1] >= SKIP_MARKER_BASE:
        failures.append("type id %d collides with the skip-marker page" % ids[-1])

    # 4. At most one extra byte can follow a type id.
    both_liquids = [i for i in ids
                    if {"LiquidContainer", "LiquidPool"} <= types[i]["flags"]]
    if both_liquids:
        failures.append("types carrying both liquid flags: %s" % both_liquids[:10])
    liquid_and_amount = [
        i for i in ids
        if "Cumulative" in types[i]["flags"]
        and types[i]["flags"] & {"LiquidContainer", "LiquidPool"}
    ]
    if liquid_and_amount:
        failures.append("types both cumulative and liquid: %s" % liquid_and_amount[:10])

    # 5. SendItem sends getDisguise().TypeID but reads the flags from the
    #    original type, so the wire id only suffices if both agree.
    disguised = [i for i in ids if "Disguise" in types[i]["flags"]]
    mismatched = []
    for type_id in disguised:
        target = types[type_id]["attrs"].get("DisguiseTarget")
        if target is None:
            mismatched.append((type_id, None))
            continue
        target_id = int(target)
        source_flags = types[type_id]["flags"] & WIRE_FLAGS
        target_flags = types.get(target_id, {"flags": set()})["flags"] & WIRE_FLAGS
        if source_flags != target_flags:
            mismatched.append((type_id, target_id))
    print("disguise types: %d, wire-relevant mismatches: %d"
          % (len(disguised), len(mismatched)))
    if mismatched:
        failures.append("disguise flag mismatches: %s" % mismatched[:10])

    counts = {
        "liquid": sum(1 for i in ids if types[i]["flags"] & {"LiquidContainer", "LiquidPool"}),
        "cumulative": sum(1 for i in ids if "Cumulative" in types[i]["flags"]),
    }
    print("types needing an extra byte: liquid %d, cumulative %d"
          % (counts["liquid"], counts["cumulative"]))

    if failures:
        for failure in failures:
            print("FAIL: %s" % failure, file=sys.stderr)
        return 1
    print("verify_object_type_invariants: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
