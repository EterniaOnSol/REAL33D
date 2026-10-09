# Presentation polish recovery and preservation - 2026-10-09

Task: UNREAL-WORLD-PRESENTATION-POLISH-001. Local verdict: IMPLEMENTED_UNVERIFIED.
Operator requested an immediate preservation commit before further live QA.
This is not PASS LOCAL or independent certification. No main integration.

## Recovered state

Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Branch: milestone/unreal-world-presentation-polish-001.
Starting local and live remote HEAD: 4305d23738454c2530007b16e759cbcaeddbca55.
Verified origin/main and live remote main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Both 393e7e8570319a35caef7072065d29cf67448475 and HEAD merge bases equal main.
Two commits outside main: 393e7e8 and 4305d23. Main worktree is clean.
No branch creation, reset, rebase, cherry-pick, main merge or frozen checkout edit.

393e7e8 already contained camera/floor/cutaway policies, entity names/health,
speech/target/follow/hover/request overlays, audit, launchers and automation.
4305d23 retained the label paint-space correction and further live findings.
On arrival the worktree was dirty: terrain-layer, wall-alias, transient HISM
material and border-offset fixes, sprite-generating launcher, hardened test
runner, October 7 documentation and September/October evidence all pre-existed.
They were audited and preserved, not reimplemented. recovered-files.json records
their starting hashes. Earlier reports remain unchanged. The prior CURRENT.md
is archived as 2026-10-09_UNREAL-WORLD-PRESENTATION-POLISH-001-before-recovery.md.

## New recovery and live evidence

Recovered October 7 18:24 session's previously unreported screenshot at
2026-10-08T00:27:21.652Z: border XY correction visibly joins the banks and the
inventory coin sprite is present, but triangular fragments remain in water.
Its EndPlay has 27 unanswered walks; it cannot establish gameplay regression.

Fresh normal session: live-20261009T060555, Account B, operator gameplay,
Unreal PID13932. Normal EndPlay at 12:10:23.789Z: 1741 commands, residual0,
unsupported0, anomalies0, synchronized viewport, duplicate/orphan0. 115 walk
requests, 104 accepted, 12 rejected, 0 unanswered, 7 external relocations.
These aggregate counters are not a one-to-one ledger. Native tests plus this
session do not resolve earlier unanswered-walk cases or every regression item.

- Settled capture: floor8, water fragments, readable player/rat labels,
  inventory coin, matching minimap position; operator confirms fragments
  flicker/change with camera movement. FLOOR_READABILITY remains FAILED.
- 12:09:56.210: covered building, upper floors hidden, player visible.
- 12:10:20.164: surface/outdoor scene, restored upper floors, visible player,
  Tom label and hover. Camera requested2133.50/actual417.65, obstructed true.
  This proves retraction, not a completed return-to-clearance check.
- Real floor8->7 movement and sector counts56->89 loads/27 unloads observed.
  No instancing/default-material substitution warning in the fresh normal log.
- One Say received/routed onto its creature; transcript retains "hello".
  Captures were after its world-text lifetime. No visual speech PASS claimed.
- 13 Use and 4 attack requests; server refusal/target-loss messages retained.
  No Follow/Stop, valid Use With or minimap-click arrival proof in this session.

Diagnostic normal-gameplay comparison: live-nonanite-20261009T061219, PID4100.
Temporary -ExecCmds="r.Nanite 0" and -NoSaveConfig, confirmed in log; no global
renderer setting or asset package changed. Manual12:13:38.798 returns to the
same floor8 water area and still shows fragments. Four walks accepted, none
unanswered, protocol errors0. Character later died during the ordinary rat
encounter (HP0, local actor removed); EndPlay's zero position is not a map
teleport. No account restoration, healing, spawning or gameplay changes.

## New runtime correction and diagnostics

BUG: retained geometry shows lower-floor dirt walls357/358/363/367 with height
220 beneath the affected floor8 water. Registry's existing short-wall scale
sets their top exactly one presentation floor above their own base. With
Real33DCoords::UnitsPerFloor=220 this coincides with upper ground/water Z=0.
Read-only source GLB audit and imported mesh inspection found flat upward top
faces, matching mesh bounds and Nanite geometry counts. Nanite-disabled live
comparison did not fix the defect; no Nanite bypass is retained in runtime.

AssetRegistry::TryResolveExperimental now scales the same already-normalized
short walls to218 instead of220, leaving2 units below the next floor surface.
Horizontal placement, source mesh/material, floor spacing and gameplay remain
unchanged. Both live and static consumers use this registry. Actual-catalog
regression checks six imported wall variants against water622: shifted down
one floor, each wall top stays below the water plane and its base stays on its
own floor. This correction is IMPLEMENTED_UNVERIFIED live, not a cleared failure.

Opt-in presentation QA now captures actual resolved speech as it is shown,
using timestamped evidence filenames, to avoid missing its six-second lifetime.
It generates no speech, changes no duration and leaves chat routing untouched.
New speech capture behavior also remains IMPLEMENTED_UNVERIFIED live.

## Checks and execution boundary

Fresh native nine-suite ClientCore PASS, C++17/C++20 libraries built.
Unreal build PASS before and after new correction; final build exit0.
Initial three-case automation PASS: CameraAndFloors, AuthoritativeWindow,
ExperimentalV08; final correction automation result is recorded in the handoff
and automation-wall-clearance.json when complete. No compilation-only live claim.
Tracked-tree/reachable-history secret_check PASS; staged check required before
the preservation commit, including all recovered untracked evidence.

Native computer-use entry point is unavailable in this session; ordinary input
was provided by the operator. No synthetic gameplay scenarios were generated.
Headless mesh inspection initially stalled at the cache service; it was stopped.
Retry hit an unavailable editor subsystem, then a corrected read-only commandlet
completed. Neither incomplete attempt is credited as a test PASS.

Server recovery: installed executable SHA256s matched prepare_wsl.sh. All three
services initially DEAD. Guarded startup refused internal lock873. /proc/873
absent was independently checked; lock preserved as game.pid.stale-polish-20261009.
Prior logs/PID/start files preserved, certified startup reused. querymanager830,
game845, login1067 passed executable/cwd/start/port/readiness checks. Hidden host
PID13268 has a bounded two-hour lifetime; recheck status before reuse. No server
source/config/map/account edits. Clients closed normally; no new session launched
after the operator's preservation instruction.

## Acceptance at preservation

PASS rows below are bounded exercised cases, with earlier live evidence retained
from CONTINUATION-20260929 where stated. No prior certified system is downgraded;
unverified rows concern this polish milestone's complete coexistence acceptance.

| Item | State / limit |
|---|---|
| CAMERA_READABILITY / CAMERA_FOLLOW / CAMERA_ZOOM | PASS, retained exercised session; fresh ordinary camera movement |
| CAMERA_COLLISION | IMPLEMENTED_UNVERIFIED; retraction shown, full return check pending |
| FLOOR_READABILITY | FAILED in latest live build;218-height correction built, live comparison pending |
| ROOF_HANDLING | PASS, retained entry/exit/underground/return and fresh covered/outdoor states |
| WALL_OCCLUSION | IMPLEMENTED_UNVERIFIED; selected cutaways shown, full clearance and corrected floor overlap pending |
| PLAYER_READABILITY / CREATURE_READABILITY / NPC_READABILITY | PASS for observed placeholder/player, rat and Tom; art completeness excluded |
| TARGET_FEEDBACK / FOLLOW_FEEDBACK / COMBAT_TARGET_FEEDBACK | PASS for retained rat target/follow captures; fresh Stop Follow still pending |
| WORLD_SPEECH / NPC_SPEECH | IMPLEMENTED_UNVERIFIED; routing present, visible speech capture pending |
| CHAT_COMPATIBILITY | PASS for Say and server feedback transcript |
| HOVER_FEEDBACK / LOOK_FEEDBACK | PASS, retained world hover and server Look; fresh hover retained |
| USE_FEEDBACK / USE_WITH_FEEDBACK / INTERACTION_FEEDBACK | IMPLEMENTED_UNVERIFIED for complete named checks |
| WIDEWORLD_COMPATIBILITY | PASS for desktop loading/sector movement/materials and authority-window automation; cooked packaging unverified |
| MINIMAP_COMPATIBILITY | IMPLEMENTED_UNVERIFIED; position/floor synchronization shown, fresh map-click arrival pending |
| GAMEPLAY_REGRESSION | IMPLEMENTED_UNVERIFIED; full interaction/follow/navigation matrix incomplete |
| CLIENTCORE_TESTS / UNREAL_BUILD | PASS |
| SECRET_CHECK | PASS for initial tree; final staged result in handoff |
| PROTOCOL_ERRORS | 0 in both fresh sessions |

V08_ART_DEFECT: missing outfit coverage; existing lamp2108/2109 assembly/UV
warnings and mountain1112/1113 source-shape limitations. No V08 art was changed.
POLISH: existing camera and screen feedback retained. OUT_OF_SCOPE: cooked
material permutations, underground static data expansion and animation coverage.
The flickering lower-wall/water overlap is a BUG, not an art completeness gate.

## Exact next work

Continue UNREAL-WORLD-PRESENTATION-POLISH-001. Launch the normal QA script with
Nanite defaults and the218-wall build; revisit the actual underground water
scene through ordinary play, capture and compare floor8/9 overlap. Then exercise
camera clearance return, actual Say/NPC speech (automatic capture now available),
valid Use With, Follow/Stop and minimap-click arrival with sector travel. Preserve
death/respawn as normal server behavior; do not edit accounts to create fixtures.
Recheck the services before startup; never duplicate a running server.

After all local criteria pass, push the milestone and recommend exactly
UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-CERTIFICATION. Until then, finish
this milestone rather than starting another feature milestone or visual editor.

Compact artifacts: unreal-world-presentation-polish-recovery-20261009/.
Raw logs/geometry/diagnostic launchers: build/unreal-world-presentation-polish-001/
recovery-20261009/ and the two live directories above. No bulk geometry/map dump
or generated asset cache is included in this preservation commit.

Final preservation checks: all three post-correction automation tests PASS with
zero errors; staged secret_check PASS and staged private-key scan has no matches.
git diff --cached --check PASS. Operator's immediate commit request is fulfilled
as a local preservation checkpoint; no push or main merge in this run.
