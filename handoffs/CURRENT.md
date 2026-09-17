# HANDOFF

Date/time: 2026-09-17
Agent: Claude
Role: UNREAL VERTICAL SLICE
Branch: `main`
Starting commit: `d3f0e5d`
Implementation commit: `da36b86`
Corrective commit: `cae6450`
Worktree: clean after the focused slice commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, `HEAD == origin/main`

## Objective

Complete `UNREAL-SLICE-001`: build the first end-to-end chain
`Fusion32 -> Protocol772Core -> semantic events -> WorldState -> Unreal bridge
-> game thread -> 3D representation`, without reimplementing Protocol 772 or
gameplay inside Unreal, and prove it with a real run rather than a mock.

## Result

`PASS`, with acceptance criterion 12 qualified, **after a corrective run**. The
first report's claim that eleven criteria held unconditionally was withdrawn.

### The correction

The first report credited the outgoing input path — Unreal input reaching
Fusion32 — to a session in which the operator never controlled B from Unreal. B
moved there because A pushed him from the original client, which Fusion32
resolved authoritatively. That is evidence of the incoming chain only.

Independently, the numbers that report quoted for criterion 6 came from a
snapshot read mid-session and then overwritten. Every retained snapshot from
that session shows `steps_requested: 0`.

Criteria 6 and 7 were withdrawn and re-established by a run in which the
operator drove B from the Unreal window: 19 requests, 18 accepted, 1 refused, 0
unanswered, each joined from key press to authoritative position by an
`input_id`, plus 6 external relocations counted apart — two of them diagonal,
which this client cannot request at all.

Full statement in `evidence/clientcore/UNREAL-SLICE-001-CORRECTION.md`.

### The rest

The original `Tibia.exe` 7.72 (Player A) and the Unreal client (Player B) in one
sanitized Fusion32 world at the same time. A appeared in Unreal by name and
position; A's steps moved his capsule; refused steps changed nothing on screen;
A left and re-entered B's viewport cleanly; a real server-side drop tore the
scene down to zero actors and an in-session reconnect rebuilt it.

Criterion 12 remains qualified: `protocol_anomalies` was zero throughout and
residual/unsupported were zero across two multi-minute windows, but
`SV_CMD_TALK` is still undecoded. It was not worked on here.

## Architecture, and what enforces it

```text
worker thread                                game thread
TcpTransport -> GameLoginSession
  -> DecodeServerUpdate / ApplyServerUpdate
  -> WorldState
  -> WorldView::Diff        --SPSC queue-->  UReal33DBridge::DrainEvents
                                              -> AReal33DWorld
                                                 -> AReal33DTile / AReal33DCreature
  <--intent queue-- RequestWalk <------------- AReal33DPlayerController
```

Three mechanisms, none of which rely on anyone remembering a rule:

1. `REAL33D.Build.cs` links `build/clientcore-windows/protocol772core.lib` and
   throws if it is absent. No protocol source exists inside the Unreal module.
2. Only `Private/Real33DBridge.cpp` may include a protocol header. No public
   header names `fusion32::protocol772`; `Real33DCoords.h` mirrors
   `MapPosition` as three integers rather than including the real one.
3. The WorldState-to-events diff lives in ClientCore as `protocol772_worldview`,
   covered by `worldview_tests.cpp`, not in a Tick function.

Movement is asymmetric on purpose. There is no code path by which a key moves an
Actor: input becomes an intent, Fusion32 decides, and the actor follows only
what WorldState confirms. A refusal therefore has nothing to undo.

## What changed in ClientCore

Minimal, semantic and tested, as the task required:

- `protocol772_worldview`: `WorldView::Diff` turns successive `WorldState`s into
  `WorldEvent`s. 8 tests.
- `TcpTransport::SetReadTimeout`, forwarded by `GameLoginSession`: separates the
  read poll interval from the connect timeout. 2 tests.
- `ObjectTypeEncoding::unpass`: parses the `UNPASS` flag from `objects.srv`, so
  presentation can tell a wall from walkable clutter without guessing. Covered
  in `initial_world_tests.cpp`.
- `MovementLedger`: separates steps this client asked for from relocations
  Fusion32 imposed. A move counts as ours only when a walk is outstanding and
  the player landed on exactly the field it asked for; a push arriving mid-walk
  does not consume the request. 6 tests, including a replay of the eight-field
  push that misled the first report, and one for a move from a field to itself.
- Four portability fixes surfaced by the first MSVC build.

No previous milestone's behaviour was altered.

## Tests and sanitizers

| Suite | Result |
| --- | --- |
| Windows MSVC `/W4 /WX /permissive-`, C++17 and C++20 | `WINDOWS CLIENTCORE: PASS` |
| transport | 22/22 |
| crypto | 25/25 |
| login, gamelogin, initial_world, movement, player_state, worldview | `PASS` |
| WSL GCC 15.2 + ASan/UBSan via CTest | 8/8 |
| `tests/secret_check.sh` | `PASS` |

`tests/build_clientcore_windows.cmd` resolves the project's longest-standing
`UNVERIFIED` item. Every suite had only ever been built by GCC under WSL; the
first native build found four defects GCC accepts silently.

## Defects found by running

Each was invisible offline. Details in the evidence file.

1. The 3000 ms socket timeout doubled as the walk-intent poll interval, so a
   keypress could wait three seconds to be sent. The operator called it "massive
   lagg" and confirmed the fix.
2. `SetReadTimeout` initially reported a bad argument through `SetFailure`,
   which marks the connection `Failed`. Caught by its own new test.
3. The transport suite's total was a hardcoded `20`; adding two cases made it
   run 22 and report 20. Counts are quoted as evidence, so it now counts.
4. `steps_accepted` counted any local-player move, not accepted requests. A live
   run recorded eight against zero requests. Renamed `local_player_moves`.
5. An unsupported opcode was a bare number. It now names the command and the
   bytes left unwalked.
6. Every non-ground object was drawn as the same block, so walkable clutter
   looked like walls. The operator reported walking "through cubes"; `UNPASS`
   now separates them.

## Environment notes worth keeping

- `/tmp` is tmpfs in Ubuntu-26.04. WSL shutting down between two commands
  silently erased the prepared runtime. Pin the VM with a long-running process.
- The Game service performs a scheduled server save and exits, logging
  `Reboot-Skript existiert nicht`. Nothing restarts it; run `start_wsl.sh`.
- Activating a window makes Windows synthesise a key burst that reaches the
  input path. Capture with `SWP_NOACTIVATE` and post keys with `PostMessage`,
  or the measurement perturbs what it measures.

## How to run it

```bat
tests\build_clientcore_windows.cmd
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
  REAL33DEditor Win64 Development -Project="%CD%\unreal\REAL33D\REAL33D.uproject" -WaitMutex
scripts\client\run_unreal_slice.cmd B
```

Server side unchanged: `prepare_wsl.sh` then `start_wsl.sh`. Player A via
`scripts\client\run_player_a.cmd`, which starts the client, patches it with the
IP Changer and opens the credentials file for the operator to read.

## Limits

One run, one operator, no independent repetition. Both characters stayed on
floor 7, so no floor transition was exercised in 3D; floor height in Unreal is a
presentation choice recorded as `UNRESOLVED` in the data. Every mesh is an
engine primitive, so nothing here demonstrates visual parity and the parity
matrix claims none. Criterion 7 rests on human observation of the original client, corroborated by machine evidence of the Unreal-originated walks accepted in the same window, but not machine-certified on the Tibia side.

## Not done, deliberately

No chat, inventory, containers, combat UI, spells, runes, final effects,
equipment visuals, full UI, minimap, audio, final art, mobile, offline full-map
conversion, map editor or server-side change. `visual/reference_pack` and the
APPROVED/REJECTED decisions were not touched.

## Suggested next milestone, not started

`CHAT-772-001`: decode `SV_CMD_TALK`. It is the only opcode standing between an
ordinary session and an unconditional criterion 12, it already forces the
operator to avoid the in-game chat during every live run, and it is small and
well bounded next to containers or trade.

Alternatives: `ROOKGAARD-P0-MOCKUPS-001` now that the registry can adopt
approved art without touching any Actor, or a floor-transition slice, which is
the one movement case the 3D client has never exercised.
