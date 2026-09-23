"""Cuts the 7.72 item sprites out of Tibia.dat/Tibia.spr into one PNG per type id.

The Unreal client draws inventory and container contents by type id, which is
what Fusion32 puts on the wire. To draw the object rather than its number it
needs a picture per id, and the only 7.72-accurate source of those is the
client data pair the REAL33D 2D client already loads.

The .dat layout below is not guessed. It is transcribed from that client's own
parser, `src/client/thingtype.cpp::ThingType::unserialize`, and its attribute
numbering from `src/client/const.h::ThingAttr`, which is the code that reads
this exact file:

  * attributes are a stream of u8 opcodes terminated by 0xFF (ThingLastAttr);
  * for 7.55-7.72 the only renumbering that parser applies is 23 -> FloorChange,
    which carries no payload either way, so the 8.6 base table is used as is;
  * a handful of attributes carry a u16 payload, two carry two, and the rest
    carry none -- getting this wrong desynchronises every later item, which is
    why the list is taken from source rather than from memory;
  * then the sprite block: width, height, an extra size byte when the thing is
    larger than one tile, layers, three pattern counts and a phase count,
    followed by that many u16 sprite ids.

Only the first frame, first pattern and first layer is written: an inventory
square shows an object at rest, not an animation.

Objects wider or taller than one tile are composed onto a single image at their
full size and then fitted into the 32x32 square, because a two-tile object drawn
as only its first 32x32 corner is a picture of the wrong thing.

The output is deliberately not committed. Those pictures are derived from
CipSoft's client data, so they are CipSoft's; `.gitignore` keeps the whole
output directory out of the repository and this script is how a clone gets
them back from its own local copy of the data. The client does not require
them: an id with no picture is drawn as its number, which is still exactly
what the server sent.

  python scripts/client/extract_item_sprites.py [things-dir] [out-dir]
"""

import os
import struct
import sys
import zlib

SPRITE_PIXELS = 32
DEFAULT_THINGS = r"C:\Users\dell\Desktop\REAL33D2D\data\things\772"
DEFAULT_OUT = r"C:\Users\dell\Desktop\fusion32\unreal\REAL33D\Resources\UI\Items"

# ThingAttr values that are followed by payload bytes, from const.h. Anything
# not listed here is a bare flag.
ATTR_U16 = {0, 8, 9, 25, 28, 29, 32}          # Ground, Writable, WritableOnce,
                                              # Elevation, MinimapColor,
                                              # LensHelp, Cloth
ATTR_TWO_U16 = {21, 24}                       # Light, Displacement
ATTR_MARKET = 33                              # not present in 7.72, handled anyway
ATTR_LAST = 255


class Reader:
    def __init__(self, data):
        self.data = data
        self.at = 0

    def u8(self):
        value = self.data[self.at]
        self.at += 1
        return value

    def u16(self):
        value = struct.unpack_from("<H", self.data, self.at)[0]
        self.at += 2
        return value

    def u32(self):
        value = struct.unpack_from("<I", self.data, self.at)[0]
        self.at += 4
        return value

    def string(self):
        length = self.u16()
        text = self.data[self.at:self.at + length]
        self.at += length
        return text


def read_attributes(reader):
    """Walks the attribute stream. Returns nothing; it exists to advance past."""
    for _ in range(ATTR_LAST):
        attr = reader.u8()
        if attr == ATTR_LAST:
            return True
        if attr in ATTR_TWO_U16:
            reader.u16()
            reader.u16()
        elif attr == ATTR_MARKET:
            reader.u16()
            reader.u16()
            reader.u16()
            reader.string()
            reader.u16()
            reader.u16()
        elif attr in ATTR_U16:
            reader.u16()
    return False


def parse_dat(path):
    """type id -> (width, height, [sprite ids of the first frame])."""
    with open(path, "rb") as handle:
        reader = Reader(handle.read())

    reader.u32()                       # signature
    item_count = reader.u16()
    reader.u16()                       # outfits
    reader.u16()                       # effects
    reader.u16()                       # missiles

    items = {}
    # Items are numbered from 100: everything below is reserved by the client.
    for type_id in range(100, item_count + 1):
        if not read_attributes(reader):
            raise ValueError(f"attribute stream never terminated at id {type_id}")

        width = reader.u8()
        height = reader.u8()
        if width > 1 or height > 1:
            reader.u8()                # exact size
        layers = reader.u8()
        pattern_x = reader.u8()
        pattern_y = reader.u8()
        pattern_z = reader.u8()        # 7.55+
        phases = reader.u8()

        total = width * height * layers * pattern_x * pattern_y * pattern_z * phases
        sprites = [reader.u16() for _ in range(total)]

        # The first layer of the first pattern of the first phase: the object
        # standing still, which is what an inventory square shows.
        first = sprites[:width * height] if sprites else []
        items[type_id] = (width, height, first)
    return items


def parse_spr(path):
    """sprite id -> 32x32 RGBA bytes, decoded lazily from the offset table."""
    with open(path, "rb") as handle:
        data = handle.read()
    reader = Reader(data)
    reader.u32()                       # signature
    # u16, not u32. That client's SpriteManager::loadSpr reads a 32-bit count
    # only above version 960; 7.72 is a 16-bit count, and reading four bytes
    # here walks the offset table off the end of the file.
    count = reader.u16()
    offsets = [reader.u32() for _ in range(count)]
    return data, offsets


def decode_sprite(data, offset):
    """One sprite, as RGBA. Empty when the entry is a hole in the file."""
    pixels = bytearray(SPRITE_PIXELS * SPRITE_PIXELS * 4)
    if offset == 0:
        return pixels
    at = offset + 3                    # skip the colour key
    size = struct.unpack_from("<H", data, at)[0]
    at += 2
    end = at + size
    written = 0
    while at < end and written < SPRITE_PIXELS * SPRITE_PIXELS:
        transparent = struct.unpack_from("<H", data, at)[0]
        at += 2
        coloured = struct.unpack_from("<H", data, at)[0]
        at += 2
        written += transparent
        for _ in range(coloured):
            if written >= SPRITE_PIXELS * SPRITE_PIXELS:
                break
            base = written * 4
            pixels[base + 0] = data[at + 0]
            pixels[base + 1] = data[at + 1]
            pixels[base + 2] = data[at + 2]
            pixels[base + 3] = 255
            at += 3
            written += 1
    return pixels


def write_png(path, width, height, rgba):
    """A minimal RGBA PNG. Avoids a Pillow dependency for a 32x32 write."""
    raw = bytearray()
    stride = width * 4
    for row in range(height):
        raw.append(0)                  # filter: none
        raw.extend(rgba[row * stride:(row + 1) * stride])

    def chunk(tag, payload):
        out = struct.pack(">I", len(payload)) + tag + payload
        return out + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", header))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        handle.write(chunk(b"IEND", b""))


def compose(width, height, sprite_ids, data, offsets):
    """Lays the thing's tiles out into one image, bottom-right anchored.

    Tibia stores a multi-tile object from its bottom-right corner backwards,
    which is why the index walk below runs in reverse. Getting this wrong draws
    a large object with its quarters transposed.
    """
    full_w = width * SPRITE_PIXELS
    full_h = height * SPRITE_PIXELS
    canvas = bytearray(full_w * full_h * 4)
    index = 0
    for row in range(height):
        for column in range(width):
            if index >= len(sprite_ids):
                break
            sprite_id = sprite_ids[index]
            index += 1
            if sprite_id == 0 or sprite_id >= len(offsets):
                continue
            tile = decode_sprite(data, offsets[sprite_id])
            ox = (width - column - 1) * SPRITE_PIXELS
            oy = (height - row - 1) * SPRITE_PIXELS
            for y in range(SPRITE_PIXELS):
                src = y * SPRITE_PIXELS * 4
                dst = ((oy + y) * full_w + ox) * 4
                canvas[dst:dst + SPRITE_PIXELS * 4] = tile[src:src + SPRITE_PIXELS * 4]
    return full_w, full_h, canvas


def fit_to_square(width, height, rgba):
    """Box-downsamples a multi-tile object to one 32x32 square.

    Every output is the same size on purpose. A Slate brush backed by a file
    takes its declared size as the shape of the pixel buffer, so a mixture of
    32x32 and 64x64 files would need a per-id size table in C++ to be read
    correctly. One size removes that table and matches the slot the picture
    goes in. Averaging rather than dropping pixels because a 2:1 nearest
    downsample of pixel art throws away half the object.
    """
    if width == SPRITE_PIXELS and height == SPRITE_PIXELS:
        return rgba
    out = bytearray(SPRITE_PIXELS * SPRITE_PIXELS * 4)
    for y in range(SPRITE_PIXELS):
        for x in range(SPRITE_PIXELS):
            x0 = x * width // SPRITE_PIXELS
            x1 = max(x0 + 1, (x + 1) * width // SPRITE_PIXELS)
            y0 = y * height // SPRITE_PIXELS
            y1 = max(y0 + 1, (y + 1) * height // SPRITE_PIXELS)
            r = g = b = a = n = 0
            for sy in range(y0, y1):
                for sx in range(x0, x1):
                    base = (sy * width + sx) * 4
                    alpha = rgba[base + 3]
                    if alpha:
                        r += rgba[base]
                        g += rgba[base + 1]
                        b += rgba[base + 2]
                    a += alpha
                    n += 1
            if not n:
                continue
            solid = max(1, sum(1 for sy in range(y0, y1) for sx in range(x0, x1)
                               if rgba[(sy * width + sx) * 4 + 3]))
            dst = (y * SPRITE_PIXELS + x) * 4
            out[dst + 0] = r // solid
            out[dst + 1] = g // solid
            out[dst + 2] = b // solid
            out[dst + 3] = a // n
    return out


def main():
    things = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_THINGS
    out = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_OUT
    os.makedirs(out, exist_ok=True)

    items = parse_dat(os.path.join(things, "Tibia.dat"))
    data, offsets = parse_spr(os.path.join(things, "Tibia.spr"))
    print(f"dat: {len(items)} items, spr: {len(offsets)} sprites")

    written = 0
    blank = 0
    for type_id, (width, height, sprite_ids) in items.items():
        if not sprite_ids or all(s == 0 for s in sprite_ids):
            blank += 1
            continue
        full_w, full_h, canvas = compose(width, height, sprite_ids, data, offsets)
        if not any(canvas[3::4]):
            blank += 1
            continue
        square = fit_to_square(full_w, full_h, canvas)
        write_png(os.path.join(out, f"{type_id}.png"),
                  SPRITE_PIXELS, SPRITE_PIXELS, square)
        written += 1

    print(f"wrote {written} item sprites to {out} ({blank} had no picture)")


if __name__ == "__main__":
    main()
