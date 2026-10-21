#!/usr/bin/env python3
"""Derive the 7.72 outfit colour palette from the classic client, exactly.

The companion to `verify_outfit_color_mapping.py`. That one proved WHICH body
part each of the four colour bytes tints; this one proves WHAT COLOUR each byte
value is.

Fusion32 cannot answer either question. It validates `Colors[n] > 132` and
stores the byte, and nothing in the server turns a colour index into a colour.
So the authority is again the classic 7.72 client, read statically: the binary
is opened, never executed.

It is not a table. The client computes every swatch from the index, and this
reproduces that computation instruction for instruction:

  reference/game -- nothing. The server has no palette.
  Tibia.exe 0x0047D36F   the palette loop: 133 swatches, i = 0..132,
                         control id 2000 + i, each colour fetched by
                         calling 0x00509280 with the index.
  Tibia.exe 0x00509280   index -> (hue, saturation, value):
                             row = index / 19 ; col = index % 19
                             col == 0 -> h = 0, s = 0, v = 1 - row/7
                             else     -> h = col/18, (s, v) = ROW_SV[row]
  Tibia.exe 0x00509080   HSV -> RGB, six 1/6-wide sectors with the
                         min = (1 - s) * v formulation, each channel
                         rounded after multiplying by 255.

Every constant below was read out of that image at the address named beside it,
and all six sectors of the conversion were decoded from the disassembly rather
than assumed from the shape of the constants. 19 * 7 = 133, which is exactly
the band `receiving.cc` lines 1440-1443 accept: 0..132.

Usage:
    python tests/verify_outfit_palette.py build/classic-client-772/app/Tibia.exe
    python tests/verify_outfit_palette.py <exe> --dump    # all 133 as a table

Exits 0 when every structural assertion against the image holds and the
generated palette satisfies the invariants the binary guarantees.
"""

from __future__ import annotations

import hashlib
import struct
import sys

IMAGE_BASE = 0x400000
EXPECTED_SHA256 = (
    "1A82FE97A39E536C29327D6D5DDE316775F537DCC01506831429B1A0433B76F8"
)

# --- the numbers, each at the address it was read from ----------------------
PALETTE_SIZE = 133      # cmp edi, 0x85 at 0x0047D37C
COLUMNS = 19            # mov ecx, 0x13 at 0x00509296, then idiv
ROWS = 7                # 133 / 19, and the 7-entry jump table at 0x005093B4
HUE_STEP = 1 / 18       # 0x00573F48, multiplied by the column
GREY_STEP = 1 / 7       # 0x00573F50, multiplied by the row
CHANNEL_MAX = 255.0     # 0x00573F28

# Saturation and value per row, in row order. Each row's case at
# 0x0050930F/21/33/45/57/69/7B loads these two constants in this order; the
# first becomes the saturation argument and the second the value argument.
ROW_SV = (
    (0.25, 1.0),    # row 0: 0x00573F40, 0x0056F110
    (0.25, 0.75),   # row 1: 0x00573F40, 0x00573F38
    (0.5, 0.75),    # row 2: 0x0056F478, 0x00573F38
    (0.667, 0.75),  # row 3: 0x00573F30, 0x00573F38
    (1.0, 1.0),     # row 4: 0x0056F110, 0x0056F110
    (1.0, 0.75),    # row 5: 0x0056F110, 0x00573F38
    (1.0, 0.5),     # row 6: 0x0056F110, 0x0056F478
)

failures: list[str] = []


def check(name: str, ok: bool, detail: str = "") -> bool:
    if ok:
        print(f"  PASS  {name}")
    else:
        print(f"  FAIL  {name}{('  -- ' + detail) if detail else ''}")
        failures.append(name)
    return ok


# --- the conversion, as 0x00509080 performs it ------------------------------

def hsv_to_rgb(h: float, s: float, v: float) -> tuple[float, float, float]:
    """Six-sector HSV, exactly as 0x00509080 computes it.

    Two early exits first, both in the binary: `v == 0` returns black before
    anything else, and `s == 0` returns `v` on all three channels without
    touching the hue. The sector bodies then all share one shape -- the low
    channel is `(1 - s) * v`, the high channel is `v`, and the third ramps
    linearly across the sector by `6 * s * v * (h - sector_start)`.
    """
    if v == 0.0:
        return 0.0, 0.0, 0.0
    if s == 0.0:
        return v, v, v

    low = (1.0 - s) * v
    span = 6.0 * s * v

    # Sector boundaries 1/6..5/6 are the five fcomp constants at 0x00573F20,
    # 0x00573F10, 0x0056F478, 0x00573F08 and 0x00573F00, tested in that order.
    if h < 1 / 6:            # 0x004090FE  red high, green rising
        return v, low + span * (h - 0.0), low
    if h < 2 / 6:            # 0x00509136  green high, red falling
        return v - span * (h - 1 / 6), v, low
    if h < 3 / 6:            # 0x00509172  green high, blue rising
        return low, v, low + span * (h - 2 / 6)
    if h < 4 / 6:            # 0x005091B1  blue high, green falling
        return low, v - span * (h - 3 / 6), v
    if h < 5 / 6:            # 0x005091EE  blue high, red rising
        return low + span * (h - 4 / 6), low, v
    return v, low, v - span * (h - 5 / 6)   # 0x00509219  red high, blue falling


def palette_color(index: int) -> tuple[int, int, int]:
    """The RGB the classic client draws for one outfit colour byte."""
    if index >= PALETTE_SIZE:
        # The binary's own guard: `cmp eax, 0x85 / jl / xor eax, eax`. An index
        # it does not recognise becomes zero rather than reading past the grid.
        index = 0
    row, col = divmod(index, COLUMNS)
    if col == 0:
        # The greyscale column. s == 0, so the hue is never consulted.
        h, s, v = 0.0, 0.0, 1.0 - row * GREY_STEP
    else:
        h = col * HUE_STEP
        s, v = ROW_SV[row]
    r, g, b = hsv_to_rgb(h, s, v)
    # `fmul 255.0` then the rounding helper, once per channel.
    return (round(r * CHANNEL_MAX), round(g * CHANNEL_MAX), round(b * CHANNEL_MAX))


# --- structural assertions against the image --------------------------------

def main(path: str, dump: bool) -> int:
    with open(path, "rb") as fh:
        data = fh.read()

    digest = hashlib.sha256(data).hexdigest().upper()
    print(f"image   {path}")
    print(f"sha256  {digest}")
    if digest != EXPECTED_SHA256:
        print("\nABORT: not the image these offsets were derived from.")
        return 2

    def at(off: int, raw: str) -> bool:
        want = bytes.fromhex(raw.replace(" ", ""))
        return data[off:off + len(want)] == want

    def dbl(va: int) -> float:
        return struct.unpack_from("<d", data, va - 0x55B000 + 0x15B000)[0]

    print("\n-- the palette loop --")
    check("133 swatches, i = 0..132", at(0x07D380, "81 FF 85 00 00 00"))
    check("control id is 2000 + i", at(0x07D3B6, "8D 97 D0 07 00 00"))
    rel = struct.unpack_from("<i", data, 0x07D3AF)[0]
    check("each colour comes from 0x00509280",
          data[0x07D3AE] == 0xE8 and 0x07D3B3 + rel == 0x109280)

    print("\n-- index -> hue, saturation, value --")
    check("the grid is 19 wide", at(0x109296, "B9 13 00 00 00"))
    check("index is divided by it", at(0x10929B, "F7 F9"))
    check("column 0 branches to greyscale", at(0x10929D, "85 D2") and at(0x1092A6, "75 41"))
    check("greyscale value is 1 - row/7",
          at(0x1092B4, "DC 0D 50 3F 57 00") and at(0x1092BD, "DC 2D 10 F1 56 00"))
    check("chromatic hue is column/18", at(0x1092F0, "DC 0D 48 3F 57 00"))
    check("the row selects (s, v) through a 7-entry table",
          at(0x1092E9, "83 F8 06") and at(0x109308, "FF 24 85 B4 93 50 00"))

    print("\n-- the constants, read from the image --")
    for va, want, name in (
        (0x00573F50, GREY_STEP, "1/7 grey step"),
        (0x00573F48, HUE_STEP, "1/18 hue step"),
        (0x00573F28, CHANNEL_MAX, "255 channel scale"),
        (0x00573F18, 6.0, "6.0 sector span"),
        (0x00573F20, 1 / 6, "1/6 sector 1 bound"),
        (0x00573F10, 2 / 6, "2/6 sector 2 bound"),
        (0x0056F478, 0.5, "0.5 sector 3 bound"),
        (0x00573F08, 4 / 6, "4/6 sector 4 bound"),
        (0x00573F00, 5 / 6, "5/6 sector 5 bound"),
        (0x00573F40, 0.25, "0.25 row saturation"),
        (0x00573F38, 0.75, "0.75 row value"),
        (0x00573F30, 0.667, "0.667 row saturation"),
        (0x0056F110, 1.0, "1.0"),
        (0x0055D8C8, 0.0, "0.0"),
    ):
        got = dbl(va)
        check(f"0x{va:08X} is {name}", abs(got - want) < 1e-12, f"found {got!r}")

    print("\n-- the row (s, v) table --")
    rowcases = (0x10930F, 0x109321, 0x109333, 0x109345, 0x109357, 0x109369, 0x10937B)
    for row, case in enumerate(rowcases):
        target = struct.unpack_from("<I", data, 0x1093B4 + row * 4)[0]
        check(f"row {row} dispatches to 0x{case + IMAGE_BASE:08X}",
              target == case + IMAGE_BASE, f"table says 0x{target:08X}")
        # Each case drops the two defaults then loads its own two constants.
        ok = at(case, "DD D8 DD D8")
        sat_va = struct.unpack_from("<I", data, case + 6)[0]
        val_va = struct.unpack_from("<I", data, case + 12)[0]
        check(f"row {row} loads (s, v) = ({ROW_SV[row][0]}, {ROW_SV[row][1]})",
              ok and abs(dbl(sat_va) - ROW_SV[row][0]) < 1e-12
              and abs(dbl(val_va) - ROW_SV[row][1]) < 1e-12,
              f"read ({dbl(sat_va)!r}, {dbl(val_va)!r})")

    print("\n-- the conversion's two early exits --")
    check("v == 0 returns black", at(0x1090A8, "C7 00 00 00 00 00"))
    check("s == 0 returns v on all three channels",
          at(0x1090D0, "DC 0D 28 3F 57 00") and at(0x1090E1, "89 01"))

    print("\n-- invariants the generated palette must satisfy --")
    grid = [palette_color(i) for i in range(PALETTE_SIZE)]
    check("the palette has 133 entries", len(grid) == PALETTE_SIZE)
    check("index 0 is white", grid[0] == (255, 255, 255), f"got {grid[0]}")
    greys = [grid[row * COLUMNS] for row in range(ROWS)]
    check("column 0 is greyscale on every row",
          all(r == g == b for r, g, b in greys), f"got {greys}")
    check("the greyscale column descends",
          all(greys[i][0] > greys[i + 1][0] for i in range(ROWS - 1)), f"got {greys}")
    check("every channel stays in 0..255",
          all(0 <= c <= 255 for px in grid for c in px))
    # Row 4 is fully saturated at full value, so its hue ramp must hit the
    # three primaries and the three secondaries exactly.
    row4 = {grid[4 * COLUMNS + col] for col in range(1, COLUMNS)}
    for name, px in (("red", (255, 0, 0)), ("yellow", (255, 255, 0)),
                     ("green", (0, 255, 0)), ("cyan", (0, 255, 255)),
                     ("blue", (0, 0, 255)), ("magenta", (255, 0, 255))):
        check(f"row 4 contains pure {name}", px in row4)
    check("an out-of-range index folds to index 0",
          palette_color(133) == grid[0] and palette_color(255) == grid[0])

    print("\n" + "=" * 68)
    if failures:
        print(f"{len(failures)} assertion(s) FAILED: {', '.join(failures)}")
        print("The palette below is NOT established.")
        return 1
    print(f"OUTFIT PALETTE: VERIFIED -- {PALETTE_SIZE} colours, "
          f"{ROWS} rows x {COLUMNS} columns, derived not tabulated")

    if dump:
        print("\nindex  r    g    b     hex       row col")
        for i, (r, g, b) in enumerate(grid):
            row, col = divmod(i, COLUMNS)
            print(f"{i:5d}  {r:3d}  {g:3d}  {b:3d}   #{r:02X}{g:02X}{b:02X}   "
                  f"{row}   {col}")
        print("\nThe four colours CheckOutfit gives a fresh character:")
        for name, idx in (("Head", 78), ("Body", 69), ("Legs", 58), ("Feet", 76)):
            r, g, b = grid[idx]
            print(f"  {name:<5} {idx:3d} -> #{r:02X}{g:02X}{b:02X}  ({r},{g},{b})")
    return 0


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 1:
        print(__doc__)
        raise SystemExit(64)
    raise SystemExit(main(args[0], "--dump" in sys.argv))
