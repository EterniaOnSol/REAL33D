# HANDOFF - independent UNREAL-DIAGONAL-INPUT-001 certification

Date/time: 2026-10-22, America/Guatemala.
Agent / role: Codex, independent certification.
Branch: `certification/unreal-diagonal-input-001-20261022`.
Worktree: `C:/Users/dell/Desktop/fusion32/build/cert-diagonal-input-20261022`.
Starting implementation: `eb17e26ed79f63bf60e6ec709b9c1540ab008694`.
Ending commit: the evidence-only certification commit carrying this handoff.
Main remains: `a868572864519b8497af30c5054b78d803b81826`.
Prior handoff: `archive/20261022-diagonal-implementation.md`.

## Outcome

CERTIFIED for the requested matrix, with explicit attribution limits below.
Milestone pushed normally, remote exact HEAD resolved, one-parent ancestry
verified, fresh certification worktree created from that full SHA. No merges,
rebases, cherry-picks, force pushes or implementation changes. Main is untouched.

All four north-facing diagonals passed on the final camera-relative source.
Real camera orbit moved yaw 270.57 -> 91.30 (179.27 degrees); W independently
confirmed north then south, and Q/E changed to southeast/southwest with exact
server deltas. Repeat presses, cardinal alternation, a natural blocked diagonal,
chat focus, WASD/arrows/mouse and Look were exercised with real SendInput.

25 controlled walk requests have exact gesture/request/response attribution.
Session totals: 26 requested, 25 accepted, one rejected, 25 local-player moves,
zero unanswered/external relocations, no duplicate sends/terminal responses.
All protocol counters zero; 448 frames / 580 commands at EndPlay. The player
returned to starting (32088,32229,7), the window closed normally, and the normal
Fusion32 services started for this certification stopped cleanly.

## Evidence and tests

Report: `evidence/certification/diagonal-input-20261022/README.md`.
Reproduce the read-only audit:

```
python evidence/certification/diagonal-input-20261022/audit_live.py evidence/certification/diagonal-input-20261022/live
```

`live/` retains before/after snapshots, every completed probe's input record
and journal delta, raw movement journal, original REAL33D log, nine selected
screenshots, launcher provenance, and audit PASS. External driver and wrapper
are certification-only files outside production Source. No game callbacks,
synthetic protocol calls or fabricated runtime/server/inventory data.

ClientCore 14/14; native 8/8; Unreal automation 10/10, zero errors;
clean Unreal build; MSVC sanity; final harness 11 PASS / 0 FAIL / 0 SKIP,
367 logs scanned clean; Git Bash secret_check 6/6.
The harness explicitly does not exercise the unrelated native runtime MultiUse
branch without --objects-srv. Full results under the report's `tests/` folder.

Two initial build attempts failed due to full disk and its damaged PCH.
Only validated untracked Intermediate caches were removed; the clean rebuild
passed. Both failures remain in evidence. Final harness relinked the same
source after rebuilding its MSVC library; live and later DLL hashes are both
recorded accurately in the report.

## Remaining UNVERIFIED and boundaries

Setup input/request 1 was an unexpected NW walk during a camera-only probe.
Its origin is UNVERIFIED, neither blamed on the user nor declared a feature
defect. It is preserved and excluded from controlled gesture attribution.
An orbit attempt lost foreground and aborted; retry 33b passed. No whole-session
claim of zero unexplained input is made. East/west-facing camera quadrants,
held-key diagonal repetition and modified diagonals were not live-certified.
Separate login-handover INIT_GAME issue: untouched and not certified here.

## Next task

Ready for separately authorized integration of this certification branch.
Do not merge automatically. Before integration fetch and re-check exact linear
ancestry and expected main; preserve the limitations above. No further feature
change or repeat deterministic tests is needed for these documentation changes.
Preserve pre-existing untracked files in the implementation workspace.
