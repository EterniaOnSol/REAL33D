# CLIENTCORE-MOVEMENT-ACTIONS-772-001

Status: `PASS` for the deterministic protocol layer. **Not** `CERTIFIED`: no
live session has walked a diagonal, rotated an object or used the parent
arrow.

ClientCore only. No Unreal, no HUD, no input, no gameplay rule, no Fusion32
server or protocol change, no 2D, no agent, no V08 asset, and no document
another milestone owns. Base `origin/main` at
`fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, on
`milestone/clientcore-movement-actions-772-001` in its own worktree,
concurrent with and disjoint from
`milestone/unreal-world-presentation-polish-001`. No main merge.

Seven client commands, none of which has a server command of its own: every
answer arrives on a path this project already decodes.

## Layouts, traced

| Command | Source | Layout |
|---|---|---|
| `CL_CMD_GO_NORTHEAST` (106) | `receiving.cc::ReceiveData:1735` → `CGoDirection(Connection, 1, -1)` | **no payload** |
| `CL_CMD_GO_SOUTHEAST` (107) | `:1736` → `CGoDirection(Connection, 1, 1)` | no payload |
| `CL_CMD_GO_SOUTHWEST` (108) | `:1737` → `CGoDirection(Connection, -1, 1)` | no payload |
| `CL_CMD_GO_NORTHWEST` (109) | `:1738` → `CGoDirection(Connection, -1, -1)` | no payload |
| `CL_CMD_TURN_OBJECT` (133) | `receiving.cc::CTurnObject` | `word x`, `word y`, `byte z`, `word TypeID`, `byte RNum` |
| `CL_CMD_CLOSE_CONTAINER` (135) | `receiving.cc::CCloseContainer` | `byte ContainerNr` |
| `CL_CMD_UP_CONTAINER` (136) | `receiving.cc::CUpContainer` | `byte ContainerNr` |

The diagonals carry no payload for the same reason the cardinals do not:
`CGoDirection` takes its offsets from the dispatcher and never reads the
buffer. Its signature has no `TReadBuffer*` at all.

Four details the source settles, each pinned by a test:

1. **`CTurnObject` does not overwrite the stack index.** `CMoveObject` and
   `CTradeObject` both force `RNum = 1` for a map object; `CTurnObject` does
   not, so the index sent is the index the server resolves with. A test
   asserts a stack index of 9 survives into byte 8.

2. **`CUpContainer` answers in two different ways.** The parent arrow is not
   a simple "show the parent": `CUpContainer` sends `SendContainer` with the
   parent when the parent is an ordinary container, and `SendCloseContainer`
   when the parent is a map or body container, because there is nothing left
   to show in the window. A client that assumed one answer would leave a
   stale window open. Both answers are already decoded by
   `UNREAL-INVENTORY-CONTAINERS-001`.

3. **Sixteen container numbers, and the server ignores the rest.**
   `cr.hh:917` is `Object OpenContainer[16]`, and both handlers test
   `ContainerNr < NARRAY(...)` and otherwise do nothing at all. The builders
   pass the byte through untouched rather than clamping, so a caller
   asserting against a capture sees exactly what it asked for.

4. **`CTurnObject` has an open question in its own source.** It carries
   `// TODO(fusion): Check Type.getFlag(ROTATE)?`, so a non-rotatable object
   is Fusion32's to refuse, not this layer's. As elsewhere, the builder also
   does not pre-apply the `isMapContainer()` refusal.

## DIAGONAL_DELAY_RULE

Two separate diagonal rules exist in `cract.cc`, and both are documented and
tested here. Neither changes gameplay: nothing in this client gates, delays
or refuses a step.

**Rule 1 — triple delay.** `cract.cc:1453-1463`:

```
// NOTE(fusion): Diagonal movement has three times the delay of a regular one.
if(DiagonalMove){
    Waypoints *= 3;
}
int Delay = (Waypoints * 1000) / this->GetSpeed();
int BeatCount = (Delay + Beat - 1) / Beat;
this->EarliestWalkTime = ServerMilliseconds + BeatCount * Beat;
```

`Waypoints` is the `WAYPOINTS` attribute of the destination field's `BANK`
object (`:1446-1448`); `Beat` defaults to 200 (`config.cc:100`) and is
script-overridable (`config.cc:183`), so it is a parameter of
`PredictStepDelayMilliseconds` rather than a baked constant.

The non-obvious part is **ordering**: the ×3 lands on `Waypoints`, before the
division and before the quantisation to a whole beat. The final interval is
therefore *not* generally three times the cardinal one. At 100 waypoints and
speed 220 the cardinal step quantises to 600 ms and the diagonal to 1400 ms,
not 1800. A client that multiplied the finished number would be wrong, and
that exact pair is asserted, along with the identity
`Predict(100, 220, diagonal) == Predict(300, 220, cardinal)` — which is
literally what the server computes.

**Rule 2 — a triple-delay diagonal must not change floor.** `cract.cc:1341`:

```
bool DiagonalMove = (DestX != OrigX && DestY != OrigY && DestZ == OrigZ);
```

Both axes must change **and the floor must not**. A diagonal that also
changes floor is not charged triple. `IsTripleDelayDiagonal` encodes this and
the floor boundary is asserted in both directions, because getting it
backwards would overstate every staircase step.

A third diagonal behaviour is recorded but not modelled: at
`cract.cc:368-369`, when a move is impossible the automatic one-floor climb
applies only when `!DiagonalMove`, so a blocked diagonal simply fails and
never resolves into a floor change. That is server pathing, observable only
live, and nothing here predicts it.

`PredictStepDelayMilliseconds` is explicitly a **prediction, never an
authority**. It exists so a caller that wants to pace input or draw a
cooldown uses the server's real arithmetic instead of inventing one, and so
the ×3 rule is written down somewhere testable. It returns 0 for a
non-positive speed, waypoints or beat rather than dividing.

## Architecture

Ordinary ClientCore builders. Declared in `movement.h` beside the cardinals,
`MoveEndpoint` and the other object-reference builders — where a reader looks
for them — and implemented in a new `src/movement_actions.cpp` so
`movement.cpp` does not grow without bound. No new header, no layering
change, no decoder: none of the seven has a server command of its own.

The `// They are intentionally not exposed here` comment that previously
explained the missing diagonals is replaced by the constants and the delay
rule it was pointing at.

## Tests

`clientcore/tests/movement_actions_tests.cpp`, registered as
`protocol772_movement_actions_tests` in `clientcore/CMakeLists.txt` and in
`tests/build_clientcore_windows.cmd`.

### GOLDEN_TESTS

- `6a` / `6b` / `6c` / `6d` — the four diagonals, one byte each
- `85617ddb7d07260b02` — turn a map object at (32097, 32219, 7), type 2854,
  stack 2
- `85ffff410003480803` — turn an object in open container 1, slot 3
- `85ffff050000260b00` — turn an object in body slot 5
- `8700` / `870f` — close container 0 and 15
- `8800` / `880f` — up container 0 and 15
- `87ff` / `88ff` — a container number past the server's table
- `850000000000000000` and `85fefffeff0fffffff` — the object reference at
  both ends of its range

Offsets are asserted against the dispatcher's own values, and a diagonal is
additionally proven to equal the composition of its two cardinals via the
existing `StepPosition`, which is what "both offsets non-zero through the
same `CGoDirection`" means.

Delay arithmetic, hand-computed against the formula: `(100, 500)` → 200 ms
cardinal and 600 ms diagonal; `(100, 1000)` → 200 ms, showing a sub-beat step
still costs a whole beat; `(100, 220)` → 600 and 1400; a non-default beat of
50 and of 1; and monotonicity in speed, in waypoints and in diagonality.

### NEGATIVE_TESTS

These seven commands have no decoder, so the negative surface is boundaries
and collisions rather than truncation:

- **Opcode isolation**: all seven opcodes are distinct from each other and
  from every command this client already sends (the four cardinals, stop, the
  four rotates, move, the three uses, look, talk, ping, logout) — 7 unique
  additions asserted against a set of 17 existing ones
- the four diagonal destinations are distinct, none equals the origin, and
  none changes floor
- `IsTripleDelayDiagonal` false for both cardinals, for standing still, and
  **for a diagonal that changes floor in either direction** — the rule's real
  boundary
- the ×3 is asserted *not* to equal three times the final cardinal delay, so
  a future refactor cannot quietly move the multiplication after the division
- delay guards: zero and negative waypoints, speed and beat each return 0
  instead of dividing
- all sixteen legal container numbers round-trip, and 16, 64 and 255 are
  passed through untouched rather than clamped
- every body slot 0–10 and every container slot 0–15 round-trips into the
  object-reference bytes, including the `0xFFFF` special-coordinate marker

## Results

- **GCC_TESTS**: `ctest` **10/10 suites PASS** via WSL `Ubuntu-26.04`,
  g++ 15.2.0, `-Wall -Wextra -Wpedantic -Werror`
- **SANITIZERS**: ASan + UBSan rebuild of all ten, **10/10 PASS**
- **MSVC_TESTS**: `tests/build_clientcore_windows.cmd` from a VS 2022 Build
  Tools x64 developer environment, **WINDOWS CLIENTCORE: PASS** — all nine
  prior suites plus `movement_actions_tests` at `/W4 /WX /permissive-`, C++17
  for the suites and C++20 for the archive the Unreal module links
- **HARNESS**: `tests/regression_harness.sh --repo-root <this worktree>`:
  `PASS_WITH_SKIPS`, 6 PASS / 0 FAIL / 3 SKIP. Its `build-sanity-msvc` check
  is `SKIP` because the harness ran from Git Bash without `vcvars64`; the
  MSVC result above was obtained by invoking the same script from a developer
  environment directly, and the harness is reporting the absence of `cl.exe`
  on its own PATH rather than contradicting it

### The MSVC pass found a defect again

The first MSVC run failed to compile this suite:

```
movement_actions_tests.cpp(280): error C2220: warning treated as error
movement_actions_tests.cpp(280): warning C4127: conditional expression is constant
```

`CHECK(kMaxOpenContainers == 16)` is a compile-time constant comparison
inside a runtime macro. GCC accepts it silently; MSVC at `/W4 /WX` does not.
Fixed as `static_assert`.

This is the **same defect class** `CLIENTCORE-EDITORS-772-001` hit, and the
third time the Windows build has caught something GCC waved through. The
working rule, now applied in both suites: a fact that is fixed at compile
time goes in a `static_assert`, and `CHECK` is for assertions that exercise a
function over inputs. Worth remembering before the next suite is written
rather than discovering a fourth time.

## Limits

- No live session. Nothing here proves a diagonal step is accepted, that an
  object rotates, or that the parent arrow reopens the right container.
- `PredictStepDelayMilliseconds` reproduces the formula, not the server's
  state: `waypoints` and `speed` must come from the caller, and this project
  does not currently read the `WAYPOINTS` attribute anywhere. The function is
  correct and unused, which is the honest state.
- The blocked-diagonal climb suppression (`cract.cc:368-369`) is recorded and
  deliberately not modelled.
- No UI. Exposing diagonals to input is `Real33DPlayerController.*` and the
  parent arrow is `Real33DHUD.*`, both held by another milestone.
