"""Generate local appearance-color metadata from the selected 7.72 DAT.

Uses REAL33D's independently validated appearance reader, never a 2D runtime
parser/model. Output contains appearance colors, no map positions or entities.
Keep the derived output in ignored Saved/Minimap; do not redistribute the DAT.
"""
import argparse
import hashlib
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "visual" / "tools"))
from tibia772 import AppearanceFile, KIND_ITEM

SELECTED_DAT = "3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dat", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = args.dat.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != SELECTED_DAT:
        raise ValueError("DAT is not the selected CLASSIC-CLIENT-772-001 artifact")
    appearances = AppearanceFile.parse(data)  # Requires exact EOF consumption.
    lines = ["REAL33D_MINIMAP_PALETTE_1 " + digest]
    for type_id, appearance in sorted(appearances.by_kind[KIND_ITEM].items()):
        if 0x1C not in appearance.options:
            continue
        color = appearance.options[0x1C][0]
        if color == 0:
            continue  # No color hint; use the explicit generic fallback.
        if color >= 216:
            raise ValueError("ground minimap color exceeds the 6x6x6 palette")
        # The classic 216-color cube: red major, green middle, blue minor.
        lines.append(f"{type_id} {color}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"DAT_SHA256={digest}\nEOF_PARSE=PASS\nAPPEARANCE_COLORS={len(lines)-1}")


if __name__ == "__main__":
    main()
