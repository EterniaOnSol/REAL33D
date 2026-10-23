# HANDOFF - resumed independent UNREAL-TURN-OBJECT-001 certification

Date: 2026-10-23. Agent: Codex, independent certification continuation.
Branch: certification/unreal-turn-object-independent-20261022.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-turn-object-independent-20261022.
Starting/implementation commit: 2827c6c82f711ec356f65db213e59c3894b597f2.
Ending commit: resolve the above certification branch; this evidence-only commit.
Prior handoff preserved in archive/20261023-before-turn-object-resumed-certification.md.

## 2026-10-23 resumed independent UNREAL-TURN-OBJECT-001 certification

**CERTIFIED** for the requested logical interaction matrix on implementation
`2827c6c82f711ec356f65db213e59c3894b597f2`, parent main `119dc2e`.
Resumed the existing certification worktree and B/B2 evidence; no restart from
zero, production/test/server edits, push or main merge.

Statue and chest rotations PASS; floors 7/6 and stack 1 route correctly, with
authoritative CHANGE_FIELD 107. The held statue retains its old client type until
the server responds. Nonrotatable ground returns NOTTURNABLE 57; server approach
and NOWAY 44 range behavior PASS. Normal Use, Ctrl Rotate, Look, drag/drop and
Ctrl amount dialog/Cancel PASS. UseWith arms without sending, then Ctrl world
target emits one use, bypasses the menu and receives authoritative refusal.

Both objects and the door are restored; inventory and backpack order match the
saved baseline. Nine independently observed TURN_OBJECT 133 calls correspond to
nine UI activations, zero duplicates observed. One additional cleanup rotation
was untraced during a normal-signal debugger detach; its exact send count is not
claimed. Some F9 copies are stale and explicitly listed; timestamped menu logs,
server trace and subsequent fresh/close captures support those actions.

Audit 38/38; all 122 retained snapshots have protocol diagnostics 0. ClientCore
14/14, native 10/10, retained Unreal automation 10/10 without warnings/errors,
MSVC archive and Unreal build PASS. Final strict harness: 11 PASS / 0 FAIL /
0 SKIP, 392 logs, zero anomalies; secret_check 6/6. Tooling/disk/cache failures
are retained. No product defect found. Visual mesh orientation and successful
rope gameplay are not certified. Certification client closed, debugger detached,
existing server services remain healthy. Ready for scoped review/integration;
main was not merged.

See [certification evidence](evidence/certification/turn-object-20261022/README.md).

## Inspection and work performed

Inspected existing worktrees/branch/HEAD, contract/status/architecture/roadmap/
parity/handoff and existing scripts/captures first. Reviewed selected Fusion32
CTurnObject, TCreature::ToDoTurn, Turn, SendChangeField and result enum/messages;
reviewed Unreal turn queue, DrainUses, world menu, UseWith guard and stack routing.
Original B/B2 evidence supplies completed matrix rows. Only the absent client
was relaunched as B3; no server restart. Normal UI restored chest and door and
completed UseWith. Kept all originals; published all structured/log artifacts
and 14 selected screenshots with SHA256 manifest, audit and validation reports.

Files changed: PROJECT_STATUS.md, PARITY_MATRIX.md, this handoff and its archive;
evidence/certification/turn-object-20261022 only. No clientcore/tests/unreal/
scripts/reference changes. Local evidence-only commit; no new push or main merge.

## Tool interruptions and exact limits

Debugger attach recovered on unchanged server PID 763. An initial resumed
batch debugger detached on normal SIGUSR1. The first cleanup rotate 2482->2480
therefore has no independent opcode count; normal client state/logs record its
result. Reattach with SIGUSR1/SIGUSR2 nostop noprint pass retained two final
rotation responses and door restoration. This is an instrumentation gap,
not a product defect. B3 PID 11324 closed gracefully; debugger 17612 detached.

Twenty-eight after-snapshot copies retain older F9 timestamps. Do not use them
as immediate post-action counter samples. Audit lists every one. Final inventory
and uses=3 come from B3/unreal_slice_evidence.json, 07:55:58.900 UTC. Authority
during the held request uses the live menu log resolving old type 2030 within
the hold, then updated type 2048 after release, plus server trace/screenshots.

UseWith regression covers arm/target/one-send/authoritative refusal with Ctrl
held, not a successful rope effect. Placeholder meshes do not prove visible
orientation. Arbitrary buried-stack selection, inventory rotation, and separate
different-floor refusal are not claimed. Player remains at 32109,32203,6.

First resumed harness lacked tool paths and sandbox UBA cache access; its
orphaned build tree was stopped with administrator-assisted taskkill. No such
process remains. Corrected certification wrapper passed strict harness and
relinked identical production sources. Both DLL hashes are in provenance.json.
Original disk-full build failures and prior interruption logs remain preserved.

## Next action

Review the bounded certification and its disclosed observation limits. No
further requested logical regression remains. Integration requires separate
authorization; do not merge/push merely because the branch is ready.

Repeatable checks:
  python evidence/certification/turn-object-20261022/audit.py
  cmd /c build\certification\final-harness-resume.cmd
  bash tests/secret_check.sh
  git diff 2827c6c -- clientcore tests unreal scripts reference
  git status --porcelain

Final healthy pre-existing services: querymanager 754, game 763, login 1504,
with executable/cwd/start/port checked. Leave them running. All original live
captures remain under build/certification/live; do not overwrite them.
