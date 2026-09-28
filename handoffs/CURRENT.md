# CURRENT - REAL33D minimap/navigation

Date/time: 2026-09-28 13:33 America/Guatemala / 19:33 UTC.
Agent: Codex primary, implementation and QA collection.
Task: UNREAL-MINIMAP-NAVIGATION-001. State: PASS local.
Independent certification: NOT_STARTED.
Branch: milestone/unreal-minimap-navigation-001.
Starting commit: a97cf25e7449a8a3ef35ef2553181c5032c7a247.
Ending commit: the closeout commit containing this handoff; resolve with
git rev-parse HEAD in this branch. Exact pushed hash is returned to the operator.
Closeout: normal milestone commit/push authorized after local PASS; no main merge.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-minimap-navigation-001.
Prior substantive handoff archived at
archive/2026-09-28_UNREAL-ITEM-USE-INTERACTION-001-integrated-main.md.

## Integration and protected checkout

Phase A completed with a clean isolated build/integration-item-use worktree.
Fetched main bd15cc0a49d8182dc1cc3732b8487859f1662044 and certified remote
milestone a97cf25e7449a8a3ef35ef2553181c5032c7a247. Ancestor check passed,
0 main-only / 3 milestone-only commits. Fast-forward only, ordinary push;
local main, origin/main and direct remote queries match a97cf25. No merge
commit or rewrite. Certification evidence/handoff preserved. Six-check secret
scan passed using explicit WSL worktree Git paths. Integration tree clean.
The primary checkout remains unrelated dirty REAL33D2D/agent work; do not
modify, clean, stash, reset, stage, commit or merge from it.

## Objective, inspection and discoveries

Implement a real observed-world minimap in the existing Unreal HUD. Full
pre-implementation audit and selected Fusion32 function references are in
docs/UNREAL_MINIMAP_NAVIGATION.md. Existing map/view decoders already handle
18x14 rows/fullscreens/floor transitions. Existing minimap was an empty frame.
Existing world-click BFS/step ledger is reusable only on current live tiles.
No REAL33D cache existed. WideWorld has separate static sectors and must never
seed discovery or route walkability. Tibia X east/Y south; floor 7 surface,
0 highest/15 deepest; world tile 100uu, floor height 220uu. Minimap is north up.
The separate 2D minimap and color formula were read conceptually; no parser or
model is copied, linked or changed. The selected DAT is read by REAL33D's own
validated visual/tools/tibia772.py reader and checked against its pinned hash.

## Changes and files

ClientCore minimap.h/.cpp: synchronized current-floor terrain-only projection,
explored retention, live/history separation, bounded transactional scoped cache.
Only ground/static color identity and stale static obstacle hint are retained;
no creatures/movable entities/stacks or claims of present occupancy. object_types adds
existing server Unmove metadata; map/protocol dimensions and opcodes unchanged.
minimap_tests plus CMake/Windows test script add the ninth native suite.

Real33DBridge owns game-thread memory and exposes semantic cells/state; worker
publishes the existing authoritative observations. Saved/Minimap caches persist
periodically and on disconnect; v2 adds static appearance colors and accepts v1
exploration migration. Real33DMinimapView/Palette are separate local view and
appearance metadata; native tests cover coordinate inverse, controls, bounds,
malformed palette rejection and isolation. MinimapPanel draws terrain, marker,
coordinates/floor and local controls. Known terrain is uniform: no live
footprint or history/live brightness split.
HUD forwards state/destinations; controller reuses RequestKnownWalk and the
normal RequestWalk ledger. The final planner uses known-terrain A* with no
viewport/distance cutoff. Unknown same-floor goals are accepted: a reachable
observed segment toward an exploration frontier is returned, the original goal
is retained, and segment completion replans after normal observations arrive.
Planning adds no terrain or live observations. Every returned step was known.
Current observed blockers override static history.
World next-step validation requires observed Bank ground and no current
blocker/creature. Changed next steps can replan; refusal/relocation/missing
answer stops. Cross-floor routes cannot be inferred. WideWorld is not
input. Static terrain is a route estimate, never current gameplay authority.

extract_minimap_palette.py validates selected 7.72 DAT and emits 2502 local
appearance colors (no positions/map seed) into ignored Saved/Minimap. Static
observed colored appearances choose terrain colors from the 216-color cube.
No V08 or creature art is modified. Operator rejected generic gray palette;
this revision replaces it with original local appearance colors. After live
acceptance, operator requested unlimited-by-viewport known-map click walking
and removal of live-tile visuals. The final map uniformly draws known terrain,
without cyan footprint or history/live dimming. Inspector
can be disabled for QA. Normal F9 and opt-in initial/settled engine captures
retain screenshots; settled waits for asset compilation. F9 Manual checkpoints
now use unique timestamped filenames, preserving every floor/control sample.
No automated gameplay
commands or fabricated movements. run_unreal_minimap_qa.ps1 checks prerequisites
and launches own project with read-only existing visual/static inputs.
summarize_minimap_qa.py produces compact sanitized session evidence.

## Tests/results and evidence

ClientCore Windows C++17 /W4 /WX, C++20 link archive: all nine suites PASS.
Pure minimap view/palette native tests: PASS. WSL ASan/UBSan: nine suites PASS.
Unreal 5.8 REAL33DEditor Win64 Development: PASS, unknown-goal build71.43sec
and unique F9 checkpoint build11.30sec. Final sanitizer suites PASS (3.60sec).
300-step historical route, static/current blockers, 302-step detour, cache,
coordinate bounds and unknown-goal segments tested. New fixture observations
allow segment continuation; planning itself changes no knowledge/observations.
Sanitized ClientCore cache probe found22steps from(32089,32197,7) to previously
rejected(32088,32184,7), using legitimate client history only. Estimate only,
no live observations/network commands. Latest1578-cell cache replay from
(32092,32165,7) accepts unknown goal(32122,32144,7), returning21observed steps
without claiming arrival, live observations0/network commands0. See
tests/minimap_cache_route_probe.cpp and cache-unknown-goal-probe.txt.
Earlier fixture narrowing warning was corrected with explicitly typed constants.
Evidence report: evidence/clientcore/UNREAL-MINIMAP-NAVIGATION-001.md.
Compact artifacts: evidence/clientcore/unreal-minimap-navigation-001/.
Raw generated logs/PNGs remain in ignored build/minimap-evidence/.

Operator session live-20260928T121445: initial (32097,32206,7), 252 known;
end (32091,32181,7), 1062 known, observed 7->6->7. 64 accepted / 3 refused /
6 unanswered from 78 requests, with 11 external relocations separately recorded.
Session live-20260928T122420: cache restored 1062, end known1198; 38 accepted /
2 refused from40 requests, no unanswered/relocations. Combined accepted N/S/E/W
examples retained. Both final protocol residual/unsupported/anomaly counters0.
Those sessions used generic colors, and early screenshots occurred during asset
preparation; they do not prove this final palette revision.

Current colored session live-20260928T123750, PID17536: restored1198 cells,
actual/anchor/minimap (32096,32212,7), live252. Settled snapshot
2026-09-28T18:40:14.219Z and PNG prove visible colored terrain, marker, readable
coordinate/floor and WideWorld scene together; no commands at that checkpoint.
Protocol/error counters0. EndPlay18:44:35.549Z at(32089,32197,7), known1232,
22 requested/accepted steps and all protocol/error counters0. Minimap clicks
generated real accepted routes, but far known destinations were rejected by
the earlier live-only planner. Operator accepted colors and requested the final
navigation/UI changes above. Do not retroactively credit them to this session.
Session live-20260928T125637, PID2400, used uniform rendering and known A*.
Closed normally18:59:25.592Z at(32092,32165,7), known1578/live252;
71requests60accepted0refused11unanswered11external. Four floor transitions,
protocol/error counters0. A26-step minimap route to(32090,32179,7) from
(32092,32203,7) issued accepted movement beyond its initial viewport; it was
interrupted, so destination arrival is not credited. Unknown clicks still failed
in that preceding revision. Uniform screenshot retained separately.
Unknown-goal sessionlive-20260928T131649 closed normally19:22:09.952Z.
Initial(32092,32165,7), known1578; end(32098,32164,7), known1986/live252.
62requested57accepted5refused0unanswered0external. AcceptedN10/E22/S9/W16;
121frames429commands; residual/unsupported/anomaly0, no duplicates/orphans.
Clickunknown(32125,32146,7) returned21known steps; after normal observations,
the executor continued with15steps retaining that original goal. Input29 east
was refused at(32110,32155,7); route stopped without a fake move or arrival.
Later known minimap route and normal world-click authoritative arrival retained.
ThreeUse/fiveAttack/twoTactics requests, three server target clears; inventory
known. No Follow request. Operator confirmed unknown-click discovery, then
"funciona todo", clarifying "no hice peticion de follow.. el resto funciona".
Preserve the distinction between operator-observed controls and per-action logs;
latest session has no pan/recenter/floor trace or open-container checkpoint.
Final Follow sessionlive-20260928T132553 closed normally19:29:31.135Z.
Initial1986cells(32098,32164,7), end2528/live252 at(32086,32150,7).
277frames770commands, residual/unsupported/anomaly0; actor/model303tiles/3creatures,
no duplicates/orphans. 98requested69accepted29refused1unanswered11external
are verbatim counters, not assumed mutually exclusive. Follow input62 from
Battle List19:28:12.295, actionfollow state19:28:12.442; three authoritative
relocations during follow interval. Cancel input63 at19:28:16.505. OneFollow/
oneCancel, fiveAttack/oneUse/twoTactics and five server target clears retained.
Operator confirmed Follow/Stop. Actual floor7->6 at19:28:53.733,
then6->7 at19:29:10.454; minimapfloor6 live78, restoredsurface live252.
All required local acceptance results are documented in the milestone report.

## Remaining unverified, blockers and risks

Follow regression smoke now passed. Render/current movement, map
expansion, unknown-goal continuation, blocked-path stop and protocol counters
have actual retained evidence. Floors have preceding live round-trip traces;
operator reports remaining controls/interactions work. Pan/zoom/recenter and
container observations do not have individual action checkpoints. Repeat those
explicitly during independent certification. Underground live travel, multi-floor
path topology and unexplored-goal arrival are not claimed. Computer-use native pipe is
unavailable despite retry/reset; operator input is needed for these actions.
The operator completed live QA; use F9 checkpoints and normal EndPlay, not raw packets,
teleportation, server DB reads, 2D agents or synthetic movement success.
WideWorld materials without instancing usage and unmatched outfits are existing
visual limitations. Do not modify V08 art or start a visual editor to fix them.

Services were initially down. A stale game PID sentinel1835 prevented startup.
An ignored guarded helper verified the exact runtime path, all ports free,
PID absent and no running game before removing only that stale lock. No saves,
rules or source were edited. Normal services then started (QM540/game555/login769);
leave the active runtime alone unless authorized service work requires otherwise.

## Exact next task/commands

Independently certify UNREAL-MINIMAP-NAVIGATION-001 from the pushed milestone
branch, using the exact reproduction and acceptance table in the evidence report.
Run scripts/client/run_unreal_minimap_qa.ps1 against the ordinary Fusion32
services; no hidden target IDs or invented movement. Last player position is
(32086,32150,7), cached2528. Capture distinct drag/zoom/Home, container/validUse,
floor and return checkpoints in addition to the already retained route evidence.
Capture compact session evidence with scripts/client/summarize_minimap_qa.py.
Copy current test/build outputs and settled PNG to evidence folder. Update
report/status/parity/handoff with exact verified versus unverified results.
Run secret check with GIT_DIR=/mnt/c/Users/dell/Desktop/fusion32/.git/worktrees/unreal-minimap-navigation-001
and GIT_WORK_TREE=/mnt/c/Users/dell/Desktop/fusion32/build/unreal-minimap-navigation-001.
Local PASS is ready for milestone-only publication; final verification is exact
local/remote equality and a clean worktree. Do not merge into main before
independent certification. Recommended next implementation milestone after
certification: UNREAL-WORLD-PRESENTATION-POLISH-001, scoped separately; the
operator's overall world-presentation concerns remain valid and unaddressed here.
