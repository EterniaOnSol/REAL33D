# Project Status

Current phase: `PHASE 2 - GAMEPLAY CLIENT PROGRAMMING` (`IN_PROGRESS`; the first 3D representation is live, a stock 2D client now completes the ordinary 7.72 flow, and V08 visual review is on operator-directed standby)
Current milestone: REAL33D-AGENT-ALDRIC-001 (opt-in autonomous 2D player with a real LLM) — `CERTIFIED` for bounded live control.
Next milestone: `NOT_STARTED`; no follow-on work is authorized beyond Aldric certification.
Last certified: REAL33D-AGENT-ALDRIC-001 `CERTIFIED` on its unmerged milestone branch; the previous MEMORY certification is `ef57363`.
Branch: `milestone/real33d-agent-aldric-001` (not merged to `main`)
Classic baseline review commit: `f65f3a7645ff40b39b7cc8399760fd4f0b69ecee`
Transport implementation commit: `abd2d0a25bd9632f5aa3955e822876268c7ca96c`
Crypto implementation commit: `64e9217ef64181d44cdce815a36b6bb1d2aa9038`
Login implementation commit: `a56add56e7ad5a11fd2e28d642bada4390b16756`
Game Login implementation commit: `3db23f9`
Initial World implementation commit: `fedd536`
Movement implementation commit: `37fa5f6`
Player State implementation commit: `af3e3ac`
Two-client slice commit: `a048150`
Visual inventory commit: `4260d98`
Visual reference pack commit: `3e0bea0`
Unreal slice commit: `da36b86`; corrective certification commit: `cae6450`
Chat commit: `e756bb7`; chat presentation commit: `cf034eb`
Dual-client research commit: `faf8852`; live capture commit: `0f9bd505`
Worktree: V08 presentation corrections and QA evidence recorded on main; final visual certification is `STANDBY` at the operator's direction
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`; V08 presentation corrections were published on `main` without force push. Screenshots are the first binaries committed and go through Git LFS per `.gitattributes`. No history was rewritten and no force push was used. The first attempt returned HTTP 403 because the stored credential belonged to `leodavidsoto`, which holds only `READ` on that repo; the operator re-authenticated `gh` as `EterniaOnSol`, which holds `admin`

Server reference source: immutable candidates selected, independently statically reviewed, materialized and `BUILD PASS`; `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` after independent live repetition
Protocol: `IN_PROGRESS`; Transport, Crypto, Login, Game Login, the `FULLSCREEN` initial world snapshot, cardinal movement with its incremental map updates, and the player/session command set are `PASS` for deterministic fixtures plus bounded local synthetic-account smoke. A whole login burst and ordinary session traffic now decode with zero residual bytes; containers, trade, the request queue and the editors remain recognized by name but unparsed. Chat was in that list until `CHAT-772-001` decoded `SV_CMD_TALK` from the three `SendTalk` overloads, with the mode-dependent tail and the modes no overload accepts both handled explicitly
Client core: Unreal-independent C++17 TCP/framing/crypto/Login/Game Login/Initial World/Movement/Player State components implemented, plus a `WorldView` adapter that diffs successive WorldStates into semantic events so the engine never sees protocol. Retained and new suites pass in normal and ASan/UBSan WSL builds. The native Windows build is no longer unverified: `tests/build_clientcore_windows.cmd` builds every source with MSVC at `/W4 /WX /permissive-`, at C++17 for portability and again at C++20 for the Unreal link, and runs all eight suites against the C++17 build. Its first run found four real portability defects that GCC accepts silently
Client: selected local Tibia 7.72 EXE/DAT/SPR/PIC set hashed and statically validated; Login, character list, Game entry and a session over 30 minutes are `PASS`; historical acquisition provenance is `UNKNOWN`
IP Changer: official Fusion32 revision `8215db18...` independently reviewed, materialized and Windows x86 `BUILD PASS`; exact 7.72 address table and fresh-modulus live patch are `PASS` for the selected client hash
Visual: `visual/` area created; master inventory regenerated entirely by tooling from `objects.srv`, `map.dat`, `origmap`, `monster.db`, `mon/`, `npc/` and the source enums. 5,690 tracker rows covering 5,003 object types, 159 monster races, 337 npcs, 152 outfit identities, 26 effects and 13 projectiles; 1,351 visual groups, so roughly three quarters of the object ids are reuse candidates. `VISUAL-ASSET-MASTER-INVENTORY-001 = PASS`. `VISUAL-REFERENCE-PACK-001 = PASS`: the 7.72 appearance data was located in the authorised local workspace, its format established by four independent validations rather than assumed, and 5,284 appearances decoded with 10,926 sprites and 5,284 previews, zero failures. The artist catalogue is rebuilt locally by one command and gitignored, since previews derive from an artifact with `UNKNOWN` provenance. `VISUAL-FULL-CATALOG-INGEST-TEST-001 = STANDBY`: 4,913 V08 models imported and mapped; UE 5.8 gallery and Fusion32 live loading were observed. Operator visual review found short and misoriented walls, floor occlusion, and camera issues. Unreal-only corrections are under review; artistic approval remains NO.
Unreal: `IN_PROGRESS`. `unreal/REAL33D/` is an Unreal Engine 5.8.2 project with one runtime module that links Protocol772Core as a prebuilt static library and contains no protocol source of its own. A worker thread owns the socket and WorldState; the game thread drains semantic events and is the only thread that touches an Actor. The first 3D representation is live: one actor per visible field, one per visible creature, a documented horizontal transform at `1 SQM = 100 Unreal Units` with 220 Unreal units of presentation floor spacing relative to the session's first anchor, and input that produces an intent for Fusion32 rather than a movement. `UReal33DAssetRegistry` resolves engine placeholders by default and command-line-gated V08 meshes for experimental QA. The project carries no `.umap`, no `.uasset` and no input asset: the scene is built in C++ on the empty engine map. The experimental UI now includes a full V08 catalog gallery for programmatic visual inspection.
Parity: `IN_PROGRESS` for state, `NOT_STARTED` for appearance. `UNREAL-SLICE-001` had the original `Tibia.exe` and the Unreal client in one Fusion32 world simultaneously, agreeing on positions, appearances, movement and viewport membership. No cell claims visual parity while every mesh is a placeholder.

Last certified test: `SERVER-RUNTIME-SMOKE-001`; independent reviewer repeated prepare, start, smoke and stop with all assertions passing
Latest functional tests: classic baseline IDs, `CLIENTCORE-TRANSPORT-772-001`, `CLIENTCORE-CRYPTO-772-001`, `CLIENTCORE-LOGIN-772-001`, `CLIENTCORE-GAMELOGIN-772-001`, `INITIALWORLD-772-001`, `MOVEMENT-772-001`, `PLAYERSTATE-772-001`, `TWO-CLIENT-VERTICAL-SLICE-001`, `VISUAL-ASSET-MASTER-INVENTORY-001`, `VISUAL-REFERENCE-PACK-001` are `PASS`; `VISUAL-FULL-CATALOG-INGEST-TEST-001` is `STANDBY`; `UNREAL-SLICE-001` is `PASS` with criterion 12 qualified, after a corrective run re-established criteria 6 and 7, which the first report had credited to a push rather than to Unreal input
Two-client result: the original `Tibia.exe` and Protocol772Core held two distinct characters in one world for 19 minutes; appearance, 11 tracked steps, viewport-edge departure and return, two accepted client-initiated moves, three refusals caused by the other player physically blocking the field, disconnect and reconnect, with 689 commands, zero residual bytes, zero anomalies and zero unsupported opcodes
Unreal slice result: the original `Tibia.exe` and the Unreal client held two distinct characters in one world simultaneously. Player A appeared in Unreal by name and position, his steps moved his capsule on the correct axis, A left and re-entered B's viewport with no duplicate or orphan, and a real server-side drop tore the scene from 418 actors to zero before an in-session reconnect rebuilt it to 434. Tile and creature actor counts equalled WorldState at every point sampled. Criterion 12 is qualified: `protocol_anomalies` was always zero and two multi-minute windows were entirely clean, but `SV_CMD_TALK` remains undecoded and the operator used the chat
Chat presentation result: speech decoded by `CHAT-772-001` is now drawn above the creature that said it. The speaker is resolved in ClientCore by matching both the talk position and, where present, the sender name against visible WorldState creatures, so Unreal receives a creature id and never a matching rule; anything not uniquely resolvable goes to a visible fallback rather than to a guessed creature. Fusion32 sends no duration and has no command to retract speech, so lifetime is client presentation and its exact 7.72 rule is unprovable from this project material, which holds the classic client only as a binary
Chat result: a live session using chat reached zero unsupported opcodes, zero protocol anomalies and zero residual bytes over 221 frames and 235 commands, with three talk commands decoded. The decoded speech position cross-checks against the same snapshot's independently tracked creature position for Player A. `TALK_YELL` could not be exercised because yelling is level-gated in 7.72 and both test characters are level 1; the channel and plain forms need a channel or a gamemaster, which this sanitized runtime has not, so they are covered deterministically only
Unreal slice correction: the first report credited criteria 6 and 7 to a session in which the operator never controlled B from Unreal. B moved there because A pushed him, which Fusion32 resolved authoritatively; that is incoming-path evidence only. Every retained snapshot from that session shows `steps_requested: 0`, so the numbers the report quoted had no preserved artifact behind them. Both criteria were withdrawn and re-established by a corrective run in which the operator drove B from the Unreal window: 19 requests, 18 accepted, 1 refused, 0 unanswered, each joined from key press to authoritative position by an `input_id`, plus 6 external relocations counted apart, two of them diagonal and therefore impossible to have been requested. A `MovementLedger` in ClientCore now makes the distinction structurally, with six deterministic tests including a replay of the original eight-field push. Details in `evidence/clientcore/UNREAL-SLICE-001-CORRECTION.md`
Current certification blockers: no verifiable original source/chain of custody for the operator-supplied local client copy; no independent repetition of the live client procedure; the Unreal slice is one run by one operator, with no floor transition exercised in 3D and criterion 7 resting on human observation alone
Closing ritual from `PLAYERSTATE-772-001` onwards: tests + sanitizers + evidence + docs + commit + handoff + push, with `tests/secret_check.sh` run before every push
Next gameplay candidate: trade or a live floor transition. Select one bounded scope from authoritative Fusion32 source before implementation. V08 certification remains on STANDBY.

Updated: 2026-09-17

## Visual ingest audit â€” 2026-09-20

`REAL33D-VISUAL-INGEST-AUDIT-001 = PASS` only for the internal `visual/` reference area; its former artist-source conclusion was corrected. `REAL33D-3DTIBIA-ART-AUDIT-001 = PASS` for a read-only inventory of the actual source clone `C:/Users/dell/3DTIBIA_leo/motor3d/assets` at `21fafb57dd86b594223bab7dbe9076d1fa380640`: 96,478 files, including four GLB and four BLEND. Four GLBs map `EXACT` to three 7.72 outfit visual identities by pixel-checked source evidence, but the source project documents all four as failed/rejected experiments. No primary-source 3D asset is ready for a P0 vertical import. `TECHNICAL_IMPORT`, `IN_ENGINE_RENDER` and `VISUAL_APPROVAL` remain `NOT_STARTED`; no Unreal, gameplay, protocol or ClientCore change was made. The chat milestone and prior functional claims above are unchanged. Corrected evidence and next step: `visual/docs/EXTERNAL_3DTIBIA_INGEST_AUDIT_2026-09-20.md`.

## Operator direction - 2026-09-21

V08 visual certification is on standby, not PASS. Preserve the current imports and inspection notes; final gallery review, live identity proof, and artistic approval remain open. Resume gameplay client programming. The next feature has not yet been selected.

## Gameplay client programming - 2026-09-21

UNREAL-PLAYER-VITALS-001 is IMPLEMENTED_UNVERIFIED. The worker publishes semantic PlayerVitals after applying PlayerData to WorldState; the game thread stores it and the HUD shows HP, mana and level, clearing them on disconnect. No protocol, server, or V08 visual asset was changed. The live log showed Player B connected and the HUD changed from unknown to HP 145/145, Mana 0/65531, Level 1. See evidence/clientcore/UNREAL-PLAYER-VITALS-001.md. The V08 catalog remains STANDBY and is not PASS.

## Wide-world streamer - 2026-09-21

UNREAL-WIDE-WORLD-001 = CERTIFIED_PASS. Unreal loads strict, generated 32×32 .wws sector caches around the authoritative anchor, keeps the complete 18×14 live window owned by WorldState, suppresses static tiles as they enter live authority, retains parsed data for cache hits, and destroys distant sector actors on the game thread. Async work is restricted to file IO and row parsing. Static baseline sectors never invent dynamic state.

Frozen V08 meshes resolve first. Missing physical V08 meshes use local gitignored classic previews as CLASSIC_SPRITE_FALLBACK; MISSING_PHYSICAL_ASSET remains separate and non-blocking. Thais exact-radius V08/fallback/missing occurrence counts are 12,286/0/0 at 32, 38,826/3/0 at 64, 71,845/8/0 at 96, and 110,073/17/0 at 128. TypeId 469 is explicitly CLASSIC_SPRITE_FALLBACK. The catalogue hash is unchanged and no V08 asset, ClientCore, protocol, server, or reference source changed. See evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

Live certification: at (32094,32203,7), three initial stationary visibility audits returned zero mismatches after correcting the classic billboard hidden-in-game default. A real round-trip across x=32096 loaded and unloaded nine sectors in each direction. Six crossing/return audits found zero static/live overlaps, zero visibility mismatches, and a live tile under the player. See evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

## Dual-client architecture closeout - 2026-09-21

`DUAL-CLIENT-ARCHITECTURE-CLOSEOUT-001`. Documentation only; no code, protocol,
server, Unreal or V08 change.

### Selected production architecture

```
                        Fusion32
                           |
             +-------------+-------------+
             |                           |
        REAL33D 2D                    REAL33D 3D
     OTClient-derived                   Unreal
             |                           |
     own parser/model            ClientCore + WorldState
             |                           |
             +---- same protocol spec ---+
                  shared fixtures
```

- `REAL33D_2D` = mehah/OTClient-derived production client.
- `REAL33D_3D` = Unreal + ClientCore production client.
- `Fusion32` = sole authoritative game server.
- Official `Tibia.exe` 7.72 = QA / parity / reference only, **not** a production client.
- IP changer = legacy QA utility only, **not** production infrastructure.

OTClient keeps its own parser and game model; ClientCore remains the 3D
implementation and the protocol reference. We do **not** replace OTClient's model
with ClientCore's WorldState: its UI rests on 1,061 Lua bindings and 207 distinct
`g_game.*`/`g_map.*` call sites over its own object model, so substitution would
be a rewrite rather than an integration. The two clients share the protocol
specification, byte-level fixtures, semantic expectations and Fusion32 authority,
so drift surfaces as a failing fixture.

### DUAL_CLIENT_LIVE_CAPTURE = PASS (`0f9bd505`)

A stock `mehah/otclient` build completed the full ordinary 7.72 flow against the
unmodified server with configuration only. `STOCK_OTCLIENT_CONNECT`, `LOGIN`,
`CHARACTER_LIST`, `GAMELOGIN`, `INITIAL_WORLD`, `CLIENT_INITIATED_MOVEMENT`,
`SAY_CHAT`, `ITEMS`, `INVENTORY`, `CONTAINERS`, `ITEM_MOVE` and
`LOGOUT_RECONNECT` are all PASS. Incoming protocol decode errors, unknown
incoming opcodes and unsupported incoming opcodes are each 0 over 103 traced
packets and 20 distinct opcodes. `SERVER_CHANGES_REQUIRED = 0`,
`STOCK_CLIENT_PARSER_PATCHES_REQUIRED = 0`, `REQUIRED_PRODUCTION_PATCHES = 1`.
`OPTION_C_VIABLE_FOR_PRODUCTION = YES`.

Remaining patch: `TALK_MODE_14 = KNOWN_PRODUCTION_PATCH_REQUIRED`,
`NOT_LIVE_TESTED`, one `protocolcodes.cpp` table entry. It needs a gamemaster
anonymous channel call this QA runtime cannot produce.

DIV-08: `CATALOGUE_STATIC_MATCH = 4990/4990`, `LIVE_CONFIRMED = 5/5` observed
TypeIds; cumulative and liquid classes are `NOT_LIVE_COVERED` because every
TypeId seen live was zero-extra-byte. TypeId 5090 is
`KNOWN_SERVER_ONLY_OR_UNREACHED_EXCEPTION`.

### Commit attribution correction (documentary; no history rewritten)

- `3fd5d1d` = `UNREAL-WIDE-WORLD-001` CERTIFIED_PASS.
- `c7bcfe7` = principally `UNREAL-PLAYER-VITALS-001` and other concurrent work,
  despite its message naming UNREAL-WIDE-WORLD-001.
  `UNREAL-PLAYER-VITALS-001` remains `IMPLEMENTED_UNVERIFIED`.

### Next milestone - REAL33D-2D-BOOTSTRAP-001 (NOT_STARTED)

Turn the successful isolated OTClient probe into a clean, reproducible
REAL33D-owned 2D client: pinned upstream commit, REAL33D branding, Fusion32
host/port, protocol 772 preset, terminal OS/type, Fusion32 public RSA modulus,
`GameEnvironmentEffect` compatibility, the TALK mode 14 entry, justified removal
of irrelevant OTClient ecosystem startup modules, a reproducible Windows build,
no embedded credentials, no private RSA/XTEA material, no accidental
`Tibia.dat`/`Tibia.spr` publication, and a login to gameplay to logout/reconnect
acceptance run.

Out of scope: shops, extended opcode 50, new TerminalTypes, 8.6 protocol, new
server features, Unreal changes, V08 changes, visual/art work.

The probe scratch tree `C:\r33dprobe` stays untouched until the bootstrap
reproduces the live PASS. Nothing in this repository references it.

## Inventory, containers and use - 2026-09-23

`UNREAL-INVENTORY-CONTAINERS-001 = CERTIFIED_PASS`. The round trip `723b557`
left explicitly uncertified was exercised against the running server on one
uninterrupted session, and all eight acceptance criteria hold:

```
RIGHT_CLICK_OPENS_CONTAINER      PASS   DISPLAYED_CONTENTS_MATCH_SERVER  PASS
MOVE_INSIDE_CONTAINER_LIVE       PASS   CLOSE_REMOVES_STATE_AND_PANEL    PASS
NESTED_CONTAINER_OWN_WINDOW      PASS   USE_WITH_ONE_VALID_TARGET        PASS
INVENTORY_EQUIPMENT_CORRECT      PASS   PROTOCOL_ERRORS                  0

residual_bytes 0   unsupported_opcodes 0   protocol_anomalies 0   over 88 commands
```

The contents drawn were compared against the server's own `1002.usr` save file
object for object, including the amount byte of a cumulative object and the
colour byte of a liquid container - the two classes `DUAL_CLIENT_LIVE_CAPTURE`
had to leave `NOT_LIVE_COVERED`. Use-with was exercised on a real
`dat/moveuse.dat` rule: flour on a bucket of water produced a lump of dough and
emptied the bucket, with one of the five flour units converted because flour is
cumulative. Closing went through `UseContainer`'s 7.72 toggle;
`CL_CMD_CLOSE_CONTAINER` and `CL_CMD_UP_CONTAINER` remain unbuilt.

Evidence, with the eight `F9` snapshots and the command log:
`evidence/clientcore/UNREAL-INVENTORY-CONTAINERS-001.md`.

### One live failure, found and fixed

The operator could not tell a bag from a barrel in the container window.
`scripts/client/extract_item_sprites.py` indexed the sprite offset table with
the sprite id, but sprite ids are one-based, so every object was drawn with the
sprite after the one it meant. Invisible when that neighbour was another frame
of the same object, glaring when it belonged to the next object. Fixed, and
`tests/verify_item_sprite_extraction.py` now holds that extractor to
`visual/tools/tibia772.py`, the reader `VISUAL-REFERENCE-PACK-001` validated:
4990 items agree on geometry and first-frame sprite ids and 8163 decoded
sprites agree byte for byte. The same test reports 8163 of 8163 mismatching
against the pre-fix indexing.

### Operator requests

Container windows now draw their whole capacity, as a 7.72 client does, rather
than their contents plus one empty square, and the containers panel grew from
200 to 420 so a backpack and a bag opened inside it both fit. Still open, and
the natural next milestone: the 2D client opens container windows minimised,
resizable, and movable between columns. None of that exists here.

## Combat and follow - 2026-09-23

`UNREAL-COMBAT-FOLLOW-001 = CERTIFIED_PASS`. ClientCore emits Fusion32's exact
7.72 `CL_CMD_ATTACK`, `CL_CMD_FOLLOW`, `CL_CMD_CANCEL` and
`CL_CMD_SET_TACTICS` bodies and owns the logical combat state in `WorldState`.
The server remains authoritative: accepted targets are silent on this protocol,
while rejection, cancellation, death, disappearance and range loss converge on
`SV_CMD_CLEAR_TARGET`. Slate and creature actors only present that shared state.

Live REAL33D acceptance covered attack and follow, target replacement, both
cancel paths, a real `Target lost` rejection, target removal, Battle List/world
agreement, world right-click attack, and right-click use/open on a real dead
rabbit container. The operator confirmed the Battle List follow flow and the
fight-stance controls. The final evidence snapshot retained correct inventory
and an open `dead rabbit` container, with 0 residual bytes, 0 unsupported
opcodes and 0 protocol anomalies over 496 decoded commands. Native ClientCore
tests and the final `REAL33DEditor Win64 Development` build pass. Evidence:
`evidence/clientcore/UNREAL-COMBAT-FOLLOW-001.md`.

All three attack stances and stand/follow are supported because
`receiving.cc::CSetTactics` implements them. Range chase remains absent because
the same handler rejects it. No damage, cooldown, HP, client-side follow
movement, protocol extension or fake Slate target state was added. V08,
WideWorld, REAL33D2D, shops, action bars, automap, reconnect handling and opcode
50 were not changed.

## Action bars - 2026-09-23

`UNREAL-ACTION-BARS-001 = IN_PROGRESS`. All three existing bars now accept
inventory/container drag bindings, local text bindings, mode changes and
clearing. Bindings persist in the local `Saved/ActionBars.json`; only TypeId and
text are saved, never a stale position or fabricated item count. On activation,
ClientCore resolves a current body/open-container instance from WorldState just
before it builds the existing use command. The HUD shades missing items and
sends nothing. Use-with enters the already implemented targeting mode; text
uses the existing TALK Say path. There is no spell list, cooldown, mana gate or
claim that the server accepted spell words.

Native ClientCore tests, including carried-item resolution after movement and
container close, pass in all eight suites. The Unreal editor build passes.
Live work is ongoing: drag/bind, local persistence file, missing-item no-send,
TALK request with server rejection, and movement have been observed; remaining
acceptance is tracked in `evidence/clientcore/UNREAL-ACTION-BARS-001.md`.

## Coin stack artwork - 2026-09-23

`REAL33D-COIN-STACK-VISUAL = IMPLEMENTED_UNVERIFIED`. The operator reported
that a 2- or 3-coin stack kept the one-coin picture despite its correct server
count. `REAL33D2D/src/client/item.cpp::Item::updatePatterns` selects one of
eight 4x2 stackable pictures at counts 1, 2, 3, 4, 5, 10, 25 and 50; the
local 7.72 `Tibia.dat` entry 3031 has exactly those eight sprite IDs. The UI
sprite extractor now writes all eight for stackable 4x2 types, and the
inventory/container slot chooses by the server-supplied amount. No gameplay,
wire format or reference source changed. Extractor comparison PASS (4,990
items; 392 stack patterns), generated 343 additional local ignored PNGs, and
`REAL33DEditor Win64 Development` build PASS. Live visual acceptance is still
pending; see `evidence/clientcore/REAL33D-COIN-STACK-VISUAL.md`.

## Chat input correction - 2026-09-23

`REAL33D-CHAT-SEND-FIX = IMPLEMENTED_UNVERIFIED`. A live operator report
found that the chat field accepted text but Enter/Send did not transmit it,
while action-bar text TALK worked. The session log recorded `not connected`
inside `SReal33DChatPanel::Send` even as movement and vitals continued to
receive Fusion32 updates. `SReal33DHUD::MakeBottomPanel` had omitted the
`Bridge` argument when constructing the embedded chat panel. That argument is
now supplied. The TALK protocol and chat field behavior were not changed.
The `REAL33DEditor Win64 Development` rebuild passed after the pre-fix game
instance closed. Live chat retest remains pending.

## REAL33D 2D autonomous agent - 2026-09-24

`REAL33D-AGENT-MVP-001 = PASS` for one autonomous 2D mock player. A new opt-in Lua gateway lives over
REAL33D2D's existing OTClient parser/game model and normal `g_game` actions.
The mock brain, observation whitelist, action validator, rolling action budget,
session logger and local Ollama Brain adapter are implemented. LuaJIT policy
and viewport tests PASS. The local QA A character's GM rights were removed
offline at the operator's request, then verified as `NONE`. Live Fusion32 runs
proved autonomous login, chat echo, movement, normal Attack/chase with visible
deer/rabbit HP loss, corpse opening, item transfer to A's bag, and subsequent
consumption of that looted item through `g_game.use`. The monitored A run kept
155/155 HP for multiple minutes. Actual Ollama inference, separate live Follow
and sustained low-HP survival remain `IMPLEMENTED_UNVERIFIED`. See
`evidence/agent/REAL33D-AGENT-MVP-001.md`.

## REAL33D 2D agent bridge - 2026-09-25

`REAL33D-AGENT-BRIDGE-001 = PASS` on branch `milestone/real33d-agent-bridge-001`,
not merged to `main`. The MVP gateway was reused and formalised, not restarted.
It now carries explicit `AgentObservation` and `AgentIntent` schemas, a closed
14-field observation whitelist and closed 11-action intent set, three-stage
validation (`schema`, `state` against freshly re-read client state, `budget`),
`session_id`/`correlation_id`/`observation_id`/`action_id` on every cycle, and a
JSONL observation/intent/validation/dispatch/result trace whose result events
join back to the originating action and observation. The canonical opt-in is
`R33D_AGENT_MODE=1`, with `R33D_AGENT=1` kept as a compatibility alias; with
neither set the module does nothing at all, which the test suite asserts.

Bridge certification is mock-only by construction: a requested LLM provider is
refused and forced back to `mock`, and the Ollama adapter is never constructed
on the bridge path. No cross-session memory participates in a bridge decision.

The live run (session `20260925T013642Z`, ordinary-rights character) proved
login, observation, movement, chat, follow, attack, cancel, inventory and
container observation, and container open/use. All 17 dispatches produced a
server-confirmed authoritative change; there were 0 incoming protocol errors and
0 validation rejections out of 51.

Auditing the inherited MVP found three real defects, each now covered by a
regression test that fails against the old code: `isCovered(0)` left the agent
blind indoors and underground, and `pendingLoot` both leaked on the loot success
path and never expired while no bag was open, which silently disabled target
selection for a whole session. Loot is now ranked by value rather than slot
order. `cancel_follow` live and sustained low-HP survival remain
`IMPLEMENTED_UNVERIFIED`. See `evidence/agent/REAL33D-AGENT-BRIDGE-001.md`.

## REAL33D 2D agent memory - 2026-09-24

`REAL33D-AGENT-MEMORY-001 = CERTIFIED` for two independent live REAL33D2D
sessions on branch `milestone/real33d-agent-memory-001`. Session A started with
zero memory records, directly observed a route and creature, and saved 20
records. A fresh Session B loaded those 20 records while the remembered place
and creature were outside the current observation, chose a memory-guided move,
passed schema/state/budget validation, dispatched through `g_game.walk`, and
observed Fusion32's resulting position change. Stale creature attack/follow
intents were rejected until a new B observation showed the creature. Identity
isolation, observation provenance, opt-in normal-client behavior, and zero
protocol errors were checked. See
`evidence/agent/REAL33D-AGENT-MEMORY-001-certification.md` and its retained
JSONL/memory/client-log artifacts. Sustained survival and movement-speed tuning
remain outside this certification. Its historical JSONL labels the call
`g_game.move` because of a trace metadata defect corrected in ALDRIC; the
runtime dispatcher source and the later ALDRIC trace establish `g_game.walk`.

## REAL33D 2D Aldric - 2026-09-24

`REAL33D-AGENT-ALDRIC-001 = CERTIFIED` for bounded real-LLM control on
`milestone/real33d-agent-aldric-001`. The opt-in real-provider Brain, versioned
veteran knowledge, bounded tactical movement, provider failure handling,
current-state validation, and JSONL decision logging are implemented.
Deterministic MVP/bridge/memory and Aldric tests, the MEMORY two-session replay,
the provider-offline failure replay, launcher regression and `secret_check`
pass. After the 2026-09-25 reboot, a fresh ordinary-rights Aldric session with
`qwen3:4b` and mock disabled produced six model decisions, three bounded
tactical continuations, five validated `g_game.walk` calls and four
server-observed position changes. The bridge trace now names the actual client
API call. Personal memory loaded and saved. A complete sanitized trace replays
successfully; the final client log had zero error/protocol-pattern matches.
Live combat, loot, supplies, trade and equipment upgrades
remain `IMPLEMENTED_UNVERIFIED`; NPC buy/sell execution is `NOT_STARTED`.
See `evidence/agent/REAL33D-AGENT-ALDRIC-001.md`.
