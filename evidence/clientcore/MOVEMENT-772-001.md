# MOVEMENT-772-001

Status: `PASS`

## Implementation

Added cardinal movement end to end on top of the existing `WorldState`:

- `map_scan` - the tile / skip-marker walk extracted from the full-screen
  decoder, now shared verbatim by `SV_CMD_FULLSCREEN`, `SV_CMD_ROW_*`,
  `SV_CMD_FLOOR_UP/DOWN` and `SV_CMD_FIELD_DATA`, exactly as the server shares
  `SendMapPoint` and `SkipFlush` between them.
- `movement` - the client walk, turn and stop commands, the incremental server
  update decoder and its application to `WorldState`.
- `object_types` - extended with the `PlaceObject` stack priority, because
  `SV_CMD_ADD_FIELD` carries no stack index.
- `worldstate` - a `viewport_anchor`, the local creature id, the anchored
  window, `viewport_synchronized()` and `visible_creature_ids()`.
- `gamelogin` - `SendCommand`, the authenticated client-to-server XTEA envelope
  `ReceiveCommand` requires past login.

The full-screen decoder was rewritten onto the shared scanner with no behaviour
change; `INITIALWORLD-772-001` still passes unmodified.

## Source traceability

See [`docs/protocol772/MOVEMENT.md`](../../docs/protocol772/MOVEMENT.md).
Primary symbols are `reference/game/src/operate.cc::Move` and
`AnnounceMovingCreature`, `reference/game/src/cract.cc::TCreature::NotifyGo`,
`NotifyTurn` and `Go`, `reference/game/src/sending.cc::SendRow`, `SendFloors`,
`SendFieldData`, `SendAddField`, `SendChangeField`, `SendDeleteField`,
`SendMoveCreature`, `SendSnapback`, `SendMessage` and `SendResult`,
`reference/game/src/map.cc::PlaceObject`, `GetObjectPriority` and `MoveObject`,
`reference/game/src/info.cc::GetObjectRNum`,
`reference/game/src/receiving.cc::ReceiveData` and `CGoDirection`, and
`reference/game/src/communication.cc::ReceiveCommand`.

Three findings shaped the design and are documented in full:

1. **The viewport anchor.** `SendRow` and `SendFloors` carry no coordinates.
   `NotifyGo` advances the player one axis at a time and only then emits them,
   so the anchor moves with rows and floor changes and never with
   `SV_CMD_MOVE_CREATURE`. Once a step completes the anchor and the local
   player's creature must agree, which makes desynchronization detectable
   instead of silent.
2. **`ADD_FIELD` carries no stack index.** `PlaceObject` inserts by a priority
   derived from flags the wire never carries, so the object type table had to
   grow a priority column. Because `PRIORITY_CREATURE` is 4 and `PRIORITY_LOW`
   is 5, an item entering an occupied field lands above the creature on it while
   a creature entering lands below any item already there.
3. **Refusal emits no map command.** A blocked step answers with
   `SV_CMD_MESSAGE` plus `SV_CMD_SNAPBACK`, so applying a snapback must leave
   the map byte-identical.

## Tests

Environment: Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5,
C++17, warnings as errors.

- Debug build: `PASS`
- CTest: `6/6 PASS` (Transport, Crypto, Login, Game Login, Initial World,
  Movement)
- ASan/UBSan CTest: `6/6 PASS`
- Hand-computed golden hex, byte for byte, each first reproduced by the literal
  port of the server emitter: `PASS`
  - `SV_CMD_ROW_EAST` empty, 3 bytes, 8 floors of a 14-field column
  - `SV_CMD_ROW_NORTH` empty, 3 bytes, 8 floors of an 18-field row
  - `SV_CMD_FLOOR_DOWN` arriving at z = 8, 7 bytes, floors 8..10
  - `SV_CMD_FLOOR_UP` arriving at z = 7, 13 bytes, floors 5..0
  - `SV_CMD_FLOOR_UP` above the surface, 1 byte, no floors
  - `SV_CMD_MOVE_CREATURE`, 12 bytes
  - `SV_CMD_MESSAGE` + `SV_CMD_SNAPBACK` refusal pair
- Twelve-step cardinal walk over a synthetic world, comparing the incrementally
  updated `WorldState` against a freshly emitted `SV_CMD_FULLSCREEN` after every
  single step: `PASS`
- `PlaceObject` insertion table, player stack position after a step, refusal
  leaving the map untouched, surface-to-underground floor pruning, the four
  field commands, creature removal, and the creature mirror retaining a
  scrolled-out creature: `PASS`
- Negatives - every truncation of each golden command, empty payload, empty type
  table, stack index at or beyond `MAX_OBJECTS_PER_POINT`, a row stepping off
  the addressable floors, an unsupported opcode consuming nothing, and the
  `MoveOriginMismatch`, `StackIndexOutOfRange` and `FieldOutsideViewport`
  anomalies each leaving the map snapshot unchanged: `PASS`

## Bounded live smoke

A temporary non-tracked harness used only the local synthetic `ACCOUNT_A` and
the runtime's public modulus, both read at run time and never recorded. It ran
prepare, start, walk, verify and stop in a single WSL invocation:

```text
object type table: declared=5003
login payload: 2323 bytes, 1 commands applied, stopped at SV_CMD_GRAPHICAL_EFFECT, 240 bytes left
start (32097,32219,7) creature_id=1001 tiles=408 things=587 creatures=2 synchronized=1
  step North (32097,32219,7) -> (32097,32218,7) move_of_local=1 tiles=395 things=560 stopped_at=end
  step North (32097,32218,7) -> (32097,32217,7) move_of_local=1 tiles=384 things=540 stopped_at=end
  step North (32097,32217,7) -> (32097,32216,7) move_of_local=1 tiles=382 things=538 stopped_at=end
  step North (32097,32216,7) -> (32097,32215,7) move_of_local=1 tiles=395 things=551 stopped_at=end
  step North (32097,32215,7) -> (32097,32214,7) move_of_local=1 tiles=401 things=571 stopped_at=end
  step North (32097,32214,7) -> (32097,32213,7) move_of_local=1 tiles=404 things=579 stopped_at=end
walked 6 accepted, 0 rejected; (32097,32219,7) -> (32097,32213,7)
refused step: 1 snapback(s) observed, none changed the map
final position (32097,32214,7) synchronized=1
fresh screen (32097,32214,7) tiles=401 things=571 creatures=2
verified 401 tiles identical between the walked state and the fresh full screen
creature mirror walked=2 fresh=2; visible walked=2 fresh=2
viewport turnover: 125 of 408 initial tiles left the window
LIVE PASS
```

What this demonstrates:

- six cardinal steps were requested with a one-byte `CL_CMD_GO_NORTH` payload
  and each produced exactly `SV_CMD_MOVE_CREATURE` for the local creature
  followed by `SV_CMD_ROW_NORTH`, with nothing else in the frame
  (`stopped_at=end`);
- after every step the anchor landed exactly on the predicted position and
  `viewport_synchronized()` held, so no row, floor change or creature move was
  lost;
- the tile and thing counts moved with the terrain rather than drifting, and the
  final walk position was reached with zero anomalies;
- a deliberately queued second walk produced a real `SV_CMD_SNAPBACK`, and the
  per-command fingerprint check proved that applying it changed neither the
  anchor, the tile count nor the thing count;
- after a clean `CL_CMD_LOGOUT` and reconnect, the server's fresh
  `SV_CMD_FULLSCREEN` reported the same position the walk had reached, with the
  same 401 tiles and 571 things, and all 401 tiles carried identical item
  stacks;
- the visible creature sets matched exactly, so the walked state held no phantom
  creature;
- 125 of the 408 original tiles had left the window, so the viewport genuinely
  moved rather than being refreshed in place.

The temporary harness source, driver script and binary were deleted and the
runtime services stopped after the run. No account id, password, modulus or key
material appears in this evidence.

## Remaining UNVERIFIED

- Only surface walking on floor 7 was observed live. Floor changes, the
  underground floor range, `FIELD_DATA`, `ADD_FIELD`, `CHANGE_FIELD` and
  `DELETE_FIELD` are covered by fixtures only.
- Diagonal walking is decoded coherently by the anchor model but was neither
  requested nor exercised, live or offline, beyond the two-row reasoning.
- The blocked-terrain refusal path (`MOVENOTPOSSIBLE` from
  `TCreature::Go`) is covered by fixtures; the live refusal came from the
  `ToDoClear` path instead, which is the same `SendSnapback` call.
- Native Windows execution and independent repetition remain unverified.

## Scope exclusions

No combat, inventory, containers, chat, path walking (`CL_CMD_GO_PATH`) or
Unreal integration was started. Server commands outside the movement set remain
undecoded and are reported by name with zero bytes consumed.
