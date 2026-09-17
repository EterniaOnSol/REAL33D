# HANDOFF

Date/time: 2026-09-16
Agent: Claude
Role: PROTOCOL772CORE MOVEMENT IMPLEMENTATION
Branch: `main`
Starting commit: `630e846`
Implementation commit: `37fa5f6`
Ending commit: this handoff commit
Worktree: clean after the focused Movement commit

## Objective

Complete `MOVEMENT-772-001`: cardinal walking end to end over the existing
`WorldState`, derived only from the Fusion32 7.72 source. Do not implement
combat, inventory or Unreal, and do not approximate diagonals.

## Inspection and source findings

Inspected, in `reference/game/src`:

- `operate.cc::Move` and `AnnounceMovingCreature` - the fixed order a creature
  move produces.
- `cract.cc::TCreature::NotifyGo`, `NotifyTurn` and `Go` - the mover's own
  viewport updates and the blocked-step throw.
- `sending.cc::SendRow`, `SendFloors`, `SendFieldData`, `SendAddField`,
  `SendChangeField`, `SendDeleteField`, `SendMoveCreature`, `SendSnapback`,
  `SendMessage`, `SendResult`.
- `map.cc::PlaceObject`, `GetObjectPriority`, `MoveObject`, `CutObject`;
  `map.hh` PRIORITY_* and `info.cc::GetObjectRNum`.
- `receiving.cc::ReceiveData`, `CGoDirection`, `CGoPath`, `CQuitGame`.
- `communication.cc::ReceiveCommand` - the client-to-server XTEA envelope.

Three findings shaped the design:

1. **The viewport anchor.** `SendRow` and `SendFloors` carry no coordinates.
   `NotifyGo` advances the player's `posx/posy/posz` one axis at a time and only
   then emits them, while `AnnounceMovingCreature` runs earlier and reads the
   still-old position. So the anchor advances with rows and floor changes and
   never with `SV_CMD_MOVE_CREATURE`. After a completed step the anchor and the
   local player's creature must agree, which is now a checked invariant rather
   than an assumption.
2. **`SV_CMD_ADD_FIELD` carries no stack index.** `PlaceObject` inserts by a
   priority derived from `BANK`/`CLIP`/`BOTTOM`/`TOP` flags the wire never
   carries, so `ObjectTypeEncoding` grew a priority column read from the same
   `dat/objects.srv`. The four flags were verified mutually exclusive across all
   5003 declared types, matching the if / else-if chain in `GetObjectPriority`.
   Because `PRIORITY_CREATURE` is 4 and `PRIORITY_LOW` is 5, an item entering an
   occupied field lands **above** the creature standing there while a creature
   entering lands **below** any item already on it.
3. **A refusal emits no map command.** `TCreature::Go` throws
   `MOVENOTPOSSIBLE` and `SendResult` answers with `SV_CMD_MESSAGE` plus
   `SV_CMD_SNAPBACK`. Queueing a second walk while one runs snaps back the same
   way through `CGoDirection` calling `ToDoClear`.

Diagonals are implemented by Fusion32 through the same `CGoDirection` with both
offsets non-zero, produce an x row then a y row, and cost three times the walk
delay. They are documented and left unexposed per the task scope; the anchor
model already reproduces their two-row sequence without approximation.

## Changes

- Added `clientcore/include/fusion32/protocol772/map_scan.h` and
  `clientcore/src/map_scan.cpp`, holding the tile / skip walk extracted from the
  full-screen decoder with no behaviour change.
- Added `clientcore/include/fusion32/protocol772/movement.h` and
  `clientcore/src/movement.cpp`.
- Extended `object_types` with `ObjectPriority` and `MapStackInsertIndex`.
- Extended `worldstate` with `viewport_anchor`, `local_creature_id`,
  `AnchoredWindow`, `viewport_synchronized`, `visible_creature_ids` and four new
  anomaly kinds.
- Added `GameLoginSession::SendCommand` for authenticated client commands.
- Rewrote `initial_world.cpp` onto the shared scanner; its suite is unchanged
  and still passes.
- Added `clientcore/tests/movement_tests.cpp` and grew the fixtures header with
  a `ServerEmitter` port of the incremental senders.
- Added `docs/protocol772/MOVEMENT.md` and
  `evidence/clientcore/MOVEMENT-772-001.md`.
- Updated project status, architecture, roadmap, parity matrix, source truth,
  both READMEs, the initial-world doc and this handoff; archived the prior
  Initial World handoff.

## Tests/results

Validated in WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5, C++17
with warnings as errors:

- Debug build: `PASS`
- CTest: `6/6 PASS`
- ASan/UBSan CTest: `6/6 PASS`
- Seven hand-computed golden hex commands, byte for byte, each first reproduced
  by the literal port of the server emitter: `PASS`
- Twelve-step cardinal walk compared against a freshly emitted
  `SV_CMD_FULLSCREEN` after every step: `PASS`
- Negatives - every truncation of each golden command, empty payload, empty type
  table, out-of-range stack indexes, a row stepping off the addressable floors,
  an unsupported opcode consuming nothing, and three application anomalies each
  leaving the map snapshot unchanged: `PASS`
- `verify_object_type_invariants.py`: `PASS`
- Live smoke, synthetic `ACCOUNT_A`: `LIVE PASS`. Six cardinal steps from
  `(32097,32219,7)` to `(32097,32213,7)`, each landing the anchor exactly where
  predicted with the viewport synchronized and zero anomalies; one observed
  snapback that changed neither anchor, tiles nor things; then a clean
  `CL_CMD_LOGOUT`, reconnect, and a fresh `SV_CMD_FULLSCREEN` reporting the same
  position with the same 401 tiles and 571 things, all item stacks identical and
  no phantom creatures. 125 of the 408 original tiles had left the window.

No account id, password, modulus or key material was recorded. The temporary
live harness, its driver script and its binary were deleted and the runtime
services stopped.

## Status and limits

`MOVEMENT-772-001 = PASS` within this bounded scope.

Remaining `UNVERIFIED`:

- Only surface walking on floor 7 was observed live. Floor changes, the
  underground range and the four field commands are fixture-covered only.
- Diagonal walking is decoded coherently but was neither exposed nor exercised.
- The live refusal came from the `ToDoClear` path; the blocked-terrain
  `MOVENOTPOSSIBLE` path is fixture-covered, through the same `SendSnapback`.
- Native Windows execution and independent repetition.

Out of scope and untouched: combat, inventory, containers, chat, path walking
(`CL_CMD_GO_PATH`), object moves and Unreal.

## Known boundary

A server command this layer does not decode yields
`ServerUpdateKind::Unsupported` with `bytes_consumed` zero, because guessing its
length would desynchronize the stream. The login burst still contains such
commands after `SV_CMD_FULLSCREEN`, starting with `SV_CMD_GRAPHICAL_EFFECT`, so
a caller processing that first frame stops there with 240 bytes preserved. Walk
frames observed live contained movement commands only.

## Operational note

WSL2 shuts the VM down between separate `wsl.exe` invocations and clears `/tmp`,
which destroys both the sanitized runtime and any build directory there. Build
under `/root/f32/...` instead, and run anything that spans prepare, start and a
live client in a single `wsl.exe` invocation. Invoke such scripts through
PowerShell, since Git Bash rewrites `/mnt/...` paths.

## Exact next task

`PLAYERSTATE-772-001`: decode the rest of the login burst and the ambient and
creature update commands, so a caller can consume a whole frame instead of
stopping at the first unsupported opcode.

Files and functions to start from, all in `reference/game/src/sending.cc`:
`SendPlayerData` (160), `SendPlayerSkills` (161), `SendPlayerState` (162),
`SendAmbiente` (130), `SendGraphicalEffect` (131), `SendTextualEffect` (132),
`SendMissileEffect` (133), `SendMarkCreature` (134), and `SendCreatureHealth`
(140) through `SendCreatureParty` (145). `SendPing` (30) and the matching
`CL_CMD_PING` (30) close the keepalive gap.

Add each decoder to `DecodeServerUpdate` in `clientcore/src/movement.cpp`, or
split it into its own module if that file grows unwieldy. Do not begin it
automatically.

Commands to reproduce this task's results:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /root/f32/build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /root/f32/build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /root/f32/build --output-on-failure
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_object_type_invariants.py /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz
```
