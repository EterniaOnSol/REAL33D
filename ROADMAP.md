# Roadmap

## Gate 0 - controlled source bootstrap (`IN_PROGRESS`)

1. `BOOTSTRAP-SOURCES-001` (`PASS`): verified all bundles, compared loose snapshots, selected independently reviewed immutable candidates, materialized curated references, and produced clean Linux builds.
2. `SERVER-BASELINE-772-001` (`NOT_STARTED`): create a reproducible sanitized runtime using selected Game + Query Manager + Login revisions and `TIBIA772=1`; generate test key/config/data; start services; prove classic 7.72 login and world entry.
3. `PROTOCOL-LOGIN-001`: fully specify character-list login plus game login, transport, framing, RSA/XTEA, source traceability, and byte fixtures. Independent review required.

Gate: selected source revisions, repeatable build commands, and fixture tests for login/framing/crypto. Documentation alone does not close the gate.

## Shortest evidence-based path to `TWO-CLIENT-VERTICAL-SLICE-001`

1. Selected candidates: Game `386fa9b...`, Login `f1c839f...`, Query Manager `edea08d...`; curated references and clean builds are complete. Runtime compatibility is not yet proven.
2. Create a sanitized runtime from verified minimum `dat`, `origmap`, NPC/monster data, fresh writable directories, credentials, and a generated shared key. Never run archived binaries or import historical users/logs/backups.
3. Start Query Manager, Game, then Login; capture exact config/ports/build hashes and certify startup plus service contracts.
4. Obtain and hash a legitimate classic Tibia 7.72 client plus required DAT/SPR/PIC and configure the supplied 7.72-capable ipchanger. This dependency is absent today.
5. Certify classic character-list login and game entry against the sanitized Fusion32 instance; retain minimal logs/trace.
6. Implement a standalone, Unreal-independent `Protocol772Core`: TCP framing, RSA public-block construction, XTEA, character-list login, game login, server-command loop, and typed semantic events. Golden byte fixtures precede live use.
7. Parse only the initial vertical-slice set proven by source: init game, rights, full screen/map point/field data, floor changes, creature descriptors, add/change/delete field, move creature, player data/skills/state, ping, messages, and disconnect/error. Preserve tile linked-list order and stack position.
8. Implement client commands required for login, logout/ping, cardinal movement/stop/turn and attack only after their exact payloads have fixtures.
9. Create a minimal Unreal desktop project that consumes Protocol772Core via a network-thread event queue and applies WorldState on the game thread. Render ground as planes and players/creatures as capsules.
10. Run both clients simultaneously with two accounts. Execute stable IDs `PARITY-GAME-ENTRY-001`, visibility both directions, four cardinal movement tests, and blocked movement. Archive trace/log/screenshots and require independent review before `CERTIFIED`.

## Later phases

After the vertical slice: WorldState/map/floors; creatures; items; inventory/containers; use/use-with; combat/follow; magic/effects; stats/skills; chat/NPC/trade; remaining parity; then 3D production. Ordering may change only with documented source evidence.
