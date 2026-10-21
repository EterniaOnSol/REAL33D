# HANDOFF - UNREAL-STACK-MOVE-CONTROLS-001

Date/time: 2026-10-21, America/Guatemala
Agent / role: Claude; implementation and live QA
Starting commit: `fbc0de992142e26cab0a80f813d2c09b10dc191a` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-stack-move-controls-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261021-unreal-outfit-color-rendering-001.md`

STACK_MOVE_CONTROLS_001_STATUS = **PASS**
MAIN_MERGE = NONE. PUSH = NONE. PROTOCOL/SERVER CHANGE = **NONE**.

## Objective and outcome

Match the classic stack-movement UX on every drag route. Delivered and proven
live against Fusion32:

```
plain drag   the whole stack
Shift+drag   one item
Ctrl+drag    ask how many, clamped to 1..the stack count
Cancel       sends nothing
```

Before this, a drag of a pile always moved the whole pile with no way to ask
for fewer, and a drag off the **ground** always sent a count of 1 — so a pile
of fifty coins on a tile moved one coin and left forty-nine behind.

## The rule has one home

`unreal/REAL33D/Source/REAL33D/Public/Real33DStackMove.h` holds the decision
and the clamp, free of Unreal types so `tests/real33d_stack_move_tests.cpp`
proves it with a plain compiler. All four routes — slot→slot, slot→ground,
ground→slot, ground→ground — end in `SReal33DHUD::RequestStackMove`.

Both bounds are the protocol's, not taste: `receiving.cc::CMoveObject` reads
one count byte and refuses a cumulative object whose count is zero, so every
answer folds into `1..min(stack, 255)`.

`FReal33DSlotRef` and the world drag carry `bHasAmount` from the wire beside
the count, because after the count is floored at one a single gold coin and a
sword read alike. That is what keeps the modifiers off a non-stackable
structurally.

The question is `SReal33DAmountPanel`, a centred modal registered like the
other three. Title is the classic client's own `Move Objects` at `0x00563C00`,
same table as `Select Outfit`'s `0x00563C78`, same hashed image.

## Two defects found while proving it, both fixed

1. **A modifier held before the press never reached the drag.**
   `MouseGesture::Press` returns `Look` for Shift and both
   `SReal33DSlot::OnMouseButtonDown` and `BeginLeftClick` returned there before
   arming the drag; Ctrl over the world hit the inspector branch and returned
   too; `EndLeftClick` completed a world drag only on a walk gesture. Holding
   Shift the natural way gave a look and **no drag at all**. Observed live as a
   20-pile Shift+drag that sent `count 20`. Look and inspect still fire first,
   unchanged; both now also arm the drag, and `EndLeftClick` completes whenever
   the pointer really travelled.

2. **Stack sprite tiers had never loaded.** The delivered art is
   `<id>_p<n>.png` (343 files); `ItemBrush` looked for `<id>_<n>.png` (zero
   files). `ItemFrameCount` therefore always returned 1 and every cumulative
   object drew its single-item picture at every amount — the exact defect
   `Real33DStackSprite.h` exists to fix, still live. `ItemFramePath` now
   accepts both spellings.

## Live QA

Account B, *Test Player B*, creature 1002. Gestures synthesised as real
OS-level input with hardware scan codes, so each went through the same Slate
path a hand would. Every required row PASS; see the table in
`evidence/clientcore/UNREAL-STACK-MOVE-CONTROLS-001.md`.

`moves_requested` rose by exactly one per intended gesture across every
snapshot (0, 2, 3, 3, 4, 5, 6, 7, 8, 9) — no stray or duplicated moves, and the
cancel produced none. EndPlay: 688 commands decoded, `residual_bytes` 0,
`unsupported_opcodes` 0, `protocol_anomalies` 0, `walks_unanswered` 0.

## Changes

New: `Real33DStackMove.h`, `Real33DAmountPanel.h/.cpp`,
`tests/real33d_stack_move_tests.cpp`,
`evidence/clientcore/UNREAL-STACK-MOVE-CONTROLS-001.md` and its
`unreal-stack-move-controls-001/` directory.
Changed: `Real33DHUD.h/.cpp`, `Real33DPanelChrome.h/.cpp`,
`Real33DPlayerController.h/.cpp`, `Real33DPanelRegistry.h/.cpp`,
`Real33DUIStyle.h/.cpp`, `Real33DInventoryPanel.cpp`,
`Real33DContainersPanel.cpp`, `Real33DPresentationTests.cpp`,
`tests/regression_harness.sh` (7th native suite), `tests/README.md`,
`PROJECT_STATUS.md`, `PARITY_MATRIX.md`.

**Nothing under `clientcore/` was modified. The suite count stays 14.**

## Tests

ClientCore 14/14 · native 7/7 · Unreal automation 10/10, 0 errors, 0 warnings ·
REAL33DEditor Win64 Development built · MSVC C++17 suites and C++20 archive ·
colour mapping 43/43 · palette 51/51 · harness **11 PASS / 0 FAIL / 0 SKIP**
over 103 scanned logs · secret_check 6/6.

## Remaining UNVERIFIED

- **A 3D pile on the ground is one mesh whatever the amount.** The asset
  registry resolves a mesh by type id with no amount tier, so twenty coins look
  like one. The 2D slot art now tiers; the world does not. This is the same
  world-mesh amount-invariance boundary the 2026-10-15 stackable certification
  already accepted, and it is an asset/registry question, not an input one.
- `kStackSpriteTiers` 10, 25, 50 stay UNVERIFIED against the client binary, as
  that header already says. The fix makes the tiers reachable; it does not
  prove those thresholds.
- Container↔container was proven between two slots of **one** open bag — both
  endpoints genuine container coordinates, but not two distinct container
  numbers. No second bag existed on the account and none was invented.
- No second client observed the same pile, so "authoritative counts both sides"
  is evidenced as source and destination counts from the server's own `F9`
  dumps, not as two observers.
- The amount question's bound is captured at the drop and not re-read when
  answered; a stack that shrank meanwhile yields a refused move. Not provoked.
- A shift+drag sends one look and one move, because the look is this client's
  existing press-time gesture. Accepted, not hidden.

## Exact next task

`UNREAL-STACK-MOVE-CONTROLS-002`, if wanted: a second open container, to prove
a move between two distinct container numbers rather than two slots of one bag.
Right-clicking the worn backpack a second time opens it under the next free
number, which is the cheapest route; a real second bag is better.

Separately and larger: `VISUAL-ITEM-STACK-MESH-001` — tier the **world** mesh
by amount in `UReal33DAssetRegistry`, so a pile on the ground reads as a pile.
`Real33DStackSprite.h` already holds the tier table and is the natural input.

## Environment

Fusion32 was started with `scripts/server/start_wsl.sh` and **stopped cleanly**
at the end of this milestone; all three services reported `All services stopped
cleanly`. One note worth carrying: mid-session the game service shut **itself**
down cleanly — full sector save (`8533464 Objekte gespeichert`),
`LogoutAllPlayers`, then `Reboot-Skript existiert nicht.` It was not caused by
any gesture here, and a restart recovered it with no data edited.
