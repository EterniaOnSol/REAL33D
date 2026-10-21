# UNREAL-STACK-MOVE-CONTROLS-001

2026-10-21, America/Guatemala. Branch `milestone/unreal-stack-move-controls-001`
from `origin/main` at `fbc0de992142e26cab0a80f813d2c09b10dc191a`.
No merge to main. No push. No protocol and no server change.

STATUS = **PASS** for every row of the required matrix, live against Fusion32.

## What the milestone had to do

Match the classic stack-movement gesture:

```
plain drag   the whole stack
Shift+drag   one item
Ctrl+drag    ask how many, clamped to 1..the stack count
cancel       sends nothing
```

across inventory↔world, inventory↔container, container↔container and
world→inventory/container, with the server authoritative throughout and no
local inventory mutation.

## The rule, in one place

`unreal/REAL33D/Source/REAL33D/Public/Real33DStackMove.h` holds the whole
decision and the clamp, free of Unreal types so
`tests/real33d_stack_move_tests.cpp` proves it with a plain compiler. All four
drag routes end in `SReal33DHUD::RequestStackMove`, so the gesture cannot mean
one thing in a panel and another on the ground, and the clamp cannot be applied
in three places and forgotten in a fourth.

Two bounds come from the protocol, not from taste:
`reference/game/src/receiving.cc::CMoveObject` reads **one** count byte and
refuses a cumulative object whose count is zero, so every answer is folded into
`1..min(stack, 255)`.

`FReal33DSlotRef` and the world drag now carry `bHasAmount` from the wire
beside the count. After the count is floored at one for the move command, a
single gold coin and a sword read alike, and only the wire can say which is a
stack. That is what keeps the modifiers off a sword structurally rather than
by luck.

### The question

`SReal33DAmountPanel`, a centred modal registered like the other three. Its
title is the classic 7.72 client's own **`Move Objects`**, at `0x00563C00` in
the image `docs/CLASSIC_CLIENT_772.md` records — the same window-title table,
four strings earlier, that gave `Select Outfit` its `0x00563C78` for the outfit
milestone, against the same image hash the harness re-checks. `Ok` is that
image's generic dialog caption at `0x00563738`. The body wording and the
stepper layout are this client's own and are not claimed otherwise.

Cancel sends nothing: no move was ever offered to Fusion32, so there is nothing
to withdraw. Escape closes it the same way, ahead of the use-with banner in the
controller's Escape chain because it is the newer modal.

## Two defects found while proving it, and fixed

### 1. A modifier held *before* the press never reached the drag

`Real33D::MouseGesture::Press` returns `Look` for Shift, and both
`SReal33DSlot::OnMouseButtonDown` and
`AReal33DPlayerController::BeginLeftClick` returned at that branch before
arming the drag; Ctrl over the world hit the inspector branch and returned
too. `EndLeftClick` then completed a world drag only when the gesture had been
a walk.

So a player holding Shift the natural way — down first, then drag — got a look
and **no drag at all**, and from the ground the pile moved whole. The gesture
only worked if the modifier was pressed *after* the drag had already started.
This was live: `move 71 … count 20` in `live/B` is a Shift+drag of a 20-pile
that moved all twenty.

The look and the inspection still fire first, unchanged. Both now also arm the
drag, and `EndLeftClick` completes a drag whenever the pointer really
travelled, keeping the gesture test on the walk branch instead. Slate raises
`OnDragDetected` only once the pointer moves, so a shift+click that stays put
behaves exactly as before — proven live at `22.46.05`, where the probe click
still produced `look 3 at world point 32097,32209,7`.

A shift+drag therefore sends one look and one move. That is accepted rather
than hidden: the look is this client's existing press-time gesture and moving
it to release would change certified behaviour for no gain here.

### 2. Stack sprite tiers had never loaded

`extract_item_sprites.py` documents and writes `<id>_<n>.png`, and
`FReal33DUIStyle::ItemBrush` looked for exactly that. The art actually
delivered under `Resources/UI/Items/` is named `<id>_p<n>.png` — **343** such
files, and **zero** `<id>_<n>.png` among them. So the frame scan always stopped
at one, `StackSpriteFrame` was always handed a frame count of 1 and always
answered frame 0, and every cumulative object drew its single-item picture at
every amount.

Gold coin 3031 ships all eight pictures — one coin, two, three, four, a short
stack beside a loose coin, then three mounds — and a pile of twenty was drawn
as one coin. That is the exact defect `Real33DStackSprite.h` was written to fix
and it was still live. `ItemFramePath` now accepts both spellings rather than
declaring one right: the delivered art has to be read today, and a future run
of the extractor must not re-break it.

Confirmed on screen: the backpack draws the **mound** for 20 coins and a
**bundle** for the ×12, where both drew a single item before.

## Live QA

Fusion32 started from `scripts/server/start_wsl.sh`, all three services
`VERIFIED` on 7171/7172/7173. REAL33D run uncooked as account B, *Test Player
B*, creature 1002. Gestures were synthesised as real OS-level input — physical
button-down, real pointer travel past Slate's threshold, real modifier keys
with hardware scan codes, physical release — so every one went through the
same Slate path a hand would use. Nothing was invoked directly in code.

Snapshots are `F9` evidence dumps: the server's own inventory and container
contents, not the client's belief. All files in
`unreal-stack-move-controls-001/live-20261021/`.

Baseline `T0`: player 32098,32210,7; body 3=2854, 4=3379, 5=3294, 6=2815,
10=2920; backpack = 3031 ×20, 3607, 3492 ×12, 3457, 3003; `moves_requested` 0.

| Row | Gesture | Wire | Authoritative result | File |
|---|---|---|---|---|
| FULL_STACK | plain drag, 3492 ×12 → ground | `move 8 … count 12` | ×12 gone from the bag entirely | `S5` |
| SHIFT_ONE | **Shift held from the start**, ground pile ×20 → bag | `move 5 … count 1` | bag slot 0 = 1 coin; look 4 fired first | `S1_shift_one_world_to_slot` |
| CTRL_AMOUNT_DIALOG | **Ctrl held from the start**, ground ×19 → bag | `move amount asked: 1..19, opening on 19` | — | `s3_ctrl_dialog.png` |
| CLAMP | `>>` at 19/19; `<<` twice from 19; `>` | stayed **19 of 19**; landed **1 of 19** (no wrap); **2 of 19** | — | `clamp_strip.png` |
| partial amount | Ok at 2 of 19 | `move amount chosen: 2 of 19` → `move 6 … count 2` | bag holds a separate ×2 stack | `S2` |
| CANCEL | Ctrl+drag ×2, then Cancel | `abandoned … nothing reached Fusion32` | `moves_requested` stayed **3**; ×2 untouched | `S3` |
| CONTAINER_CONTAINER | Ctrl+drag, bag slot 1 → bag slot 6 | `1..1` → `move 7 … count 1` | both endpoints container coordinates | `S4` |
| INVENTORY_WORLD | sword body 5 → ground | `move 9 … count 1` | body slot 5 emptied | `S6a` |
| WORLD_DESTINATIONS | **Shift held**, sword ground → body 5 | `move 11 … count 1` | body slot 5 = 3294; non-stackable ignored Shift | `S6b` |
| INVENTORY_CONTAINER | **Ctrl held**, sword body 5 → bag slot 5 | `move 12 … count 1`, **no dialog** | bag index 0 = 3294 | `S7a` |
| INVENTORY_CONTAINER | bag slot 5 → body slot 5 | `move 13 … count 1` | body slot 5 filled | `S7b` |

Stack counts exercised: **1** (`1..1`, `move 7`), **2** (`1..2` cancelled,
`count 2` chosen), **12** and **19/20** (10+).

`moves_requested` rose by exactly one per intended gesture across every
snapshot — 0, 2, 3, 3, 4, 5, 6, 7, 8, 9 — so no gesture produced a stray or
duplicated move, and the cancel produced none at all.

### Protocol health

EndPlay snapshot: 366 frames, **688 server commands decoded**,
`residual_bytes` **0**, `unsupported_opcodes` **0**, `protocol_anomalies` **0**,
`walks_unanswered` **0**, `last_diagnostic` `"none"`. Every intermediate
snapshot carries the same four zeroes.

The harness `protocol-log-check` scanned 103 logs including all twelve of these
and reported no anomalies.

## Tests

ClientCore 14/14 · native 7/7 · Unreal automation **10/10**, 0 errors,
0 warnings, including the new `REAL33D.Presentation.StackMoveControls` ·
REAL33DEditor Win64 Development built, `Result: Succeeded` ·
MSVC C++17 suites and the C++20 archive both built ·
outfit colour mapping 43/43 · outfit palette 51/51 ·
harness **11 PASS / 0 FAIL / 0 SKIP** · secret_check 6/6.

## Remaining UNVERIFIED

- **On the ground a 3D pile is one mesh whatever the amount.** The asset
  registry resolves an object's mesh by type id with no amount tier, so twenty
  coins on a tile look exactly like one. The 2D slot art now tiers correctly;
  the 3D world does not, and making it do so is an asset and registry question
  with no bearing on what reaches the wire. Not attempted here, and not
  claimed. Photographed in `z_ground20.png`.
- `kStackSpriteTiers` 10, 25 and 50 remain UNVERIFIED against the client
  binary, exactly as `Real33DStackSprite.h` already says. The fix above makes
  the tiers reachable; it does not newly prove those three thresholds.
- Only one open container existed on this character, so container↔container was
  proven between two slots of one open bag — both endpoints genuine container
  coordinates on the wire, but not two distinct container *numbers*. A second
  bag was not available to this account and none was invented.
- A second client observing the same ground pile was not run. "Authoritative
  counts both sides" is evidenced here as source and destination counts read
  from the server's own `F9` dumps, not as two observers.
- The amount question's bound is captured when the drag is released and is
  deliberately not re-read when the answer arrives. A stack that shrank while
  the window was open yields a refused move, which changes nothing on screen.
  That path was not provoked live.
