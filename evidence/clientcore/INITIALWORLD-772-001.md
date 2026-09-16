# INITIALWORLD-772-001

Status: `PASS`

Supersedes the roadmap placeholder `WORLDSTATE-INIT-772-001`.

## Implementation

Added `protocol772_initial_world` on top of the validated Transport, Crypto,
Login and Game Login layers:

- `object_types` - the `LIQUIDCONTAINER`/`LIQUIDPOOL`/`CUMULATIVE` table the
  item encoding depends on, plus a parser for the server's `dat/objects.srv`.
- `worldstate` - the minimal structures the decoded message supports: map
  window, floors, ordered tile stacks, item and creature things, and a mirror of
  the server known-creature table.
- `initial_world` - the bounded `FULLSCREEN` decoder, `ApplyFullScreen`, the
  `DecodeInitialWorld` facade and `ServerCommandName`.

`gamelogin` is unchanged; this layer consumes the bytes it already preserved.

## Source traceability

See [`docs/protocol772/INITIAL_WORLD.md`](../../docs/protocol772/INITIAL_WORLD.md).
Primary symbols are `reference/game/src/sending.cc::SendFullScreen`,
`SendMapPoint`, `SkipFlush`, `SendMapObject`, `SendItem`, `SendOutfit`,
`SendString`, `reference/game/src/connections.cc` lines 219-222 and
`NewKnownCreature`, `reference/game/src/connections.hh::TConnection` and
`ServerCommand`, `reference/game/src/objects.hh::TYPEID_*`,
`reference/game/src/crmain.cc::TCreature::GetHealth`,
`reference/game/src/crplayer.cc` lines 199-206, and
`reference/game/src/communication.cc::SendData`.

## Data invariants

`tests/verify_object_type_invariants.py tibia-game.tarball.tar.gz`:

```text
declared object types: 5003 (min 0, max 5090)
reserved container ids present: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
disguise types: 74, wire-relevant mismatches: 0
types needing an extra byte: liquid 35, cumulative 81
verify_object_type_invariants: PASS
```

This establishes that nothing is declared in `11..98`, that the creature markers
`97`/`98`/`99` cannot collide with a map item, that no type id approaches the
`0xFF00` skip-marker page, that at most one extra byte follows a type id, and
that every disguise target shares its source's wire-relevant flags.

## Tests

Environment: Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5,
C++17, warnings as errors.

- Debug build: `PASS`
- CTest: `5/5 PASS` (Transport, Crypto, Login, Game Login, Initial World)
- ASan/UBSan CTest: `5/5 PASS`
- Hand-computed golden hex fixtures, byte for byte: `PASS`
  - empty surface screen, 22 bytes, floors 7..0, 2016 skipped positions
  - empty underground screen, 16 bytes, floors 6..10, 1260 skipped positions
  - populated tile, four things including a creature, cumulative and liquid item
- `FullScreenEncoder` (a literal port of the server emitter) reproduces each
  golden fixture exactly: `PASS`
- Structural cases - floor offsets, scan order, `(0, 0xFF)` between adjacent
  tiles, skip run crossing a floor boundary, saturated marker splitting,
  ten-object tile, all three creature kinds, both outfit branches: `PASS`
- Negative cases - every truncation of the populated golden message, wrong
  opcode, empty payload, empty type table, `PlayerZ > 15`, reserved container
  id, unknown type id, eleven-object tile, over-wide skip run, over-long
  creature name, and four malformed `objects.srv` inputs: `PASS`
- `WorldState` application, word-97 eviction and anomaly reporting: `PASS`
- Tie-in with `ParseGameInitialMessage`'s preserved tail: `PASS`

## Bounded live smoke

A temporary non-tracked harness used only the local synthetic `ACCOUNT_A` and
the runtime's public modulus, both read at run time and never recorded. It
performed Login on `127.0.0.1:7171`, Game Login on `127.0.0.1:7172`, and decoded
the real `FULLSCREEN`:

```text
object type table: declared=5003 max_type_id=5090
character list: 1 entry, game port 7172
game message: FullScreenUnparsed bytes=2208
  INIT_GAME creature_id=1001 beat=50 bug_reports=0
preserved FULLSCREEN tail: 2200 bytes
FULLSCREEN decoded
  player=(32097,32219,7)
  window min=(32089,32213) size=18x14
  floors=8 first=7 last=0 step=-1
  described_tiles=408 skipped_tiles=1608 scanned=2016
  things=587 creatures=2
  bytes_consumed=2075 remaining=125
  next unsupported command: SV_CMD_GRAPHICAL_EFFECT (opcode 131) at offset 2075
WorldState: tiles=408 things=587 known_creatures=2 anomalies=0
  creature id=1001 name=[Test Player A] health=100 dir=2 outfit=128 speed=220 at=(32097,32219,7) stack=1
  creature id=1073741827 name=[Cipfried] health=100 dir=3 outfit=57 speed=100 at=(32097,32217,7) stack=1
player tile stack (2):
  [0] item type=410
  [1] creature Introduced id=1001
item coverage: liquid=0 cumulative=0 max_type_id=4815
round trip: 2075 bytes re-encoded identically
real-table extra bytes: liquid type 2524 and cumulative type 1781 decoded correctly
LIVE PASS
```

What this demonstrates:

- `described_tiles + skipped_tiles == 2016`, exactly `8 * 18 * 14`, so the skip
  arithmetic consumed the window with no drift;
- re-encoding the decoded world through the literal server port reproduced all
  2075 bytes, so the decode preserved every field, stack and skip run;
- the byte immediately after the message is `SV_CMD_GRAPHICAL_EFFECT`, exactly
  the command `crplayer.cc` line 202 emits after `SendFullScreen`, and its 125
  bytes were preserved rather than consumed;
- both creatures arrived as word-97 introductions with no anomalies, consistent
  with an empty known-creature table at login;
- the player's own character decoded at stack position 1 above ground type 410,
  and `Cipfried` is the Rookgaard NPC the sanitized world actually contains.

Three consecutive runs produced identical structure; only `Cipfried`'s position
and direction varied, as expected for a wandering NPC.

The temporary harness source and binary were deleted and the runtime services
stopped after the run. No account id, password, modulus or key material appears
in this evidence.

## Remaining UNVERIFIED

- The observed screen carried no liquid or cumulative item, so those two
  extra-byte paths were exercised against the real `objects.srv` table with
  synthetic bytes rather than with server-emitted bytes.
- Only floors 7..0 were observed live; the underground floor range is covered by
  fixtures only.
- Native Windows execution and independent repetition remain unverified.

## Scope exclusions

No movement, combat, inventory, containers, chat, client command encoding or
Unreal integration was started. Every server command other than `FULLSCREEN`
remains unparsed and is reported by name.
