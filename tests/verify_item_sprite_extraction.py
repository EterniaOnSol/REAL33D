"""Checks the UI item extractor against the validated 7.72 appearance reader.

`scripts/client/extract_item_sprites.py` is a second reader of the same
Tibia.dat/Tibia.spr pair that `visual/tools/tibia772.py` already reads, and the
second one existed without anything holding it to the first. It drew every
object with the sprite after the one it meant, because sprite ids are one-based
and it indexed the offset table directly. That is invisible for an object whose
next sprite is another frame of itself and glaring for one whose next sprite
belongs to the following object: a bag came out as a barrel.

Two readers of one format need a test that says they agree, so this compares:

  1. geometry and first-frame sprite ids, for every item, and
  2. the decoded pixels of the first sprite, for every item that has one.

Check 2 is the one that catches an offset error; check 1 catches a divergence in
the option table or the geometry order before it can reach the pixels.

    python tests/verify_item_sprite_extraction.py [things-dir]

`things-dir` holds Tibia.dat and Tibia.spr and defaults to the same local client
data the extractor uses. Neither file is in this repository.
"""

import importlib.util
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_THINGS = os.path.join(ROOT, "build", "classic-client-772", "app")


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    things = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_THINGS
    dat = os.path.join(things, "Tibia.dat")
    spr = os.path.join(things, "Tibia.spr")
    for path in (dat, spr):
        if not os.path.exists(path):
            print("MISSING %s" % path)
            return 2

    extractor = load("extract_item_sprites",
                     os.path.join(ROOT, "scripts", "client", "extract_item_sprites.py"))
    tibia772 = load("tibia772", os.path.join(ROOT, "visual", "tools", "tibia772.py"))

    mine = extractor.parse_dat(dat)
    data, offsets = extractor.parse_spr(spr)

    theirs = tibia772.AppearanceFile.read(dat)
    items = theirs.by_kind[tibia772.KIND_ITEM]
    sprites = tibia772.SpriteFile.read(spr)

    failures = []

    if len(mine) != len(items):
        failures.append("item count: extractor %d, reference %d" % (len(mine), len(items)))

    geometry_checked = 0
    pixels_checked = 0
    stack_patterns_checked = 0
    for type_id, (width, height, first, stack_pictures) in sorted(mine.items()):
        reference = items.get(type_id)
        if reference is None:
            failures.append("%d: absent from the reference reader" % type_id)
            continue
        if (width, height) != (reference.width, reference.height):
            failures.append("%d: geometry %dx%d vs %dx%d"
                            % (type_id, width, height, reference.width, reference.height))
            continue
        expected_first = reference.sprite_ids[:width * height]
        if first != expected_first:
            failures.append("%d: first frame %s vs %s" % (type_id, first, expected_first))
            continue
        geometry_checked += 1

        if stack_pictures:
            if len(stack_pictures) != 8 or reference.pattern_x != 4 or reference.pattern_y != 2:
                failures.append("%d: invalid stack pattern geometry" % type_id)
            for pattern, ids in enumerate(stack_pictures):
                begin = pattern * reference.layers * width * height
                expected = reference.sprite_ids[begin:begin + width * height]
                if ids != expected:
                    failures.append("%d: stack pattern %d %s vs %s"
                                    % (type_id, pattern, ids, expected))
                stack_patterns_checked += 1

        for sprite_id in first:
            if sprite_id == 0 or not sprites.has(sprite_id):
                continue
            got = bytes(extractor.decode_sprite(data, offsets[sprite_id - 1]))
            # The reference reader hands back RGBA tuples; the extractor works
            # in a flat RGBA buffer. Flatten one rather than reshape the other.
            want = bytes(channel for pixel in sprites.decode(sprite_id)
                         for channel in pixel)
            if got != want:
                failures.append("%d: sprite %d decodes differently" % (type_id, sprite_id))
                break
            pixels_checked += 1

    print("items compared:            %d" % len(mine))
    print("geometry + ids agree:      %d" % geometry_checked)
    print("sprite pixels compared:    %d" % pixels_checked)
    print("stack patterns compared:   %d" % stack_patterns_checked)
    print("failures:                  %d" % len(failures))
    for line in failures[:20]:
        print("  " + line)
    if len(failures) > 20:
        print("  ... and %d more" % (len(failures) - 20))
    print("RESULT: %s" % ("PASS" if not failures else "FAIL"))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
