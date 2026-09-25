# Parity Matrix

> **2D client note (2026-09-21).** The `2D<->3D parity` column now refers to
> **REAL33D 2D**, the mehah/OTClient-derived production client, not to the
> official `Tibia.exe` 7.72, which is QA/parity/reference only.
> `DUAL_CLIENT_LIVE_CAPTURE = PASS` (`0f9bd505`) established the 2D side of the
> ordinary 7.72 flow on a stock build with configuration only: zero server
> changes, zero parser patches, and zero incoming decode errors, unknown
> opcodes or unsupported opcodes. Rows below marked *2D live* were exercised in
> that session. The 3D columns are unchanged by it.

| Feature | Server understood | Protocol specified | Unreal implemented | Tested | 2D<->3D parity | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Connection | Service topology/runtime PASS; classic live route PASS; client outer framing and crypto source-traced | Transport/framing/RSA/XTEA, Login and Game Login implemented; persistent session smoke PASS | Yes. `UReal33DBridge` runs Protocol772Core on a worker thread and publishes semantic events to the game thread; the module links the prebuilt `protocol772core.lib` and contains no protocol source | 4/4 retained/new suites normal and sanitized; bounded Game Login smoke PASS; native Windows MSVC build PASS | Partial. Unreal and the original `Tibia.exe` held a session against one Fusion32 world at the same time | IN_PROGRESS |
| Login / character list | Source traced; sanitized Login runtime and exact classic client flow PASS | Source-traced request/response and typed character list implemented | Yes, through the bridge. Credentials and modulus are read from the sanitized runtime at launch; no login UI | `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and `CLIENTCORE-LOGIN-772-001` PASS | Partial. Both clients authenticated against the same Login service in one run | IN_PROGRESS |
| Game login | Source traced; sanitized Game runtime and classic `JoinGame` flow PASS | Source-traced request, initial messages and persistent session implemented; fullscreen decoding now handed to the Initial World layer | Yes. World entry through to a drawn scene, plus in-session reconnect after a real server-side drop | `CLASSIC-GAME-ENTRY-772-001`, `CLIENTCORE-GAMELOGIN-772-001` and `UNREAL-SLICE-001` PASS | Partial. Both clients entered the same world; a real drop and reconnect left no residue | IN_PROGRESS |
| Keepalive | Source traced: server-initiated `SendPing` from the connection timer and `EmergencyPing`; `CPing` is a no-op | `SV_CMD_PING` decoded and `CL_CMD_PING` built | Yes. 20-second `CL_CMD_PING` from the worker; a real 90-round drop under machine load was observed and recovered from | `PLAYERSTATE-772-001` fixtures; the drop and recovery are recorded in `UNREAL-SLICE-001` | Not compared | IN_PROGRESS |
| Map | Source traced for `FULLSCREEN` and the incremental deltas: window, scan order, skip markers, tile stacks, viewport anchor and pruning | `FULLSCREEN`, `ROW_*`, `FIELD_DATA` and `ADD`/`CHANGE`/`DELETE_FIELD` decoded and applied to WorldState | Yes. `WorldView` diffs WorldState into tile events; `AReal33DTile` spawns, updates and destroys one actor per visible field | `INITIALWORLD-772-001` and `MOVEMENT-772-001` PASS; live walk matched a fresh server `FULLSCREEN` tile for tile | Partial, by state and not by appearance. Tile actors matched WorldState exactly at every sampled point of the live run; no visual comparison was attempted | IN_PROGRESS |
| Floors | Source traced: the `FULLSCREEN` floor range and the `SendFloors` sets, with the anchor shift NotifyGo applies | Floor set, offsets, `FLOOR_UP`/`FLOOR_DOWN` and the per-floor pruning decoded and applied | Transform only. `Real33D::ToWorld` inverts z and spaces floors by a presentation-chosen 100 uu, which is `UNRESOLVED` in the data | `MOVEMENT-772-001` fixtures cover both directions and the empty case; no live floor change observed, in `UNREAL-SLICE-001` either | No | IN_PROGRESS |
| Movement | Source traced: `Move` ordering, `NotifyGo`, `CGoDirection`, the refusal paths | Cardinal walk/turn/stop commands and `MOVE_CREATURE`/`SNAPBACK`/`MESSAGE` implemented; diagonals and `GO_PATH` deliberately unexposed | Yes, and deliberately one-way. Input becomes an intent for Fusion32; there is no code path by which a key moves an Actor, so a refusal changes nothing on screen. A `MovementLedger` separates steps this client asked for from relocations Fusion32 imposed, such as being pushed | `MOVEMENT-772-001`, `TWO-CLIENT-VERTICAL-SLICE-001` PASS; `UNREAL-SLICE-001` corrective run PASS: 19 operator-driven requests from Unreal resolved as 18 accepted and 1 refused, each joined from key press to authoritative position by an `input_id`, with 6 external relocations counted apart | Partial. Operator-driven Unreal steps were seen on the original client (human observation, machine-corroborated), and the original client's steps moved the Unreal actor | IN_PROGRESS |
| Creatures | Source traced for the three descriptor forms, the known-creature table and its lifetime, creature relocation and the six attribute updates | Descriptors, the known-creature mirror, creature moves and the 140-145 attribute updates decoded and applied; the mirror now retains entries exactly as `TConnection::KnownCreatureTable` does | Yes. One `AReal33DCreature` per visible creature, named, facing per `enums.hh`, with interpolation applied to drawing only and never to the logical position | The four suites above plus `UNREAL-SLICE-001`, where creature actors equalled WorldState's visible creatures at every sampled point, with zero duplicate spawns and zero orphan events | Partial. Both players were represented simultaneously in both clients through appearance, movement, viewport exit and return | IN_PROGRESS |
| Items | Source traced for map items, including the object type flags and stack priority the wire omits | Map item encoding and `PlaceObject` stack insertion decoded via an explicit object type table; inventory and container items not started | Presence and passability only. Stack order is preserved and the `UNPASS` flag from `objects.srv` separates blocking objects from walkable clutter; every mesh is a placeholder | `INITIALWORLD-772-001` and `MOVEMENT-772-001` PASS; extra-byte, priority and `UNPASS` paths verified against the real `objects.srv` | No. Nothing about item appearance is comparable while every mesh is an engine primitive | IN_PROGRESS |
| Inventory | Source traced: `SendSetInventory` / `SendDeleteInventory` and the `INVENTORY_*` body positions in `enums.hh` | `SET_INVENTORY` (120) and `DELETE_INVENTORY` (121) decoded and applied to WorldState, which holds all ten body slots | Yes. The body panel draws the ten slots from WorldState; a changed slot republishes the whole set so the panel can never mix two moments. Stackable 4x2 item pictures now select from the actual 7.72 art by server count | `UNREAL-INVENTORY-CONTAINERS-001` PASS for state; `REAL33D-COIN-STACK-VISUAL` extractor/build PASS, live visual check pending | 2D live; 3D state live, stack imagery pending | IN_PROGRESS |
| Containers | Source traced: `SendContainer`, `SendCloseContainer`, `SendCreateInContainer`, `SendChangeInContainer`, `SendDeleteInContainer` in `sending.cc`, and `UseContainer`'s open/close toggle in `moveuse.cc` | All five decoded and applied; an open replaces the container wholesale, a create prepends, a change replaces in place, a delete erases, a close clears the entry. The special coordinate `y = CONTAINER_FIRST + number` stays inside ClientCore | Yes. One window per open container, capacity-sized grid, drag between any two addressable slots. `CL_CMD_CLOSE_CONTAINER` and `CL_CMD_UP_CONTAINER` are not built: closing goes through the 7.72 use toggle and the parent arrow is drawn inert | `UNREAL-INVENTORY-CONTAINERS-001` PASS: open, contents matching the server save object for object including amount and liquid bytes, a move that both windows followed, a nested bag as its own window with `has_parent`, and a close that removed the entry | 2D live; 3D live | IN_PROGRESS |
| Use / use-with | Source traced: `CUseObject`, `CUseTwoObjects`, `CUseOnCreature` in `receiving.cc`, including the open-container slot byte `CUseObject` refuses out of range | `CL_CMD_USE_OBJECT` (130), `CL_CMD_USE_TWO_OBJECTS` (131) and `CL_CMD_USE_ON_CREATURE` (132) built byte for byte | Yes. Right button uses, shift+right begins a use-with that sends nothing until a second click names a slot, a creature or a field; Escape discards it | Byte-exact vectors for all three commands and for the container number a nested open picks; `UNREAL-INVENTORY-CONTAINERS-001` PASS live for the two-object form against a real `dat/moveuse.dat` rule (flour on a bucket of water became dough and emptied the bucket) | 3D live for use and use-with-object; the creature and field forms are built but not live | IN_PROGRESS |
| Action bars | REAL33D2D `game_actionbar.lua` and `ActionButtonLogic.lua`: drag assignment, local slot mappings, Use, SelectUseTarget and chatText; Fusion32 `CUseObject`, `CUseTwoObjects`, `CUseOnCreature`, `CTalk` | No new wire form. ClientCore resolves a current carried TypeId instance from WorldState before existing 130/131/132 commands; missing items send nothing | Three 2D styled bars, item drop, right-click mode/text/clear, missing-item dither, local JSON persistence; text uses real Say | ClientCore native suite PASS; live acceptance in progress in `UNREAL-ACTION-BARS-001.md` | 3D behavior under live review; no cooldown/spell-list parity claim | IN_PROGRESS |
| Combat / follow | `connections.hh` opcodes 160/161/162/190 and server 163; `receiving.cc::CSetTactics`, `CAttack`, `CCancel`; `crcombat.cc::TCombat::SetAttackDest`, `StopAttack`; `sending.cc::SendClearTarget` | Exact byte builders and tests; one `WorldState::combat` record updated only after send and cleared by server command/session reset; semantic `CombatChanged` events | Yes. Battle rows and world creatures share one target; explicit Follow/Stop control; right-click creature attacks; red attack and blue follow feedback; real stance buttons | Live request/state/rejection/removal evidence plus final snapshot in `UNREAL-COMBAT-FOLLOW-001.md` | 3D live | CERTIFIED |
| Magic / effects | Source traced for the wire form of the graphical, textual and missile effects and creature marking | Decoded and surfaced as typed events; they carry no WorldState semantics and store nothing. Spell casting itself is untouched | No | `PLAYERSTATE-772-001` fixtures; one live graphical effect observed at login | No | IN_PROGRESS |
| Stats / skills | Source traced: `SendPlayerData`, `SendPlayerSkills`, `SendPlayerState` and the `CheckState` flag table | Decoded and applied to WorldState; `PLAYER_STATE` is only emitted when the flags change | Partial. A player-facing HUD shows server-owned HP, mana and level from `PlayerData`; skills have no UI yet | `PLAYERSTATE-772-001` PASS; `UNREAL-PLAYER-VITALS-001` compiled and its live log showed a connected Player B and updated HUD text, with visual inspection pending | No | IN_PROGRESS |
| Chat | Source traced: `SV_CMD_TALK` and its three `SendTalk` overloads, the mode-dependent tail, and the `TALK_MODE` values no overload accepts | Server talk decoded into a typed semantic event; existing client TALK builder sends Say and supported modes | Positional speech, transcript and editable chat line exist. A missing HUD-to-chat Bridge argument, which blocked Enter/Send despite a live session, is fixed in source but awaiting rebuild/live retest. Action-bar text already used the real TALK path | `CHAT-772-001` protocol tests PASS; `REAL33D-CHAT-SEND-FIX` remains IMPLEMENTED_UNVERIFIED until a live chat request reaches Fusion32 | Partial; exact presentation is not classic-parity | IN_PROGRESS |
| NPC | Located | No | No | No | No | NOT_STARTED |
| Trade | Located | No | No | No | No | NOT_STARTED |
| Death / loot | Located, untraced | No | No | No | No | NOT_STARTED |
| Social systems | Partial locations | No | No | No | No | NOT_STARTED |

`Located` means a likely authoritative implementation symbol was found. It is not protocol understanding and never means `PASS`.

`Partial` in the parity column means the same world state was represented in both clients at the same time and agreed. It never means the two clients look alike: every mesh in Unreal is an engine primitive, so no cell in this matrix claims visual parity and none can until the visual pipeline approves art.

`SERVER-RUNTIME-SMOKE-001` is infrastructure evidence only. The classic-client PASS results validate the 2D baseline, not an Unreal client. They do not change any 2D-to-3D parity cell to PASS.

`CLIENTCORE-TRANSPORT-772-001 = PASS` is limited to the portable TCP and outer-framing component. It does not prove crypto, application login, world entry or 2D-to-3D parity.

`CLIENTCORE-CRYPTO-772-001 = PASS` is limited to deterministic RSA/XTEA, key lifecycle and encrypted inner-packet behavior. It does not prove application Login, Game entry or parity.

`INITIALWORLD-772-001 = PASS` is limited to decoding the one `FULLSCREEN` snapshot the server sends at login and folding it into a minimal WorldState. It proves no rendering and no 2D-to-3D parity.

`MOVEMENT-772-001 = PASS` is limited to cardinal walking and the incremental map updates a step produces. It proves no combat, inventory, container, chat, diagonal or path movement, no rendering and no 2D-to-3D parity. Its live evidence covers surface walking on floor 7 only; floor changes and the field commands are fixture-covered.

`TWO-CLIENT-VERTICAL-SLICE-001 = PASS` demonstrates that the original Tibia 7.72 client and Protocol772Core coexist as two independent clients in one authoritative Fusion32 world: appearance, movement tracking, viewport-edge departure and return, client-initiated movement observed from the other client, movement refused because the other player physically occupied the destination field, and disconnect/reconnect without ghosts. It proves protocol coexistence and world consistency only. Player B has no presentation at all, so no 2D-to-3D parity cell changes on its account, and the `Unreal implemented` column stays `No` everywhere.

`PLAYERSTATE-772-001 = PASS` closes the command set an ordinary session emits: a real login burst and walking session decoded with zero residual bytes and no unsupported opcode. It proves decoding, not behaviour. Inventory, buddy, outfit-chooser and effect commands are decoded for structure and length but deliberately store nothing, and containers, trade, the request queue and the editors remain undecoded and still report by name with zero bytes consumed. Chat was in that list until `CHAT-772-001` decoded `SV_CMD_TALK`. No row in this matrix reaches `PASS` on protocol decoding alone.

`REAL33D-VISUAL-INGEST-AUDIT-001 = PASS` describes REAL33D's internal reference area only; it was the wrong artist source. `REAL33D-3DTIBIA-ART-AUDIT-001 = PASS` for read-only inventory and bounded 7.72 outfit identity cross-checks in the separate 3DTIBIA clone. Four experimental GLBs map exactly to outfit identities, but none is approved or imported. Technical import, in-engine rendering and director visual approval remain `NOT_STARTED`; no parity row changes. See `visual/docs/EXTERNAL_3DTIBIA_INGEST_AUDIT_2026-09-20.md`.

`VISUAL-FULL-CATALOG-INGEST-TEST-001 = STANDBY`: 4,913/4,913 V08 GLBs imported and deterministic TypeId-to-mesh mapping checks passed. The UE 5.8 gallery and live Fusion32 client load the experimental catalog, but full visual certification remains open. The operator confirmed mailbox visuals and wall 01294; all wall orientation, floor visibility, camera feel, warning severity, and representative gallery screenshots require final review. TEST_IMPORTED=YES; APPROVED=NO; READY=NO; PRODUCTION_INTEGRATED=NO.


Operator direction (2026-09-21): V08 visual certification is STANDBY, with no new PASS claim. The inspector notes and latest live-session journal are retained for later review while gameplay client programming resumes.

UNREAL-WIDE-WORLD-001 = CERTIFIED_PASS. Strict generated 32×32 sector caches stream outside the complete WorldState-owned 18×14 live rectangle; async work reads and parses rows, while the game thread owns actor/component creation and destruction. Static tiles entering live authority are suppressed, distant sectors unload, parsed data is cached, and missing V08 meshes resolve to identity-retaining local classic sprite proxies or a separately counted non-blocking placeholder. This adds static-map reach and makes no dynamic-world or final-art parity claim. See evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

Live round-trip at x=32096: 9 sector loads and 9 unloads in each direction; all crossing/return audits had zero visibility mismatches and zero static/live overlaps. TypeId 469 uses the exact classic sprite fallback.

## REAL33D 2D live coverage - DUAL_CLIENT_LIVE_CAPTURE (`0f9bd505`)

Stock `mehah/otclient`, configuration only, against the unmodified Fusion32.

| Feature | 2D live result | Authoritative evidence |
| --- | --- | --- |
| Connect / login / character list | PASS | server: `loggt ein an Socket 16` |
| Game login | PASS | `op=10 (0x0A)` INIT_GAME, `onGameStart` |
| Initial world | PASS | `op=100 (0x64)` FULLSCREEN; `pos=(32098,32205,7)` |
| Client-initiated movement | PASS | N/S/E/W exact round trip; `op=109` + row updates; `op=181` SNAPBACK decoded when refused |
| Say chat | PASS | `op=170 (0xAA)`, echo in 20 ms, `mode=1` TALK_SAY |
| Items | PASS | tile stack `n=2`, TypeId 870 |
| Inventory | PASS | 4 slots at login, 3 after the move |
| Containers | PASS | `op=110` open, `op=111` close |
| Item move | PASS | `op=121` + `op=112` 68 ms after the request; persisted across logout |
| Logout / reconnect | PASS | two distinct server-side socket records |
| Combat / follow | NOT_EXERCISED | out of the milestone's scope |

Incoming protocol decode errors, unknown incoming opcodes and unsupported
incoming opcodes: **0 each**, over 103 traced packets and 20 distinct opcodes.
The only `[error]` lines were outbound OTClient ecosystem noise
(`ExtendedIds.Locale = 1`, `GAME_SHOP_CODE = 201`), blocked client-side by
`m_enableSendExtendedOpcode = false` and never transmitted to Fusion32.

## REAL33D 2D agent gateway - REAL33D-AGENT-MVP-001

| Agent path | Evidence | State |
| --- | --- | --- |
| OTClient state to bounded `AgentObservation` | LuaJIT viewport test excludes offscreen creature and tile; live HP/position, chat, inventory and open-container observations | PASS |
| Mock Brain to validated intent to ordinary `g_game` action | LuaJIT policy/budget test; live login, Say echo, server-observed movement, container open/use, loot move and food use | PASS |
| Attack and chase | Normal-rights A sent `INTENT attack`; deer 100→4%, rabbit 73→6% with ordinary client chase mode and server clear-target | PASS |
| Loot and eat | Visible `dead rabbit[3577]` → bag gains 3577 after `move_item`; subsequent `use` removes 3577 from same character's bag | PASS |
| Separate Follow and low-HP survival | Validated dispatcher and deterministic policy tests; no qualifying rights-free live Follow or sustained low-HP run | IMPLEMENTED_UNVERIFIED |
| Local Ollama Brain provider | Same interface and validator; mocked HTTP response test; no local model installed | IMPLEMENTED_UNVERIFIED |

The gateway does not change Fusion32 gameplay, protocol 7.72, ClientCore,
WorldState or Unreal. The agent sees only current REAL33D2D state and its
on-screen map subset; it cannot query server files or the cached full map.

## REAL33D 2D agent bridge - REAL33D-AGENT-BRIDGE-001

| Bridge path | Evidence | State |
| --- | --- | --- |
| `AgentObservation` / `AgentIntent` schemas and strict validation | Closed 11-action intent set and 14-field observation whitelist; unknown action/field, wrong type, out-of-range, control character and six forbidden observation fields rejected by test | PASS |
| Three-stage gate before dispatch | `schema`, then `state` against freshly re-read client state, then `budget`; live session recorded 51 validations, 0 rejections | PASS |
| Stale-target refusal | Intent formed against one observation refused against a later one where the creature or tile is gone | PASS |
| Correlation ids and JSONL trace | `session_id`/`correlation_id`/`observation_id`/`action_id`; result carries the originating ids plus `observed_in`; sorted-key deterministic encoding | PASS |
| Opt-in `R33D_AGENT_MODE`, human client unchanged | With neither switch set, `init()` schedules no timer, connects no event and leaves `ProtocolGame.onOpcode` untouched | PASS |
| Live mock scenario against Fusion32 | Session `20260925T013642Z`: login, observation, movement, chat, follow, attack, cancel, inventory/container observation, open/use; all 17 dispatches `authoritative_change=true`; 0 protocol errors | PASS |
| Observation blind indoors/underground (MVP defect) | `isCovered(0)` reported every tile covered under a roof or below ground; fixed to the player's floor, regression test fails against the old code | PASS |
| Target selection disabled after loot (MVP defect) | `pendingLoot` leaked on the success path and never expired without an open bag; both fixed with regression tests | PASS |
| Loot ranked by value | Live `dead rat` held worm x2 in slot 0 and gold coin x2 in slot 1; old code took the worm, ranking now takes the coins | PASS |
| `cancel_follow` live | Validated dispatcher and deterministic tests; brain converts a confirmed follow into an attack, so not separately dispatched live | IMPLEMENTED_UNVERIFIED |
| Sustained low-HP survival | Retreat policy behaved as designed but lost a character at 8/160 HP in a rat cave | IMPLEMENTED_UNVERIFIED |
| LLM brain | Out of scope; unreachable in bridge mode by construction | OUT_OF_SCOPE |

No cross-session memory participates in a bridge decision, and the observation
whitelist is enforced by the schema rather than by convention.

## REAL33D 2D agent memory - REAL33D-AGENT-MEMORY-001

| Agent path | Evidence | State |
| --- | --- | --- |
| Observation-derived, identity-isolated memory | LuaJIT round trip, provenance, isolation and malformed-record tests | PASS |
| Memory choice through current-state action validator | LuaJIT navigation and invisible-target/item/tile refusal tests | PASS |
| Two live sessions with persistence and guided navigation | Retained A/B JSONL, A memory JSON and live evidence test: A new/0 to saved/20; fresh B loaded/20, goal initially invisible, validated `g_game.walk`, authoritative y change; historical trace path label corrected in ALDRIC | CERTIFIED |
| Survival and route recovery after death | First session reached 2/160 HP; second cycled on another floor. Floor-selection regression now has a deterministic test only | IMPLEMENTED_UNVERIFIED |

Memory is an opt-in 2D Brain input, not an addition to AgentObservation or to
the allowed actions. Stale attack/follow and item intents were rejected against
B's current observation, and another character's memory failed identity
validation. See `evidence/agent/REAL33D-AGENT-MEMORY-001-certification.md`.

## REAL33D 2D Aldric - REAL33D-AGENT-ALDRIC-001

| Agent path | Evidence | State |
| --- | --- | --- |
| Curated, versioned veteran knowledge with current observation precedence | Auditable `knowledge/v1.lua`, explicit Tibia 7.72 identity, bounded retrieval and deterministic tests | PASS |
| Provider-independent strategic Brain and opt-in Ollama adapter | Deterministic malformed-output, timeout, provider-error and mock-exclusion tests | PASS |
| Real model controlling ordinary-rights Aldric through the 2D client | Fresh post-reboot session `20260925T191803Z`: six model decisions, mock disabled, loaded/saved memory, five validated `g_game.walk` calls and four correlated server-observed position changes; sanitized trace replay PASS | CERTIFIED |
| Autonomous combat, loot, supplies, trade and equipment improvement | Conceptual veteran knowledge and legal action bridge exist; not exercised in the certified Aldric session | IMPLEMENTED_UNVERIFIED |
| NPC buy/sell execution | No safe buy/sell action exists in the current agent bridge | NOT_STARTED |

This milestone does not change Fusion32 gameplay, protocol 7.72, the ordinary
human client path, or any 3D/Unreal parity cell. The current observation and
validator remain the final action gate for memory and static knowledge leads.
