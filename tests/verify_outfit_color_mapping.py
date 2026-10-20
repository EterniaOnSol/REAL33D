#!/usr/bin/env python3
"""Prove which body part each of the four 7.72 outfit colour bytes tints.

Fusion32 cannot answer this. `reference/game/src/cr.hh` line 21 declares
`uint8 Colors[4]` and every use in the server -- CheckOutfit, CSetOutfit's
bound test, SendOutfit's SendBytes -- addresses them by index only. The server
never names a body part, so the only authority in reach is the classic 7.72
client that talks to it, and the only way to read that authority without
running it is statically.

So this walks the local `Tibia.exe` and asserts, at fixed addresses, the six
links that together carry an index from a dialog row to a byte on the wire:

  1. the dialog builds four colour rows in a loop over i = 0..3, and a
     four-entry jump table selects the row's label from i;
  2. clicking row i stores i as the dialog's selected slot;
  3. clicking a swatch stores its value into colours[slot];
  4. SV_CMD_OUTFIT's four colours load into colours[0..3] in wire order;
  5. the OK button passes &colours[0] and the outfit id to the sender;
  6. the sender writes opcode 211, the id word, then colours[0..3] in
     ascending order.

Every address below is a file offset into the exact image recorded in
`docs/CLASSIC_CLIENT_772.md`; the image base is 0x400000 and `.text` maps
file+0x400000, so the listed VAs are directly comparable to a disassembler.

Read-only. The binary is opened, never executed -- AGENTS.md forbids running
legacy binaries, and nothing here needs to.

Usage:
    python tests/verify_outfit_color_mapping.py build/classic-client-772/app/Tibia.exe

Exits 0 and prints the mapping when every link holds, non-zero on the first
one that does not. A byte that does not match means this is a different image,
not a different mapping: re-derive before believing the printed result.
"""

from __future__ import annotations

import struct
import sys

IMAGE_BASE = 0x400000
EXPECTED_SHA256 = (
    "1A82FE97A39E536C29327D6D5DDE316775F537DCC01506831429B1A0433B76F8"
)

# The dialog object's own layout, as the instructions below read it.
COLORS_FIELD = 0x25C  # int colours[4], one per row
OUTFIT_FIELD = 0x26C  # the selected outfit id
SLOT_FIELD = 0x278  # which of the four rows is selected

SLOT_CONTROL_BASE = 0x3E8  # 1000 + i addresses row i
SWATCH_CONTROL_BASE = 0x7D0  # 2000 + value addresses a palette swatch

failures: list[str] = []
notes: list[str] = []


def check(name: str, ok: bool, detail: str = "") -> bool:
    """Record one link. Collects rather than raises, so one run reports all."""
    if ok:
        print(f"  PASS  {name}")
    else:
        print(f"  FAIL  {name}{('  -- ' + detail) if detail else ''}")
        failures.append(name)
    return ok


def main(path: str) -> int:
    with open(path, "rb") as handle:
        data = handle.read()

    import hashlib

    digest = hashlib.sha256(data).hexdigest().upper()
    print(f"image   {path}")
    print(f"bytes   {len(data)}")
    print(f"sha256  {digest}")
    if digest != EXPECTED_SHA256:
        print(
            "\nABORT: this is not the image these offsets were derived from.\n"
            f"expected {EXPECTED_SHA256}"
        )
        return 2

    def at(off: int, raw: str) -> bool:
        """True when `raw` hex sits at file offset `off`."""
        want = bytes.fromhex(raw.replace(" ", ""))
        return data[off:off + len(want)] == want

    def cstr(va: int) -> str:
        off = va - 0x55B000 + 0x15B000  # .rdata: VA 0x55B000 <- file 0x15B000
        return data[off:data.index(b"\0", off)].decode("latin-1")

    print("\n-- the dialog is the outfit chooser --")
    check("'Select Outfit' title present", at(0x163C78, "53 65 6C 65 63 74 20 4F 75 74 66 69 74 00"))
    check(
        "the window's own description names body parts",
        at(0x15CAA0, "43 68 6F 6F 73 65 20 61 6E 20 6F 75 74 66 69 74"),
    )

    print("\n-- link 1: four rows, i = 0..3, label chosen from i --")
    # 0x07D26C  xor edi,edi            i = 0
    # 0x07D270  cmp edi,4 / jge exit   loop bound
    # 0x07D279  cmp edi,3 / ja default guard
    # 0x07D285  jmp [edi*4+0x47D648]   the table
    check("i starts at zero", at(0x07D269, "33 FF EB 03"))
    check("loop runs while i < 4", at(0x07D270, "83 FF 04 0F 8D"))
    check("table guarded by i <= 3", at(0x07D279, "83 FF 03"))
    check("dispatch is jmp [edi*4 + 0x0047D648]", at(0x07D285, "FF 24 BD 48 D6 47 00"))
    check("i is incremented, then control returns to the loop top",
          at(0x07D334, "47 33 DB E9 34 FF FF FF"))

    # The four cases, each `mov dword [ebp+8], <label>` then a jmp to common code.
    cases = {
        0x0047D28C: ("Head", 0x563830),
        0x0047D295: ("Body", 0x563828),
        0x0047D29E: ("Legs", 0x563820),
        0x0047D2A7: ("Feet", 0x563818),
    }
    for va, (label, literal) in cases.items():
        off = va - IMAGE_BASE
        ok = at(off, "C7 45 08 " + struct.pack("<I", literal).hex(" "))
        check(f"case at 0x{va:08X} loads a literal", ok)
        if ok:
            check(f"  that literal is {label!r}", cstr(literal) == label,
                  f"found {cstr(literal)!r}")

    print("\n-- the jump table itself --")
    mapping: dict[int, str] = {}
    for index in range(4):
        target = struct.unpack_from("<I", data, 0x07D648 + index * 4)[0]
        entry = cases.get(target)
        if entry is None:
            check(f"index {index} -> known case", False, f"target 0x{target:08X}")
            continue
        mapping[index] = entry[0]
        print(f"  index {index} -> 0x{target:08X} = {entry[0]!r}")

    print("\n-- link 2: row i becomes the selected slot --")
    # 0x07D2C3  lea edx,[edi+0x3E8]        control id = 1000 + i
    # 0x07D303  mov [esi+edi*4+0x30], eax  rows[i] = widget, same i
    # 0x07128F  add eax,-0x3E8 / mov [esi+0x278],eax   slot = id - 1000
    check("row i gets control id 1000+i", at(0x07D2C3, "8D 97 E8 03 00 00"))
    check("row widget stored at rows[i]", at(0x07D302, "89 44 BE 30"))
    check("a click stores id-1000 as the slot",
          at(0x07128E, "05 18 FC FF FF 89 86 78 02 00 00"))

    print("\n-- link 3: a swatch writes colours[slot] --")
    # 0x0712CD  cmp eax,0x7D0 / 0x0712D4 cmp eax,0x8A7   swatch id guard
    # 0x0712DB  mov edx,[esi+0x278]                      edx = slot
    # 0x0712E1  add eax,-0x7D0                           value = id - 2000
    # 0x0712E6  mov [esi+edx*4+0x25C], eax               colours[slot] = value
    check("swatch ids guarded from below (2000)", at(0x0712C9, "3D D0 07 00 00"))
    check("swatch ids guarded from above (2215)", at(0x0712D0, "3D A7 08 00 00"))
    check("slot read back before the store", at(0x0712D7, "8B 96 78 02 00 00"))
    check("value is id - 2000", at(0x0712DD, "05 30 F8 FF FF"))
    check("store is colours[slot] = value", at(0x0712E2, "89 84 96 5C 02 00 00"))
    notes.append(
        "The swatch guard spans 2000..2215, which is 216 ids, while the server "
        "refuses any colour above 132 (receiving.cc lines 1440-1443). The guard "
        "is a control-id range test, not a count of populated swatches, so it "
        "does not establish how many colours the palette actually offers. "
        "Treat 132 as the bound; the palette's true size is UNVERIFIED here."
    )

    print("\n-- link 4: SV_CMD_OUTFIT loads colours[0..3] in wire order --")
    # 0x07D0ED  mov ecx,[eax+0x04] / mov [esi+0x25C],ecx
    # 0x07D0F6  mov edx,[eax+0x08] / mov [esi+0x260],edx
    # 0x07D105  mov ecx,[eax+0x0C] / mov [esi+0x264],ecx
    # 0x07D10E  mov edx,[eax+0x10] / mov [esi+0x268],edx
    inbound = [
        (0x07D0EF, "8B 48 04", 0x07D0F2, "89 8E 5C 02 00 00", 0),
        (0x07D0F8, "8B 50 08", 0x07D101, "89 96 60 02 00 00", 1),
        (0x07D107, "8B 48 0C", 0x07D10A, "89 8E 64 02 00 00", 2),
        (0x07D110, "8B 50 10", 0x07D113, "89 96 68 02 00 00", 3),
    ]
    for load_off, load, store_off, store, index in inbound:
        check(f"inbound colour {index} load", at(load_off, load))
        check(f"inbound colour {index} -> colours[{index}]", at(store_off, store))

    print("\n-- link 5: OK passes the array and the id to the sender --")
    # 0x071375  mov ecx,[esi+0x26C]        the outfit id
    # 0x07137B  lea eax,[esi+0x25C]        &colours[0]
    # 0x071383  call 0x00409830            SendSetOutfit(id, colours)
    check("id loaded from the dialog", at(0x071375, "8B 8E 6C 02 00 00"))
    check("&colours[0] taken", at(0x07137B, "8D 86 5C 02 00 00"))
    check("pushed as (colours, id)", at(0x071381, "50 51"))
    rel = struct.unpack_from("<i", data, 0x071384)[0]
    check("call targets the sender at 0x00409830",
          data[0x071383] == 0xE8 and 0x071388 + rel == 0x009830,
          f"resolved 0x{0x071388 + rel + IMAGE_BASE:08X}")

    print("\n-- link 6: the sender writes 211, the id, then colours[0..3] --")
    # 0x009850  mov esi,[ebp+0x0C]     esi = the colours argument
    # 0x00993F  push 0xD3              opcode 211
    # then [esi+0], [esi+4], [esi+8], [esi+0x0C] in that order
    check("second argument is the colours array", at(0x009850, "8B 75 0C"))
    check("opcode pushed is 211 (0xD3)", at(0x00993E, "68 D3 00 00 00"))
    check("the id is the first argument", at(0x00994F, "8B 45 08"))
    outbound = [
        (0x00995B, "8B 0E", 0),
        (0x009966, "8B 56 04", 1),
        (0x009972, "8B 46 08", 2),
        (0x00997E, "8B 4E 0C", 3),
    ]
    for off, raw, index in outbound:
        check(f"wire byte {index} reads colours[{index}]", at(off, raw))

    print("\n-- and the request side, for completeness --")
    check("GET_OUTFIT pushes 210 (0xD2)", at(0x0096B4, "68 D2 00 00 00"))

    print("\n" + "=" * 68)
    if failures:
        print(f"{len(failures)} link(s) FAILED: {', '.join(failures)}")
        print("The mapping below is NOT established. Keep indexed labels.")
        return 1

    print("COLOUR SLOT MAPPING: VERIFIED against the classic 7.72 client")
    for index in range(4):
        print(f"  Colors[{index}]  wire byte {index}  ->  {mapping[index]}")
    if notes:
        print("\nNoted, not claimed:")
        for note in notes:
            print(f"  - {note}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__)
        raise SystemExit(64)
    raise SystemExit(main(sys.argv[1]))
