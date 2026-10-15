## Preserved continuation checkpoint - 2026-10-14 21:53 America/Guatemala

Task UNREAL-WORLD-PRESENTATION-POLISH-001; agent Codex, implementation/local QA.
Overall IMPLEMENTED_UNVERIFIED. User authorized commit/push and later accepted the
house restoration. This checkpoint preserves that correction plus lamp/world-drag
implementation with explicit live limitations; it is not a milestone PASS.
Starting HEAD54f6abc6b323fee014c79d46b929b621a19ae7c5; ending commit is the commit
containing this entry. Branch milestone/unreal-world-presentation-polish-001.
Remote milestone and main verified unchanged before checkpoint; normal push only.

Build PASS182.70s; three presentation/WideWorld/V08 automation suites PASS; native
mouse gesture tests PASS; prior nine ClientCore suites retained; secret_check PASS.
Fresh login live-20261014T214746 PID23020 reached32095,32205,7. Automatically saved
MinimapSettled PNG/JSON shows complete outdoor presentation and upright lamps.
Received lit type2109 at32094,32206,7 renders meshID_02108, bounds36x36x170;
filtered geometry retained. This proves the received lit-state geometry alias only.
No agent Use/drag command was sent. Windows activation timed out twice, including
fresh returned-window recovery (exact error: computer-use request timed out:
activate_window). Per computer-use recovery rules, stopped issuing app input.
The client remains open, not normally closed; no EndPlay result claimed for this
session. Snapshot37 commands has residual bytes0, unsupported0, anomalies0 and
zero movement/Use requests. This is login health, NOT a compact gameplay regression.

Evidence in resume folder: lamp-drag-login.png/json, lamp-drag-login-lamps.json,
automation-lamp-drag.json, unreal-build-lamp-drag.txt. Prior roof/restoration/speech,
Follow/Stop/container/minimap evidence retained. No production art/reference edits.
Pending: live lamp ON->OFF->ON with authoritative state and visible light; world
object drag and creature push with authoritative movement; valid Use With source/
target and feedback; current-policy camera collision/walls; fresh Look and complete
compact regression. No final acceptance weakened; readiness for independent
certification NO. Camera keeps manual distance and no proximity wall hiding.
Next: select the open REAL33D window afresh after control recovery, use nearby lamp
at32094,32206,7 (currently lit), F9 before/after. Then legitimate world drag. Use
WorldActor evidence/PlayerController BeginWorldDrag+CompleteWorldDrag and TileActor
ApplyStack; never seed inventory or edit server data to satisfy tests.
## Lamp and world-drag build checkpoint - 2026-10-14 21:48 America/Guatemala

UNREAL-WORLD-PRESENTATION-POLISH-001 remains IMPLEMENTED_UNVERIFIED. Starting
HEAD=54f6abc6b323fee014c79d46b929b621a19ae7c5; remote milestone matches; origin/main
remains fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf (verified this run).
Lamp/world-drag build PASS (182.70s). Fresh CameraAndFloors, AuthoritativeWindow,
and ExperimentalV08 automation suites PASS; secret_check PASS. Evidence:
automation-lamp-drag.json and unreal-build-lamp-drag.txt in the resume evidence folder.
These do not establish live lamp switching or object/creature drag success.
Live verification is starting with the same existing server runtime; no server data,
production mesh, protocol, or ClientCore changes. Accepted house restoration retained.
## Accepted restoration and new operator scope - 2026-10-14 16:36 local

Operator confirmed the roof fix, reported black distant scenery and incomplete
house exteriors, then accepted the updated restoration live ("ahora lo probe y esta
bien"). Prior capture had1662 stale HISM trees; post-fix F9 has3652 current trees.
ApplyView now rebuilds changed component trees once per visibility batch and refreshes
bounds. Selected UE5.8 HISM UpdateInstanceTransform can leave same-position scale
changes in place with an outdated cluster tree. Build31.35s PASS and three suites
PASS, including real HISM zero-scale/restore twice with geometry bounds checked.
Session live-20261014T163152 is closed normally; EndPlay273 commands with residual,
unsupported and anomalies0. It has22 unanswered walks (operator movement included),
so this is NOT a complete gameplay regression PASS. Keep that limitation explicit.
Current picture/evidence: restoration-and-unlit-lamp PNG/JSON; prior exterior and
component diagnostic retained. QA-only F9 now includes static render-component stats.

New explicit operator requests within presentation continuation: reuse street lamp
OFF geometry for ON and toggle its light on server state; connect left-click world
drag to move objects/push creatures. Runtime objects.srv2108 has ChangeTarget2109,
Brightness0;2109 has ChangeTarget2108,Brightness7. Selected moveuse.cc::UseChangeObject
and operate.cc CHANGEUSE dispatch are authority, no version-specific override here.
Mapping2109->2108 and live-only point light have been implemented, not yet live tested.
World drag now captures received source identity on press, revalidates it on release,
uses existing RequestMoveObject count1 and shows Move requested; short click still
walks. Creature type99 derives from selected objects.hh; receiving.cc::CMoveObject
uses existing command for objects/creatures. No speculative movement or new opcode.
No static light claims about unseen live state. No production asset changed.
New lamp/drag build running; tests/live checks pending. No commit/push yet. Overall
IMPLEMENTED_UNVERIFIED; no independent certification or main merge. Remaining original
gaps still include Use With and full compact regression, camera/walls acceptance.
## Live roof regression - 2026-10-14 16:18 America/Guatemala

ROOF_MULTIFLOOR_REGRESSION=PASS (bounded temple case). Fresh rebuilt session
live-20261014T161327, PID16568. Operator handled cave exit/manual movement before
handing control to the agent. Agent then clicked the minimap at 22:16:13.271 UTC:
target32098,32217,7; authoritative arrival22:16:20.152. One west step reproduced
original F9 coordinate32097,32217,7. Roof above empty floor6 is now suppressed,
player and current-floor walls/columns remain visible. F9 at22:17:39.323 shows
requested=actual1413.50, cutaway_meshes0, hide_upper_floors=true. Pitch/yaw differ
slightly from the operator's original capture; position and zoom distance match.
Manual zoom2733.50->1413.50 remained under operator input, without retraction.

Agent minimap exit to32097,32210,7 arrived authoritatively; F9 at22:18:25.569 shows
hide_upper_floors=false, requested=actual1413.50, cutaways0. Upper geometry restored
and player stays visible outdoors. Latest snapshot:724 commands, residual bytes0,
unsupported opcodes0, protocol anomalies0,67 accepted/5 refused/0 unanswered walks,
4 external relocations during operator movement/floor changes. The engine emitted
its previously observed AppTime IsInGameThread Ensure during startup; this is not
a protocol error and no clean-engine/no-Ensure result is claimed.

Evidence: temple-roof-after-wide, temple-roof-after, temple-roof-outside PNG/JSON;
roof-session-events.txt. Existing world/NPC speech, Use/container, Follow/Stop and
minimap passes are retained with their exact prior preconditions. Current-policy
CAMERA_COLLISION, WALL_OCCLUSION, USE_WITH_FEEDBACK, LOOK_SMOKE and full compact
GAMEPLAY_REGRESSION remain IMPLEMENTED_UNVERIFIED. Available inventory remains
three coins; no valid Use With source/target demonstrated or runtime inventory edited.
V08 cave continuity, finishes373/377, missing outfits, lamps/mountains remain art work.
Overall IMPLEMENTED_UNVERIFIED; independent certification readiness NO. No merge.
Client is left open for operator review at32097,32210,7. No further input while the
operator reviews. Original rejected snapshot was not published; latest roof fix is
verified only for the documented case. User clarification of any other F9 defect
is pending. All changes remain preserved in the milestone worktree.
## Roof correction checkpoint - 2026-10-14 afternoon America/Guatemala

This update supersedes the publication and active-process notes below. The operator
rejected the no-cutaway version before commit/push; neither was executed. Local and
remote milestone HEAD remain 54f6abc6b323fee014c79d46b929b621a19ae7c5; main remains
fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. Pending edits/evidence are preserved.
Task: UNREAL-WORLD-PRESENTATION-POLISH-001, runtime presentation only. No automatic
camera approach or proximity wall/object hiding; no production art or protocol edits.

Inspected F9 20261014T090025_554: player 32097,32217,7 is obscured by roof409 at
32097,32217,5; floor6 in this column has no covering content. UpdateFloorVisibility
previously checked only Z-1. It now checks all received overhead tiles in the same
column via HasCoverAbove. This uses live tile content only. It retains the established
floor/roof treatment; camera proximity never hides walls or objects.

Unreal build PASS (73.14s). Three presentation/authoritative-window/V08 automation
suites PASS, including roof-over-empty-storey regression and outdoor/top-floor cases.
Live verification remains pending; overall IMPLEMENTED_UNVERIFIED, readiness NO.
Artifacts: temple-roof-before.png/json, temple-roof-observed-column.json,
automation-multifloor-roof.json and unreal-build-multifloor-roof.txt in
unreal-world-presentation-polish-resume-20261014 (relative to evidence/clientcore).
The prior live PID5600 ended normally; it is not still running. Local services were
restarted through start_wsl.sh after verifying no game services/listeners or internal
lock existed: verified querymanager519/game540/login717; bounded host keeper11124.
Next: live return to temple, capture roof/player before/after crossing doorway,
then remaining genuine Use With, camera/walls and compact regression. Prior named
speech/NPC/minimap/Use/Follow evidence retained; no new PASS from compilation alone.
## Preservation checkpoint - 2026-10-14 02:59:57 America/Guatemala

Operator explicitly requested commit and push of this version despite remaining
QA gaps. This supersedes the earlier publication-only-after-PASS condition.
State remains IMPLEMENTED_UNVERIFIED; READY_FOR_INDEPENDENT_CERTIFICATION=NO.
Final no-cutaway Unreal build PASS (107.10s), three automation suites PASS,
nine existing native ClientCore executable suites PASS. First final automation
attempt failed before tests because sandboxed Zen cache initialization failed;
the fresh escalated retry completed all three tests with zero errors.
Secret check will run on the staged checkpoint before commit/push.
No V08/server/protocol changes, main merge or independent certification.

New final-policy live client PID5600, session live-20261014T025802, was opened
and is still loading/running at checkpoint; no complete final-policy live
acceptance is claimed. Keep it available to the operator, do not launch a duplicate.
Newly closed this run: minimap click attribution/arrival; legitimate NPC speech;
fresh world speech/expiry, Use/container and Follow/Stop retained in artifacts.
Remaining: camera/wall readability under the operator's no-hiding policy,
Use With targeting/cancel/valid authoritative result, fresh Look/floor transition
and completion of the compact regression. Prior 601-command closed session has
residual bytes0, unsupported opcodes0, protocol anomalies0.
Historical wall-cutaway PASS does not certify the newly requested no-hiding mode.
No acceptance criterion was weakened. All earlier evidence retained.

Next task: use existing final-policy client; F9 at near walls/zoom/indoors;
exercise Look, floor transition and genuine Use With from an available item.
Only three coins were last observed in inventory. Do not edit runtime inventory
or invent a successful use. Inspect UpdateCamera, MeasureCameraObstruction,
HUD::BeginUseTarget/HandleSlotPicked and retained new evidence. Current source
and every pending result are preserved by the commit containing this checkpoint.
# Presentation polish resumed after interrupted agent, 2026-10-14

## Subsequent operator correction: no proximity disappearance

At approximately02:50 local, operator explicitly said walls/objects must not
disappear when close. This supersedes the earlier instruction to keep cutaways.
Both live tiles and static sectors now expose const MeasureCameraObstruction:
diagnostics only, no SetVisibility, collision changes or instance scaling.
Removed CutAway policy and its obsolete positive test. Current-floor objects
remain present; original roof/floor visibility and static/live suppression remain.
UpdateCamera keeps manual zoom. The `cutaway_meshes` evidence field remains0
for comparison with older captures. `obstructed` now reports all intersecting
visible bounds, including foreground walls formerly hidden; it does not imply
that the camera actually moved or that the view is unplayable.

CAMERA_COLLISION and WALL_OCCLUSION are IMPLEMENTED_UNVERIFIED on this changed
policy until live review. Earlier cutaway PASS remains historical evidence only.
No art was changed. Final no-cutaway build/tests/live observations follow below.

Session1 normal EndPlay08:43:46.942:601 commands,76 accepted/4 refused/0 unanswered
walks,4 external relocations while Follow operated,4 received/resolved speeches,
no residual bytes/unsupported opcodes/anomalies, no duplicate/orphan actors.
Follow screenshot08:42:35.110 and final target0/followingfalse plus server clear
corroborate the fresh Follow/Stop controls. Bookcase2437 opened from normal Use
and six empty slots displayed. Speech expiry screenshot08:40:47.096 retains
chat history after world text disappears. These compact artifacts are preserved.

Second normal session live-20261014T024803 closed normally for rebuilding;
no final-policy acceptance is credited to it. Existing runtime was reused.
Git-for-Windows tests/secret_check.sh PASS after source change; current tracked
tree/history scan, not a staged publication check for still-untracked artifacts.

Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IMPLEMENTED_UNVERIFIED.
Agent: Codex, implementation and local QA; no independent certification.
Worktree: build/unreal-world-presentation-polish-001.
Starting HEAD and verified remote: 54f6abc6b323fee014c79d46b929b621a19ae7c5.
Base/main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.

The first audit stopped on the stale requested HEAD/clean-state mismatch. The
operator then explained the agent had been closed accidentally and authorized
resuming the actual preserved state. Existing uncommitted operator QA documents,
screenshots and handoff were preserved. Historical CONTINUATION-20261009 is unchanged.
The prior handoff is archived in
handoffs/archive/2026-10-14_UNREAL-WORLD-PRESENTATION-POLISH-001-before-camera-resume.md.

## Authorized camera change

Operator explicitly confirmed removal of automatic camera approach, retaining
manual zoom and foreground wall cutaway. AReal33DWorld::UpdateCamera now assigns
the smoothed manual distance directly instead of clamping it to SafeDistance.
The obstruction detector still reports potential uncut geometry; it no longer
moves the camera. Floor visibility, cutaway policy, server and V08 assets unchanged.
This does not itself prove collision readability acceptance.

Build PASS: REAL33DEditor Win64 Development, Result: Succeeded, 32.29 seconds.
Automation PASS: CameraAndFloors, AuthoritativeWindow, ExperimentalV08; all three
Success, zero errors. Existing LimitCamera helper test covers the diagnostic
intersection calculation, not automatic retraction (now deliberately disabled).
Compact build and automation results: unreal-world-presentation-polish-resume-20261014/.

## Live preconditions and new results

Reused verified existing Fusion32 querymanager 618/game 633/login 846, with
exe/cwd/start/port ownership and readiness checks. No runtime or account reset.
No other Unreal process was running. Normal QA launcher Account B opened PID14880,
session build/unreal-world-presentation-polish-001/live-20261014T022908.
Windows computer-use initialized successfully. Operator also moved/greeted during
this session; do not attribute every command to agent input.

- MINIMAP_COMPATIBILITY / MINIMAP_CLICK_SMOKE PASS: agent clicked the actual
  minimap at 08:32:05.608 UTC; log `minimap destination: 32094,32204,7`, followed
  by four authoritative east steps from 32090,32204,7 and `left-click walk arrived`
  at 08:32:07.465. F9 08:32:47.330 PNG/JSON shows the same player and minimap
  coordinate, synchronized viewport, 252 live cells, 4438 remembered cells.
  This is bounded presentation regression, not minimap recertification.
- NPC_SPEECH PASS: normal local `hi`, NPC Seymour naturally reached inside the
  school. Received 08:36:14.167 `Hello, Test Player B. What do you need?`, resolved
  speaker1073742035. Automatic 08:36:14.137 PNG visually inspected: actual reply
  above Seymour, player hi above player; JSON transcript retains both. No fake
  message, server edit or scripted response. NPC later says its normal goodbye.
- WORLD_SPEECH retained PASS and fresh hi displayed; timeout/chat persistence
  follow-up capture still to be retained.
- At the school, requested and actual manual camera distances both1533.50;
  upper floors hidden, both player and NPC readable. This is not yet the required
  genuinely obstructive camera case or a completed camera acceptance suite.
- Latest inspected 08:36:14 snapshot: residual bytes0, unsupported opcodes0,
  protocol anomalies0. Final session health still pending.

## Remaining work

Camera obstructive case/zoom/indoor/floor-change coverage; Use With source,
targeting/cancel and legitimate target/result; compact remaining live regression.
Only inventory object at 08:32:47 was three coins3031. No supported Use With
success can be claimed from this inventory. Existing Use/container/Follow/Stop
PASS evidence from OPERATOR-QA-20261014 remains preserved.

V08_ART_ISSUES retained: cave continuity, walls373/377, outfits, lamps and
mountains. No art patch, no certification, no main merge, no PASS LOCAL claim.
Final commit/push remains conditional on the original acceptance criteria.
