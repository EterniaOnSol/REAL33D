# Parity Matrix

| Feature | Server understood | Protocol specified | Unreal implemented | Tested | 2D<->3D parity | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Connection | Service topology/runtime PASS; classic live route PASS; client outer framing and crypto source-traced | Transport/framing/RSA/XTEA, Login and Game Login implemented; persistent session smoke PASS | No | 4/4 retained/new suites normal and sanitized; bounded Game Login smoke PASS | No | IN_PROGRESS |
| Login / character list | Source traced; sanitized Login runtime and exact classic client flow PASS | Source-traced request/response and typed character list implemented | No | `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and `CLIENTCORE-LOGIN-772-001` PASS | No | IN_PROGRESS |
| Game login | Source traced; sanitized Game runtime and classic `JoinGame` flow PASS | Source-traced request, initial messages and persistent session implemented; fullscreen decoding now handed to the Initial World layer | No | `CLASSIC-GAME-ENTRY-772-001` and `CLIENTCORE-GAMELOGIN-772-001` PASS | No | IN_PROGRESS |
| Keepalive | Source traced: server-initiated `SendPing` from the connection timer and `EmergencyPing`; `CPing` is a no-op | `SV_CMD_PING` decoded and `CL_CMD_PING` built | No | `PLAYERSTATE-772-001` fixtures; not observed in the short live session | No | IN_PROGRESS |
| Map | Source traced for `FULLSCREEN` and the incremental deltas: window, scan order, skip markers, tile stacks, viewport anchor and pruning | `FULLSCREEN`, `ROW_*`, `FIELD_DATA` and `ADD`/`CHANGE`/`DELETE_FIELD` decoded and applied to WorldState | No | `INITIALWORLD-772-001` and `MOVEMENT-772-001` PASS; live walk matched a fresh server `FULLSCREEN` tile for tile | No | IN_PROGRESS |
| Floors | Source traced: the `FULLSCREEN` floor range and the `SendFloors` sets, with the anchor shift NotifyGo applies | Floor set, offsets, `FLOOR_UP`/`FLOOR_DOWN` and the per-floor pruning decoded and applied | No | `MOVEMENT-772-001` fixtures cover both directions and the empty case; no live floor change observed | No | IN_PROGRESS |
| Movement | Source traced: `Move` ordering, `NotifyGo`, `CGoDirection`, the refusal paths | Cardinal walk/turn/stop commands and `MOVE_CREATURE`/`SNAPBACK`/`MESSAGE` implemented; diagonals and `GO_PATH` deliberately unexposed | No | `MOVEMENT-772-001` PASS; six live cardinal steps plus an observed snapback that changed nothing | No | IN_PROGRESS |
| Creatures | Source traced for the three descriptor forms, the known-creature table, creature relocation and the six attribute updates | Descriptors, the known-creature mirror, creature moves and the 140-145 attribute updates decoded and applied | No | `INITIALWORLD-772-001`, `MOVEMENT-772-001` and `PLAYERSTATE-772-001` PASS; live visible creature set matched the server with no phantoms | No | IN_PROGRESS |
| Items | Source traced for map items, including the object type flags and stack priority the wire omits | Map item encoding and `PlaceObject` stack insertion decoded via an explicit object type table; inventory and container items not started | No | `INITIALWORLD-772-001` and `MOVEMENT-772-001` PASS; extra-byte and priority paths verified against the real `objects.srv` | No | IN_PROGRESS |
| Inventory | Located | No | No | No | No | NOT_STARTED |
| Containers | Located | No | No | No | No | NOT_STARTED |
| Use / use-with | Located | No | No | No | No | NOT_STARTED |
| Combat / follow | Located | No | No | No | No | NOT_STARTED |
| Magic / effects | Source traced for the wire form of the graphical, textual and missile effects and creature marking | Decoded and surfaced as typed events; they carry no WorldState semantics and store nothing. Spell casting itself is untouched | No | `PLAYERSTATE-772-001` fixtures; one live graphical effect observed at login | No | IN_PROGRESS |
| Stats / skills | Source traced: `SendPlayerData`, `SendPlayerSkills`, `SendPlayerState` and the `CheckState` flag table | Decoded and applied to WorldState; `PLAYER_STATE` is only emitted when the flags change | No | `PLAYERSTATE-772-001` PASS; live stats and skills matched a fresh Rookgaard character | No | IN_PROGRESS |
| Chat | Located | No | No | No | No | NOT_STARTED |
| NPC | Located | No | No | No | No | NOT_STARTED |
| Trade | Located | No | No | No | No | NOT_STARTED |
| Death / loot | Located, untraced | No | No | No | No | NOT_STARTED |
| Social systems | Partial locations | No | No | No | No | NOT_STARTED |

`Located` means a likely authoritative implementation symbol was found. It is not protocol understanding and never means `PASS`.

`SERVER-RUNTIME-SMOKE-001` is infrastructure evidence only. The classic-client PASS results validate the 2D baseline, not an Unreal client. They do not change any 2D-to-3D parity cell to PASS.

`CLIENTCORE-TRANSPORT-772-001 = PASS` is limited to the portable TCP and outer-framing component. It does not prove crypto, application login, world entry or 2D-to-3D parity.

`CLIENTCORE-CRYPTO-772-001 = PASS` is limited to deterministic RSA/XTEA, key lifecycle and encrypted inner-packet behavior. It does not prove application Login, Game entry or parity.

`INITIALWORLD-772-001 = PASS` is limited to decoding the one `FULLSCREEN` snapshot the server sends at login and folding it into a minimal WorldState. It proves no rendering and no 2D-to-3D parity.

`MOVEMENT-772-001 = PASS` is limited to cardinal walking and the incremental map updates a step produces. It proves no combat, inventory, container, chat, diagonal or path movement, no rendering and no 2D-to-3D parity. Its live evidence covers surface walking on floor 7 only; floor changes and the field commands are fixture-covered.

`PLAYERSTATE-772-001 = PASS` closes the command set an ordinary session emits: a real login burst and walking session decoded with zero residual bytes and no unsupported opcode. It proves decoding, not behaviour. Inventory, buddy, outfit-chooser and effect commands are decoded for structure and length but deliberately store nothing, and chat, containers, trade, the request queue and the editors remain undecoded and still report by name with zero bytes consumed. No row in this matrix reaches `PASS` on protocol decoding alone.
