# Architecture

## Target boundaries

The wire/crypto path below is source-traced but `UNVERIFIED` until packet fixtures and live tests exist.

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

## Source architecture discovered

The archived Fusion32 system consists of game, login, querymanager, web, and ipchanger Git bundles plus a large legacy runtime. The game README states querymanager is required for game startup, login produces character lists, and web manages accounts. Game sources use GNU Make, C++11, Linux/pthreads, librt, and OpenSSL libcrypto. No Unreal tree or classic client was found.

Protocol 7.72 is compile-time behavior (`TIBIA772`). The default archived game Makefile does not enable it and therefore must not be treated as a verified 7.72 build.
