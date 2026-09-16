# Parity Matrix

| Feature | Server understood | Protocol specified | Unreal implemented | Tested | 2D<->3D parity | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Connection | Service topology/runtime PASS; client framing partial | Partial | No | Network connect/close only | No | IN_PROGRESS |
| Login / character list | Source traced; sanitized Login runtime PASS | No | No | No classic client | No | IN_PROGRESS |
| Game login | Source traced; sanitized Game runtime PASS | Partial | No | No classic client | No | IN_PROGRESS |
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

`SERVER-RUNTIME-SMOKE-001` is infrastructure evidence only. It does not change any 2D-to-3D parity cell to PASS.
