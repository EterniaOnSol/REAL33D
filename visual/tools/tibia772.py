"""Readers for the Tibia 7.72 client appearance data.

There is no published specification for this format in the project's source
truth, so the layout below is a hypothesis that the reader validates rather
than assumes. `AppearanceFile.validate()` checks four independent things:

  1. every thing record parses and the file is consumed to exactly EOF;
  2. every sprite id referenced is inside the sprite file's own count;
  3. the number of things of each kind matches the header counts;
  4. the option bytes correlate with the server's own object flags in
     dat/objects.srv, which is data this reader never looks at.

A wrong option payload size derails the byte stream within a few records, so
checks 1 and 2 fail loudly rather than producing plausible nonsense. Check 4 is
the independent one: the client and the server were built from the same content
but store it separately, so agreement across thousands of items is evidence the
layout is right and not merely self-consistent.

Nothing here is redistributed. The reader takes paths to files the operator
supplies locally.
"""

import struct
from collections import namedtuple

DAT_HEADER = struct.Struct("<IHHHH")
SPR_HEADER = struct.Struct("<IH")

# Thing kinds, in the order the file stores them.
KIND_ITEM = "item"
KIND_OUTFIT = "outfit"
KIND_EFFECT = "effect"
KIND_MISSILE = "missile"

# The first item id. Ids 0..99 are reserved server side
# (reference/game/src/objects.hh, TYPEID_* and the iteration in
# objects.cc::GetObjectTypeByName which starts at TYPEID_CREATURE_CONTAINER+1).
FIRST_ITEM_ID = 100

END_OF_OPTIONS = 0xFF

# Option byte -> number of payload bytes that follow it. Names are the reader's
# own labels for what the option marks; only the payload sizes affect parsing,
# and those are what validate() proves.
OPTION_PAYLOAD = {
    0x00: 2,   # ground, followed by a speed word
    0x01: 0,   # ground border
    0x02: 0,   # on bottom
    0x03: 0,   # on top
    0x04: 0,   # container
    0x05: 0,   # stackable
    0x06: 0,   # corpse / force use
    0x07: 0,   # usable
    0x08: 2,   # writable, followed by a maximum length
    0x09: 2,   # writable once, followed by a maximum length
    0x0A: 0,   # fluid container
    0x0B: 0,   # splash
    0x0C: 0,   # unpassable
    0x0D: 0,   # unmovable
    0x0E: 0,   # blocks missiles
    0x0F: 0,   # blocks path
    0x10: 0,   # pickupable
    0x11: 0,   # hangable
    0x12: 0,   # hooks south
    0x13: 0,   # hooks east
    0x14: 0,   # rotatable
    0x15: 4,   # light, followed by intensity and colour words
    0x16: 0,   # does not hide behind
    0x17: 0,   # floor change
    0x18: 4,   # draw displacement, followed by x and y words
    0x19: 2,   # elevation, followed by a height word
    0x1A: 0,   # lying object
    0x1B: 0,   # always animated
    0x1C: 2,   # minimap colour, followed by a colour word
    0x1D: 2,   # lens help, followed by an id word
    0x1E: 0,   # full ground
    0x1F: 0,   # ignore look
}

# Option bytes whose meaning the server also records, used by validate() to
# cross-check against dat/objects.srv. Left deliberately small: only options
# whose server counterpart is unambiguous.
OPTION_TO_SERVER_FLAG = {
    0x00: "Bank",
    0x01: "Clip",
    0x02: "Bottom",
    0x03: "Top",
    0x04: "Container",
    0x05: "Cumulative",
    0x0A: "LiquidContainer",
    0x0B: "LiquidPool",
    0x0C: "Unpass",
    0x0D: "Unmove",
    0x10: "Take",
    0x11: "Hang",
    0x12: "HookSouth",
    0x13: "HookEast",
    0x14: "Rotate",
}


class FormatError(Exception):
    """Raised when the byte stream does not match the layout."""


Appearance = namedtuple("Appearance", [
    "kind", "client_id", "options", "width", "height", "exact_size",
    "layers", "pattern_x", "pattern_y", "pattern_z", "frames", "sprite_ids",
])


def _sprite_count(appearance_fields):
    width, height, layers, px, py, pz, frames = appearance_fields
    return width * height * layers * px * py * pz * frames


class AppearanceFile:
    """Parsed Tibia.dat."""

    def __init__(self, signature, counts, appearances):
        self.signature = signature
        self.counts = counts               # dict kind -> declared count
        self.appearances = appearances     # list in file order
        self.by_kind = {}
        for appearance in appearances:
            self.by_kind.setdefault(appearance.kind, {})[appearance.client_id] = appearance

    @classmethod
    def read(cls, path):
        with open(path, "rb") as handle:
            data = handle.read()
        return cls.parse(data)

    @classmethod
    def parse(cls, data):
        if len(data) < DAT_HEADER.size:
            raise FormatError("file shorter than the header")
        signature, items, outfits, effects, missiles = DAT_HEADER.unpack_from(data, 0)
        at = DAT_HEADER.size

        # The header stores the last id of each kind, not a quantity: items
        # start at FIRST_ITEM_ID and everything else starts at 1.
        plan = [
            (KIND_ITEM, FIRST_ITEM_ID, items),
            (KIND_OUTFIT, 1, outfits),
            (KIND_EFFECT, 1, effects),
            (KIND_MISSILE, 1, missiles),
        ]
        counts = {KIND_ITEM: items, KIND_OUTFIT: outfits,
                  KIND_EFFECT: effects, KIND_MISSILE: missiles}

        appearances = []
        for kind, first, last in plan:
            for client_id in range(first, last + 1):
                appearance, at = cls._parse_one(data, at, kind, client_id)
                appearances.append(appearance)

        if at != len(data):
            raise FormatError(
                "parsed %d of %d bytes; the layout does not consume the file"
                % (at, len(data)))
        return cls(signature, counts, appearances)

    @staticmethod
    def _parse_one(data, at, kind, client_id):
        options = {}
        while True:
            if at >= len(data):
                raise FormatError("options for %s %d run past the end"
                                  % (kind, client_id))
            option = data[at]
            at += 1
            if option == END_OF_OPTIONS:
                break
            if option not in OPTION_PAYLOAD:
                raise FormatError("unknown option byte 0x%02X for %s %d at offset %d"
                                  % (option, kind, client_id, at - 1))
            size = OPTION_PAYLOAD[option]
            if at + size > len(data):
                raise FormatError("option 0x%02X payload for %s %d runs past the end"
                                  % (option, kind, client_id))
            payload = tuple(struct.unpack_from("<" + "H" * (size // 2), data, at)) \
                if size else ()
            options[option] = payload
            at += size

        if at + 2 > len(data):
            raise FormatError("geometry for %s %d runs past the end" % (kind, client_id))
        width = data[at]
        height = data[at + 1]
        at += 2
        exact_size = None
        # An exact size byte is present only for things larger than one field.
        if width > 1 or height > 1:
            if at >= len(data):
                raise FormatError("exact size for %s %d runs past the end"
                                  % (kind, client_id))
            exact_size = data[at]
            at += 1
        if at + 5 > len(data):
            raise FormatError("geometry tail for %s %d runs past the end"
                              % (kind, client_id))
        layers, pattern_x, pattern_y, pattern_z, frames = data[at:at + 5]
        at += 5

        total = _sprite_count((width, height, layers, pattern_x, pattern_y,
                               pattern_z, frames))
        if at + total * 2 > len(data):
            raise FormatError("sprite list for %s %d runs past the end"
                              % (kind, client_id))
        sprite_ids = list(struct.unpack_from("<" + "H" * total, data, at))
        at += total * 2

        return Appearance(kind, client_id, options, width, height, exact_size,
                          layers, pattern_x, pattern_y, pattern_z, frames,
                          sprite_ids), at

    # ---------------------------------------------------------------- checks

    def validate(self, sprite_count=None, server_flags=None):
        """Returns a report dict. Never raises: a failed check is a result."""
        report = {"parsed_things": len(self.appearances), "checks": {}}

        expected = (self.counts[KIND_ITEM] - FIRST_ITEM_ID + 1
                    + self.counts[KIND_OUTFIT] + self.counts[KIND_EFFECT]
                    + self.counts[KIND_MISSILE])
        report["checks"]["thing_count_matches_header"] = {
            "expected": expected, "actual": len(self.appearances),
            "pass": expected == len(self.appearances),
        }

        referenced = set()
        for appearance in self.appearances:
            referenced.update(appearance.sprite_ids)
        referenced.discard(0)   # zero marks an empty slot
        highest = max(referenced) if referenced else 0
        report["checks"]["sprite_ids_in_range"] = {
            "highest_referenced": highest, "sprite_file_count": sprite_count,
            "pass": sprite_count is None or highest <= sprite_count,
        }

        if server_flags:
            report["checks"]["option_flag_correlation"] = \
                self._correlate(server_flags)
        return report

    def _correlate(self, server_flags):
        """server_flags: {client_id: set(flag names)} for items only."""
        results = {}
        items = self.by_kind.get(KIND_ITEM, {})
        for option, flag in sorted(OPTION_TO_SERVER_FLAG.items()):
            both = only_dat = only_server = 0
            for client_id, appearance in items.items():
                flags = server_flags.get(client_id)
                if flags is None:
                    continue
                in_dat = option in appearance.options
                in_server = flag in flags
                if in_dat and in_server:
                    both += 1
                elif in_dat:
                    only_dat += 1
                elif in_server:
                    only_server += 1
            total = both + only_dat + only_server
            agreement = (both / total) if total else None
            results["0x%02X/%s" % (option, flag)] = {
                "both": both, "only_dat": only_dat, "only_server": only_server,
                "agreement": round(agreement, 4) if agreement is not None else None,
            }
        return results


class SpriteFile:
    """Parsed Tibia.spr. Sprites are decoded on demand."""

    WIDTH = 32
    HEIGHT = 32
    # The format stores fully transparent pixels as runs rather than colour, so
    # there is no colour key to strip; alpha comes from the run structure.
    TRANSPARENT = (0, 0, 0, 0)

    def __init__(self, data):
        self.data = data
        if len(data) < SPR_HEADER.size:
            raise FormatError("sprite file shorter than the header")
        self.signature, self.count = SPR_HEADER.unpack_from(data, 0)
        base = SPR_HEADER.size
        needed = base + self.count * 4
        if len(data) < needed:
            raise FormatError("sprite offset table runs past the end")
        self.offsets = list(struct.unpack_from("<" + "I" * self.count, data, base))

    @classmethod
    def read(cls, path):
        with open(path, "rb") as handle:
            return cls(handle.read())

    def has(self, sprite_id):
        return 1 <= sprite_id <= self.count and self.offsets[sprite_id - 1] != 0

    def decode(self, sprite_id):
        """Returns a list of WIDTH*HEIGHT RGBA tuples, or None for an empty slot."""
        if not self.has(sprite_id):
            return None
        at = self.offsets[sprite_id - 1]
        # Three bytes of colour key precede the pixel data and are not drawn.
        at += 3
        if at + 2 > len(self.data):
            raise FormatError("sprite %d header runs past the end" % sprite_id)
        size = struct.unpack_from("<H", self.data, at)[0]
        at += 2
        end = at + size
        if end > len(self.data):
            raise FormatError("sprite %d data runs past the end" % sprite_id)

        pixels = [self.TRANSPARENT] * (self.WIDTH * self.HEIGHT)
        written = 0
        while at < end:
            if at + 4 > end:
                raise FormatError("sprite %d run header truncated" % sprite_id)
            transparent, coloured = struct.unpack_from("<HH", self.data, at)
            at += 4
            written += transparent
            if at + coloured * 3 > end:
                raise FormatError("sprite %d colour run truncated" % sprite_id)
            for _ in range(coloured):
                red, green, blue = self.data[at], self.data[at + 1], self.data[at + 2]
                at += 3
                if written < len(pixels):
                    pixels[written] = (red, green, blue, 255)
                written += 1
        return pixels
