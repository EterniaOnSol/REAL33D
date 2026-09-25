# REAL33D-COIN-STACK-VISUAL

State: `IMPLEMENTED_UNVERIFIED` (live visual check pending).

Operator report: gold coin stack amount was correct as a bottom-right number,
but a stack of two or three still displayed the one-coin icon.

Source: `REAL33D2D/src/client/item.cpp::Item::updatePatterns` (no version guard)
selects `(x,y)` patterns `(0..3,0)` at amounts 1..4, then `(0..3,1)` at
5, 10, 25, 50. `thingtype.cpp::ThingType::getSpriteIndex` places each pattern
after the previous pattern's layer and tile sprites. The selected local
7.72 `Tibia.dat` entry 3031 is stackable, 1x1, one layer, 4x2 patterns,
one phase, with sprite IDs 410..417. `Tibia.spr` supplies the pictures.

Cause: `scripts/client/extract_item_sprites.py::parse_dat` kept only the first
pattern; `FReal33DUIStyle::ItemBrush` keyed its PNG cache only by TypeId.

Change: the extractor writes seven additional, ignored local PNGs for every
stackable 4x2 type. `SReal33DSlot` passes the decoded amount to `ItemBrush`,
which chooses the same threshold pattern as the 2D client. The count label,
WorldState and protocol are unchanged. An unavailable variant falls back to
the base icon. No REAL33D2D, Fusion32 gameplay or proprietary sprite data is
committed.

Checks executed:

- `tests/verify_item_sprite_extraction.py` against the local 7.72 pair: PASS,
  4,990 items, 8,163 sprite pixels and 392 stack patterns compared, 0 failures.
- Extractor produced 4,913 base pictures and 343 stack variants. Gold coin
  `3031.png`, `3031_p1.png`, `3031_p2.png` were inspected and visibly contain
  one, two and three coins respectively.
- `REAL33DEditor Win64 Development` build: PASS.

Pending: in the running REAL33D session, split a real server-owned stack into
amounts 1, 2 and 3, confirm both picture and count in inventory/container;
then confirm a 5+ stack uses its corresponding variant. Do not mark PASS from
file inspection alone.
