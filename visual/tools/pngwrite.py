"""Minimal RGBA PNG writer.

Deliberately dependency free so a fresh clone can rebuild the reference pack
with nothing but a standard Python install. Pillow would do this better, but
requiring it would put an install step between the artist and the catalogue.
"""

import struct
import zlib


def _chunk(tag, payload):
    body = tag + payload
    return (struct.pack(">I", len(payload)) + body
            + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF))


def write_rgba(path, width, height, pixels):
    """pixels: a flat sequence of (r, g, b, a) tuples, row major."""
    if width <= 0 or height <= 0:
        raise ValueError("width and height must be positive")
    if len(pixels) != width * height:
        raise ValueError("expected %d pixels, got %d" % (width * height, len(pixels)))

    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter type 0, none
        row = pixels[y * width:(y + 1) * width]
        for red, green, blue, alpha in row:
            raw += bytes((red, green, blue, alpha))

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(_chunk(b"IHDR", header))
        handle.write(_chunk(b"IDAT", zlib.compress(bytes(raw), 6)))
        handle.write(_chunk(b"IEND", b""))


def scale_nearest(pixels, width, height, factor):
    """Nearest neighbour only. The reference must stay pixel exact."""
    if factor == 1:
        return pixels, width, height
    out = []
    for y in range(height):
        row = pixels[y * width:(y + 1) * width]
        scaled_row = []
        for pixel in row:
            scaled_row.extend([pixel] * factor)
        for _ in range(factor):
            out.extend(scaled_row)
    return out, width * factor, height * factor
