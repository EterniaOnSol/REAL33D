# Presentation polish continuation, 2026-09-29

Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IN_PROGRESS.
Agent: Codex, recovery audit and local QA. No independent certification.

## Recovery boundary

Starting/local/remote polish HEAD: 393e7e8570319a35caef7072065d29cf67448475.
Branch: milestone/unreal-world-presentation-polish-001.
Verified remote main and merge-base: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
`git merge-base --is-ancestor` succeeded. Exactly one polish commit is outside
main. No new branch, merge, reset, rebase, cherry-pick or main modification.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Main worktree was clean. Frozen agent checkout remained untouched.

Recovery found seven modified files: PROJECT_STATUS.md, PARITY_MATRIX.md,
run_unreal_presentation_qa.ps1, test_unreal_presentation.ps1, DefaultInput.ini,
Real33DWorldActor.cpp and Real33DWorldOverlay.cpp. The resume report, its six
artifacts and archived handoff were also already untracked. These are preserved
pre-existing work, not implementation authored by this continuation. In
particular the paint-space fix, all-live-floor geometry diagnostics and unique
automation output directory support already existed on entry.

393e7e8 already contained the camera/floor/cutaway implementation, projected
entity/health/target/follow/speech overlay, interaction request/hover feedback,
15-area audit, launchers and original tests/evidence, including a live projection
defect. The recovered screenshot independently inspected in this continuation
shows aligned player and Seymour labels. It is historical, not a fresh test.

## Fresh checks

- ClientCore: PASS. `tests/build_clientcore_windows.cmd` with VS2022 vcvars64;
  C++17/C++20 library builds and all nine native suites passed.
- Unreal build: PASS. UE5.8 Build.bat REAL33DEditor Win64 Development,
  -Project=<worktree>/unreal/REAL33D/REAL33D.uproject -WaitMutex -NoUBA.
  Result Succeeded, 4.27 seconds.
- Unreal automation: PASS for the two named test cases only. Both
  REAL33D.Presentation.CameraAndFloors and
  REAL33D.WideWorld.AuthoritativeWindow produced Success, failed=0 in completed
  reports at checks-20260929T164317 and checks-20260929T164349. Engine log recorded
  Test Queue Empty, two tests performed and requested exit status 0. The wrapping
  PowerShell process returned -1073741510 during exit; this is recorded rather
  than treating its exit code as successful. The first attempt was retried before
  inspection established that its test report had already completed.
- Full `git diff --check`: FAILED on a pre-existing extra blank line at
  DefaultInput.ini:99. Its 87 added engine settings were already present on
  recovery. They are not silently removed or attributed to this run.

Compact fresh outputs: unreal-world-presentation-polish-continuation-20260929/.
Raw checks/recovery scripts: build/unreal-world-presentation-polish-001/
continuation-20260929T1645/. Existing evidence remains in place.

## Live prerequisites and runtime recovery

Native desktop control is unavailable in this session. Operator agreed to supply
ordinary gameplay inputs while Codex inspects F9 screenshots/state journals.
First normal Account B launch: live-20260929T164538, PID 5928. Login failed because
the local login service was unreachable. No gameplay acceptance from that launch.
The operator closed the offline window.

All three baseline services were stopped. Their installed executable SHA-256
values matched scripts/server/prepare_wsl.sh's certified source-built values.
These were not archived legacy executables. Startup then encountered a stale
internal lock. reference/game/src/main.cc::LockGame rejects any nonzero PID in
SAVEPATH/game.pid without testing whether that process exists; no version guard
surrounds this helper. PID 3845 was absent. Its lock was renamed in the same
runtime directory to game.pid.stale-polish-20260929T1650, preserving its contents.
No server source, config, map, account or gameplay rule was edited.

Several WSL commands returned -1073741510 and started services subsequently
disappeared. A bounded hidden `wsl ... sleep 3600` process (Windows PID 13112)
was started to retain the QA host while investigating. The next guarded startup
uses exact executable paths to exclude WSL's unrelated system process named
login, and preserves a further stale lock only after confirming PID absence.
Runtime recovery outcomes and live acceptance follow below when established.

## Audit limits

The existing overlay uses matching viewport/OnPaint paint geometry for projection
and tick-space geometry for the desktop cursor. UE5.8 SWidget.h exposes both;
the historical aligned capture supports the narrow fix, not DPI/window-movement
acceptance. Creature selection reads current semantic combat state. Speech uses
the existing resolved CreatureId; unresolved speech remains in the transcript.

Wall fins/misorientation remain BUG (suspected, attribution pending), not an
excuse to rotate or replace V08 meshes. Existing registry rotations and aliases
remain unchanged. The opt-in geometry inventory contains current client render
components only and stays in ignored raw evidence. Missing outfit coverage and
lamp2108/2109 assembly/UV issues remain V08_ART_DEFECT as previously documented.

No new protocol, authority, viewport, opcode50, map data, V08 source art,
REAL33D2D, agent work or live visual editor changes.

## Operator-directed preservation closeout, 16:58 America/Guatemala

Final milestone state: IMPLEMENTED_UNVERIFIED, not PASS LOCAL. Operator stopped
further work to request commit/push because of limited remaining tokens.
No new runtime rendering implementation was authored in this continuation:
recovered source edits were audited, rebuilt and exercised. Existing generated
DefaultInput settings are preserved; only its extra trailing blank line was
removed, with a byte-for-byte raw backup kept locally.

SECRET_CHECK = PASS; completed detached Git Bash retry. Final staged rerun is
required before commit/push. Earlier partial scan was interrupted and not credited.
The persistent startup succeeded: querymanager813/game830/login999 each verified
by executable/cwd/start-time/port. No server logic or saved gameplay edits.

Fresh gameplay session: live-20260929T165406, Unreal PID12184. Last retained
manual checkpoint 22:57:30.473Z: (32097,32207,7), 73 accepted walks, 18 rejected,
0 unanswered, 17 external relocations; 888 decoded commands, unsupported0,
anomalies0, residual0, viewport synchronized. No EndPlay artifact exists and
no Unreal process was found at closeout; do not claim a clean session shutdown.

Named checks, with precondition normal Account B input against recovered Fusion32:
- CAMERA_READABILITY / CAMERA_FOLLOW / CAMERA_ZOOM: PASS for the exercised session.
  Close/distant captures 22:54:54.242/22:54:59.177 show aligned labels; operator
  explicitly confirmed attachment during window movement and smooth/readable
  follow/zoom. No claim for all DPI configurations.
- PLAYER_READABILITY / CREATURE_READABILITY / NPC_READABILITY: PASS for the
  visible player placeholder, rat and Tom labels/bodies in the selected captures;
  this does not certify missing outfit art or every occlusion case.
- TARGET_FEEDBACK / COMBAT_TARGET_FEEDBACK / FOLLOW_FEEDBACK: PASS for selected
  rat label/outline agreeing with actual combat state in 22:56:49.991 and
  22:56:56.745. Stop Follow is not independently captured (cancel counter0).
- HOVER_FEEDBACK: PASS for ordinary tile/creature outlines/hints in the captures.
- LOOK_FEEDBACK: PASS for observed server sign description in indoor screenshot.
  USE_FEEDBACK / USE_WITH_FEEDBACK / INTERACTION_FEEDBACK: IMPLEMENTED_UNVERIFIED;
  Use requests and floor transitions occurred but no complete feedback/valid
  Use With proof was retained.
- ROOF_HANDLING: PASS for building entry 22:56:06.548 (upper floors hidden), exit
  22:56:09.402 (restored), underground 22:56:37.342 and surface return22:57:30.473.
- FLOOR_READABILITY: FAILED in the underground water-edge case; severe visible
  coplanar overlap. WALL_OCCLUSION / CAMERA_COLLISION: IMPLEMENTED_UNVERIFIED;
  geometry tests and cutaway counters alone do not close the full live checks.
- WORLD_SPEECH / NPC_SPEECH: IMPLEMENTED_UNVERIFIED. One Say was received but
  no screenshot during its world-text lifetime; no proven NPC speech capture.
  CHAT_COMPATIBILITY: PASS for retained player Say and server Look/refusal text.
- WIDEWORLD_COMPATIBILITY / MINIMAP_COMPATIBILITY / GAMEPLAY_REGRESSION:
  IMPLEMENTED_UNVERIFIED overall. Loading, movement, roof changes and floor7->8->7
  map synchronization observed; no full sector/map-click/remaining regression run.

BUG findings, fixes still pending:
1. Underground water ground622 and overlay borders4785/4789-4796 have identical
   4.50-high bounds and coincident projected centers on the same tile. Inspect
   TileActor::ApplyStack: first walkable nonground Index1 currently receives zero
   stack lift. Check the matching StaticSector::Build offset and establish a
   presentation-only layer separation with tests and before/after replay.
2. Visible stone-wall aliases1295/1301/1303 reuse1294 at yaw90. At known live
   floor6 y32200, x32079..32086, these form an X row but each has bounds
   X29/Y100/Z220: perpendicular fins. Inspect AssetRegistry::TryResolveExperimental
   and local classic previews before changing a per-TypeId orientation. No wall
   rotation or V08 mesh replacement was performed in this run.
V08_ART_DEFECT: missing outfit coverage and existing lamp2108/2109 assembly/UV
warnings remain separately tracked; provisional mountain reliefs1112/1113 and
other source-shape limitations are not runtime fixes. Art completeness is not
an acceptance requirement. All original evidence and raw geometry are preserved;
no map/geometry bulk dump or secret is added to Git.

Next task remains this same milestone: fix/prove the two observed rendering bugs,
finish outstanding live matrix, then independent certification after full local
PASS. No feature milestone or merge to main is recommended before that.

Final staged secret_check = PASS. Final diff --cached --check = PASS after trimming only trailing whitespace in the new compact build-log copy; raw build log preserved. Publication is the operator-requested preservation checkpoint, not milestone completion.
