# HANDOFF - UNREAL-LOGIN-CHARACTER-SELECT-UI-001 recertification

Date/time: 2026-10-26T21:32:43.808361+00:00. Agent: Codex.
Role: independent credential/session recertification, operator-assisted death.
Branch: `certification/login-ui-recert-20261026`.
Starting commit/tested production head: `444a89e268289cad31fd64260b04762bef73dfdc`.
Ending commit: the evidence-only commit containing this handoff (resolve with
`git log -1 --format=%H -- handoffs/CURRENT.md`).
Worktree: `C:/Users/dell/Desktop/fusion32/build/cert-login-ui-recert-20261026`.
Main remains `1ac7245da68d602697c59f3241277ed43577737b`.

## Objective and authority

Inspect actual ancestry, normally push the intended fix, then certify on a fresh
worktree without merging main or the failed certification head. Test credentials,
scanner, minimap restart, invalid login, entry/logout/death/relogin, displacement,
single INIT_GAME and world cleanup/rebuild. Deliver reproducible evidence and the
requested suites/build/harness. No production or server edits.

## Inspection and discoveries

Six required startup documents read; Git status/branch/HEAD/remotes and sole
parents checked. Actual chain: main -> bf54da4 -> 3562dd0 -> 316d888 -> 444a89e.
3b82a9a is a sibling evidence branch, not an ancestor. Fix branch pushed normally
from bf54da4 to 444a89e, then remote hashes verified. No force/rewrite/main merge.
Protocol/server/ClientCore are unchanged from bf54da4.

Inspected Real33DBridge account logging, storage salt/scope, worker/session
ownership; Real33DLoginQA Start/Record/BuildScript; Real33DLoginScreen credential
clearing/phase visibility; Real33DWorldActor ClearWorld/HandleEvent; session and
reconnect headers; production credential scanner and harness. Authoritative
game JoinGame, Die, TPlayer::Death and TCreature destructor inspected at selected
game 386fa9b under the recorded 772 guards. Exact details in the evidence README.

## Changes and files

Only evidence under `evidence/certification/login-ui-recert-20261026/`,
PROJECT_STATUS.md, PARITY_MATRIX.md, this handoff and prior-handoff archive.
Runtime output copied unchanged with SHA256 manifest; no redaction needed.
No tests, protocol, production source, reference files or server data edited.
Primary worktree's pre-existing untracked directories preserved.

## Tests/results and PASS

CERTIFIED for the bounded requested matrix. Raw IDs/passwords 0/0; eight new
screenshots reviewed; four planted-value failures plus clean control PASS.
Minimap scope/salt preserved over a real process restart; cached cells loaded.
Invalid login and five QA entry/logout cycles PASS. All successful entries:
one INIT_GAME, zero protocol errors, actor rebuild from WorldState.
Displacement and explicit user reclaim: both displaced owners remain at
selection without auto reconnect; >80-second hold captured.
Literal death by rats at 21:26:24.546Z, HP zero, selection at 21:26:54Z;
cleanup zero actors/local identity/inventory/containers. Same PID 11680 and
start time re-enters temple at 21:27:30Z, HP 150, one INIT_GAME; first north
step accepted. Operator assisted the death and explicitly confirmed the reclaim.

ClientCore 14/14; native 12/12; Unreal automation 15/15; fresh build 39 actions;
final relink PASS. Final strict harness 12 PASS / 0 FAIL / 0 SKIP over 107 logs;
secret_check 6/6; independent evidence verification 109 PASS / 0 FAILED.

## Evidence and reproduction

`evidence/certification/login-ui-recert-20261026/README.md`: acceptance matrix, provenance, source notes, interruptions/limits.
`verify.py`: runnable from evidence directory without live server, checks stored
observations. `manifest.json`: hashes, no output redactions. `reproduce/`: scripts
originally executed from this worktree's ignored build/recert directory.
Fresh runtime logs/snapshots/8 PNGs in live/. Initial/final harness and both the
failed isolated automation startup and successful desktop suite retained.

## Remaining UNVERIFIED, blockers and risks

No blocker for this scoped certification. Multi-character/empty/multi-world
lists, waiting-list/game-login-error variants, lost-link retry, nonzero premium
days, additional death variants and art parity remain UNVERIFIED.
Raw account IDs remain in historical commits as already reported by the fix;
no history rewrite was authorized. Current evidence and new output scan clean.
Scanner needles cover this runtime's four credentials, not arbitrary secrets.
The legitimate death penalty remains. Cache salt/private baseline are ignored
and excluded from evidence. All QA clients closed normally; three services healthy.

## Environment recovery and critical context

Existing server services started normally from DEAD. No stale-lock edit needed.
First isolated Unreal automation failed at DDC before tests; desktop rerun 15/15.
The duplicate coordinator's output lock was resolved. Target-window focus failed
twice without input; existing Win32 certification driver then succeeded.
Computer-use node_repl was unavailable. Credential forms were never captured.
An intended click on restart client's Enter Game occurred after an operator
had already re-entered and therefore produced a world walk; it is not attributed
as login proof. User's subsequent explicit reclaim is documented, not a retry.

## Exact next task

Review this evidence-only certification and normally published fix branch.
Do not merge main or push certification without authorization.
Commands from this worktree:
`python evidence/certification/login-ui-recert-20261026/verify.py`
`git log --format="%H %P %s" 1ac7245..HEAD`
`git diff 444a89e -- clientcore unreal reference tests scripts/server`
`git status --short`
Production diff must remain empty. Integration candidate is this certification
head over 444a89e, never the failed 3b82a9a branch.
