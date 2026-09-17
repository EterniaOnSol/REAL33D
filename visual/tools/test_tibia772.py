#!/usr/bin/env python3
"""Tests for the 7.72 appearance and sprite readers.

Every fixture here is built byte by byte by the test itself, so the suite runs
without the client data and nothing proprietary is committed. The real client
files are exercised separately by build_reference_pack.py, whose validation
report is the evidence that the layout matches the shipped data.

  python3 visual/tools/test_tibia772.py
"""

import os
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import pngwrite                       # noqa: E402
import tibia772 as t                  # noqa: E402


def thing(options=b"", width=1, height=1, exact=None, layers=1,
          px=1, py=1, pz=1, frames=1, sprites=None):
    """Builds one thing record exactly as the reader expects to find it."""
    total = width * height * layers * px * py * pz * frames
    if sprites is None:
        sprites = list(range(1, total + 1))
    assert len(sprites) == total, "fixture sprite count must match the geometry"
    out = bytearray(options)
    out.append(t.END_OF_OPTIONS)
    out += bytes((width, height))
    if width > 1 or height > 1:
        out.append(exact if exact is not None else 32)
    out += bytes((layers, px, py, pz, frames))
    for sprite_id in sprites:
        out += struct.pack("<H", sprite_id)
    return bytes(out)


def dat(items=None, outfits=None, effects=None, missiles=None, signature=0x1234):
    """Builds a whole file. Counts in the header are last ids, not quantities."""
    items = items or []
    outfits = outfits or []
    effects = effects or []
    missiles = missiles or []
    last_item = t.FIRST_ITEM_ID + len(items) - 1 if items else t.FIRST_ITEM_ID - 1
    header = struct.pack("<IHHHH", signature, last_item, len(outfits),
                         len(effects), len(missiles))
    return header + b"".join(items + outfits + effects + missiles)


def spr(sprite_blobs, signature=0x5678):
    """sprite_blobs: list of raw pixel-data bytes, or None for an empty slot."""
    count = len(sprite_blobs)
    header = struct.pack("<IH", signature, count)
    offsets_at = len(header)
    body_at = offsets_at + count * 4
    offsets = []
    body = bytearray()
    for blob in sprite_blobs:
        if blob is None:
            offsets.append(0)
            continue
        offsets.append(body_at + len(body))
        body += b"\x00\x00\x00"                      # colour key, not drawn
        body += struct.pack("<H", len(blob))
        body += blob
    return header + struct.pack("<" + "I" * count, *offsets) + bytes(body)


def run(transparent, coloured_pixels):
    """One RLE run: a transparent count then that many RGB triples."""
    out = struct.pack("<HH", transparent, len(coloured_pixels))
    for red, green, blue in coloured_pixels:
        out += bytes((red, green, blue))
    return out


class AppearanceParsing(unittest.TestCase):

    def test_minimal_file(self):
        data = dat(items=[thing()])
        parsed = t.AppearanceFile.parse(data)
        self.assertEqual(len(parsed.appearances), 1)
        appearance = parsed.appearances[0]
        self.assertEqual(appearance.kind, t.KIND_ITEM)
        self.assertEqual(appearance.client_id, t.FIRST_ITEM_ID)
        self.assertEqual(appearance.sprite_ids, [1])

    def test_every_kind_is_numbered_from_its_own_base(self):
        data = dat(items=[thing(), thing()], outfits=[thing()],
                   effects=[thing()], missiles=[thing()])
        parsed = t.AppearanceFile.parse(data)
        self.assertEqual([a.client_id for a in parsed.appearances
                          if a.kind == t.KIND_ITEM], [100, 101])
        for kind in (t.KIND_OUTFIT, t.KIND_EFFECT, t.KIND_MISSILE):
            self.assertEqual([a.client_id for a in parsed.appearances
                              if a.kind == kind], [1])

    def test_pattern_z_is_part_of_the_sprite_count(self):
        # 1x1, one layer, 4x4 pattern, one z, one frame is sixteen sprites.
        # This is the shape of the first ground in the shipped data.
        data = dat(items=[thing(px=4, py=4)])
        appearance = t.AppearanceFile.parse(data).appearances[0]
        self.assertEqual(appearance.pattern_z, 1)
        self.assertEqual(len(appearance.sprite_ids), 16)

        # Dropping patternZ from the layout would misread the frame count as a
        # sprite id and overrun, which is what makes the field detectable.
        with_z = dat(items=[thing(px=2, py=2, pz=3, frames=2)])
        appearance = t.AppearanceFile.parse(with_z).appearances[0]
        self.assertEqual(len(appearance.sprite_ids), 2 * 2 * 3 * 2)

    def test_exact_size_only_for_multi_field_things(self):
        single = t.AppearanceFile.parse(dat(items=[thing()])).appearances[0]
        self.assertIsNone(single.exact_size)
        wide = t.AppearanceFile.parse(
            dat(items=[thing(width=2, height=1, exact=64)])).appearances[0]
        self.assertEqual(wide.exact_size, 64)
        self.assertEqual(len(wide.sprite_ids), 2)

    def test_option_payloads_are_consumed(self):
        # ground takes a speed word, light takes two words, a plain flag none.
        options = bytes([0x00]) + struct.pack("<H", 150) \
            + bytes([0x0C]) \
            + bytes([0x15]) + struct.pack("<HH", 7, 215)
        appearance = t.AppearanceFile.parse(
            dat(items=[thing(options=options)])).appearances[0]
        self.assertEqual(appearance.options[0x00], (150,))
        self.assertEqual(appearance.options[0x0C], ())
        self.assertEqual(appearance.options[0x15], (7, 215))

    def test_trailing_bytes_are_rejected(self):
        data = dat(items=[thing()]) + b"\x00\x00"
        with self.assertRaises(t.FormatError) as caught:
            t.AppearanceFile.parse(data)
        self.assertIn("does not consume the file", str(caught.exception))

    def test_truncated_file_is_rejected(self):
        data = dat(items=[thing(px=4, py=4)])
        for cut in range(1, len(data)):
            with self.assertRaises(t.FormatError):
                t.AppearanceFile.parse(data[:cut])

    def test_unknown_option_byte_is_rejected(self):
        data = dat(items=[thing(options=bytes([0x7E]))])
        with self.assertRaises(t.FormatError) as caught:
            t.AppearanceFile.parse(data)
        self.assertIn("unknown option byte", str(caught.exception))

    def test_short_header_is_rejected(self):
        with self.assertRaises(t.FormatError):
            t.AppearanceFile.parse(b"\x00\x01\x02")


class Validation(unittest.TestCase):

    def test_counts_and_sprite_range(self):
        data = dat(items=[thing(sprites=[5]), thing(sprites=[9])],
                   outfits=[thing(sprites=[3])])
        parsed = t.AppearanceFile.parse(data)
        report = parsed.validate(sprite_count=10)
        self.assertTrue(report["checks"]["thing_count_matches_header"]["pass"])
        self.assertTrue(report["checks"]["sprite_ids_in_range"]["pass"])
        self.assertEqual(report["checks"]["sprite_ids_in_range"]["highest_referenced"], 9)

    def test_sprite_id_beyond_the_sprite_file_fails(self):
        parsed = t.AppearanceFile.parse(dat(items=[thing(sprites=[99])]))
        report = parsed.validate(sprite_count=10)
        self.assertFalse(report["checks"]["sprite_ids_in_range"]["pass"])

    def test_option_flag_correlation(self):
        # Two items, both ground in the client and both Bank on the server.
        ground = bytes([0x00]) + struct.pack("<H", 100)
        parsed = t.AppearanceFile.parse(
            dat(items=[thing(options=ground), thing(options=ground)]))
        report = parsed.validate(sprite_count=10, server_flags={
            100: {"Bank"}, 101: {"Bank"}})
        entry = report["checks"]["option_flag_correlation"]["0x00/Bank"]
        self.assertEqual(entry["both"], 2)
        self.assertEqual(entry["only_dat"], 0)
        self.assertEqual(entry["only_server"], 0)
        self.assertEqual(entry["agreement"], 1.0)

    def test_correlation_notices_disagreement(self):
        parsed = t.AppearanceFile.parse(dat(items=[thing()]))   # no ground option
        report = parsed.validate(sprite_count=10, server_flags={100: {"Bank"}})
        entry = report["checks"]["option_flag_correlation"]["0x00/Bank"]
        self.assertEqual(entry["only_server"], 1)
        self.assertEqual(entry["agreement"], 0.0)


class SpriteDecoding(unittest.TestCase):

    def test_empty_slot(self):
        sprites = t.SpriteFile(spr([None]))
        self.assertEqual(sprites.count, 1)
        self.assertFalse(sprites.has(1))
        self.assertIsNone(sprites.decode(1))

    def test_single_opaque_run(self):
        blob = run(0, [(10, 20, 30)])
        sprites = t.SpriteFile(spr([blob]))
        pixels = sprites.decode(1)
        self.assertEqual(len(pixels), 32 * 32)
        self.assertEqual(pixels[0], (10, 20, 30, 255))
        self.assertEqual(pixels[1], (0, 0, 0, 0))

    def test_transparent_prefix_is_skipped(self):
        blob = run(5, [(1, 2, 3), (4, 5, 6)])
        pixels = t.SpriteFile(spr([blob])).decode(1)
        for index in range(5):
            self.assertEqual(pixels[index], (0, 0, 0, 0))
        self.assertEqual(pixels[5], (1, 2, 3, 255))
        self.assertEqual(pixels[6], (4, 5, 6, 255))

    def test_multiple_runs_accumulate(self):
        blob = run(1, [(9, 9, 9)]) + run(2, [(8, 8, 8)])
        pixels = t.SpriteFile(spr([blob])).decode(1)
        self.assertEqual(pixels[1], (9, 9, 9, 255))
        self.assertEqual(pixels[4], (8, 8, 8, 255))

    def test_a_full_opaque_sprite(self):
        blob = run(0, [(1, 1, 1)] * (32 * 32))
        pixels = t.SpriteFile(spr([blob])).decode(1)
        self.assertTrue(all(pixel[3] == 255 for pixel in pixels))

    def test_overrun_is_rejected(self):
        # Claims more coloured pixels than the blob carries.
        blob = struct.pack("<HH", 0, 4) + b"\x01\x02\x03"
        with self.assertRaises(t.FormatError):
            t.SpriteFile(spr([blob])).decode(1)

    def test_truncated_offset_table_is_rejected(self):
        data = spr([run(0, [(1, 2, 3)])])
        with self.assertRaises(t.FormatError):
            t.SpriteFile(data[:8])

    def test_writes_beyond_the_sprite_are_dropped_not_crashed(self):
        blob = run(32 * 32 - 1, [(1, 1, 1), (2, 2, 2), (3, 3, 3)])
        pixels = t.SpriteFile(spr([blob])).decode(1)
        self.assertEqual(pixels[-1], (1, 1, 1, 255))


class PngWriting(unittest.TestCase):

    def test_writes_a_readable_png(self):
        pixels = [(255, 0, 0, 255), (0, 255, 0, 255),
                  (0, 0, 255, 255), (0, 0, 0, 0)]
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "t.png")
            pngwrite.write_rgba(path, 2, 2, pixels)
            with open(path, "rb") as handle:
                data = handle.read()
        self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
        self.assertIn(b"IHDR", data[:32])
        self.assertIn(b"IEND", data[-12:])
        width, height = struct.unpack_from(">II", data, 16)
        self.assertEqual((width, height), (2, 2))

    def test_size_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                pngwrite.write_rgba(os.path.join(directory, "t.png"), 2, 2,
                                    [(0, 0, 0, 0)])

    def test_nearest_neighbour_scaling_duplicates_exactly(self):
        pixels = [(1, 1, 1, 255), (2, 2, 2, 255)]
        scaled, width, height = pngwrite.scale_nearest(pixels, 2, 1, 3)
        self.assertEqual((width, height), (6, 3))
        self.assertEqual(scaled[:6], [(1, 1, 1, 255)] * 3 + [(2, 2, 2, 255)] * 3)
        self.assertEqual(scaled[6:12], scaled[:6])
        self.assertEqual(len(set(scaled)), 2)   # no interpolation introduced


if __name__ == "__main__":
    unittest.main(verbosity=2)
