# UNREAL-STACKABLE-SPRITE-PATTERNS-001

Status: `PASS`. A stackable object now draws the picture that matches the
amount Fusion32 reported, instead of always drawing the single-object
picture. No protocol, server or V08 production-art change.

Base `origin/main` at `1a15ddbd79a23a2841d882584afaa82fabd9f74d`, developed on
`milestone/unreal-stackable-sprite-patterns-001` in its own worktree.

## How it was found

Not by a test. During the NPC shop smoke test the operator looked at a pile of
gold in the backpack and said the picture was the same one a single coin uses.
It was.

## Root cause

`scripts/client/extract_item_sprites.py` took `sprites[:width * height]` — the
first frame only — for every object. That is right for an ordinary object,
whose other frames are animation phases an inventory square should not show.
It is wrong for a cumulative one, whose extra frames are *the pictures of
different amounts*.

Proven against the shipped client data rather than argued:

| type | pattern_x | pattern_y | sprites in `Tibia.dat` | extractor wrote |
|---|---|---|---|---|
| 3031 gold coin | 4 | 2 | **8** (410..417) | 1 (sprite 410) |
| 3267 dagger | 1 | 1 | 1 | 1 |
| 3294 short sword | 1 | 1 | 1 | 1 |

A non-stackable has exactly one picture, which is why nothing looked wrong
until someone counted coins. The amount itself was never lost: it arrives
correctly and was always drawn as the number beside the icon.

## The rule, and what backs each part of it

`unreal/REAL33D/Source/REAL33D/Public/Real33DStackSprite.h`.

```
amount   1  2  3  4  5..9  10..24  25..49  50+
frame    0  1  2  3   4      5       6      7
```

The three parts of that table have different evidential strength and the
header says so rather than presenting them as equally proven:

- **The number of pictures is read, never assumed.** It is `pattern_x *
  pattern_y` for the object in `Tibia.dat`, and at runtime it is the count of
  files the extractor actually wrote. Nothing is keyed to a type id.
- **Tiers 1..5 are confirmed by the art.** Decoding sprites 410..417 and
  looking at them shows literally one, two, three and four coins, then a short
  stack beside a loose coin. The contact sheet is reproducible with the probe
  described below.
- **Tiers 10, 25 and 50 are the documented 7.72 step values and are
  `UNVERIFIED` at byte level.** The art fixes their *order* — three
  progressively larger mounds — but not their thresholds, and no server data
  defines them, because this is a client presentation rule: Fusion32 only ever
  sends the amount. They are recorded as the convention, not as a proof.

Which objects are cumulative comes from the data: `Tibia.dat` attribute `0x05`,
whose server counterpart is the `Cumulative` flag in `objects.srv`.
`visual/tools/tibia772.py` already maps the two to each other, which is how
they are known to agree. **No gold-coin id and no 8-tier assumption is
hardcoded anywhere.** 49 cumulative objects gained frames from one run,
including platinum (3035) and crystal (3043) coins and spears (3277).

The rule is generic in frame count. An object with fewer pictures than the
table has tiers stops early and its last picture covers everything above it.
An object with more cannot have them addressed, because nothing here says what
amount they would stand for, and inventing thresholds would be worse than
leaving them unused.

## Changes

| File | Change |
|---|---|
| `unreal/.../Public/Real33DStackSprite.h` | new; the rule, header-only and free of Unreal types so a plain compiler can test it |
| `scripts/client/extract_item_sprites.py` | reads `ATTR_CUMULATIVE`; keeps every frame; writes `<id>.png` plus `<id>_1.png`.. for cumulative objects only |
| `unreal/.../Public/Real33DUIStyle.h`, `Private/Real33DUIStyle.cpp` | `ItemBrush(TypeId, Amount)`; `ItemFrameCount` probes the files once per id; cache keyed by id *and* frame |
| `unreal/.../Private/Real33DPanelChrome.cpp` | the slot asks for the picture of the amount it is already showing |
| `unreal/.../Public/Real33DBridge.h`, `Private/Real33DBridge.cpp` | `FReal33DThing` carries `bHasAmount`/`Amount` as received |
| `tests/real33d_stackable_sprite_tests.cpp` | new suite |
| `tests/regression_harness.sh` | registers the new suite so it actually gates |

`<id>.png` deliberately keeps its bare name, so every existing reader of those
files is unaffected and only the added amount pictures are suffixed.

### Inventory, container, world

- **Inventory** and **container** both already passed the authoritative count
  into the slot widget for the count text; that same count now also selects the
  picture. One call site, `Real33DPanelChrome.cpp`.
- **World**: `FReal33DVisual` is mesh plus material. The live 3D world has no
  sprite path for objects — one static mesh per type id — so there is no frame
  to choose between, and per-amount meshes would be new V08 art, which this
  milestone is explicitly not allowed to create. What changed is that the
  authoritative amount is now carried into `FReal33DThing` instead of being
  dropped at the bridge, so the world layer holds the truth and the follow-up
  below has what it needs. **A ground pile still renders as one mesh. That is
  stated, not quietly skipped.**

## Tests

`tests/real33d_stackable_sprite_tests.cpp`, compiled `-Wall -Wextra -Wpedantic
-Werror`:

- every tier boundary, from both sides — each tier begins at its own amount and
  one below still belongs to the previous picture — plus the whole table
  spelled out so a change to it has to be deliberate;
- count 1, and count 0 (what a non-cumulative object reports);
- high counts: 51, 100, 255 (the wire maximum, a u8) and `UINT32_MAX`;
- a non-stackable: one picture, frame 0 whatever the amount claims;
- malformed or missing pattern data: frame counts of 0, -1 and -1000 degrade to
  the first picture rather than indexing out of bounds; 2 and 3 frames stop
  early; 16 frames stay inside the object's own range;
- two properties over 0..300 amounts and 1..12 frame counts: the result is
  always a valid index, and it never decreases as the amount grows.

## Results

| Gate | Result |
|---|---|
| `real33d-native-tests` | **4 passed, 0 failed** (was 3; the new suite is registered and gating) |
| ClientCore CTest | **13/13 PASS**, unchanged |
| MSVC | **`WINDOWS CLIENTCORE: PASS`** |
| `REAL33DEditor Win64 Development` | **`Result: Succeeded`** |
| Regression harness | **`PASS_WITH_SKIPS`, 0 FAIL**; only `protocol-log-check` SKIP, no machine logs in scope |
| Unreal presentation suites | `CameraAndFloors`, `WideWorld.AuthoritativeWindow`, `AssetRegistry.ExperimentalV08` all `Success`, 0 errors |

### Live

`build/unreal-world-presentation-polish-001/live-20261015T201747`, account B
against the running Fusion32. The launcher regenerates the pictures with the
updated extractor, so the client under test used the new frames: *"49
cumulative object(s) contributed 351 extra amount picture(s)"*.

| Received state | Icon drawn | Rule says |
|---|---|---|
| `3031 x12` | coin mound, count text `12` | frame 5 (10 ≤ 12 < 25) |
| `3031 x1` | single coin, count text `1` | frame 0 |

Operator confirmed it independently: *"ahora se ve bien en la mochila"*. The
shovel and rope in the same panel are pixel-identical across both captures, so
non-stackables are visibly unaffected. `residual_bytes`, `unsupported_opcodes`
and `protocol_anomalies` are 0 in every snapshot of the session and the client
logged no error.

**A second stackable type was not live-tested: the character owns none.** Gold
coins were the only cumulative object available, and nothing was bought or
spawned to manufacture one. The generality is evidenced at the data layer
instead — 49 cumulative types produced frames from the same unmodified rule —
not by a second live case that was not obtained.

## Limits

- Tiers 10/25/50 remain `UNVERIFIED` against the client binary. See above.
- The 3D world still draws one mesh per type id regardless of amount.
- Nothing here changes the protocol, the server, or any V08 asset.

## Found but deliberately not fixed here

Two further defects the operator reported during this work. Both are real and
reproducible; neither belongs in a sprite milestone.

1. **A stack is moved one object at a time.** `Real33DPlayerController.cpp`
   hardcodes `count=1` on both world-drag routes, while `DropInventoryOnWorld`
   correctly uses the item's real count. The live log shows the asymmetry
   plainly: `slot-to-world drag 32: object=3031 ... count=4` against
   `world-to-slot drag 34: object=3031 ... count=1`, which is why picking a
   pile up takes one drag per coin. The operator's requested behaviour, which
   is the 7.72 client's:
   - plain drag moves the **whole stack**;
   - **Shift** + drag moves exactly **one**;
   - **Ctrl** + drag opens a **"how many?"** amount dialog.

   The data gap behind it is already closed by this milestone:
   `FReal33DThing` now carries the amount.

2. **No diagonal movement keys.** ClientCore gained the four diagonal walk
   commands on main (`DiagonalWalkCommandOpcode`, opcodes 106..109), but the
   Unreal client binds only W/A/S/D and the arrows. The operator asked for
   **Q, E, Z and C** to walk the diagonals.
