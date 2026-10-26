# HANDOFF - UNREAL-LOGIN-CHARACTER-SELECT-UI-001 independent review

Date: 2026-10-25 America/Guatemala / 2026-10-26 UTC.
Agent: Codex. Role: independent certifier, with operator-assisted death/relogin.
Branch: certification/login-ui-independent-20261025.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-login-ui-independent-20261025.
Starting commit: bf54da4c576859fdd7ac7188d7a9f925e966d421.
Ending commit: the evidence commit containing this handoff (git rev-parse HEAD).
Main: 1ac7245da68d602697c59f3241277ed43577737b, unchanged. No main merge.
Normal implementation push verified; user explicitly authorized certification
commit and normal push near the end of the session.

Objective: independently certify the full login/select/logout/death/relogin
matrix on the exact implementation, using a fresh linear worktree.

Inspection: mandatory startup docs and Git status/branch/HEAD/remotes; server
source provenance/version guard; bridge account/game workers, session flow,
login widgets, world teardown, reconnect policy and QA driver. SOURCE_NOTES.md
lists exact files/functions. Existing unrelated/untracked work and other
certification worktree were preserved. No production or reference edits.

Result: FAILED complete certification due to four logged account-ID occurrences;
passwords zero. Invalid login, one-row server list/selection, entry, five
same-process logout/relogin cycles, literal death -> selection, same-process
post-death relogin, actor cleanup/rebuild, one INIT_GAME/session, immediate
movement, takeover without automatic ping-pong and protocol counters all PASS.
A deliberate operator reclaim occurred during takeover; not automatic retry.
Earlier idle close/re-entry in A-art is retained separately from death.

Death at 04:13:56.957Z (server message), HP zero; selection at 04:14:26.913Z.
Re-entry in the same PID 2512 at 04:15:26.069Z; first movement accepted at
04:15:29.552Z. Two post-death steps; final (32098,32218,7), near Cipfried.
Operator requested existing art; reused local asset junctions without edits.
World/player art visible; Cipfried remains a placeholder. User may close client;
all relevant evidence was preserved before permission to close was given.

Tests: ClientCore 14/14; native 11/11; Unreal 14/14, zero suite errors/warnings;
fresh build and final relink PASS; strict harness 11 PASS / 0 FAIL / 0 SKIP;
secret_check 6/6; evidence verifier 37/37. Later death snapshots separately
verified after the strict harness. Initial preparation failures, interrupted
build, sandboxed launch and corrected verifier assumption are documented.

Files changed: PROJECT_STATUS.md, PARITY_MATRIX.md, this handoff and archived
implementation handoff; evidence/certification/login-ui-independent-20261025/.
Raw working evidence remains under build/certification in this worktree.
Published text copies redact runtime account identifiers; originals' hashes
are retained. No password or runtime credential file is published.

UNVERIFIED: true multi-character switch (only one offered row), empty/multi-world
lists, game-service login-error/waiting list, lost-link reconnect, nonzero premium
days, broader death variants and visual certification.

Blocker/risk: literal credential-absence criterion fails in product logs even
though the generic secret checker passes. Do not silently reclassify an account
identifier as absent; no production fix belongs in this evidence-only audit.

Exact next task: remove/redact account-ID logging in
unreal/REAL33D/Source/REAL33D/Private/Real33DBridge.cpp::BeginAccountLogin and
Real33DLoginQA.cpp::Start on a new implementation revision, then rerun affected
login tests/scan. Do not merge main from this certification.
Recheck evidence: python evidence/certification/login-ui-independent-20261025/verify.py.
Recheck Git: git status --short; git log --format="%H %P" 1ac7245..HEAD.
Recheck server: wsl -d Ubuntu-26.04 -- bash scripts/server/status_wsl.sh
/mnt/c/Users/dell/Desktop/fusion32.

Critical context: runtime is /var/lib/fusion32-server-baseline-772-0 in
Ubuntu-26.04. A verified stale game PID lock was archived with approval before
startup. Three services were healthy; no server gameplay data was edited.
Real death penalty is retained. Credentials never belong in evidence or chat.