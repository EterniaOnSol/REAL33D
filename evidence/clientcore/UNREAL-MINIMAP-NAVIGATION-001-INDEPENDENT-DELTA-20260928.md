# UNREAL-MINIMAP-NAVIGATION-001 — independent delta certification

UNREAL_MINIMAP_NAVIGATION_CERTIFICATION = CERTIFIED
DELTA_RUN = PASS

Execution date: 2026-09-28 America/Guatemala. Recorded UTC timestamps below fall on 2026-09-29. Operator supplied normal Unreal inputs; Codex independently checked engine captures, normal protocol movement ledger, observed client cache and source authority. No feature or gameplay-rule changes.

## Repository and execution state

- Certification context: `C:/Users/dell/Desktop/fusion32/build/unreal-minimap-certification-20260928`, an existing independent clone with its own `.git` directory. The primary repository's worktrees were inventoried before selection; this clean, exact-head certification context was retained.
- Branch: `milestone/unreal-minimap-navigation-001`.
- STARTING_HEAD and initial remote milestone HEAD: `34e9bfd91f8863b642536221427ce297779d441a`; START_WORKTREE_CLEAN = YES. Fetch before closeout confirmed that same remote HEAD.
- BASE / origin/main: `a97cf25e7449a8a3ef35ef2553181c5032c7a247`; no merge or main update.
- Frozen agent checkout/branch at `b9ed335b02e1241555842ca954cf298cfbd93478` was not switched or modified. Existing ignored static art/WideWorld inputs there were read only; all generated evidence/cache/palette outputs belonged to the independent clone. No autonomous-agent runtime was used.
- Fresh ordinary Fusion32 restart: QM3830 / Game3845 / Login4049, existing WSL baseline 7.72 runtime. Readiness/ports/process provenance checked; no world, DB, configuration or character position edited. Normal Account B via unchanged `scripts/client/run_unreal_minimap_qa.ps1`, existing proven Unreal build. Sessions: `live-20260928T184735`, `live-20260928T185306`, `live-20260928T185943`; all closed normally.
- Native computer-use initialization remained unavailable (`failed to write kernel assets`, OS error 3), so the operator performed the inputs. Confirmation alone was never substituted for missing log/capture evidence.
- Previous reports remain unchanged: [local report](UNREAL-MINIMAP-NAVIGATION-001.md) and [independent report](UNREAL-MINIMAP-NAVIGATION-001-INDEPENDENT-20260928.md). All their independently proven PASS results are retained. Builds/tests and previous unknown arrival were not repeated.
- Evidence-only closeout commit is the publishing commit containing this file (`git log -1`). Normal fast-forward push targets only the milestone branch. Repository facts and preserved prior-report hashes: [repository.json](unreal-minimap-navigation-independent-delta-20260928/repository.json).

Supplemental files below live in `unreal-minimap-navigation-independent-delta-20260928/`. Raw runtime logs and `.r33map` copies remain ignored under `build/`; no bulk map or credentials are published. JSON snapshots omit chat and unrelated diagnostic text. PNGs are unmodified F9 captures.

## South coordinate checkpoint

Session 3, fixed browse center `(32089.50,32224.51)`, selected floor 7. Before F9 `01:01:54.474Z`: BEFORE_X=32090, BEFORE_Y=32217, BEFORE_Z=7. Normal keyboard Down-arrow input 1 requested `01:02:00.165Z`, sent `01:02:00.187Z`, observed `01:02:00.281Z`, accepted by Fusion32 `01:02:00.357Z`. After F9 `01:02:05.302Z`: AFTER_X=32090, AFTER_Y=32218, AFTER_Z=7.

MINIMAP_MARKER_BEFORE: yellow pixel bounds `(1207,63)-(1211,66)` in the 1280x720 F9 PNG. MINIMAP_MARKER_AFTER: `(1207,65)-(1211,68)`. The visible marker moves `(0,+2)` rendered pixels with no intervening pan, zoom or recenter. X and Z are unchanged; authoritative Y increases one tile and the marker moves down. The same-color terrain remains fixed. Earlier session 2 captures were west movement and were not counted as south; the operator explained trying camera-relative `S`. Keyboard Down supplied the valid checkpoint.

The retained independent report supplies manual north, east and west evidence. Together these establish no X/Y swap, mirroring or rotation error. Pan intentionally disables follow; Home restores follow. This was expected behavior and required no code change.

SOUTH_CHECKPOINT = PASS; COORDINATE_MAPPING = PASS. See `south-checkpoint.json`, `south-before.json/png`, `south-after.json/png`.

## Floor transition

Legitimate staircase movement, selected recorded round trip:

| Event | Authoritative player / anchor / minimap | Current displayed floor | Observation UTC |
| --- | --- | --- | --- |
| Before staircase | `(32098,32191,7)` | 7, follow, marker visible | F9 `01:10:32.816Z` |
| North onto staircase, input148/request136 | `(32098,32189,6)` | 6, follow, marker visible | observation `01:10:33.790Z`; F9 `01:10:34.601Z` |
| South back, input149/request137 | `(32098,32191,7)` | 7, follow | observation `01:10:36.127Z` |
| Retained return capture after ordinary approach to bookcase | `(32101,32195,7)` | 7, follow, correct known terrain and marker | F9 `01:10:58.007Z`, then `01:11:59.409Z` |
| Browse old floor without physical movement | `(32101,32195,7)` | 6, browse, terrain present, no player marker | F9 `01:12:02.352Z` |
| Home restores current floor | `(32101,32195,7)` | 7, follow, terrain and marker restored | F9 `01:12:20.377Z` |

Floor selection and marker followed the received Z immediately. Floor-6 terrain differed from floor 7. Old-floor browsing contained zero marker-yellow pixels; current-floor captures contained 14. The floor-7 map rectangle before browsing and after Home had identical pixel SHA256 `de33a90e7ed327eb5b5c5bd109e95556bbce2fae5cc2766ba248d70def7ad78d`. The saved client cache retained all 1606 prior floor-7 cells with identical hints, with separate final counts: floor6=162, floor7=1968. No floor data overwrite or stale marker was observed. Another ordinary round trip during approach is present in the ledger; only the selected fully captured trip above closes this checkpoint.

FLOOR_TRANSITION = PASS. See `floor-transition-checkpoint.json`, five selected floor JSON/PNG pairs, `floor-cache-retention.json`, `cache-after-delta.json`.

## Known minimap destination arrival

START_COORD = `(32090,32218,7)`; actual TARGET_COORD = `(32084,32230,7)`; KNOWN_BEFORE_CLICK = YES. Read-only query of the cache copied before session 2 already contained the target, ground/color103, obstacle hint0, SHA256 `881a4e83bd1a93cc198ade3364c3d3f30ab413dafdaf93391be74a18d10f1f9c`. Terrain hints do not assert current walkability.

`minimap destination` at `01:03:44.409Z` proves this was a minimap click. The planner requested 18 ordinary cardinal steps, inputs/requests4–21; all were sent through ClientCore/protocol and accepted by Fusion32. Arrival at `01:03:54.079Z` explicitly used authoritative player `(32084,32230,7)`. F9 `01:03:56.209Z` agrees in player, anchor and minimap. No teleport or fabricated success. Earlier world-click arrival from session 1 was not used for this checkpoint.

KNOWN_DESTINATION_ARRIVAL = PASS. See `known-actual-target-before.json`, `known-arrival-checkpoint.json`, `known-arrival.json/png`.

## Blocked route / server-refused next step

REQUESTED_TARGET = known `(32092,32197,7)`; actual minimap click `01:05:00.069Z`, start `(32087,32227,7)`, planned41 steps. LAST_VALID_POSITION = `(32087,32221,7)` after six accepted north steps.

REJECTED_OR_BLOCKED_STEP = input35/request35, north to `(32087,32220,7)`, sent `01:05:03.134Z`. SERVER_RESULT = Fusion32 refused request35; authoritative position remained `(32087,32221,7)`. NAVIGATION_RESULT = `left-click walk stopped: refusal, relocation or missing answer`, `01:05:03.216Z`, followed by the recorded refusal at `01:05:03.221Z`. No arrival was claimed for the target.

A normal manual south request36 occurred during this route and was accepted to `(32087,32222,7)` at `01:05:03.350Z`. The exact server refusal cause is not exposed: this is the permitted legitimate server-refused-step case, not evidence that the refused tile is permanently nonwalkable. A subsequent route legitimately entered that tile. No artificial blocker or rule modification was introduced. Local route expectation did not advance the authoritative character or override the refusal. F9 `01:05:11.385Z` retains the stopped state and actual position.

BLOCKED_ROUTE = PASS. See `blocked-target-before.json`, `blocked-route-checkpoint.json`, `server-refusal-after.json/png`, `session-3-live-trace.txt`, movement ledger in `session-3-summary.json`.

## Path interruption

A separate minimap route to `(32093,32195,7)` started at `01:05:22.146Z` from `(32087,32222,7)`, planned35 cardinal steps. Inputs37–39 legitimately moved north through `(32087,32221,7)`, `(32087,32220,7)`, `(32087,32219,7)`.

While request39 was active, normal manual south input40 was requested `01:05:23.245Z`; it was accepted from `(32087,32219,7)` to `(32087,32220,7)` at `01:05:23.888Z`. The route ceased continuing toward its goal, and no arrival was claimed. Subsequent gameplay actions came from new user requests. The held actual position and absence of a next automatic step are retained in the trace/ledger; operator confirmation corroborates the stop. This case does not rely on a fabricated post-interruption F9 snapshot.

PATH_INTERRUPTION = PASS. See `path-interruption-checkpoint.json` and session-3 trace/summary.

## Authoritative boundary

- All positions used above came from applied normal game state, with synchronized local creature, anchor and minimap snapshots. Prediction only requested adjacent commands; it never wrote authoritative player position.
- The known target and next-step hints existed before the refused route. They could not force Fusion32 to accept request35. The server refusal left position unchanged, stopped navigation and prevented false arrival.
- Manual control replaced an active navigation goal; actual accepted server state won. Normal staircase and Follow relocations also won over same-floor local movement expectations.
- Source inspected at this unchanged candidate: `reference/game/src/receiving.cc::CGoDirection` applies normal directional offsets, runs authoritative movement and sends refusal/snapback results; `sending.cc::SendFloors` and `cract.cc::TCreature::NotifyGo` supply received floor/position updates. Selected game source revision is `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, inventoried in `docs/protocol772/SOURCE_TRUTH.md`. The 7.72 build uses the `TIBIA772` login/version branch in `communication.cc`; these movement/floor helpers have no local version guard.
- `clientcore/src/minimap.cpp::ProjectMinimapFloor` records only the observed current-floor viewport; `FindNavigationPath` is const and cannot discover terrain. `Real33DBridge.cpp` updates minimap/player from applied WorldState. `Real33DWorldActor.cpp::FindKnownWalkPath`/`IsKnownWalkTile` use client-observed terrain/live blockers. `Real33DPlayerController.cpp::Request`/`TickClickWalk` reset, stop or safely replan after manual override/refusal/relocation; arrival requires actual player==goal. `Real33DMinimapPanel.cpp` draws the marker only on the authoritative player's floor.
- WideWorld is a separate read-only presentation input and is not consulted by the navigation authorization path. Retained independent unknown-boundary evidence proves unknown terrain was not silently treated as verified walkable; it advances only after normal descriptions make the next segment known. That already-proven test was not repeated.
- No hidden server map, DB, runtime character state or omniscient route was consulted. Server access was limited to ordinary service readiness/startup; cache queries read only legitimate client-owned observations and issued zero commands.

AUTHORITATIVE_BOUNDARY = PASS, combining the new refusal/interruption evidence with the retained independent knowledge-boundary evidence. No code regression was established or fixed.

## Follow / Stop smoke

Normal visible target creature1073743184: battle-list Follow input72 at `01:08:15.075Z`, state active `01:08:15.314Z`; F9 `01:08:17.509Z` has following=true and target_actor_visible=true, with `[F]` visible. Normal general cancel input73 at `01:08:18.689Z` cleared target at `01:08:18.846Z`; F9 `01:08:20.522Z` has following=false, target0. Subsequent manual west input74/request64 accepted `01:09:39.235Z`, `(32087,32234,7) → (32086,32234,7)`, proves normal movement control returned.

FOLLOW_SMOKE = PASS; STOP_FOLLOW_SMOKE = PASS. See `follow-stop-checkpoint.json`, `follow-active.json/png`, `follow-stopped.json/png`. No full combat/follow recertification claim.

## Container-open smoke

Player `(32101,32195,7)` used legitimate world bookcase type2435 at `(32101,32194,7)`, stack1, normal request163 at `01:10:54.512Z`, open-as container0. F9 `01:10:58.007Z` records uses_requested=1 and open_containers=1, server container `bookcase`, capacity6, has_parent=true, object_count0. The UI visibly opens six empty slots (`0 of 6`), faithfully reflecting the legitimate empty content.

CONTAINER_OPEN_SMOKE = PASS. See `container-open-checkpoint.json`, `container-open.json/png`. No inventory/container certification repeated.

## Complete delta-session protocol result

All three normal EndPlay records and all27 F9/automatic/final snapshots: UNSUPPORTED_OPCODES=0, PROTOCOL_ANOMALIES=0, residual_bytes=0. Last session closed `01:13:03.801Z` with last_diagnostic=`none`, synchronized viewport, player/anchor/minimap `(32101,32195,7)`. No duplication or orphan failure observed. PROTOCOL_ERRORS = 0.

Gameplay refusals are not decoder errors. Final session retains137 accepted steps,11 refused ledger entries,16 authoritative external relocations and4 unanswered ledger entries around staircase transitions. These counters are preserved without relabeling them as accepted steps; authoritative relocation/floor state remained correct and no false arrival was credited. See `protocol-result.json`, `session-3-end.json` and all three sanitized session summaries.

## Final consolidated certification matrix

| Criterion | Result | Evidence origin |
| --- | --- | --- |
| MINIMAP_RENDER / PLAYER_MARKER / CURRENT_FLOOR | PASS | Retained independent report |
| KNOWN_MAP_UPDATE / KNOWN_MAP_PERSISTENCE | PASS | Retained independent report |
| PAN / ZOOM / RECENTER / CLICK_TO_WALK | PASS | Retained independent report |
| COORDINATE_MAPPING / SOUTH_CHECKPOINT | PASS | New south capture pair + retained N/E/W |
| FLOOR_TRANSITION | PASS | New real7→6→7, old-floor no-marker and restoration |
| KNOWN_DESTINATION_ARRIVAL | PASS | New known minimap click and authoritative18-step arrival |
| BLOCKED_ROUTE | PASS | New legitimate Fusion32-refused route step, no fake arrival |
| PATH_INTERRUPTION | PASS | New manual override of active route |
| AUTHORITATIVE_BOUNDARY | PASS | New refusal/interruption + retained knowledge boundary |
| FOLLOW_SMOKE / STOP_FOLLOW_SMOKE | PASS | New active/cleared F9 and returned manual control |
| CONTAINER_OPEN_SMOKE | PASS | New normal world-use/server container/UI |
| UNKNOWN_DESTINATION_ACCEPTED / UNKNOWN_DESTINATION_ARRIVAL / UNKNOWN_DESTINATION_BOUNDARY | PASS | Retained independent report, second unknown arrival |
| PROGRESSIVE_DISCOVERY / ROUTE_EXTENSION | PASS | Retained independent report |
| LIVE_VIEWPORT_UNCHANGED / WIDEWORLD_COMPATIBILITY | PASS | Retained independent report |
| CLIENTCORE_TESTS / UNREAL_BUILD | PASS | Retained independently executed tests/build; no source changes |
| SECRET_CHECK | PASS | Final staged and committed evidence scan, `secret-check-final.txt` |
| UNSUPPORTED_OPCODES / PROTOCOL_ANOMALIES / PROTOCOL_ERRORS | 0 | Complete new delta sessions |
| UNREAL_MINIMAP_NAVIGATION_CERTIFICATION | CERTIFIED | All required retained and delta checkpoints pass |

Remaining required minimap certification items: none. Preserved prior results are not downgraded. Certification-only publication; main unchanged and MERGED_TO_MAIN = NO. WORLD-PRESENTATION-POLISH has not started. Await a separate explicit integration instruction.
