# CURRENT - UNREAL-WORLD-PRESENTATION-POLISH-001

Date: 2026-10-09, America/Guatemala. Agent: Codex, implementation/local QA.
State: IMPLEMENTED_UNVERIFIED. Independent certification has not run.
Branch: milestone/unreal-world-presentation-polish-001.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Certified main/base: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Preservation commit: ce2e02db4c7c1cc4010db59169d9357532640301.
Continuation HEAD: the commit containing this handoff; use git rev-parse HEAD.
Remote polish: 4305d23738454c2530007b16e759cbcaeddbca55 (verified this evening).
No push or main merge. User's commit instruction authorizes local checkpoints.

## Recovery and implementation

Verified certified main -> 393e7e8 -> 4305d23 ancestry before editing. Preserved
all existing dirty work and evidence; never reset/rebased/merged the branch.
393e7e8 already supplied camera/floor/cutaway policies, entity labels/health,
speech/target/follow and interaction overlays. 4305d23 improved label paint-space
alignment and added diagnostics. Frozen agent checkout remains untouched.

ce2e02d preserved recovered terrain separation, wall-alias yaw, transient HISM
material support, dirt-border XY placement, inventory-generation launcher and
strict automation runner. This run added 2uu ceiling clearance to normalized
walls and opt-in evidence capture when actual local speech is received/resolved.
The water fix is now proven live and accepted by the operator (see below).

The operator rejected the raised cave walls. Actual source bounds and catalog
metadata show 20 provisional sprite reliefs only 4.5uu high were stretched to
218uu. TryResolveExperimental now preserves native proportions for provisional
reliefs <=10uu. Volumetric walls retain the 218uu presentation height. This shared
registry applies to live tiles and WideWorld. Actual-mesh regression tests check
seven relief variants and a volumetric wall, retaining water-clearance checks.
No V08 package, map, ClientCore, protocol, opcode 50, viewport, gameplay rule,
REAL33D2D or frozen agent work was changed. Cave volumetric art remains incomplete.

## Executed verification

PASS: fresh native ClientCore nine suites and C++17/C++20 builds; final Unreal
build; CameraAndFloors, AuthoritativeWindow and ExperimentalV08 automation, all
Success with zero errors; staged secret_check. See compact artifacts below.
Build/automation alone are not visual acceptance.

Water live PASS: live-20261009T140845, manual frame 20:11:28.952 UTC at floor 8.
Former triangular lower-wall fragments are absent; banks/water continuous after
camera orbit. Operator explicitly says water looks good. No Nanite override.
EndPlay: 36 accepted walks, seven refused, zero unanswered; protocol errors zero.

Relief candidate session live-20261009T142059 ended normally at 20:28:25 UTC.
881 commands; 16 accepted walks, one rejected, zero unanswered; eight Uses,
one attack, one item move; WideWorld loads/unloads 69/7. Viewport synchronized;
minimap matches local position/floor; duplicate/orphan and protocol errors zero.
No manual or speech screenshot was captured. Relief appearance is UNVERIFIED.
The operator confirmed this evening that they have not tried the change yet.

PASS retained for exercised camera readability/follow/zoom, roof handling,
player/creature/NPC readability, target/follow/combat-target/hover/Look/chat,
and desktop WideWorld loading/sector/material coexistence. These are bounded
cases documented in retained September29 and October9 reports. Full floor
readability (current relief candidate), camera collision return, wall occlusion,
visible Say/NPC speech, complete Use/Use With, fresh Follow/Stop, minimap-click
arrival and full gameplay regression remain IMPLEMENTED_UNVERIFIED.
Earlier unanswered-walk sessions are not silently cleared by newer zero counts.

V08 art issues: missing outfit coverage, lamp 2108/2109 assembly/UV, mountain
1112/1113 provisional shapes, and 20 thin cave reliefs lacking volumetric wall
art. Art completeness is not a runtime PASS gate. Cooked material support is
unverified; current acceptance is desktop. No replacement assets were introduced.

## Runtime and evidence

Active normal QA: live-20261009T191234, PID 1228; launched with the final relief
build, default renderer, -game and presentation/minimap QA. Not the live visual
editor. User was asked to visit the cave, orbit, press F9, then Say hola.
Native UI input is unavailable to this agent; use ordinary operator gameplay.
Do not fabricate fixtures or use hidden map/server state to fill the matrix.

Before launch, existing WSL querymanager 830/game 845/login 1067 passed
identity/cwd/start/port checks. Do not start duplicates. No server/account reset.
Earlier recovery preserved an independently verified stale PID lock and reused
the existing certified startup. Four frozen evidence hashes remained unchanged.

Current report: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-CONTINUATION-20261009.md.
Recovery report: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RECOVERY-20261009.md.
Compact artifacts: evidence/clientcore/unreal-world-presentation-polish-continuation-20261009/
and unreal-world-presentation-polish-recovery-20261009/.
Raw: build/unreal-world-presentation-polish-001/recovery-20261009/ and live-*.
Prior handoff preserved at archive/2026-10-09_UNREAL-WORLD-PRESENTATION-POLISH-001-before-relief.md.
No bulk geometry/map dump, generated cache or credentials committed.

## Exact next work

Continue this milestone. Inspect the new cave F9 and real received Speech frames;
obtain operator visual review. Exercise camera clearance/return, wall occlusion,
Use/Use With, Follow/Stop and minimap-click arrival with sector travel when
naturally available. Do not redo already proven fixes or launch visual editing.
Check running client/services before reopening. Normal launcher:
scripts/client/run_unreal_presentation_qa.ps1 -Account B.
Tests: scripts/client/test_unreal_presentation.ps1 with the existing experimental
catalog and a NEW evidence directory. Avoid overwriting previous evidence.

Only after all minimum criteria pass: update evidence, secret_check, commit/push
the milestone, verify clean local==remote; mark PASS LOCAL pending certification.
Never merge main here. Exact next milestone after PASS:
UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-CERTIFICATION.
