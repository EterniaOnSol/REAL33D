# CURRENT - Presentation polish preservation checkpoint

Date/time: 2026-09-29 16:58 America/Guatemala. Agent: Codex, recovery/QA.
Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IMPLEMENTED_UNVERIFIED.
Branch: milestone/unreal-world-presentation-polish-001.
Starting HEAD: 393e7e8570319a35caef7072065d29cf67448475.
Base/main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf, remote and ancestry verified.
Ending commit: this preservation commit; resolve with git rev-parse HEAD.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
No reset/rebase/new branch/main merge. Frozen agent checkout untouched.

Operator requested immediate commit/push due to limited remaining tokens.
Preserved all recovered source/config/evidence changes; no new runtime fix in
this continuation. Trimmed one extra EOF blank line from pre-existing generated
DefaultInput.ini settings after saving a raw byte-for-byte copy. Prior CURRENT
is already archived identically in archive/2026-09-29_UNREAL-WORLD-PRESENTATION-POLISH-001-before-resume.md.

Inspected startup contract/docs, Git/worktrees/remote/ancestry, existing milestone
and resume evidence, overlay geometry, camera/floor/creature/speech/interaction
code, AssetRegistry transforms and current-live-component diagnostics.
Fresh native ClientCore nine suites PASS; Unreal build PASS; CameraAndFloors and
AuthoritativeWindow automation Success. Wrapping automation shell exited with
-1073741510 after completed reports; don't erase that caveat. Secret check PASS;
rerun staged before publication. Compilation is not gameplay parity.

Fresh live normal Account B session live-20260929T165406 proved bounded camera
readability/follow/zoom (operator confirmation), aligned player/NPC labels,
rat attack/follow labels and outlines agreeing with combat state, hover, server
Look/chat readability, roof entry/exit and floor7->8->7. Full gameplay regression,
Use With, Stop Follow, world/NPC speech and map-click/sector coexistence remain
IMPLEMENTED_UNVERIFIED. Underground floor readability FAILED: severe water-edge
overlap. Perpendicular wall fins also remain unfixed. Not PASS LOCAL.
Last captured position32097,32207,7 at22:57:30.473Z; commands888, protocol errors0,
73 accepted walks,18 rejected,0 unanswered,17 external relocations. No EndPlay
artifact; Unreal process absent when checked. Do not claim clean shutdown.

Evidence: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-CONTINUATION-20260929.md
and unreal-world-presentation-polish-continuation-20260929/. Earlier checkpoint
and RESUME reports/artifacts preserved. Raw session/geometry and commands remain
under build/unreal-world-presentation-polish-001/. No bulk map dumps published.

Runtime recovery: certified service executable hashes verified. Stale internal
PID locks preserved under dated names only after verifying PID absence. Server
source/config/gameplay/map unchanged. Persistent hidden WSL host Windows PID13112
runs sleep3600; last verified services querymanager813/game830/login999. Recheck
before reuse. Raw continuation-20260929T1645/ contains guarded recovery scripts,
logs and input/binary hashes. Do not recreate/reset the server or accounts.

Exact next steps/files:
1. TileActor::ApplyStack and StaticSectorActor::Build: ground622 plus flat border
4785/4789-4796 occupy same height; Index1 currently has zero layer lift. Fix runtime
layer separation consistently, with targeted test and paired live before/after.
2. AssetRegistry::TryResolveExperimental: aliases1295/1301/1303 reuse1294 at yaw90;
known live X-row at y32200,z6 has boundsX29/Y100/Z220. Confirm classic orientation
from local previews, then correct only proven runtime transforms; no V08 redesign.
3. Continue outstanding live matrix, including speech during its visible lifetime,
valid Use With, Stop Follow, map-click and sector transitions. Do not manufacture
scenarios or claim PASS from counters alone. Missing outfit art and lamp warnings
stay separate V08_ART_DEFECT findings.
4. Rebuild using UE5.8 Build.bat REAL33DEditor Win64 Development -WaitMutex -NoUBA;
run tests/build_clientcore_windows.cmd under VS2022 vcvars64 and
powershell -ExecutionPolicy Bypass -File scripts/client/test_unreal_presentation.ps1.
Launch normal gameplay via scripts/client/run_unreal_presentation_qa.ps1 -Account B.
F9 records screenshot/state/current-live geometry. Preserve earlier artifacts.

Publication: push only this existing milestone, verify local==remote and clean
worktree. No main merge. Full local PASS must precede independent certification.
