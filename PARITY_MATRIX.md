# Parity Matrix

| Feature | Server understood | Protocol specified | Unreal implemented | Tested | 2D<->3D parity | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Connection | Service topology/runtime PASS; classic live route PASS; client outer framing and crypto source-traced | Transport/framing/RSA/XTEA, Login and Game Login implemented; persistent session smoke PASS | No | 4/4 retained/new suites normal and sanitized; bounded Game Login smoke PASS | No | IN_PROGRESS |
| Login / character list | Source traced; sanitized Login runtime and exact classic client flow PASS | Source-traced request/response and typed character list implemented | No | `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and `CLIENTCORE-LOGIN-772-001` PASS | No | IN_PROGRESS |
| Game login | Source traced; sanitized Game runtime and classic `JoinGame` flow PASS | Source-traced request, initial messages and persistent session implemented; fullscreen decoding now handed to the Initial World layer | No | `CLASSIC-GAME-ENTRY-772-001` and `CLIENTCORE-GAMELOGIN-772-001` PASS | No | IN_PROGRESS |
| Keepalive | Located | No | No | No | No | NOT_STARTED |
| Map | Source traced for the `FULLSCREEN` snapshot: window, scan order, skip markers, tile stacks | `FULLSCREEN` decoded into a minimal WorldState; map deltas and row/field updates not started | No | `INITIALWORLD-772-001` PASS including a live byte-for-byte round trip | No | IN_PROGRESS |
| Floors | Source traced: floor ranges and per-floor offsets inside `FULLSCREEN` | Floor set and offsets decoded; `FLOOR_UP`/`FLOOR_DOWN` not started | No | `INITIALWORLD-772-001` fixtures cover surface and underground ranges; live run observed the surface range only | No | IN_PROGRESS |
| Movement | Located | No | No | No | No | NOT_STARTED |
| Creatures | Source traced for the three `FULLSCREEN` descriptor forms and the known-creature table | Descriptors decoded and mirrored in WorldState; creature update commands not started | No | `INITIALWORLD-772-001` PASS; two live creatures decoded with no anomalies | No | IN_PROGRESS |
| Items | Source traced for map items, including the object type flags the wire omits | Map item encoding decoded via an explicit object type table; inventory and container items not started | No | `INITIALWORLD-772-001` PASS; extra-byte paths verified against the real `objects.srv` | No | IN_PROGRESS |
| Inventory | Located | No | No | No | No | NOT_STARTED |
| Containers | Located | No | No | No | No | NOT_STARTED |
| Use / use-with | Located | No | No | No | No | NOT_STARTED |
| Combat / follow | Located | No | No | No | No | NOT_STARTED |
| Magic / effects | Located | No | No | No | No | NOT_STARTED |
| Stats / skills | Located | No | No | No | No | NOT_STARTED |
| Chat | Located | No | No | No | No | NOT_STARTED |
| NPC | Located | No | No | No | No | NOT_STARTED |
| Trade | Located | No | No | No | No | NOT_STARTED |
| Death / loot | Located, untraced | No | No | No | No | NOT_STARTED |
| Social systems | Partial locations | No | No | No | No | NOT_STARTED |

`Located` means a likely authoritative implementation symbol was found. It is not protocol understanding and never means `PASS`.

`SERVER-RUNTIME-SMOKE-001` is infrastructure evidence only. The classic-client PASS results validate the 2D baseline, not an Unreal client. They do not change any 2D-to-3D parity cell to PASS.

`CLIENTCORE-TRANSPORT-772-001 = PASS` is limited to the portable TCP and outer-framing component. It does not prove crypto, application login, world entry or 2D-to-3D parity.

`CLIENTCORE-CRYPTO-772-001 = PASS` is limited to deterministic RSA/XTEA, key lifecycle and encrypted inner-packet behavior. It does not prove application Login, Game entry or parity.

`INITIALWORLD-772-001 = PASS` is limited to decoding the one `FULLSCREEN` snapshot the server sends at login and folding it into a minimal WorldState. It proves no map delta, no movement, no rendering and no 2D-to-3D parity. The Map, Floors, Creatures and Items rows are `IN_PROGRESS` for that snapshot only.
