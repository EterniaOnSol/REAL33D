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
    -> future Protocol772 decoder
```

`TcpTransport` owns socket lifecycle and byte I/O. `FrameDecoder` incrementally preserves partial input and extracts every complete outer packet in order. `FramedConnection` composes them without crypto or protocol behavior. `protocol772_crypto` performs RSA public operations, XTEA key/block processing and encrypted inner-length/padding validation. `protocol772_login` builds the source-traced 7.72 Login request and parses only MOTD/error/character-list responses. `protocol772_gamelogin` builds Game Login, owns the persistent session and recognizes only initial authentication messages; fullscreen/map bytes remain preserved. Neither layer applies gameplay state or calls Unreal. Full behavior and source traceability are in `docs/protocol772/TRANSPORT.md`, `docs/protocol772/CRYPTO.md`, `docs/protocol772/LOGIN.md` and `docs/protocol772/GAMELOGIN.md`.

## Source architecture discovered

The archived Fusion32 system consists of game, login, querymanager, web, and ipchanger Git bundles plus a large legacy runtime. The game README states querymanager is required for game startup, login produces character lists, and web manages accounts. Game sources use GNU Make, C++11, Linux/pthreads, librt, and OpenSSL libcrypto. No Unreal tree or classic client was found.

Protocol 7.72 is compile-time behavior (`TIBIA772`). The default archived game Makefile does not enable it and therefore must not be treated as a verified 7.72 build.
