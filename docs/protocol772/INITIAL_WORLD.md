# Protocol772Core Initial World (`FULLSCREEN`)

Task: `INITIALWORLD-772-001` (the roadmap previously listed this slot as
`WORLDSTATE-INIT-772-001`)

Status: `PASS` for deterministic byte fixtures, negative cases, ASan/UBSan and
one bounded local synthetic-account live smoke. This task decodes the initial
world snapshot only. Movement, combat, inventory, containers, chat, client
command encoding and Unreal remain out of scope and unimplemented.

## Boundary

```text
Game Login -> INIT_GAME (+ optional RIGHTS)
  -> preserved tail -> DecodeInitialWorld
       -> DecodeFullScreen -> FullScreenMessage
       -> ApplyFullScreen  -> WorldState
       -> remaining_bytes  -> named but unparsed server commands
```

`ParseGameInitialMessage` already recognized `FULLSCREEN` and preserved its
bytes verbatim. This layer consumes exactly those bytes; the Game Login layer is
unchanged.

## Source truth

| Rule | Source symbol | Interpretation | Confidence |
| --- | --- | --- | --- |
| Command value | `reference/game/src/connections.hh::ServerCommand` | `SV_CMD_FULLSCREEN = 100` | High |
| Header | `sending.cc::SendFullScreen` lines 445-448 | byte 100, LE word PlayerX, LE word PlayerY, byte PlayerZ | High |
| Terminal geometry | `connections.cc` lines 219-222 | offsets 8/6, width 18, height 14; assigned once in the constructor and never reassigned | High |
| Window | `sending.cc::SendFullScreen` lines 428-431 | `MinX = PlayerX - 8`, `MinY = PlayerY - 6`, `MaxX = MinX + 17`, `MaxY = MinY + 13` | High |
| Floor range | `sending.cc::SendFullScreen` lines 433-443 | `PlayerZ <= 7` gives floors 7..0 descending; otherwise `PlayerZ-2 .. min(PlayerZ+2, 15)` ascending | High |
| Scan order | `sending.cc::SendFullScreen` lines 451-457 | floor loop, then x outer, then y inner | High |
| Floor offset | `sending.cc::SendFullScreen` lines 452-455 | each point is read at `(x + ZOffset, y + ZOffset, z)` with `ZOffset = PlayerZ - PointZ` | High |
| Tile emission | `sending.cc::SendMapPoint` | a tile with objects flushes the pending skip, then emits up to `MAX_OBJECTS_PER_POINT = 10` objects in linked-list order; `Skip` is incremented for every point either way | High |
| Skip markers | `sending.cc::SkipFlush` | emits `(min(Skip, 255), 0xFF)` pairs until `Skip` is exhausted | High |
| Item | `sending.cc::SendItem` | LE word `getDisguise().TypeID`, then one byte of liquid colour if `LIQUIDCONTAINER` or `LIQUIDPOOL`, then one byte of amount if `CUMULATIVE` | High |
| Creature | `sending.cc::SendMapObject` lines 217-268 | words 99/98/97 select known, outdated and newly introduced descriptors | High |
| Outfit | `sending.cc::SendOutfit` and `cr.hh::TOutfit` | LE word outfit id; `0` is followed by a LE word object type, otherwise by four colour bytes (the struct union makes them exclusive) | High |
| String | `sending.cc::SendString` | LE word length then raw bytes; `strlen` is cast to `uint16`, so the `0xFFFF` escape of the RSA login block never appears | High |
| Known-creature table | `connections.hh::TConnection` and `connections.cc::NewKnownCreature` | 150 slots; word 97 carries the id of the slot being evicted, `0` when the slot was free | High |
| Health | `crmain.cc::TCreature::GetHealth` | a percentage of maximum hit points, floored to `1` while alive | High |
| Login order | `crplayer.cc` lines 199-206 | `SendInitGame`, `SendRights`, `SendFullScreen`, then `GraphicalEffect` and the rest of the login burst | High |
| Frame atomicity | `communication.cc::SendData` and `sending.cc::FinishSendData` | everything committed is flushed as one packet, and an overflowing command is discarded whole rather than truncated, so a `FULLSCREEN` never straddles two frames | High |

## Wire layout

```text
100                      SV_CMD_FULLSCREEN
u16 PlayerX
u16 PlayerY
u8  PlayerZ
repeat per scanned position, in floor / x / y order:
    tile data or skip marker
```

A scanned position is either described or skipped:

* **Described tile** - one or more objects, then the `(N, 0xFF)` marker that
  both terminates the object list and announces that the next `N` positions are
  empty.
* **Skip marker at a fresh position** - `(N, 0xFF)` means this position and the
  next `N` are empty.

A marker is recognised by peeking a little-endian word `>= 0xFF00`. The
asymmetry above is exactly what the server produces: `SkipFlush` drains a run of
`S` empty positions into markers whose counts sum to `S + 1`, and the first
marker loses one position when it doubles as a tile terminator. The final
`SkipFlush` always emits at least `(0, 0xFF)`, so every `FULLSCREEN` ends on a
marker and the described and skipped counts sum exactly to
`floors * 18 * 14`.

### Objects

```text
u16 word
  word >= 0xFF00  -> skip marker, ends the tile
  word == 99      -> known creature:    u32 id, u8 direction
  word == 98      -> outdated creature: u32 id, descriptor
  word == 97      -> new creature:      u32 evicted id, u32 id, string name, descriptor
  otherwise       -> item type id, plus its flag-driven extra bytes

descriptor := u8 health%, u8 direction, outfit,
              u8 light brightness, u8 light colour, u16 speed,
              u8 playerkilling mark, u8 party mark
outfit     := u16 outfit id, then u16 object type when the id is 0,
              otherwise four colour bytes
```

## The item encoding is not self-describing

`SendItem` decides an item's length from server-side object type flags that the
wire never carries. A decoder therefore needs the same table, which is why
`ObjectTypeTable` is an explicit dependency rather than hidden knowledge.
`LoadObjectTypeTableFromObjectsSrv` reads the `TypeID` and `Flags` records of
the server's own `dat/objects.srv`; nothing else in that file affects decoding.

`tests/verify_object_type_invariants.py` proves the following against the
shipped `dat/objects.srv` (5003 declared types, ids 0..5090):

1. Declared ids are `0..10`, `99` and `100..5090`. Nothing occupies `11..98`,
   so the creature markers `97` and `98` cannot collide with an item, and `99`
   is the creature container itself, which `SendMapObject` never routes through
   `SendItem`.
2. The highest id is far below `0xFF00`, so a type id can never be mistaken for
   a skip marker.
3. No type carries both liquid flags, and no type is both cumulative and
   liquid. At most one extra byte follows a type id.
4. `SendItem` writes `getDisguise().TypeID` but reads the flags from the
   original type. All 74 disguise types share their target's wire-relevant flag
   set, so the id on the wire is sufficient. This is a property of this dataset,
   not a guarantee of the protocol, and it is re-checked by the script.

## Decoder behaviour

`DecodeFullScreen` walks the payload once with a bounds-checked cursor. Every
failure is an explicit `MapDecodeError` carrying the byte offset and a short
detail; no branch falls back to a default or drops bytes.

| Error | Meaning |
| --- | --- |
| `NotFullScreen` | the payload does not begin with opcode 100 |
| `Truncated` | a read ran past the end of the payload |
| `InvalidPlayerFloor` | `PlayerZ > 15`, which the server's floor loop cannot address |
| `ReservedObjectTypeId` | a server-internal container type appeared as a map object |
| `UnknownObjectTypeId` | the type table does not declare the id, so the length is unknown |
| `TooManyObjectsInTile` | more than `MAX_OBJECTS_PER_POINT` objects without a marker |
| `CreatureNameTooLong` | a name longer than `TCreature::Name` can hold |
| `SkipRunExceedsWindow` | a skip run outlived the scanned window |
| `EmptyObjectTypeTable` | no type table was supplied |

Order is preserved throughout: tiles keep emission order within a floor, and a
thing's index inside `MapTile::things` is its stack position. Bytes after the
`FULLSCREEN` stay in `remaining_bytes`, and `DecodeInitialWorld` names the next
command through `ServerCommandName` without parsing it.

## WorldState

`WorldState` holds only what this message establishes: the map window, the
described floors and tiles, and a mirror of the server's known-creature table.
`ApplyFullScreen` folds creature descriptors in emission order, applies word-97
evictions, and reports `WorldStateAnomaly` entries when a descriptor references
or evicts a creature the mirror does not hold. Anomalies never discard data; the
referenced creature still enters the mirror with whatever the protocol provided.

## Verification

`clientcore/tests/initial_world_tests.cpp` covers:

* three hand-computed golden hex fixtures (an empty surface screen, an empty
  underground screen and a populated tile) checked byte for byte;
* `FullScreenEncoder`, a literal port of `SendFullScreen`, `SendMapPoint`,
  `SkipFlush`, `SendMapObject`, `SendItem` and `SendOutfit`, asserted to
  reproduce each golden fixture exactly before it is used for richer cases;
* floor offsets, scan order, adjacent tiles separated by `(0, 0xFF)`, skip runs
  crossing a floor boundary, and saturated marker splitting;
* all three creature descriptor kinds and both outfit branches;
* every truncation of the populated golden message, each required to fail as
  `Truncated`;
* reserved and unknown type ids, an eleven-object tile, an over-wide skip run,
  an over-long creature name and the exact-limit name;
* `objects.srv` loader positives and four malformed inputs;
* `WorldState` application, eviction, anomaly reporting and the tie-in with
  `ParseGameInitialMessage`.

WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5: CTest 5/5 `PASS`
normally and under ASan/UBSan.

Live smoke against the sanitized runtime and the synthetic `ACCOUNT_A`: the real
2075-byte `FULLSCREEN` decoded into 8 floors, 408 described tiles, 1608 skipped
tiles (2016 scanned, exactly the window), 587 things and 2 creatures with zero
anomalies, and re-encoded byte for byte through the server port. The 125
trailing bytes were preserved and identified as `SV_CMD_GRAPHICAL_EFFECT`,
matching `crplayer.cc` line 202. Details in
`evidence/clientcore/INITIALWORLD-772-001.md`.
