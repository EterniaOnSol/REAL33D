# UNREAL-MINIMAP-NAVIGATION-001

State: PASS local. Independent certification: NOT_STARTED.
Branch: milestone/unreal-minimap-navigation-001.
Base: a97cf25e7449a8a3ef35ef2553181c5032c7a247.
Worktree: build/unreal-minimap-navigation-001 (isolated).

## Main integration

Fetched origin in clean build/integration-item-use. Remote main was exactly
bd15cc0a49d8182dc1cc3732b8487859f1662044; certified milestone was exactly
a97cf25e7449a8a3ef35ef2553181c5032c7a247. Ancestor check passed; divergence
was 0 main-only / 3 milestone-only. `git merge --ff-only` advanced local main,
ordinary `git push origin main` succeeded. Local main/origin/main and direct
remote main/milestone queries all matched a97cf25. No merge commit or rewrite.
Integration worktree clean; original certification report and handoff present.
Worktree-aware secret check passed all six checks with no Git errors.
Primary dirty checkout was not used for mutation, commit, merge or cleanup.

## Implementation and authority

Audit was written before implementation in
`docs/UNREAL_MINIMAP_NAVIGATION.md`, including the selected source functions,
floor transforms, independent 2D conceptual references and exact gaps.
The new ClientCore projection is gated by synchronized authoritative state,
only the actual current floor and unchanged 18x14 bounds. History stores ground,
static color-source identity and a stale terrain obstacle hint.
No creature, movable-item stack, hidden entity or assertion of current
historical walkability. Static terrain can estimate a route, with each real
adjacent step validated separately against normal live state and Fusion32.
WideWorld never supplies minimap knowledge or path safety. No protocol changes.

Custom terrain cache is bounded, transactional and world/character scoped.
v2 adds static color-source hints; v1 exploration can migrate. Each reconnect
clears live/player validity until real observations arrive. Save is periodic or
on normal disconnect. Derived palette/cache/assets stay ignored and local.
Selected DAT hash and the independent reader are recorded in the audit. The
palette contains appearance colors only, no world coordinates or map seed.

Slate map lives in the existing HUD. North up, +X right, +Y down; marker uses
actual coordinates. Floor changes restore actual-floor follow. Drag/wheel,
buttons and recenter only change the map camera. Minimap click walking accepts
same-floor unknown destinations too. Known-terrain A* finds complete routes;
otherwise a known reachable segment toward the destination's exploration
frontier is walked. The original goal is retained across segments, with replanning
as normal observations arrive. No unknown tile is returned or added to memory.
No viewport/distance cutoff; cross-floor navigation remains unsupported.
Actual current blockers override history. The executor validates each
adjacent live step, waits for acceptance, and can replan a changed next tile.
Refusal/relocation/missing answer stops it. No WideWorld route input.
The latest UI uniformly draws terrain without the live-viewport rectangle or
history dimming, as requested by the operator after accepting the colors.

## Executed evidence

Artifacts: `unreal-minimap-navigation-001/` beside this report. Generated raw
session logs/screenshots remain under ignored `build/minimap-evidence/`.
Compact JSON is reproduced with `scripts/client/summarize_minimap_qa.py`;
it whitelists coordinates, observation counts, movement ledgers and counters,
and excludes runtime configuration, credentials, chat and bulk map data.

Session live-20260928T121445, PID 13148, normal operator movement:
initial actual/anchor/minimap position (32097,32206,7), known/live 252/252.
Known terrain grew to 1062. Observed actual floor sequence 7 -> 6 -> 7.
EndPlay 2026-09-28T18:18:33.792Z: (32091,32181,7), 1062/252.
78 requested walks: 64 accepted, 3 refused, 6 unanswered; 11 external
relocations are kept separate from accepted requests. Final residual bytes,
unsupported opcodes and protocol anomalies each 0. Accepted north/south/west
steps are retained. No item-use/container/combat/follow intents were exercised.

Session live-20260928T122420, PID 17840, normal operator movement:
loaded the first session's 1062 cells from cache, initial (32091,32181,7).
EndPlay 2026-09-28T18:27:07.314Z: (32096,32212,7), known/live 1198/252.
40 requested walks: 38 accepted, 2 refused, 0 unanswered, 0 external
relocations. Accepted south/west/east steps retained. All three protocol/error
counters 0. No fresh regression intents. Inspector hidden for this launch.

Those runs used the initial generic palette. The first automatic screenshots
were taken during shader/mesh preparation, with stale initial HUD paint; they
do not prove final map/world rendering. QA timing has been corrected to start
at first authoritative minimap observation and wait for asset compilation.
The operator explicitly rejected the gray palette. The new revision supplies
the original 216-color DAT hints for ground/static observed appearances.
Those two sessions do not prove the final palette revision.

Session live-20260928T123750, PID17536: restored1198; settled screenshot
2026-09-28T18:40:14.219Z proves colored terrain/marker/coordinates/floor and
WideWorld together. Operator accepted the colors. EndPlay18:44:35.549Z at
(32089,32197,7), known1232/live252; 22 requested/accepted steps, no refusal,
unanswered or external relocation; all protocol/error counters0. Actual
minimap clicks generated normal routes and accepted movements, but distant
known clicks were rejected by the then-existing live-window planner. This
caused the operator's request for the A* revision and removal of live highlighting.

Final unknown-goal planner validation: Windows native nine suites/pure
view+palette PASS; ASan/UBSan nine suites PASS (3.60sec); Unreal build PASS
(71.43sec, then unique F9 checkpoint fix 11.30sec). Core tests cover a 300-step
historical corridor with no live observations, 302-step detour, unknown-goal
segments and continuation after real fixture observations, off-floor rejection,
static/current blockers, cache reload and bounds. Planning leaves knowledge and
live observation counts unchanged; every returned segment cell was known.
`tests/minimap_cache_route_probe.cpp` replayed the legitimate local1232-cell
cache through actual sanitized ClientCore code: from(32089,32197,7) to previously
rejected(32088,32184,7), known route FOUND/22steps, live observations0/network
commands0. It proves an estimate, not gameplay success. Probe output retained.
Uniform UI/known-A* session live-20260928T125637, PID2400, closed normally.
EndPlay18:59:25.592Z: (32092,32165,7), known1578/live252. 71 requested,
60 accepted, 0 refused, 11 unanswered and 11 external relocations separately
recorded. Protocol counters0. Four floor transitions were recorded. A minimap
click at18:58:40.236 requested (32090,32179,7) from(32092,32203,7), a 26-step
route beyond the live window. Accepted click-walk requests continued outside
the starting viewport; interruption prevented crediting destination arrival.
The screenshot shows uniform known terrain without a live-tile rectangle or
brightness split. Unknown goals still failed in that revision; not final evidence.

The latest cache-only probe uses those1578 legitimately retained cells. From
(32092,32165,7), previously rejected goal(32122,32144,7) is unknown; the new
planner finds a 21-step observed segment, does not claim arrival, and has
live observations0/network commands0. This is planning evidence only.
Latest unknown-goal live revision: live-20260928T131649, PID16540.
Every F9 checkpoint now has a timestamped filename, preserving floor/control
comparisons rather than overwriting the preceding checkpoint.

This session closed normally at19:22:09.952Z. Initial position(32092,32165,7),
known1578/live252; final authoritative/anchor/minimap(32098,32164,7),
known1986/live252. 62requested57accepted5refused0unanswered0external;
accepted directions N10/E22/S9/W16. 121frames429commands; residual bytes,
unsupported opcodes and protocol anomalies each0. Tile/creature counts match
WorldState, no duplicate/orphan events. Terrain expanded only with observations.

At19:20:41.260 a minimap click chose unexplored(32125,32146,7), returning a
21-step observed segment ending(32105,32157,7). At19:20:55.257, after normal
accepted movement and new terrain, another15-step segment retained the original
goal. Input29 east was rejected at(32110,32155,7); at19:21:00.316 the route
stopped, leaving the actual player there. No fake move/arrival. Later known
minimap click(32092,32167,7) generated a25-step route; a subsequent world click
ended at authoritative(32098,32164,7). Do not credit either interrupted minimap
route with destination arrival. Native planning also covers blocked goals/detours.

Operator explicitly reported that clicking black terrain advances and discovers
terrain, then reported "funciona todo" after the bundled QA request. These
statements are retained in operator-feedback.json. They supplement the logs.
Three Use intents and five Attack intents were sent; two tactics changes and
three server target clears were received. Inventory stayed known. These do not
prove a successful usable-item/container/follow interaction: no Follow request,
no retained open-container checkpoint. The session has no floor transition or
pan/recenter log. Asked specifically about those missing traces, the operator
clarified "no hice peticion de follow.. el resto funciona". Thus visual/control
results rely on operator observation, with automated traces limited as stated.
Follow was not exercised in that session; it is verified in the next run below.
Earlier sessions prove floor movement separately; controls and fresh regression
results must be distinguished from those prior results. Last timestamped F9 PNG
shows the uniform colored map and 9/100health after real combat.

Final regression session live-20260928T132553, PID844, closed normally
19:29:31.135Z. Restored1986 cells at(32098,32164,7); ended(32086,32150,7),
known2528/live252. 277frames770commands; residual/unsupported/anomaly0;
98requested69accepted29refused1unanswered11external (ledger counters verbatim,
not assumed mutually exclusive). Accepted N27/E6/S15/W21. Actor counts equal
WorldState303tiles/3creatures, no duplicates/orphans. Floor7->6->7 restored
surface knowledge in the final revision; the final timestamped F9 PNG retained.

19:28:12.295: normal Battle List Follow input62, observed target1073743109.
19:28:12.442: normal client combat state action=follow. Three authoritative
external relocations were recorded during the follow interval, separate from
manual accepted walks. 19:28:16.505: normal general cancel input63, target clears.
Operator confirmed "Follow y Stop funcionaron". Snapshot19:28:36.074Z records
one Follow/one cancel, no protocol errors. Final run also has five Attack,
one Use/two tactics requests and five server target clears. No positive Follow
ack exists in this protocol; an intent/state trace alone is not such an ack.

## Local acceptance

| Criterion | Result and basis |
| --- | --- |
| MINIMAP_RENDER / PLAYER_MARKER | PASS: inspected loaded current-revision screenshots, readable colored terrain/marker/readout; operator accepted colors. |
| COORDINATE_MAPPING / LIVE_MOVEMENT_SYNC | PASS: N/S/E/W authoritative ledger deltas, matching observation coordinates, snapshot actual/anchor/minimap coordinates, operator movement review. North up; follow keeps marker centered while terrain moves. |
| CURRENT_FLOOR / FLOOR_TRANSITION | PASS: current Z7 and final-revision 7->6->7 observations/player-state round trip; operator confirms floor behavior. Underground live travel not exercised. |
| KNOWN_MAP_UPDATE / KNOWN_MAP_PERSISTENCE_DURING_TEST | PASS: known1578->1986->2528, actual cache restoration on reconnect and retained floor/return observations; no static-map seeding. |
| PAN / ZOOM / RECENTER | PASS, operator-observed: requested drag/zoom/Home test, operator explicitly confirmed all except then-untested Follow. No individual pan/recenter log or comparison PNG retained; repeat during certification. Native control tests supplement that live report. |
| CLICK_TO_WALK / PATH_INTERRUPTION | PASS: real unknown-goal segment continuation, known-click movement, server refusal stops without local advancement. Arrival at unexplored goal not claimed. |
| AUTHORITATIVE_BOUNDARY / LIVE_VIEWPORT_UNCHANGED | PASS: unchanged18x14 bounds, observed live cells<=252 (78 on floor6); no mutable entities in history, no discovery from planning, next-step/server ledger validation, real refusal preserved. |
| WIDEWORLD_COMPATIBILITY | PASS: loaded world/minimap screenshots together and normal streaming logs; no route/discovery data from WideWorld. Existing visual-material/outfit limitations remain. |
| Inventory/container/use/combat/follow regression smoke | PASS at smoke level: known inventory, normal Use/Attack/tactics/Follow/cancel requests with server responses and operator report that the rest works. Open/close container and successful Use effects lack individual retained checkpoints; no claim of new full interaction certification. |
| PROTOCOL_ERRORS | 0 residual bytes, unsupported opcodes and anomalies at every retained final snapshot. |
| CLIENTCORE_TESTS / UNREAL_BUILD / SECRET_CHECK | PASS: nine Windows/native and ASan/UBSan suites, pure view/palette tests, final Unreal builds, six-check secret scan. |

PASS is local functional QA, not CERTIFIED. Independent reviewer must repeat
the original procedure, especially controls and container/use checkpoints.

## Reproduction

1. Build ClientCore with `tests/build_clientcore_windows.cmd` in an x64 VS
   environment (C++17 /W4 /WX and C++20 link library); run the ninth minimap
   suite and the eight existing suites. Build/run
   `tests/real33d_minimap_view_tests.cpp` with C++17 /W4 /WX.
2. Sanitizers: CMake clientcore Debug with address,undefined sanitizers and
   frame pointers; build then `ctest --output-on-failure` (nine suites).
3. Build REAL33DEditor Win64 Development using UE 5.8 Build.bat, -WaitMutex.
4. Start the normal selected Fusion32 services with
   `scripts/server/start_wsl.sh`; never inspect DB state for minimap discovery.
5. Run `scripts/client/run_unreal_minimap_qa.ps1`. It accepts explicit runtime,
   asset worktree, static-data root, selected ClassicDat and PythonLauncher
   paths. It validates local palette input and launches the isolated project.
6. Wait for assets. F9 records snapshot and UI PNG. Check spawn coordinates,
   north/south/east/west, out-and-back, legitimate floor change and return;
   capture both floors. Drag, zoom, browse floor and Home; capture before/after.
   Click valid current live, historical and unknown same-floor destinations;
   verify unknown segments continue after normal discovery. Test a blocked goal;
   verify real accepted/refused movements, cancellation and no local success.
7. Exercise real available inventory/container/use/combat/follow interactions.
   Missing prerequisites do not count as a regression PASS. Close normally.
8. Summarize sessions, retain selected final PNGs and build/test/secret outputs.
   Run secret check with explicit WSL GIT_DIR/GIT_WORK_TREE for this worktree.

## Closeout and remaining limits

All minimum criteria above have local executed-test or operator-observed live
results. Commit/push this milestone branch after final scan; do not merge main.
No independent certification, underground live trip, full multi-floor routing,
unexplored-goal destination arrival or comprehensive original-client/art parity
is claimed. Operator controls/container reports have the checkpoint limits
listed above. V08 art and existing WideWorld material/outfit defects are unchanged.
Exact next task: independently certify UNREAL-MINIMAP-NAVIGATION-001 before
integration. Recommended next implementation milestone after certification:
UNREAL-WORLD-PRESENTATION-POLISH-001, reflecting the operator's overall 3D
presentation feedback; define that scope separately before changing art.
