# HANDOFF

Date/time: 2026-09-16
Agent: Claude
Role: PROTOCOL772CORE INITIAL WORLD IMPLEMENTATION
Branch: `main`
Starting commit: `4e522c9`
Implementation commit: `fedd536`
Ending commit: this handoff commit
Worktree: clean after the focused Initial World commit

## Objective

Complete `INITIALWORLD-772-001`: decode the initial world state received after
Game Login, starting with `FULLSCREEN`, exclusively from the Fusion32 7.72
source truth, into a verifiable semantic representation. Do not implement
movement, combat, inventory or Unreal.

The roadmap listed this slot as `WORLDSTATE-INIT-772-001`; the task was executed
under the ID `INITIALWORLD-772-001` and both names are recorded in the docs.

## Inspection and source findings

Inspected, in `reference/game/src`:

- `sending.cc::SendFullScreen` - header, floor range selection, x-outer /
  y-inner scan, per-floor `ZOffset = PlayerZ - PointZ`.
- `sending.cc::SendMapPoint`, `SkipFlush` - the `(Count, 0xFF)` skip encoding
  and the `MAX_OBJECTS_PER_POINT = 10` bound.
- `sending.cc::SendMapObject`, `SendItem`, `SendOutfit`, `SendString` - item
  and creature encoding, including creature words 97/98/99.
- `connections.cc` lines 219-222 - terminal offsets 8/6 and size 18x14,
  assigned once in the constructor and never reassigned.
- `connections.cc::KnownCreature`, `NewKnownCreature`, `connections.hh::TConnection` -
  the 150-slot known-creature table and the eviction id carried by word 97.
- `crmain.cc::TCreature::GetHealth` - health is a percentage.
- `crplayer.cc` lines 199-206 - the login burst order.
- `communication.cc::SendData`, `sending.cc::FinishSendData` - all committed
  output is flushed as one packet, and an overflowing command is discarded
  whole, so a `FULLSCREEN` never straddles two frames.
- `objects.hh::TYPEID_*`, `objects.cc::LoadObjects`, `GetObjectTypeByName`.

Three findings shaped the design:

1. **The item encoding is not self-describing.** `SendItem` emits an extra byte
   only when the object type carries `LIQUIDCONTAINER`, `LIQUIDPOOL` or
   `CUMULATIVE`, and those flags never travel on the wire. A decoder cannot walk
   the stream without the server's object type table, so `ObjectTypeTable` is an
   explicit injected dependency loaded from `dat/objects.srv`, not knowledge
   baked into the parser.
2. **The skip marker is context-dependent.** `SkipFlush` drains a run of `S`
   empty positions into markers whose counts sum to `S + 1`. The first marker
   loses one position when it also terminates a described tile. Both readings
   are implemented and both are covered by fixtures.
3. **`SendItem` sends `getDisguise().TypeID` but reads flags from the original
   type.** The wire id is only sufficient if the two agree. Verified against the
   shipped data: all 74 disguise types share their target's wire-relevant flag
   set. This is a dataset property, not a protocol guarantee, and it is
   re-checked by `tests/verify_object_type_invariants.py`.

## Changes

- Added `clientcore/include/fusion32/protocol772/object_types.h` and
  `clientcore/src/object_types.cpp`.
- Added `clientcore/include/fusion32/protocol772/worldstate.h` and
  `clientcore/src/worldstate.cpp`.
- Added `clientcore/include/fusion32/protocol772/initial_world.h` and
  `clientcore/src/initial_world.cpp`.
- Added the `protocol772_initial_world` CMake target, the
  `clientcore/tests/initial_world_tests.cpp` suite and
  `clientcore/tests/fixtures/fullscreen_772_vectors.h`.
- Added `tests/verify_object_type_invariants.py`.
- Added `docs/protocol772/INITIAL_WORLD.md` and
  `evidence/clientcore/INITIALWORLD-772-001.md`.
- Updated project status, architecture, roadmap, parity matrix, both READMEs and
  this handoff; archived the prior Game Login handoff.

`gamelogin` was not modified. The new layer consumes exactly the tail that
`ParseGameInitialMessage` already preserved, and a test asserts that boundary.

## Tests/results

Validated in WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5, C++17
with warnings as errors:

- Debug build: `PASS`
- CTest: `5/5 PASS` (Transport, Crypto, Login, Game Login, Initial World)
- ASan/UBSan CTest: `5/5 PASS`
- Three hand-computed golden hex fixtures, byte for byte: `PASS`
- The literal port of the server emitter reproduces each golden fixture exactly
  before being used for richer cases: `PASS`
- Negative cases - every truncation of the populated golden message, wrong
  opcode, empty payload, empty type table, `PlayerZ > 15`, reserved container
  id, unknown type id, eleven-object tile, over-wide skip run, over-long
  creature name, four malformed `objects.srv` inputs: `PASS`
- `verify_object_type_invariants.py`: `PASS` (5003 types, 0 mismatches)
- Live smoke, synthetic `ACCOUNT_A` on the sanitized runtime: `LIVE PASS`.
  The real 2075-byte `FULLSCREEN` decoded into 8 floors, 408 described tiles and
  1608 skipped tiles - exactly the 2016-position window - 587 things and 2
  creatures with no anomalies, and re-encoded byte for byte through the server
  port. The 125 trailing bytes were preserved and identified as
  `SV_CMD_GRAPHICAL_EFFECT`, matching `crplayer.cc` line 202.

No account id, password, modulus or key material was recorded. The temporary
live harness source and binary were deleted and the runtime services stopped.

## Status and limits

`INITIALWORLD-772-001 = PASS` within this bounded scope.

Remaining `UNVERIFIED`:

- The observed live screen carried no liquid or cumulative item, so those two
  extra-byte paths were exercised against the real `objects.srv` table with
  synthetic bytes rather than server-emitted bytes.
- Only the surface floor range was observed live; the underground range is
  covered by fixtures only.
- Native Windows execution and independent repetition.

Out of scope and untouched: movement, combat, inventory, containers, chat,
client command encoding and Unreal. Every server command other than
`FULLSCREEN` remains unparsed and is reported by name through
`ServerCommandName`.

## Operational note

The sanitized runtime at `/tmp/fusion32-server-baseline-772-${UID}` was found in
a broken state (Game dead, Query Manager and Login alive), restarted cleanly via
`scripts/server/stop_wsl.sh` then `start_wsl.sh`, used for the smoke, and
stopped cleanly afterwards. It still exists and does not need re-preparing.

## Exact next task

`MOVEMENT-772-001`: source-trace and decode the map deltas - `SV_CMD_ROW_NORTH`
(101) through `SV_CMD_ROW_WEST` (104), `FIELD_DATA` (105), `ADD_FIELD` (106),
`CHANGE_FIELD` (107), `DELETE_FIELD` (108), `MOVE_CREATURE` (109), `FLOOR_UP`
(190) and `FLOOR_DOWN` (191) - plus the client movement commands
`CL_CMD_GO_*` (100-109) and `CL_CMD_ROTATE_*` (111-114).

Files and functions to start from: `reference/game/src/sending.cc::SendRow`,
`SendFloors`, `SendFieldData`, `SendAddField`, `SendChangeField`,
`SendDeleteField`, `SendMoveCreature`; `reference/game/src/receiving.cc` for the
client commands; `reference/game/src/cract.cc` lines 1340-1390 for which command
the server picks per movement.

`SendRow`, `SendFloors` and `SendFieldData` reuse `SendMapPoint` and `SkipFlush`
verbatim, so the tile, item, creature and skip decoding in
`clientcore/src/initial_world.cpp` applies unchanged; only the headers and the
scanned window differ. Do not begin it automatically.

Commands to reproduce this task's results:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/f32-iw-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/f32-iw-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/f32-iw-build --output-on-failure
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_object_type_invariants.py /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz
```
