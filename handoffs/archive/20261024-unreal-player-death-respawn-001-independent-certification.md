# HANDOFF - UNREAL-PLAYER-DEATH-RESPAWN-001 independent certification

Date: 2026-10-24. Agent: Codex, independent certification.
Branch: certification/unreal-player-death-respawn-independent-20261023.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-player-death-20261023.
Starting audit: 20db3a62e567e96595cc554a0092ff0bb30103ab.
Ending commit: resolve this certification branch; evidence/docs only.
Main/origin/main: 28b5524c17b98a8e279125c4994e528b6430f380, unchanged.

## 2026-10-24 independent UNREAL-PLAYER-DEATH-RESPAWN-001 certification

**CERTIFIED** for one operator-assisted, independently observed QA death and
fresh relogin. Audit 20db3a6 normally pushed; sole parent main 28b5524 verified.
Fresh certification branch; no production changes, main merge or cert push.

Rats killed QA A on floor 8. Server HP zero and exact EventMessage
`You are dead.\n` observed; orderly close 29.639780 seconds later. Client
cleared all actors, local identity, inventory/containers and combat. Automatic
reconnects zero, including overlap while a fresh client successfully relogged
and moved. Relogin: exactly one INIT_GAME, temple (32097,32219,7), HP 150/150,
mana 0/0, level 1, experience 90; equipment matches server after backpack loss.
First operator step and independent later step accepted. No fabricated state.

Evidence verification 95/95; replay 3,205 commands, zero errors/anomalies.
ClientCore 14/14, native 10/10, Unreal automation 10/10; initial fresh build and
final relink succeeded. Final strict harness 11 PASS / 0 FAIL / 0 SKIP;
70 logs / zero protocol anomalies; Git Bash secret_check 6/6.

UNVERIFIED: active attack/follow target at death (neutral reset observed),
additional character/penalty/attacker/floor variants, repeated deaths and
comprehensive mesh/animation coverage. Brother artwork loaded on request.
One stale post-movement snapshot is excluded as movement proof; fresh capture,
wire response and accepted-movement log agree. Slow art loads and stale WSL
PID-lock recovery are documented. The audit's Die/Logout description is
corrected: Die sets DEAD; the creature destructor calls Logout(30,true).

Both clients closed normally; debugger detached; three QA services healthy.
The legitimate death penalty remains. See
[independent evidence](evidence/certification/player-death-20261023/README.md).

Next: review the evidence-only certification commit for the observed scope.
Do not merge main or push certification without explicit authorization.
Reproduction: evidence/certification/player-death-20261023/README.md and verify.py.
Recorded live data, immutable raw logs, initial/final harness and automation are
under that directory, with manifest hashes. Original ignored working artifacts
remain in build/certification. Local art junctions reference the existing
unreal-world-presentation-polish-001 worktree; no art source was modified.
At final cleanup querymanager PID 513, game 542 and login 1065 were healthy;
debugger 1119 detached. If WSL has since restarted, verify identities and stale
locks before starting services. Never infer runtime state from these old PIDs.
