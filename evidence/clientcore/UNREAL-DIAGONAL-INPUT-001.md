# UNREAL-DIAGONAL-INPUT-001

2026-10-22, America/Guatemala. Branch `milestone/unreal-diagonal-input-001`
from `origin/main` at `a868572864519b8497af30c5054b78d803b81826`.
No merge to main. No push. No protocol and no server change.

STATUS = **PASS** for the bindings, the gate and the authoritative movement.
One live row is UNVERIFIED and is named below.

## The audit came first, and it found nothing to preserve

`Q`, `E`, `Z` and `C` were **entirely unbound** in REAL33D: no `BindKey`, no
`BindAxisKey`, and the module overrides no Slate `OnKeyDown` or `OnKeyChar`
anywhere. The only behaviour they had was Slate's own text input, which still
works and is tested below. So nothing was taken away from them.

ClientCore already had everything needed on the wire —
`BuildDiagonalWalkCommand`, `DiagonalDirection`, opcodes 106..109,
`DiagonalStepPosition`, `PredictStepDelayMilliseconds` — and **none of it was
used anywhere in Unreal**. This milestone wires it up; it invents no opcode.

## The ledger had to learn what a diagonal is

`MovementLedger` was cardinal-only: `NoteLocalMove` compared the landing field
against `StepPosition(from, cardinal)`. A diagonal changes **both** axes, so
that comparison can never match one.

The consequence, had it been left alone, is specific and bad: every accepted
diagonal would have been counted an **external relocation** — the ledger's word
for "Fusion32 moved us and we did not ask" — and the request would then have
sat outstanding until it expired **unanswered**. One perfectly correct step
would have reported as two defects, in the two counters this project reads most
carefully.

So `WalkStep` now carries which of 7.72's two shapes a request was, and
`NoteRequestSent` has a `DiagonalDirection` overload. `direction` stays the
cardinal on both result structs, so every existing caller and test is
untouched. This is client bookkeeping; nothing about the wire changed.

Six new cases in the existing `movement_tests` suite cover it, so the ClientCore
suite count stays **14**: all four diagonals accepted, a diagonal never counted
external, the wrong field still refused, cardinals and diagonals interleaved in
order, a refused diagonal reported against its own direction, and `WalkStep`
agreeing with the standalone step functions.

## The direction numbering has one home now

The direction travelled through the bridge as a bare `uint8` that two files
printed from two copies of a four-entry table. Four more directions across two
copies is how they drift, and the walk journal would have filed every diagonal
as `none`. `Real33DWalkDirection.h` holds the numbering — cardinals keep the
server's `enums.hh` 0..3, diagonals sit at 4..7 in `DiagonalDirection` order —
and `tests/real33d_walk_direction_tests.cpp` proves it with a plain compiler.
It is this client's own numbering for its own events and is never put on the
wire.

## The keys are camera-relative, and that was a correction

They were first wired as **absolute** compass directions, reasoning that the
task names them by compass. That was wrong in the hand, and the operator caught
it live:

> *walking south while facing south, pressing Q or E sent the player north.*

W/A/S/D in this client are camera-relative through
`AReal33DWorld::CameraRelativeDirection`. The diagonal keys sit around them, so
an absolute Q meant the keys beside each other disagreed about where the player
was looking: W had turned with the camera and Q had not.

They are now relative corners — `E` forward-right, `C` back-right, `Z`
back-left, `Q` forward-left — resolved against the camera by
`CameraRelativeDiagonal`. Both enumerations turn clockwise and the corner
between cardinal C and cardinal C+1 is diagonal C, so the mapping is one
addition rather than a sixteen-entry table.

**At the default north-facing camera this reproduces the milestone's named
table exactly** — Q northwest, E northeast, Z southwest, C southeast — which is
the same orientation in which W means north.

## Live QA

Fusion32 from `scripts/server/start_wsl.sh`, all three services `VERIFIED`.
REAL33D run uncooked as account B, *Test Player B*, creature 1002. Keys were
synthesised as real OS-level input with hardware scan codes, so each went
through the same path a hand would.

**Attribution was the hard part and the method is part of the result.** The
machine had a second person working on it throughout, so counters alone could
not say whose walk was whose. Every measured step therefore records the highest
input id beforehand, waits for a quiet window, presses its keys, and accepts
the result **only** if the journal entries past that id are exactly the ones it
asked for, in order. Anything else is reported contaminated and retried. No
contaminated window is reported as a result anywhere below.

### Session D1 — the four diagonals, absolute build

| Key | Direction | From → To | Δ |
|---|---|---|---|
| Q | northwest | 32101,32205 → 32100,32204 | (−1,−1) |
| E | northeast | 32100,32204 → 32101,32203 | (+1,−1) |
| Z | southwest | 32101,32203 → 32100,32204 | (−1,+1) |
| C | southeast | 32100,32204 → 32101,32205 | (+1,+1) |

Ids 63–66, contiguous, one `sent` and one `accepted` each. The loop closes back
on the starting field, which it must: the four offsets sum to zero.

**Repeat** — four northwests, ids 67–70, 32101,32205 → 32097,32201, Δ(−4,−4).

**Alternating cardinal and diagonal** — ids 78–83: south accepted, **southeast
refused**, east accepted, northwest accepted, north accepted, southwest
accepted. The refusal is the blocked-diagonal row: position unchanged
(`from == to`), and it is reported against **southeast**, its own direction,
rather than a cardinal nobody asked for — which is the ledger change working.

**Chat focus** — in a verified quiet window: `Enter`, then Q, E, Z, C.
**Zero** new walk intents, position identical before and after, and the client
logged `chat input active; movement keys are inert` then `chat input released;
movement keys are live`. The box held `qezc` (`chat_typed_qezc.png`).

**End of session**: 849 commands decoded, 124 walks requested, 111 accepted,
13 refused, **0 unanswered**, **0 external relocations**,
`residual_bytes` 0, `unsupported_opcodes` 0, `protocol_anomalies` 0,
`last_diagnostic` `none`. 44 of those walks were diagonal.

Two facts worth stating plainly from that tally: `local_player_moves_total`
(111) equals `walks_accepted` (111), so **every** position change in the
session was attributed to a walk this client asked for and none fell through to
"external" — diagonals included; and **no input id was answered twice**, which
is the no-duplicate-packets requirement measured rather than asserted.

### Session D2 — the correction, facing south

Camera orbited ~180°, then the facing confirmed *by the server*: `W` walked
**south**. Then:

| Key | Direction | From → To | Δ |
|---|---|---|---|
| Q | **southeast** | 32088,32216 → 32089,32217 | (+1,+1) |
| E | **southwest** | 32089,32217 → 32088,32218 | (−1,+1) |

Both carry on southward. Before the fix these went north. Ids 35–36, validated.
End of session: 601 commands, residual/unsupported/anomalies all 0.

## Tests

ClientCore **14/14** (six new diagonal-ledger cases inside `movement_tests`) ·
native **8/8** (`real33d_walk_direction_tests`, 14 cases) ·
Unreal automation **10/10**, 0 errors, 0 warnings ·
REAL33DEditor Win64 Development built · MSVC C++17 suites and C++20 archive ·
harness **11 PASS / 0 FAIL / 0 SKIP** over 105 logs · secret_check 6/6.

## Remaining UNVERIFIED

- **All four corners at the default north-facing camera were not re-measured
  live on the camera-relative build.** The four-diagonal table above is from
  the absolute build, where north-facing output is identical by construction,
  and `FacingNorthReproducesTheTasksNamedTable` asserts that equality — but the
  live confirmation on the fixed build is of the south-facing pair only. Three
  attempts to re-measure were abandoned because the validator kept finding
  concurrent input; none of them is reported as a result.
- The diagonal delay rule is respected by **not** inventing a gate.
  `movement.h` is explicit that `PredictStepDelayMilliseconds` is a prediction
  that must never delay or refuse a step, and the four keys are `IE_Pressed`
  with no auto-repeat, so there is no interval to pace. Nothing in this client
  consults the x3 rule, and nothing new does.
- A shift/ctrl-modified diagonal was not explored; the keys ignore modifiers.
- Held-key diagonal movement does not exist by design (press-only, like the
  arrows), so no "walk while held" behaviour was tested.
- A **pre-existing** login-handover gap was observed and is recorded separately
  in `unreal-diagonal-input-001/OBSERVED-login-handover-initgame.md`:
  relaunching a session seconds after killing one makes the client log
  `SV_CMD_INIT_GAME is not decoded`. It fires on the first frame, before any
  walk, in a path this milestone does not touch, and both measured sessions
  were clean. That session measured nothing and is deliberately not carried as
  milestone evidence. Suggested follow-up: `UNREAL-LOGIN-HANDOVER-001`.
