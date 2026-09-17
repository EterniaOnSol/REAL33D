# Protocol772Core Movement

Task: `MOVEMENT-772-001`

Status: `PASS` for deterministic byte fixtures, negative cases, ASan/UBSan and
one bounded local synthetic-account live smoke. This task covers cardinal
walking and the incremental world updates a step produces. Combat, inventory,
containers, chat, path walking and Unreal remain out of scope.

## Boundary

```text
WorldState -> BuildWalkCommand -> XTEA -> framing -> Fusion32
Fusion32 -> framing -> XTEA -> DecodeServerUpdate -> ApplyServerUpdate -> WorldState
```

`movement` adds no I/O of its own. `GameLoginSession::SendCommand` performs the
authenticated client-to-server encryption the server requires past login.

## What a step actually emits

`reference/game/src/operate.cc::Move` drives a creature move in this fixed
order, and the ordering is the whole design:

1. `Creature->NotifyTurn(Con)` updates the facing. No packet.
2. `AnnounceMovingCreature(id, Con)` sends `SV_CMD_MOVE_CREATURE` to every
   nearby player, the mover included. It runs *before* `MoveObject`, so
   `SendMoveCreature` reads the creature's still-old `posx/posy/posz` as the
   origin and the destination container's coordinates as the destination.
3. `MoveObject(Obj, Con)` relocates the object in the map.
4. `Creature->NotifyGo()` advances the mover's own position one axis at a time
   and emits `SV_CMD_FLOOR_UP`/`SV_CMD_FLOOR_DOWN` and `SV_CMD_ROW_*` as it
   goes.

So a cardinal step delivers, in one encrypted frame:

```text
SV_CMD_MOVE_CREATURE   the creature leaves one tile stack and joins another
SV_CMD_ROW_<direction> the column or row the viewport just revealed
```

### The viewport anchor

`SendRow` and `SendFloors` carry **no coordinates**. They are built from
`Connection->GetPosition()`, which `NotifyGo` has already stepped. The decoder
therefore tracks a `viewport_anchor` and applies the same axis step the server
did:

| Command | Anchor change |
| --- | --- |
| `SV_CMD_ROW_NORTH` (101) | `y -= 1` |
| `SV_CMD_ROW_EAST` (102) | `x += 1` |
| `SV_CMD_ROW_SOUTH` (103) | `y += 1` |
| `SV_CMD_ROW_WEST` (104) | `x -= 1` |
| `SV_CMD_FLOOR_UP` (190) | `x += 1`, `y += 1`, `z -= 1` |
| `SV_CMD_FLOOR_DOWN` (191) | `x -= 1`, `y -= 1`, `z += 1` |

`SV_CMD_MOVE_CREATURE` never moves the anchor. This yields a self-checking
invariant: once a step completes, the anchor and the local player's creature
position must agree. `WorldState::viewport_synchronized()` reports it, and a
mismatch is a lost or misapplied command rather than something to paper over.

`NotifyGo` runs the z loop first, then x, then y. A diagonal step therefore
emits two rows anchored at intermediate positions, which the anchor model
reproduces exactly because each row steps the anchor itself.

## Wire layouts

All traced from `reference/game/src/sending.cc`.

```text
SV_CMD_ROW_*      101..104  opcode, then the revealed edge, tile encoding
SV_CMD_FIELD_DATA 105       opcode, u16 x, u16 y, u8 z, one field, tile encoding
SV_CMD_ADD_FIELD  106       opcode, u16 x, u16 y, u8 z, one object
SV_CMD_CHANGE_FIELD 107     opcode, u16 x, u16 y, u8 z, u8 stack index, one object
SV_CMD_DELETE_FIELD 108     opcode, u16 x, u16 y, u8 z, u8 stack index
SV_CMD_MOVE_CREATURE 109    opcode, u16 x, u16 y, u8 z, u8 stack index,
                            u16 x, u16 y, u8 z                        (12 bytes)
SV_CMD_MESSAGE    180       opcode, u8 mode, SendString text
SV_CMD_SNAPBACK   181       opcode, u8 direction                       (2 bytes)
SV_CMD_FLOOR_UP   190       opcode, then the new floors, tile encoding
SV_CMD_FLOOR_DOWN 191       opcode, then the new floors, tile encoding
```

The tile encoding is the one already decoded for `SV_CMD_FULLSCREEN`:
`SendRow`, `SendFloors` and `SendFieldData` call `SendMapPoint` and `SkipFlush`
unchanged. Only the scanned rectangle differs, which is why
`clientcore/src/map_scan.cpp` now owns that walk and all four commands share it.

### Scanned rectangles

`SendRow` keeps the full floor range of `SendFullScreen` and collapses one axis
(lines 487-502):

| Direction | Rectangle relative to the stepped anchor |
| --- | --- |
| North | 18 wide by 1, at `MinY` |
| South | 18 wide by 1, at `MaxY` |
| East | 1 by 14 tall, at `MaxX` |
| West | 1 by 14 tall, at `MinX` |

`SendFloors` (lines 525-558) sends only floors the client cannot already have:

| Situation | Floors |
| --- | --- |
| Up, arriving at z = 7 | 5 down to 0 |
| Up, arriving below the surface | `z - 2` only |
| Down, arriving at z = 8 | 8 up to 10 |
| Down, arriving deeper, `z + 2 <= 15` | `z + 2` only |
| Anything else | none, and the command is a single opcode byte |

`SendFieldData` scans exactly one absolute position with no floor offset.

## Stack order is not carried by ADD_FIELD

`SV_CMD_ADD_FIELD` sends an object with no stack index.
`reference/game/src/map.cc::PlaceObject` decides where it lands from the
object's priority, which `GetObjectPriority` derives from flags the wire never
carries:

```text
BANK 0, CLIP 1, BOTTOM 2, TOP 3, creature container 4, everything else 5
```

`MoveObject` calls `PlaceObject` with `Append = false`, which the function then
forces true for anything that is neither a creature nor a low object. The two
resulting rules are:

* append classes (`BANK`, `CLIP`, `BOTTOM`, `TOP`) insert after every object of
  their own priority, before the first strictly higher one;
* non-append classes (creature, low) insert before the first object of their own
  priority or higher.

A consequence worth stating: `PRIORITY_CREATURE` is 4 and `PRIORITY_LOW` is 5,
so an item dropped onto an occupied field lands **above** the creature standing
there, while a creature entering a field lands **below** any item already on it.

This is the second place where the 7.72 map encoding is not self-describing, so
`ObjectTypeEncoding` now carries the priority alongside the liquid and
cumulative flags, both read from the server's own `dat/objects.srv`. The flags
were verified mutually exclusive across all 5003 declared types, matching the
if / else-if chain in `GetObjectPriority`.

## Client commands

`reference/game/src/receiving.cc::ReceiveData` dispatches the walk opcodes to
`CGoDirection(Connection, OffsetX, OffsetY)`, which reads nothing further from
the buffer. A cardinal walk is therefore a **one-byte payload**:

| Command | Opcode | Offset |
| --- | --- | --- |
| `CL_CMD_GO_NORTH` | 101 | `0, -1` |
| `CL_CMD_GO_EAST` | 102 | `+1, 0` |
| `CL_CMD_GO_SOUTH` | 103 | `0, +1` |
| `CL_CMD_GO_WEST` | 104 | `-1, 0` |
| `CL_CMD_GO_STOP` | 105 | stops the queue |
| `CL_CMD_ROTATE_*` | 111..114 | facing only |

Past login every client packet is XTEA-encrypted with the same envelope the
server uses, and `ReceiveCommand` requires the outer size to be a multiple of
eight.

### Diagonals

Fusion32 does implement them: `CL_CMD_GO_NORTHEAST` (106) through
`CL_CMD_GO_NORTHWEST` (109) reach the same `CGoDirection` with both offsets
non-zero, `NotifyGo` emits an x row and then a y row, and
`NotifyGo` multiplies the step delay by three for a diagonal move. They are
deliberately **not exposed** by `BuildWalkCommand`, per the task scope. No
approximation was added: the anchor model already reproduces the two-row
sequence correctly because each row steps the anchor itself, so the parser stays
coherent if a diagonal ever arrives.

## Refusal

A blocked step throws `MOVENOTPOSSIBLE` from
`reference/game/src/cract.cc::TCreature::Go`, and
`reference/game/src/sending.cc::SendResult` answers with
`SV_CMD_MESSAGE` carrying "Sorry, not possible." followed by `SV_CMD_SNAPBACK`.
Queueing a second walk while one is still running also snaps back, through
`CGoDirection` calling `ToDoClear`.

No map command is emitted in either case. `ApplyServerUpdate` therefore touches
nothing but the local player's confirmed facing when it applies a snapback, and
the test suite asserts the whole map snapshot is byte-identical across a
refusal.

## Pruning

A row or floor command describes only the newly revealed fields. Tiles the
viewport left behind are dropped, and floors outside
`ViewportFloorRange(anchor.z)` are dropped whole, which is what lets a surface
to underground transition replace floors 5..0 with 8..10 while keeping 7 and 6.

`WorldState::known_creatures` is a mirror of
`reference/game/src/connections.hh::TConnection::KnownCreatureTable` and, like
the server's table, deliberately retains creatures that scrolled out of view;
`NewKnownCreature` only drops an entry when it needs the slot.
`WorldState::visible_creature_ids()` returns the creatures actually standing on
a stored tile, which is the set comparable with a freshly connected session.

## Verification

`clientcore/tests/movement_tests.cpp` covers:

* hand-computed golden hex for `SV_CMD_ROW_EAST`, `SV_CMD_ROW_NORTH`,
  `SV_CMD_FLOOR_DOWN`, `SV_CMD_FLOOR_UP`, an empty floor change,
  `SV_CMD_MOVE_CREATURE` and a refusal pair, each first reproduced byte for byte
  by the literal port of the server emitter;
* the row covering only the revealed edge, floor sets, floor-change ranges and
  the single-opcode case;
* the full `PlaceObject` insertion table, including the creature-below-item
  ordering;
* a twelve-step cardinal walk over a synthetic world where, after every step,
  the incrementally updated `WorldState` is compared against a freshly emitted
  `SV_CMD_FULLSCREEN` at the new position and must match exactly;
* the player's stack position following the priority rules after a step;
* a refusal leaving the entire map snapshot untouched;
* a surface-to-underground floor change keeping exactly floors 6..10 and
  pruning every tile outside the new window;
* `FIELD_DATA`, `ADD_FIELD`, `CHANGE_FIELD` and `DELETE_FIELD`, including
  creature removal;
* the creature mirror retaining a creature that scrolled out of view while
  `visible_creature_ids()` drops it;
* negatives: every truncation of each golden command, empty payload, empty type
  table, stack indexes at or beyond `MAX_OBJECTS_PER_POINT`, a row stepping off
  the addressable floors, an unsupported opcode consuming nothing, and the
  three application anomalies.

WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5: CTest 6/6 `PASS`
normally and under ASan/UBSan.

Live smoke against the sanitized runtime and the synthetic `ACCOUNT_A`: six
cardinal steps, each landing the anchor exactly where predicted with the
viewport synchronized and no anomalies; one observed snapback that changed
nothing; then a reconnect whose fresh `SV_CMD_FULLSCREEN` matched the walked
state tile for tile. Details in
`evidence/clientcore/MOVEMENT-772-001.md`.

## Limits

An opcode this layer does not decode yields
`ServerUpdateKind::Unsupported` with `bytes_consumed` zero, because its length
is unknown and guessing would desynchronize the stream. The login burst still
contains such commands after `SV_CMD_FULLSCREEN`, starting with
`SV_CMD_GRAPHICAL_EFFECT`, so a caller processing that first frame stops there.
Walk frames observed live contained movement commands only.
