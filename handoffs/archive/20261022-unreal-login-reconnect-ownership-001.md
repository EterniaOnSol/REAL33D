# HANDOFF - Independent combined login/handover certification

Date/time: 2026-10-22, America/Guatemala. Agent: Codex, independent certification.
Status: **CERTIFIED** for the user's combined matrix; ready for requested integration.
Branch: certification/unreal-login-combined-20261022.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022.
Starting implementation: c7b3c3dc2b31d5e476e4a5039e0959d99a9ed4b8.
Ending commit: the certification commit carrying this handoff.
Main unchanged: 73c01e330ef35cb3ba8b1fa65bec75596a8f183c.
Strict chain: main -> 41e9225ce47ed7dd4148bac065c56db12411a54c -> c7b3c3d.
Implementation branch milestone/unreal-login-reconnect-ownership-001 pushed normally.
No main merge, no cert push, no production/test/reference/server changes.
Prior handoff: archive/20261022-unreal-login-reconnect-ownership-001-implementation.md.

## Verified

Cold login; four intentional takeovers; displaced reconnects 0; new owners stable
over 30 seconds; INIT_GAME/world once per session; immediate authoritative moves;
no ownership ping-pong. Login service unavailable: exactly three retries after
5.050/5.049/5.051-second waits, cap preserved. Service restored inside window:
first retry successful. Established RemoteClosed on server shutdown: no new
retries over 37.850 seconds. Protocol residual/unsupported/anomalies all zero.

Independent source/fixture/live confirmation of handshake command boundaries,
absence of protocol displacement message, and RemoteClosed/Failed propagation
through the bridge to the tested reconnect policy. Hardware server observation
recorded exactly four TakeOver/CLEAR_TARGET pairs and six INIT_GAME/FULLSCREEN.
Six successful sessions, 12/12 real-key accepted moves, zero duplicate requests.

ClientCore 14/14; native 10/10; Unreal automation 10/10, errors/warnings zero;
fresh Unreal build and MSVC sanity PASS; harness 11 PASS / 0 FAIL / 0 SKIP;
Git Bash secret_check 6/6. Final live audit scans 125 files, all protocol counters 0.

Evidence and reproducible scripts:
../evidence/certification/login-combined-20261022/README.md
Audit from root:
python evidence/certification/login-combined-20261022/audit_live.py evidence/certification/login-combined-20261022/live build/certification/review-audit.json

## Tooling, limits, next step

H2 hold completion hit foreground activation refusal; input was safely refused.
Only observations retried, original failure retained; no login/movement replay.
Certification-only AttachThreadInput fallback retains the foreground/PID guard.
All clients closed normally, observer detached, all QA services stopped cleanly.
Changes in this commit are docs and certification-only helpers/raw evidence.

Known limit: server death after established session also maps to RemoteClosed
and therefore does not auto-reconnect. Normal shutdown was live-tested; abrupt
process death was not separately injected. Silent network loss remains UNVERIFIED.
ParseGameInitialMessage remains out of scope/UNVERIFIED. ReadNextMessage DOES call
it; bridge walks retained message_bytes independently of classification. Do not
repeat archived implementation prose claiming the parser is not reached.
Other cleanup prologues/classic takeover/hours-long holds were not exercised.

Next task: only when requested, fetch and verify main -> 41e9225 -> c7b3c3d ->
this certification commit remains linear, then normal fast-forward integration
and push. Preserve all documented limitations. No integration done in this task.
