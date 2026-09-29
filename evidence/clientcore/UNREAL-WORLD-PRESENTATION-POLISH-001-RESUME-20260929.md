# Presentation polish recovery and preservation

State: IMPLEMENTED_UNVERIFIED. Operator requested commit/push before live QA.
Date: 2026-09-29 America/Guatemala. This is not PASS LOCAL or certification.

## Recovered state

Local/remote polish HEAD: 393e7e8570319a35caef7072065d29cf67448475.
Merge-base and verified remote main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Exactly one polish commit followed main. No reset, new branch, merge or rebase.
Worktree: build/unreal-world-presentation-polish-001. Frozen agent checkout untouched.

393e7e8 already contained the 15-area audit, camera/floor/cutaway implementation,
projected labels/health/target/follow/speech, hover/request feedback, QA launchers,
tests and evidence of a displaced-label defect. Three uncommitted files were
present on recovery: WorldOverlay.cpp, WorldActor.cpp and the QA launcher.
Those edits supply the projection-space fix and opt-in live geometry diagnostics.
They are preserved in this checkpoint, not attributed as new implementation.

## Changes and evidence recovered in this run

- Overlay projection now uses viewport paint geometry together with OnPaint
  geometry. Cursor containment uses tick/desktop geometry. This avoids adding
  the window's desktop offset to the painted labels, speech and outlines.
- QA launcher enables the existing presentation geometry inventory. Inventory
  reads only current live tile render components; this run removes its same-floor
  restriction so upper-wall defects can be attributed from known client objects.
  No unseen map/server query or authority change is introduced.
- Automation launcher accepts EvidenceDirectory and defaults to a new timestamped
  directory, preserving earlier reports when rerun.
- Recovered live session 20260929T000214 was already on disk before this run.
  Its manual screenshot at 06:03:16.493Z visibly aligns Test Player B above the
  body and shows a readable Seymour label and hover outline. This is limited
  screenshot evidence, not proof across DPI, window moves or other camera angles.
  It is compared with the retained after-projection-defect.png at the prior
  checkpoint; positions and camera settings differ, so not a controlled visual A/B.
- Recovered EndPlay: 315 accepted / 17 refused / 52 unanswered walks and 54
  external relocations; 16 Looks, 3 Uses, no attack/follow/Say. Snapshot has
  unsupported_opcodes=0, protocol_anomalies=0, residual_bytes=0, synchronized
  viewport/minimap and no duplicate/orphan actors. Counters are not success proof
  for Use, nor does movement establish visual acceptance. Look replies are retained.
- Existing movement journal records 7->8->7 and 7<->6 relocations. No paired
  floor screenshots prove readability. All original raw evidence stays in place.

## Checks executed now

CLIENTCORE_TESTS = PASS: tests/build_clientcore_windows.cmd, native MSVC C++17
and C++20 builds and all nine suites; full compact result included.
UNREAL_BUILD = PASS: UE5.8 Build.bat REAL33DEditor Win64 Development,
-Project=<worktree>/unreal/REAL33D/REAL33D.uproject -WaitMutex -NoUBA;
successful retry took 17.47 seconds. First build was interrupted with process
exit -1073741510 and no compiler result; its raw log is preserved separately.
SECRET_CHECK = PASS: Git Bash tests/secret_check.sh; rerun on staged checkpoint.
git diff --check = PASS.

Fresh Unreal automation = IMPLEMENTED_UNVERIFIED: direct PowerShell execution
was blocked by execution policy; process-local Bypass retry started engine
initialization but was interrupted. No completed test report exists. Prior
CameraAndFloors/AuthoritativeWindow PASS remains historical evidence only.
No new gameplay session was launched. Operator agreed to assist with native
gameplay inputs, then requested preservation before that QA started.

## Classification and remaining acceptance

BUG: displaced projection, fixed in source with one recovered aligned capture;
window movement/DPI and speech/target projection still need live proof.
BUG (suspected, attribution pending): reported wall rotations; inspect live
TypeIds, bounds and existing AssetRegistry::TryResolveExperimental transforms
before any correction. Do not rotate V08 models wholesale.
POLISH: camera/follow/zoom/clearance, roof/wall cutaway and entity/interaction
feedback remain IMPLEMENTED_UNVERIFIED against the full live acceptance matrix.
V08_ART_DEFECT: known incomplete outfit coverage; catalog lamp2108/2109 assembly/
UV warnings remain art-dependent. No V08 source/import assets were changed.
OUT_OF_SCOPE: protocol/opcode50, gameplay/server/DB, WideWorld architecture,
expanded viewport, REAL33D2D, agent experiment and live visual editor.

All required live camera/floor/roof/wall/entity/target/follow/interaction/speech/
WideWorld/minimap/gameplay-regression categories remain IMPLEMENTED_UNVERIFIED.
Do not infer PASS from compilation or earlier certified systems. Protocol errors
are zero in the recovered session only; no fresh-session claim is made.

Compact artifacts: evidence/clientcore/unreal-world-presentation-polish-resume-20260929/.
Raw new checks: build/unreal-world-presentation-polish-001/resume-20260929/.
Raw recovered live session: build/unreal-world-presentation-polish-001/live-20260929T000214/.
No secrets, geometry/map bulk dumps or generated caches are published.

Next: complete this milestone, beginning with projection under window movement,
camera/zoom and known wall identification. Then indoor/roof/floor/underground,
normal interactions, attack/follow/stop, Say/NPC speech, minimap routes and sector
crossings with F9 before/after captures. Recommend independent certification of
UNREAL-WORLD-PRESENTATION-POLISH-001 only after full local PASS; no new feature
milestone is recommended before these gaps are closed. No merge to main.
