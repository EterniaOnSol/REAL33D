# Parity Matrix

| Feature | Server understood | Protocol specified | Unreal implemented | Tested | 2D<->3D parity | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Connection | Service topology/runtime PASS; classic live route PASS; client outer framing and crypto source-traced | Transport/framing/RSA/XTEA implemented; application Login pending | No | Transport 20/20 and Crypto 25/25 normal/sanitized tests PASS; classic Login->Game connection and >30-minute session PASS | No | IN_PROGRESS |
| Login / character list | Source traced; sanitized Login runtime and exact classic client flow PASS | Source-traced request/response and typed character list implemented; live new-client exchange unverified | No | `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and offline `CLIENTCORE-LOGIN-772-001` PASS | No | IN_PROGRESS |
| Game login | Source traced; sanitized Game runtime and classic `JoinGame` flow PASS | Partial, no byte fixtures | No | `CLASSIC-GAME-ENTRY-772-001` PASS | No | IN_PROGRESS |
| Keepalive | Located | No | No | No | No | NOT_STARTED |
| Map | Located | No | No | No | No | NOT_STARTED |
| Floors | Located | No | No | No | No | NOT_STARTED |
| Movement | Located | No | No | No | No | NOT_STARTED |
| Creatures | Located | No | No | No | No | NOT_STARTED |
| Items | Located | No | No | No | No | NOT_STARTED |
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
