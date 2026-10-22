# HANDOFF - UNREAL-DIAGONAL-INPUT-001

Date/time: 2026-10-22, America/Guatemala
Agent / role: Claude; implementation and live QA
Starting commit: `a868572864519b8497af30c5054b78d803b81826` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-diagonal-input-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261022-cert-stack-live-sendinput.md`

DIAGONAL_INPUT_001_STATUS = **PASS**
MAIN_MERGE = NONE. PUSH = NONE. PROTOCOL/SERVER CHANGE = **NONE**.

## Objective and outcome

Classic diagonal movement on Q/E/Z/C, over ClientCore's existing
`BuildDiagonalWalkCommand` and opcodes 106..109 — which existed and were used
nowhere in Unreal until now.

## Audit first, as the task required

`Q`, `E`, `Z`, `C` were **entirely unbound**: no `BindKey`, no `BindAxisKey`,
and the module overrides no Slate `OnKeyDown`/`OnKeyChar` anywhere. Nothing was
taken from them but Slate's own text input, which still works and is tested.

## Two things this needed that were not obvious

1. **The ledger had to learn what a diagonal is.** `MovementLedger` compared
   the landing field against `StepPosition(from, cardinal)`. A diagonal moves
   both axes, so every accepted diagonal would have been counted an **external
   relocation** and the request would then have expired **unanswered** — one
   correct step reported as two defects, in the two counters this project reads
   most carefully. `WalkStep` now carries which shape a request was;
   `direction` stays the cardinal so every existing caller and test is
   untouched. Client bookkeeping; the wire is unchanged. Six new cases inside
   the existing `movement_tests`, so ClientCore stays **14 suites**.

2. **The direction numbering needed one home.** It crossed the bridge as a bare
   `uint8` printed from two copies of a four-entry table; the walk journal would
   have filed every diagonal as `none`. `Real33DWalkDirection.h` now holds it,
   with a new native suite.

## The camera-relative correction

The keys were first wired as **absolute** compass directions. The operator
caught it live: *walking south while facing south, Q and E sent the player
north*, because W/A/S/D are camera-relative and the keys beside them were not.

They are now relative corners — E forward-right, C back-right, Z back-left,
Q forward-left — resolved by `CameraRelativeDiagonal`. At the default
north-facing camera this reproduces the named table exactly, which is the same
orientation in which W means north.

## Live QA, and the attribution method

A second person was working the machine throughout. Every measured step records
the highest input id beforehand, waits for a quiet window, presses, and accepts
the result **only** if the journal entries past that id are exactly the ones it
asked for, in order. Contaminated windows are retried, never reported.

D1: four diagonals (ids 63–66) with exact deltas and the loop closing; repeat
(67–70) Δ(−4,−4); alternating cardinal/diagonal (78–83) including a refused
southeast with position unchanged, reported against its own direction; chat
focus with **zero** walk intents and `qezc` in the box. End of session: 849
commands, 124 requested / 111 accepted / 13 refused / **0 unanswered** /
**0 external**, all protocol counters 0. `local_player_moves_total` equals
`walks_accepted`, and no input id was answered twice.

D2: facing confirmed by the server (W walked south), then Q → **southeast**,
E → **southwest**. Both southward; before the fix they went north.

## Tests

ClientCore 14/14 · native 8/8 · Unreal automation 10/10, 0 errors, 0 warnings ·
REAL33DEditor Win64 Development built · MSVC archive · harness **11 PASS /
0 FAIL / 0 SKIP** over 105 logs · secret_check 6/6.

## Remaining UNVERIFIED

- All four corners at the **default north-facing** camera were not re-measured
  live on the camera-relative build; the live confirmation there is the
  south-facing pair. Three attempts were abandoned to concurrent input rather
  than reported.
- The diagonal delay rule is respected by **not** inventing a gate:
  `movement.h` says the prediction must never delay or refuse a step, and the
  keys are `IE_Pressed` with no auto-repeat.
- Modified and held-key diagonals do not exist by design.
- A **pre-existing** login-handover gap is recorded in
  `evidence/clientcore/unreal-diagonal-input-001/OBSERVED-login-handover-initgame.md`.

## Exact next task

`UNREAL-DIAGONAL-INPUT-002` if wanted: re-measure the four corners at the
default facing on a machine nobody else is using, which is all that is missing.
Separately `UNREAL-LOGIN-HANDOVER-001` for the `SV_CMD_INIT_GAME` observation.

## Environment

Fusion32 was started with `scripts/server/start_wsl.sh` and **stopped cleanly**
(`All services stopped cleanly`). No server data was edited.
