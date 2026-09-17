# Roadmap

## Gate 0 - controlled server and classic functional baseline (`IN_PROGRESS` for classic certification)

1. `BOOTSTRAP-SOURCES-001` (`PASS`): verified all bundles, compared loose snapshots, selected independently reviewed immutable candidates, materialized curated references, and produced clean Linux builds.
2. `SERVER-BASELINE-772-001` (`PASS`): sanitized preparation, fresh key/config/data, three-service startup, internal authorization, world load, network smoke and shutdown are `CERTIFIED` under independently repeated `SERVER-RUNTIME-SMOKE-001`; classic Login, character list, Game entry and a sustained session also passed against this baseline.
3. `CLASSIC-CLIENT-772-001` (`IN_PROGRESS`): the official Fusion32 IP Changer source is selected, materialized and `BUILD PASS`; its runtime-specific 7.72 configuration is reproducibly generated. The selected local EXE/DAT/SPR/PIC set is hashed, statically validated and live-compatible. `IPCHANGER-772-LIVE-001`, `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001`, `CLASSIC-GAME-ENTRY-772-001` and `CLASSIC-SESSION-SUSTAIN-001` are `PASS`. Historical client acquisition provenance is `UNKNOWN` and independent functional repetition is pending.
4. `PROTOCOL-LOGIN-001`: transport, framing, RSA/XTEA, character-list Login and Game Login are source-traced with byte fixtures; independent review remains required for certification.

Gate: selected source revisions, repeatable build commands, and fixture tests for login/framing/crypto. Documentation alone does not close the gate.

## Gate 1 - Protocol772Core (`IN_PROGRESS`)

1. `CLIENTCORE-TRANSPORT-772-001` (`PASS`): portable TCP lifecycle, buffered stream reads/writes, source-traced two-byte outer framing, endpoint/direction limits, explicit errors and 20 deterministic normal plus ASan/UBSan cases. Native Windows execution and independent repetition remain unverified; neither is required to begin the next bounded task.
2. `CLIENTCORE-CRYPTO-772-001` (`PASS`): source-traced raw 128-byte RSA public operation, decimal modulus/fixed exponent, secure move-only XTEA key lifecycle, exact XTEA inner length/padding and byte-for-byte fixtures; 25 Crypto plus 20 retained Transport cases pass normally and under ASan/UBSan.
3. `CLIENTCORE-LOGIN-772-001` (`PASS`): character-list request/response and typed result, with deterministic fixtures and bounded local synthetic-account smoke.
4. `CLIENTCORE-GAMELOGIN-772-001` (`PASS`): Game Login, authenticated persistent session and initial `INIT_GAME`/optional `RIGHTS`/recognized-unparsed `FULLSCREEN` handoff, without implementing the full opcode surface.
5. `INITIALWORLD-772-001` (`PASS`): the `FULLSCREEN` initial world snapshot - header, floor ranges, scan order, skip markers, ordered tile stacks, items and all three creature descriptor forms - decoded into a minimal `WorldState`, with the object type flag table it depends on, golden fixtures, negative tests and a live round trip that re-encoded the real message byte for byte. Every other server command stays named but unparsed.
6. `MOVEMENT-772-001` (`PASS`): cardinal walk commands, the coordinate-less `SV_CMD_ROW_*` and `SV_CMD_FLOOR_UP/DOWN` resolved through a source-traced viewport anchor, `MOVE_CREATURE`, the four field commands, `SNAPBACK` and `MESSAGE`, plus the `PlaceObject` stack priority the wire omits. Golden fixtures, negative tests and a live walk whose final state matched a fresh server `FULLSCREEN` tile for tile. Diagonals are documented and left unexposed.
7. `PLAYERSTATE-772-001` (`PASS`): ping, ambience, the four effect commands, the six creature attribute updates, player data/skills/state, clear target, the inventory pair, the buddy trio and the first-login outfit chooser. A real login burst and ordinary session traffic now decode with zero residual bytes and no unsupported opcode. Only commands with demonstrated semantics reach WorldState.

Gate: deterministic byte fixtures, negative tests and source traceability for each completed layer. Unreal does not enter this gate.

## Shortest evidence-based path to `TWO-CLIENT-VERTICAL-SLICE-001`

1. Selected candidates, curated references, explicit 7.72 builds and a resettable sanitized runtime are complete. Independently repeated `SERVER-RUNTIME-SMOKE-001` certifies Query Manager -> Game -> Login startup, world load and clean shutdown within its stated scope.
2. The selected operator-supplied local Tibia 7.72 EXE/DAT/SPR/PIC set is hashed and functionally usable; original source/chain of custody remains `UNKNOWN`. Obtain a verifiably sourced authorized copy only if provenance certification is required.
3. The built Fusion32 IP Changer revision `8215db18...` and generated `fusion32` entry passed static address checks and live host/port/fresh-modulus patching against the selected executable. Never execute an archived IP Changer binary.
4. Classic character-list login, Game entry, initial world processing and a session exceeding 30 minutes are `PASS`; independent repetition with sanitized screenshots is still required for functional `CERTIFIED` status.
5. The standalone, Unreal-independent `Protocol772Core` now decodes an entire ordinary session: TCP framing, RSA/XTEA, character-list login, game login, the `FULLSCREEN` world snapshot, cardinal movement and the player/session command set are all `PASS`. What remains is presentation, and then the feature-specific command groups (chat, containers, trade).
6. Parse only the initial vertical-slice set proven by source: init game, rights, full screen/map point/field data, floor changes, creature descriptors, add/change/delete field, move creature, player data/skills/state, ping, messages, and disconnect/error. Preserve tile linked-list order and stack position. This set is complete.
7. Implement client commands required for login, logout/ping, cardinal movement/stop/turn and attack only after their exact payloads have fixtures. Cardinal movement, stop, turn, logout and ping are done; attack remains.
8. Create a minimal Unreal desktop project that consumes Protocol772Core via a network-thread event queue and applies WorldState on the game thread. Render ground as planes and players/creatures as capsules.
9. Run both clients simultaneously with two accounts. Execute stable IDs `PARITY-GAME-ENTRY-001`, visibility both directions, four cardinal movement tests, and blocked movement. Archive trace/log/screenshots and require independent review before `CERTIFIED`.

## Later phases

After the vertical slice: WorldState/map/floors; creatures; items; inventory/containers; use/use-with; combat/follow; magic/effects; stats/skills; chat/NPC/trade; remaining parity; then 3D production. Ordering may change only with documented source evidence.
