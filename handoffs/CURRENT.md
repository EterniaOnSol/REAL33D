# CURRENT - Presentation polish preservation commit, live clearance check pending

Date: 2026-10-09 America/Guatemala. Agent: Codex, implementation/local QA.
Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IMPLEMENTED_UNVERIFIED.
Branch: milestone/unreal-world-presentation-polish-001.
Starting HEAD and last verified remote: 4305d23738454c2530007b16e759cbcaeddbca55.
Ending commit: the preservation commit containing this handoff (git rev-parse HEAD).
Base/main/origin/main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Operator explicitly requested immediate commit; no main merge or PASS claim.

## Recovery and changes

Safe ancestry verified: certified main -> 393e7e8 -> 4305d23. Both live remote
heads matched local refs. Main clean; polish had retained source/tests/docs and
untracked evidence from September29/October7. All preserved and audited first.
393e7e8 already implemented camera, floor/cutaway, labels, speech/target/follow,
hover/request overlays and automation. 4305d23 added paint-space alignment and
live diagnostics/evidence. No branch reset, merge, rebase or frozen checkout edit.

Recovered source fixes now included: flat stack separation, wall-alias yaw,
transient HISM material adapter, dirt-border XY restoration; launcher generates
inventory images and requires existing brother-creature catalog; test runner
requires fresh named successful results. Production V08 packages untouched.

This run added a2uu clearance to already-normalized wall height (218 vs220).
Lower-floor dirt wall caps were coplanar with water one floor above. New actual-
mesh tests cover wall357/358/363/367/379/1294 vs water622. Both live and static
consumers use the registry. Nanite-disabled gameplay still showed fragments;
no Nanite override is retained in runtime/config. Speech now triggers timestamped
opt-in QA evidence when genuinely received/resolved; chat/duration unchanged.
No ClientCore, protocol, opcode50, viewport, server rule/map, 2D or agent edits.

## Verification

PASS: native ClientCore nine suites, C++17/C++20 libraries; Unreal build before
and after correction (final exit0); final CameraAndFloors, AuthoritativeWindow,
ExperimentalV08 automation all Success/errors0, expected catalog warning only.
Final automation: recovery-20261009/automation-wall-clearance/automation-report/.
Initial tracked-tree/history secret_check PASS. Final staged scan result is
recorded below once completed. No live validation of218-wall or auto-speech delta.

Fresh normal operator session live-20261009T060555 closed normally:
1741 commands, protocol errors0, duplicate/orphan0, synchronized viewport;
115 requested walks,104 accepted,12 rejected,0 unanswered,7 external relocations.
Counts are aggregate categories, not one-to-one accepted/refused totals.
Floor8->7, covered building/outdoor states, camera retraction, player/rat/Tom,
hover, real Use/attack/refusals, Say transcript and sector loads56->89/unloads27
retained. Speech captures missed its lifetime. Full Use With, Stop Follow and
map-click arrival checks not completed. Prior unanswered-walk sessions remain
unresolved; no blanket gameplay-regression PASS.

Comparison live-nonanite-20261009T061219 confirmed r.Nanite0 and still flickered.
Character died in ordinary rat encounter; final missing local actor/zero position
is death state, not a map teleport. No account/gameplay manipulation. Both fresh
sessions have residual0/unsupported0/anomalies0. Clients are closed normally.

PASS retained for exercised camera readability/follow/zoom, roof handling,
player/creature/NPC readability, target/follow/hover/Look/chat. WideWorld desktop
loading/sector/material coexistence PASS; cooked material support unverified.
FLOOR_READABILITY remains FAILED for last live candidate; proposed correction is
IMPLEMENTED_UNVERIFIED live. Camera collision return, wall occlusion, speech,
Use/Use With complete feedback, minimap-click coexistence and full gameplay
regression remain IMPLEMENTED_UNVERIFIED. No independent certification.

V08_ART_DEFECT: missing outfits; lamp2108/2109 assembly/UV and mountain1112/1113
source-shape limitations retained separately. Art completeness is not runtime PASS.

## Evidence and runtime

Report: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RECOVERY-20261009.md.
Compact artifacts: evidence/clientcore/unreal-world-presentation-polish-recovery-20261009/.
Raw: build/unreal-world-presentation-polish-001/recovery-20261009/ and live dirs above.
All original reports/captures retained. Prior handoff archived verbatim as
archive/2026-10-09_UNREAL-WORLD-PRESENTATION-POLISH-001-before-recovery.md.
No raw geometry/map dump, generated cache, credential or production art committed.

Fusion32 binaries matched prepare_wsl.sh hashes. Stale internal PID873 absent
from /proc; lock preserved as game.pid.stale-polish-20261009. Prior service
logs/PID/start records preserved. Existing startup verified querymanager830,
game845,login1067 identities/cwd/start/ports/readiness. Hidden WSL host PID13268
has two-hour lifetime; verify current status before reuse. No server reset.

## Exact next step

Continue this same milestone, not the live visual editor. Check running services,
then launch scripts/client/run_unreal_presentation_qa.ps1 -Account B normally
(no Nanite override). Follow normal server death/respawn behavior. Revisit the
underground water scene and compare218-wall rendering; capture camera rotation,
clearance/return, speech (automatic capture), actual Use With, Follow/Stop and
minimap-click arrival/sector coexistence. Do not manufacture gameplay fixtures.

Read-only commands: git status; git rev-parse HEAD; git ls-remote origin
refs/heads/main refs/heads/milestone/unreal-world-presentation-polish-001.
Server status: wsl -d Ubuntu-26.04 -u root -- bash
/mnt/c/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001/scripts/server/status_wsl.sh
/mnt/c/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Do not duplicate running services or overwrite evidence directories.

Only after all local acceptance passes: push milestone, verify clean local==remote,
then UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-CERTIFICATION. No main merge.

Final preservation checks: staged secret_check PASS; staged private-key scan no matches;
final three-case automation PASS (errors0); git diff --cached --check PASS.
No push in this preservation run; remote remains4305d23. Main remains clean.
