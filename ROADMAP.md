# Roadmap

## Gate 0 - controlled server baseline (`IN_PROGRESS`)

1. `BOOTSTRAP-SOURCES-001` (`PASS`): verified all bundles, compared loose snapshots, selected independently reviewed immutable candidates, materialized curated references, and produced clean Linux builds.
2. `SERVER-BASELINE-772-001` (`IN_PROGRESS`): sanitized preparation, fresh key/config/data, three-service startup, internal authorization, world load, network smoke and shutdown are `CERTIFIED` under independently repeated `SERVER-RUNTIME-SMOKE-001`. Classic login, character list and world entry are `BLOCKED_CLIENT_ABSENT`.
3. `CLASSIC-CLIENT-772-001` (`BLOCKED`): the official Fusion32 IP Changer source is selected, materialized and `BUILD PASS`; its runtime-specific 7.72 configuration is reproducibly generated. Obtain a legitimate exact Tibia 7.72 client, hash/provenance it, live-validate the source-defined addresses, then execute `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and `CLASSIC-GAME-ENTRY-772-001` with generated synthetic credentials.
4. `PROTOCOL-LOGIN-001`: fully specify character-list login plus game login, transport, framing, RSA/XTEA, source traceability, and byte fixtures. Independent review required.

Gate: selected source revisions, repeatable build commands, and fixture tests for login/framing/crypto. Documentation alone does not close the gate.

## Shortest evidence-based path to `TWO-CLIENT-VERTICAL-SLICE-001`

1. Selected candidates, curated references, explicit 7.72 builds and a resettable sanitized runtime are complete. Independently repeated `SERVER-RUNTIME-SMOKE-001` certifies Query Manager -> Game -> Login startup, world load and clean shutdown within its stated scope.
2. Obtain and hash a legitimate classic Tibia 7.72 client plus required DAT/SPR/PIC. This dependency is absent today.
3. Use the built Fusion32 IP Changer revision `8215db18...` and generated `fusion32` server entry; validate its 7.72 address table against the exact supplied executable. Never execute an archived IP Changer binary.
4. Certify classic character-list login and game entry against the sanitized Fusion32 instance; retain minimal logs/trace.
5. Implement a standalone, Unreal-independent `Protocol772Core`: TCP framing, RSA public-block construction, XTEA, character-list login, game login, server-command loop, and typed semantic events. Golden byte fixtures precede live use.
6. Parse only the initial vertical-slice set proven by source: init game, rights, full screen/map point/field data, floor changes, creature descriptors, add/change/delete field, move creature, player data/skills/state, ping, messages, and disconnect/error. Preserve tile linked-list order and stack position.
7. Implement client commands required for login, logout/ping, cardinal movement/stop/turn and attack only after their exact payloads have fixtures.
8. Create a minimal Unreal desktop project that consumes Protocol772Core via a network-thread event queue and applies WorldState on the game thread. Render ground as planes and players/creatures as capsules.
9. Run both clients simultaneously with two accounts. Execute stable IDs `PARITY-GAME-ENTRY-001`, visibility both directions, four cardinal movement tests, and blocked movement. Archive trace/log/screenshots and require independent review before `CERTIFIED`.

## Later phases

After the vertical slice: WorldState/map/floors; creatures; items; inventory/containers; use/use-with; combat/follow; magic/effects; stats/skills; chat/NPC/trade; remaining parity; then 3D production. Ordering may change only with documented source evidence.
