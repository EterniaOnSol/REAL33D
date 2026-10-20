# HANDOFF - UNREAL-OUTFIT-DIALOG-001

Date/time: 2026-10-20, America/Guatemala
Agent / role: Claude; implementation and operator-assisted live QA
Starting commit: `b8e7433baeddc2875943f519d33a5cb9380f3eaa` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-outfit-dialog-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261020-trade-ui-independent-certification.md`

OUTFIT_DIALOG_001_STATUS = PASS LOCAL
MAIN_MERGE = NONE. PUSH = NONE.

## Objective

The real 7.72 outfit chooser on the ClientCore outfit protocol
`CLIENTCORE-OUTFIT-772-001` built: send GET_OUTFIT, open only from the server's
answer, obey the server's band, preview and select an outfit, edit the four
colour bytes, send SET_OUTFIT, close without the acknowledgement that does not
exist, and report a failure honestly. Plus the two existing bridge gaps.
Isolated modal, minimal HUD edits, no protocol or server change.

## Discoveries worth keeping

- **The colour slot mapping is provable, and now proven.** This was the one open
  question in the task. Fusion32 cannot answer it — `cr.hh` line 21 is
  `uint8 Colors[4]` and every use is by index — but the classic 7.72 client can,
  statically. Its chooser builds four rows in one loop and selects each caption
  through a **four-entry jump table at `0x0047D648`** whose cases are literally
  `"Head"`, `"Body"`, `"Legs"`, `"Feet"` in index order; row `i` takes control id
  `1000 + i` and stores `i` as the selected slot; a swatch writes
  `colours[slot] = id - 2000`; and the sender writes opcode 211 then
  `colours[0..3]` from ascending offsets. **`Colors[0..3]` = Head, Body, Legs,
  Feet.** `tests/verify_outfit_color_mapping.py` asserts 43 byte patterns and
  fails closed; it is also a harness check. The binary is read, never executed.
- **The band cannot be computed client-side.** The 7.72 *game* protocol never
  carries this player's sex — it lives in the querymanager's character record
  and never reaches this connection. So `SV_CMD_OUTFIT`'s two trailing words are
  the only honest source, and nothing but that command may open a chooser.
- **Both bridge outfit copies were incomplete, and the fix is structural.** Each
  copied `outfit_id` and `disguised_as_object` and dropped the four colour bytes
  and a disguise's `object_type`, on every SV_CMD_CREATURE_OUTFIT and every
  FULLSCREEN creature. ClientCore stored the whole descriptor all along
  (`movement.cpp` line 1148). `FReal33DOutfit` plus one `ToUnrealOutfit` makes
  the gap impossible to reintroduce; three `static_assert`s pin the restated
  constants to ClientCore's.
- **The entry point is documented by the client itself.** Its help text at
  `0x00565734`: *"To change your outfit, right-click on your character and select
  'Set outfit' from the context menu."* That right-click was previously a no-op —
  `InteractUnderCursor` returns early for the local player.
- **A submit can never be confirmed.** `CSetOutfit` returns void and its
  `CREATURE_OUTFIT_CHANGED` broadcast is conditional on the player wearing their
  own outfit, so an invisible or illusioned player gets no 142 at all. The window
  closes on submit, which is also what the classic client does at `0x0047138B`.

## Changes

New: `Real33DOutfitSelection.h` (Slate-free rules), `Real33DOutfitPanel.h/.cpp`,
`tests/real33d_outfit_selection_tests.cpp`,
`tests/verify_outfit_color_mapping.py`,
`evidence/clientcore/UNREAL-OUTFIT-DIALOG-001.md` and its
`unreal-outfit-dialog-001/` directory.
Changed: `Real33DBridge.h/.cpp` (the outfit struct pair, one event kind, the 200
decode, two commands as intents with local validation, `NoteOutfit`, five
counters, three `static_assert`s, both gap fixes), `Real33DCreatureActor.h/.cpp`,
`Real33DWorldActor.cpp` (whole descriptor + five snapshot counters),
`Real33DHUD.h/.cpp` (+80 lines, no deletions), `Real33DPlayerController.cpp`,
`Real33DPanelRegistry.h/.cpp`, `Real33DPresentationTests.cpp`,
`tests/regression_harness.sh`, `tests/README.md`, `PARITY_MATRIX.md`,
`PROJECT_STATUS.md`.

No ClientCore file changed; the suite count stays 14.

## The defect the first live run found

Run 1 opened the chooser twice, obeyed the band, selected two outfits and got
both authoritative updates — but **both submits carried `78,69,58,76`, the
server's own `CheckOutfit` defaults.** No colour was ever changed.

Not a protocol or model bug: the outfit id *did* change across both submits, so
the draft and the bindings worked. The cause was this milestone's own UI. Each
colour row's **label was a button** whose only effect was to highlight the row —
copied from the 2D window, where the four rows select which one a *shared
palette* edits. This window gives every row its own arrows, so the label was a
control that looked live and was not, and the arrows went untouched.

And **nothing logged a colour step**, so "the operator did not press the arrows"
and "the arrows are broken" were indistinguishable in the evidence. Both fixed:
the label is plain text, the arrows are wider, and every preview step logs its
before/after value and the whole four-tuple.

## Tests

ClientCore 14/14, native 6/6, Unreal automation 8/8 with zero errors and
warnings, MSVC archive, REAL33DEditor build with zero warnings, colour-mapping
verifier 43/43, harness 10 PASS / 0 FAIL / 0 SKIP, harness selftest 10/10,
Git Bash secret_check 6/6.

Live run 2, one real account on REAL33D, operator-driven, nothing clicked by
software: 3 requests -> 3 server-opened choosers, `band=136..139` every time,
outfit previewed `137 -> 136 -> 139` wrapping at `First`, **47 colour steps
across all four slots (12/12/11/12)**, 3 submits, 3 authoritative updates for
creature 1002, **0 refusals, 0 residual bytes, 0 unsupported opcodes, 0
anomalies**.

**The reopen lines are the strongest evidence.** Submit `77,67,56,73` reopened as
`current=139 colours=77,67,56,73`; submit `76,66,55,72` reopened as
`76,66,55,72`. Exact, twice. That proves in one line that 211 encoded the
colours, Fusion32 stored all four, 200's decode carries them back, and the
colour bridge fix works — before it those bytes would have returned `0,0,0,0`.

Evidence: `evidence/clientcore/UNREAL-OUTFIT-DIALOG-001.md`, timelines in
`evidence/clientcore/unreal-outfit-dialog-001/`.

## Harness corrections

Two, and the second was a real defect. `real33d-native-tests` names its suites
explicitly rather than globbing — deliberate, so a deleted suite fails instead of
vanishing — which means a new suite must be added, and this was the first one to
prove the list can be forgotten. And the new `outfit-color-mapping` check
selected its interpreter with `command -v python3`, which on Windows resolves to
an **App Execution Alias** that exists on PATH, prints "Python was not found" and
exits 49; the check reported FAIL on a host with a working `python`.
`working_python()` now probes by running the interpreter. The harness runs 10
checks.

## Remaining UNVERIFIED

- **No outfit refusal occurred live**, locally or server-side: the controls
  cannot produce an out-of-band outfit or a colour above 132. So the 0 refusals
  is the absence of a refusal, not a handled one. The local-refusal path is wired
  to a `ClientNotice` and a counter and is covered by tests only.
- **The GAMEMASTER_OUTFIT path** — `CGetOutfit` answering *"You may not change
  your outfit."* with no SV_CMD_OUTFIT — was unreachable without editing server
  data, which was out of scope.
- **The colours change nothing on screen.** They are decoded, carried, stored on
  the actor, submitted and proven to round-trip, but `ResolveOutfit` selects a
  mesh from the outfit id alone and the palette's **byte-to-RGB mapping is not
  derived here**; it lives in the client's `.dat`/`.spr`. The chooser shows each
  slot's numeric index and no swatch rather than a fabricated colour.
- **The palette's true size.** 132 is the server's bound; the client's swatch
  guard spans 216 control ids but is a range test, not a count.
- No object disguise was seen live. Only the 136 band on one character, so the
  128 series and the premium `+3` were not seen live.

## Exact next task

`UNREAL-OUTFIT-PALETTE-001`, if the colours are wanted on screen: derive the
7.72 palette's byte-to-RGB table from `Tibia.dat`/`Tibia.spr` or the swatch
drawing code near `0x004712xx`, prove it the way the slot mapping was proven,
then tint in `UReal33DAssetRegistry` — never in an Actor and never in
Protocol772Core. Read `sending.cc::SendOutfit` and `Real33DOutfitSelection.h`
first. Until that table is proven, `SlotText` must keep showing the index.

Alternatively `UNREAL-OUTFIT-REFUSAL-QA-001`: reach a real refusal by driving
`UReal33DBridge::RequestSetOutfit` from an automation test rather than from the
panel, whose controls deliberately cannot produce one.

## Environment

Unchanged: VS2022 BuildTools MSVC 14.44.35207, UE 5.8; the harness selftest needs
`cl.exe` on PATH. `--objects-srv` must be a Git-Bash-style path (`/c/...`), not
`C:\...` — a Windows path reaches the WSL compiler unconverted and
`real33d_item_use_policy_tests` fails on `file.good()`.

**One server-side recovery, recorded in full.** Fusion32 would not start:
`Initialisierungsfehler: Game-Server is already running, PID file exists`. The
lock at `game/state/save/game.pid` named PID 562, which was not alive, with
nothing listening on 7171-7173 and no game process at all. It was **moved aside,
not deleted** — kept at `build/unreal-outfit-dialog-001/stale-game.pid.bak` — and
the server then started cleanly. No server data was edited and no gameplay state
was touched. The server was left running.
