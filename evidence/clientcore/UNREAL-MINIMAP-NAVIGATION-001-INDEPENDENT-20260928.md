# UNREAL-MINIMAP-NAVIGATION-001 independent certification

Independent run: IMPLEMENTED_UNVERIFIED. Milestone remains PASS LOCAL,
NOT CERTIFIED. No reproducible failure of a feature was established, but
required live coverage is incomplete. The operator requested immediate commit
and push before the outstanding checkpoints could be reproduced. Publishing
this evidence does not certify the milestone.

Task: UNREAL-MINIMAP-NAVIGATION-001-INDEPENDENT-20260928.
Reviewer: Codex, fresh repository/execution context, manual operator app inputs.
Date: 2026-09-28, America/Guatemala (UTC-06).
Allowed scope: candidate checks/build/live QA and evidence/status/handoff only.
No implementation, gameplay, protocol, art, REAL33D2D, agents, original-checkout
or main changes. WORLD-PRESENTATION-POLISH was not started.

## Repository and prerequisites

Fresh remote clone: build/unreal-minimap-certification-20260928.
Branch: milestone/unreal-minimap-navigation-001.
Candidate/local/remote at start and before closeout:
ea53eccc58d29fd5fa358108a4278853aa21b130.
Base/origin/main and direct remote main:
a97cf25e7449a8a3ef35ef2553181c5032c7a247.
Git status was empty before certification-generated evidence. Fetch and direct
ls-remote again matched both expected refs immediately before closeout.
The new evidence commit is a descendant of the unchanged candidate, not a
replacement implementation. Its hash is available from the publishing commit.

Startup contract/status/architecture/roadmap/parity/handoff read. Source,
launchers, tests and reference sources remain identical to the candidate.
Previous local-PASS report and evidence were not overwritten. The previous
handoff was archived byte-for-byte. The original dirty checkout was not used
for Git integration, edits, clean/reset/stash/commit/merge operations.

Computer-use failed with native pipe unavailable on initialization, retry and
kernel reset. No PowerShell UI automation workaround was used. Engine-generated
F9 JSON/PNG and normal operator inputs provide this run's live evidence.
Only confirmations for actions evidenced in these new sessions are credited.

Ordinary Fusion32 services were stopped/started normally after checking there
were no established game clients: Query Manager2444/Game2457/Login2726,
7173/7172/7171 ready with verified identities. No gameplay/config/DB edits.
Normal account B was used. Existing ignored V08/BrotherCreatures/UI prerequisites
were copied unchanged for rendering; no minimap cache or prior QA was copied.
The existing static WideWorld cache/previews were read only. The unchanged
launcher generated 2502 appearance colors from the selected validated classic
DAT; the palette contains appearance metadata, not map coordinates.

## Independently executed checks

Run commands are preserved in run-checks.cmd. In the new clone:

- tests/build_clientcore_windows.cmd under VS2022 x64: C++17 /W4 /WX;
  all nine suites PASS, including minimap/navigation tests. C++20 link library
  rebuilt. No code or fixture adjustment was made.
- cl /nologo /EHsc /W4 /WX /permissive- /std:c++17 on
  tests/real33d_minimap_view_tests.cpp, then executable: PASS.
- UE5.8 Build.bat REAL33DEditor Win64 Development, isolated project,
  -WaitMutex: PASS, 29actions, 341.26seconds.
- Candidate tests/secret_check.sh with valid standalone-clone WSL Git:
  six checks PASS. Final tracked-evidence scan is performed before publishing.

Compilation and unit tests are not counted as live verification.

## Three distinct layers and source audit

LIVE VIEWPORT: current synchronized WorldState, player and mutable things.
ClientCore minimap.cpp::ProjectMinimapFloor projects only the actual current
floor inside unchanged 18x14 bounds. Each retained observation reported 252
live cells. Total WorldState tiles include protocol floors and are not an
expanded same-floor gameplay viewport.

KNOWN MINIMAP: legitimately received terrain observations, bounded scoped local
cache, no creatures/movable stacks. Ground, static color source and stale static
obstacle hints remain separate from current occupancy. Bridge::NoteMinimap
accepts worker-published observations from normal applied protocol state only.
Cache load does not claim that mutable entities or current walkability exist.

WIDEWORLD STATIC: independent visual sectors/radius64 outside the live window.
It is not an input to discovery, the scoped cache, FindKnownWalkPath or live
next-step validation. Loaded screenshots show world context beyond the small
known map; selecting an unknown goal did not reveal that context in the minimap.

KnownMinimap::FindNavigationPath is const. A* traverses only retained usable
cells. Unknown neighbors are compared geometrically when selecting a reachable
known frontier; they are never inserted, marked walkable or appended to a path.
World::FindKnownWalkPath overlays current live blockers; IsKnownWalkTile requires
received ground and current blockers/creatures, excluding WideWorld actors.
Controller::RequestKnownWalk/TickClickWalk retain the goal, issue normal
RequestWalk and await the server ledger/expected authoritative coordinates.
The refusal/relocation/unanswered reset and next-step replanning branches were
inspected, but an active-route authoritative obstruction was NOT reproduced.

Selected Fusion32 sending.cc::SendFullScreen/SendRow/SendFloors and
receiving.cc::CGoDirection/cardinal dispatch were inspected in the existing
772 reference baseline. No new protocol or gameplay behavior was substituted.
Slate paint uses uniform classic216 colors, north up, +X east/right and +Y
south/down, yellow actual-player cross and current floor label; no live-tile
highlight. New screenshots confirm colored terrain and black unexplored area.

## Exact new live sessions

### Session1: live-20260928T135328, PID10560

No .r33map existed before launch. Initialization delayed rendering while assets
loaded. First received map: (32087,32150,7), known252, live252. The ordinary
connection closed/reconnected; stable loaded position was (32097,32219,7),
known504, marker/selected floor7 (MinimapInitial at19:56:10.791UTC).
This reconnect and both legitimate received areas explain the initial504;
no static-map seed or local teleport was introduced.

Manual north examples: (32097,32219,7)->(32097,32218,7), request1 accepted,
F9 19:56:31.877; then north to32217/32216/32215, F9 each. Marker coordinates
match authoritative position. Later manual west32095->32094 and east32094
->32095->32096->32097 atY32205 were accepted with matching observations.
No manual south checkpoint exists; south movement in session2 was navigation.
Thus the full requested manual N/S/E/W coordinate-mapping test is incomplete.

Known terrain expanded504->576 during four north steps, then820/842 while
traveling. At19:56:47 and19:56:59 minimap pans changed local centers only.
F9 snapshots at19:56:59.579,19:57:16.137 and19:57:22.960 retain the same
(32097,32215,7), known576, walks4, uses0. Operator freshly confirmed marker and
pan/zoom/Home behavior; scale changes are visible, and the pan/zoom interval
emitted no gameplay movement. Home was separately traced in session2.

A minimap destination (32095,32203,7) used known segments and arrived at that
actual coordinate,19:57:31.171UTC. Five ordinary world Uses were requested;
Use19 on object2109 at(32094,32206,7) produced received relocation32203->32204
->32205, with retained known820. Inventory state was known. No opened container
or Follow/cancel was recorded. A no-reachable-route event at19:57:07 lacks a
retained exact blocked goal; it is not sufficient for the required blocked test.
Normal close19:58:10.964UTC saved842cells.

### Session2: live-20260928T140031, PID13228

Cache load accepted842cells; first observation (32097,32205,7) still842,
live252, no movement. Cache SHA256 before/after restart was identical:
bb9426e7d965206fda58d0c1868629487aa4aca70b642e3b2a98e934c70fe03f.
The previous temple coordinate(32097,32219,7), outside this live window,
remained known. Operator confirmed retained terrain; F9 20:01:27.644UTC.
This proves cross-process/cache retention in addition to same-session retention.

World Look1 at(32100,32203,7),20:01:48.670, received "You see a sign."
Pan center(32098,32179.97),20:02:19.947; Home20:02:18.324 and20:02:20.733.
Position and commands remained stationary before navigation began.

UNKNOWN_TARGET_COORDINATE=(32068,32238,7), click20:02:52.465UTC.
KNOWN_MAP_BOUNDARY_AT_CLICK=842cells, floor7; min/max X32079..32106,
Y32144..32226. Those bounds summarize sparse observed areas, not a filled map.
Exact saved-cache membership says target unknown with no terrain hint. Cache
count agreed with live count before click, and no observations intervened.
INITIAL_ROUTE_STATE=28cardinal steps ending at known(32089,32225,7), original
goal retained. unknown-initial-segment-known.json verifies ALL28 actual initial
segment step targets were already in the before-click cache. No unexplored
full route was fabricated, and selecting the goal added no map cells.

UNKNOWN_ROUTE_EXTENSION_EVENT: (32088,32225,7) was absent before click. Normal
received map observations during movement included the covering new row by
20:03:01.957 (player32090,32218,7; known946). At20:03:05.979 player was
(32089,32225,7), known1075. At20:03:06.283 the planner extended15steps for
original goal32068,32238,7; its next normal RequestWalk was to32088,32225,7,
accepted at20:03:06.570. The after-cache query confirms that cell now known.
The new knowledge arose after ordinary movement/descriptions; WideWorld was
already rendering before the click and did not populate these cells.
Operator confirmed only progressive reveal while moving.

The original unknown run was replaced at20:03:11.201 with a world-click route
to known(32086,32222,7), which really arrived20:03:14.622. Original unknown
arrival=NOT_PROVEN; that replacement arrival is not its success. This is not a
server-obstruction test or a verified known MINIMAP-click checkpoint.

A SECOND minimap goal(32069,32232,7),20:03:20.183, was also unknown: all prior
received windows had minX>=32078, and the initial cache query is absent. It
advanced through18-,8- and1-step known segments, with ordinary accepted steps,
and actually arrived at(32069,32232,7),20:03:34.365, F9 20:03:47.145.
Known1494. This second unknown arrival is proved; the first is still NOT_PROVEN.
Normal close20:03:49.515 saved1494. All65 requests accepted, no refused or
unanswered step, no floor change, Follow/cancel or opened container.

### Session3: live-20260928T140632, PID16204

Reopened to obtain the missing checkpoints after the operator's broad later
confirmations conflicted with session2's closed log. Cache1494loaded. Client
closed during initialization, EndPlay20:07:56.636, before any manual F9;
no movement/Follow/Use/Look/container or floor-transition checkpoint.
A queued normal map observation was processed at exit; this does not prove UI
or gameplay tests. The operator then explicitly requested immediate commit/push.

## Decision and acceptance results

| Criterion | Independent result |
| --- | --- |
| MINIMAP_RENDER / PLAYER_MARKER | PASS, loaded colored HUD map and actual position |
| COORDINATE_MAPPING | IMPLEMENTED_UNVERIFIED, manual south checkpoint missing |
| CURRENT_FLOOR | PASS for observedZ7; other floors not credited |
| FLOOR_TRANSITION | IMPLEMENTED_UNVERIFIED, no new real7->6->7 observations |
| KNOWN_MAP_UPDATE / KNOWN_MAP_PERSISTENCE | PASS, legitimate expansion and restart842/hash |
| PAN / ZOOM / RECENTER | PASS at UI smoke level, fresh operator/captures/stationary trace |
| LIVE_MOVEMENT_SYNC | PASS on recorded actual steps/marker state |
| CLICK_TO_WALK | PASS for new minimap intents/real protocol movement |
| KNOWN_DESTINATION_ARRIVAL | IMPLEMENTED_UNVERIFIED for required known-minimap click; known world-click arrival observed |
| BLOCKED_ROUTE / PATH_INTERRUPTION | IMPLEMENTED_UNVERIFIED, no exact known blocked goal or active-route authoritative contradiction |
| UNKNOWN_DESTINATION_ACCEPTED | PASS |
| UNKNOWN_DESTINATION_ARRIVAL | PASS for second target only; first NOT_PROVEN |
| UNKNOWN_DESTINATION_BOUNDARY / PROGRESSIVE_DISCOVERY / ROUTE_EXTENSION | PASS for known-only initial segment and newly observed continuation |
| AUTHORITATIVE_BOUNDARY | IMPLEMENTED_UNVERIFIED overall; no hidden knowledge/local success observed, server-obstruction live coverage missing |
| LIVE_VIEWPORT_UNCHANGED / WIDEWORLD_COMPATIBILITY | PASS,18x14 observations and loaded separate static context |
| CLIENTCORE_TESTS / UNREAL_BUILD | PASS, new execution |
| SECRET_CHECK | PASS, candidate and final tracked evidence scans |
| PROTOCOL_ERRORS / UNSUPPORTED_OPCODES | 0 in every retained snapshot; no protocol anomaly identified |
| Login/manual movement/inventory known/worldLook/normalUse smoke | PASS within recorded limits |
| Follow/Stop and container opening smoke | IMPLEMENTED_UNVERIFIED, no new requests/open-container state |

Underground travel was not exercised. No unknown-goal omniscient route,
WideWorld walkability, fake arrival or enlarged live window was observed.
However the missing required checks prevent CERTIFIED. Later broad operator
reports were received after session2 closed and cannot replace missing live
floor/Follow/container/obstruction observations. No acceptance criterion was
removed and no code was patched to ease certification.

## Reproduction and exact next task

Use the published unchanged candidate source with the new report/evidence.
Run the preserved build commands and unchanged run_unreal_minimap_qa.ps1 in
an isolated project. F9 captures JSON/PNG; normal close preserves cache.
Use summarize_minimap_qa.py to emit sanitized sessions. cache_boundary_reader.py
reads ONLY the legitimate client-owned .r33map and outputs queried cells/count/
bounds/hash, never modifies it or issues commands. Raw caches remain ignored;
no proprietary bulk map data, secrets, raw packet captures or chat dumps are
committed. live-trace.txt is a narrow whitelist of observations/movement/UI/
Look/Use events; six selected engine screenshots supplement the three summaries.

Resume independent certification of THIS candidate: manual south and complete
cardinal marker checkpoints; real7->6->7 and per-floor restored map; known
MINIMAP destination with actual arrival; exact known blocked target and active
route server refusal/current-obstacle stop/replan; Follow then Stop; a container
open with F9 while open. Access to the correct active client/normal available
container is required. Do not implement fixes or start polish. Native computer
use requires an externally restored pipe; otherwise an operator must execute
and capture these steps. Build/tests already passed unchanged source; rerun only
if source changes or a specific unresolved concern requires it.

Publish this evidence/status-only commit to milestone/unreal-minimap-navigation-001
on the operator's explicit immediate-closeout request. Final tracked secret scan,
ordinary push, remote equality and cleanliness are checked at publication.
Main remains a97cf25e7449a8a3ef35ef2553181c5032c7a247; no merge authorized here.
