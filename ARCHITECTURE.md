# Architecture

## Target boundaries

The Unreal-independent TCP/outer-framing and Crypto portions below are implemented and deterministically tested under `CLIENTCORE-TRANSPORT-772-001 = PASS` and `CLIENTCORE-CRYPTO-772-001 = PASS`. Protocol commands/events, WorldState and Unreal remain unimplemented or unverified as stated below.

```text
Classic Tibia 7.72 ---------+
                           v
                    Fusion32 server
                           ^
                           |
Unreal input -> ClientCommand encoder -> TCP/framing -> RSA/XTEA
                                                     |
Unreal presentation <- Unreal adapter <- WorldState <- semantic event queue
                                                     ^
                           TCP/framing -> RSA/XTEA -> Protocol772 parser
```

Fusion32 owns validity, movement, combat, loot, stats, creatures, items, containers, NPCs, and quests. The Unreal client owns input capture, protocol representation, logical client state, and 3D presentation only.

## Required separation

- `Transport`: TCP I/O and byte ownership.
- `Packet framing`: outer little-endian length and encrypted payload boundaries.
- `Crypto`: RSA login block and XTEA session blocks.
- `Protocol772`: bytes to/from typed commands and semantic events.
- `WorldState`: player, tile thing stacks, creatures, items, inventory, containers, combat, stats, skills, chat, and NPC-visible state.
- `Unreal adapter`: consumes stable WorldState changes on the game thread.
- `Presentation`: placeholder or production visuals; never gameplay authority.

Expected threading boundary: network thread produces immutable semantic events; the game thread applies them to WorldState and presentation. This is a design constraint, not yet an implemented or tested model.

## Implemented client-core boundary

`clientcore/` is a portable C++17 component with no Unreal dependency:

```text
TCP byte stream
    -> TcpTransport
    -> FrameDecoder / EncodeFrame
    -> owned FramedPacket
    -> RSA/XTEA CryptoStage
    -> Login / Game Login
    -> Initial World (FULLSCREEN) -> WorldState
    -> Movement (ROW / FLOOR / FIELD / MOVE_CREATURE / SNAPBACK) -> WorldState
    -> Player State (stats / skills / state / ambience / creature updates) -> WorldState
    -> future Protocol772 decoders for chat, containers and trade
```

`TcpTransport` owns socket lifecycle and byte I/O. `FrameDecoder` incrementally preserves partial input and extracts every complete outer packet in order. `FramedConnection` composes them without crypto or protocol behavior. `protocol772_crypto` performs RSA public operations, XTEA key/block processing and encrypted inner-length/padding validation. `protocol772_login` builds the source-traced 7.72 Login request and parses only MOTD/error/character-list responses. `protocol772_gamelogin` builds Game Login, owns the persistent session and recognizes only initial authentication messages; fullscreen/map bytes remain preserved. `protocol772_initial_world` consumes exactly those preserved bytes, decodes the `FULLSCREEN` snapshot and folds it into a minimal `WorldState`; every other server command stays named but unparsed. No layer calls Unreal. Full behavior and source traceability are in `docs/protocol772/TRANSPORT.md`, `docs/protocol772/CRYPTO.md`, `docs/protocol772/LOGIN.md`, `docs/protocol772/GAMELOGIN.md` and `docs/protocol772/INITIAL_WORLD.md`.

`protocol772_movement` adds the client walk/turn/stop commands and the incremental server updates a step produces. `map_scan` owns the tile and skip-marker walk that `SV_CMD_FULLSCREEN`, `SV_CMD_ROW_*`, `SV_CMD_FLOOR_UP/DOWN` and `SV_CMD_FIELD_DATA` all share, mirroring how the server shares `SendMapPoint` and `SkipFlush` between them. Behaviour, source traceability and limits are in `docs/protocol772/MOVEMENT.md`.

The map encoding is not self-describing in two places, and both are handled the same way. `reference/game/src/sending.cc::SendItem` decides an item's on-wire length from server object type flags the protocol never carries, and `reference/game/src/map.cc::PlaceObject` decides where an added object lands in a tile stack from a priority derived from other flags, while `SV_CMD_ADD_FIELD` carries no stack index. `ObjectTypeTable` therefore holds both the length flags and the priority, and is an explicit, injected dependency of the decoders, loaded from the server's own `dat/objects.srv`, rather than knowledge baked into a parser.

`protocol772_player_state` closes the command set an ordinary session emits, so a caller can walk a decrypted payload to its end rather than stopping at the first opcode it cannot size. It routes through the same single `DecodeServerUpdate` entry point. Only commands with demonstrated semantics reach `WorldState`: player stats, skills, state flags, ambient light and the creature attribute updates. Effects, inventory, the buddy list and the first-login outfit chooser are decoded, typed and surfaced but store nothing, because those features are not yet claimed. Details in `docs/protocol772/PLAYER_STATE.md`.

`SV_CMD_ROW_*` and `SV_CMD_FLOOR_UP/DOWN` carry no coordinates at all: `reference/game/src/cract.cc::TCreature::NotifyGo` advances the player one axis at a time and only then emits them. `WorldState` therefore tracks a `viewport_anchor` that those commands step, while `SV_CMD_MOVE_CREATURE` never does. Once a step completes the anchor and the local player's creature must agree, which turns viewport desynchronization into a reported condition instead of silent drift.

## Source architecture discovered

The archived Fusion32 system consists of game, login, querymanager, web, and ipchanger Git bundles plus a large legacy runtime. The game README states querymanager is required for game startup, login produces character lists, and web manages accounts. Game sources use GNU Make, C++11, Linux/pthreads, librt, and OpenSSL libcrypto. No Unreal tree or classic client was found.

Protocol 7.72 is compile-time behavior (`TIBIA772`). The default archived game Makefile does not enable it and therefore must not be treated as a verified 7.72 build.
