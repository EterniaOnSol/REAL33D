# Independent combined login / handover certification

**CERTIFIED** for the requested matrix on exact implementation
`c7b3c3dc2b31d5e476e4a5039e0959d99a9ed4b8`.
This covers UNREAL-LOGIN-HANDOVER-001 and UNREAL-LOGIN-RECONNECT-OWNERSHIP-001.
No production, test, protocol or server source changes. No main merge.

## Provenance and reproduction

Strict sole-parent chain, independently fetched/resolved:

```
73c01e330ef35cb3ba8b1fa65bec75596a8f183c (main)
  -> 41e9225ce47ed7dd4148bac065c56db12411a54c
  -> c7b3c3dc2b31d5e476e4a5039e0959d99a9ed4b8
```

Implementation branch `milestone/unreal-login-reconnect-ownership-001` was pushed
normally before certification; remote identity matched. Fresh worktree:
`C:/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022`, branch
`certification/unreal-login-combined-20261022`, created at exact c7b3c3d.
Certification HEAD is the commit carrying this report. Main remains at 73c01e33.

All live sessions used the same freshly built DLL SHA-256:
`6152cb9301b5ba46fa2cc17a7a5e2869c03b4fa579feae7e6e822da3e5cba84b`. No post-live rebuild or source edits.
Normal ignored UI item resources and the static classic-client validation inputs
were copied from the earlier certification workspace. No legacy binary executed.

Recompute all live gates from the retained evidence:

```powershell
python evidence/certification/login-combined-20261022/audit_live.py evidence/certification/login-combined-20261022/live build/certification/review-audit.json
```

`live_session.py` launches unchanged REAL33D with normal runtime/account-B flags.
`native_ui_driver.py` sends real Win32 keyboard scan codes, checks exact PID and
foreground ownership, and uses real F9 for snapshots. No direct Slate callbacks,
game API commands, synthetic protocol calls, fabricated inventory or server state.
`run_chain.ps1` keeps each displaced client open through the new owner's hold,
then closes that old window normally before starting the next intentional client.

## Independent root-cause confirmation

1. **Handshake identity belongs at a command boundary, not a fixed offset.**
   `reference/game/src/crplayer.cc::TPlayer::TakeOver` assigns the new connection
   before cleanup. `crcombat.cc::StopAttack(0)` calls `sending.cc::SendClearTarget`
   before `SendInitGame` and `SendFullScreen`. CLEAR_TARGET is one byte, placing
   INIT_GAME at offset 1, not 0. `Real33DBridge.cpp::ProcessPayload` now invokes
   `HandshakeCommandAt` at the current decoder-established command boundary and
   reads the creature ID relative to it; it does not search arbitrary bytes.
   The native fixture `[CLEAR_TARGET][INIT_GAME][FULLSCREEN]` reaches offset 9
   with creature 1002. That fixture uses a stand-in game decoder; full map decode
   is independently proved by the live world evidence.
2. **Takeover sends no displacement message.** In
   `reference/game/src/connections.cc::TConnection::JoinGame`, the old connection's
   CharacterID is zeroed before `Logout(0, true)` (lines around 244-252).
   `Logout` therefore skips `StartLogout`; it only enters CONNECTION_LOGOUT.
   `Process` then calls `Disconnect`, marking the connection unusable and
   signalling the connection thread. No displacement opcode is sent on that path.
3. **The transport distinction reaches retry policy.**
   `clientcore/src/tcp_transport.cpp::ReadSome` latches RemoteClosed on EOF and
   selected socket errors; other non-timeout errors latch Failed. Later reads
   can report NotConnected without erasing the latched state.
   `GameLoginSession::connection_state()` exposes that state; the bridge copies
   it into `FReal33DEvent::bOrderlyClose`; WorldActor stores the reason and calls
   the tested `ShouldReconnect` policy. Closed does not retry; Lost preserves
   the three-attempt cap and strictly greater than five-second delay.
   A refused login connection takes the Failed event path with bOrderlyClose
   false. The live takeover and login-outage cases exercise opposite decisions.

`server_trace.py` independently observed actual server function entries using
four hardware execution breakpoints: 4 TakeOver, 4 non-null SendClearTarget,
6 SendInitGame and 6 SendFullScreen calls. Every takeover has the ordered
same-connection sequence TakeOver -> CLEAR_TARGET -> INIT_GAME -> FULLSCREEN.
This is call observation, **not raw packet capture**. No inferior function calls,
data writes or executable patching. The null-connection CLEAR_TARGET at normal
logout is retained but cannot send anything. GDB detached before server shutdown;
`teardown.txt` proves TracerPid 0 and clean service shutdown.

## Live matrix

Normal REAL33D -> Fusion32, same account/character, seven actual client processes:
C0, four intentional successors H1-H4, Budget, and Recovery. Each new owner
walked south `(0,+1,0)` and north `(0,-1,0)` through real keyboard input and
returned to authoritative `(32086,32228,7)`. Exactly one sent and one accepted
journal record per input, 12 accepted requests total, no rejected/unanswered
requests or external relocation in these probes.

| Session | Connect / INIT / world | Automatic retries | Hold seconds | First move after world, seconds | Accepted / requested |
|---|---:|---:|---:|---:|---:|
| C0 | 1 / 1 / 1 | 0 | 37.700 | 3.237 | 2 / 2 |
| H1 | 1 / 1 / 1 | 0 | 39.611 | 3.091 | 2 / 2 |
| H2 | 1 / 1 / 1 | 0 | 277.851 | 2.971 | 2 / 2 |
| H3 | 1 / 1 / 1 | 0 | 40.621 | 3.024 | 2 / 2 |
| H4 | 1 / 1 / 1 | 0 | 40.611 | 3.951 | 2 / 2 |
| Recovery | 1 / 1 / 1 | 1 | 37.686 | 2.733 | 2 / 2 |

The hold starts after the immediate movement probes. Every takeover owner
remained stable for more than 30 seconds while its displaced predecessor was
still open. C0-H3 each recorded one orderly close, zero reconnect attempts, and
an offline/cleared snapshot after the hold. H4 remained owner until deliberately
closed for the service tests. Exactly four TakeOver calls occurred, matching
the four intentional client launches; there was **no ownership ping-pong**.

H2's long hold reflects a certification tooling interruption, not a new login:
Windows refused foreground activation for the old-client capture at 10:40:42 UTC,
and later refused the new-client observation at 10:43:38. The helper aborted
before input both times. Original failure console and successful observations
are retained. Only observations were retried; movement/login were not replayed.
A certification-only AttachThreadInput focus fallback was added, retaining the
exact foreground guard and real SendInput. Subsequent old-client captures record
that fallback succeeding. Production code and deterministic tests were unchanged.

### Retry budget and recovery

Only the normal login service was stopped by identity-checked SIGTERM; game and
querymanager stayed running. `login_service.py` controls that executable's normal
PID/start files and lifecycle, not game data. Budget recorded four login failures
(initial plus three retries), no successful login, and no fourth retry during
12 additional seconds after the final failure.

Failure-to-retry delays were **5.050, 5.049, 5.051 seconds**. Each failed connection
also consumed about 3.1 seconds; the five-second wait is after failure, not a claim
that wall-clock spacing between attempts is only five seconds.

Recovery started while login was still down. The service was restored
**1.396 seconds** after the first failure was observed. The first scheduled retry
connected successfully, decoded INIT_GAME/FULLSCREEN, accepted both moves, and
held ownership for 37.686 seconds. Its one retry is expected and is not a takeover
reclaim. The observer recorded a cold initialization, not an extra TakeOver.

### Established RemoteClosed

After Recovery's stable session, the observer was detached and the normal QA
services were shut down cleanly. The still-open client reported
`Fusion32 closed this connection; not reconnecting automatically` and remained
offline for **37.850 seconds** at the final F9 snapshot, with **zero additional
retries**. Its earlier recovery retry remained the sole retry in that process.
The client was then closed normally. All QA clients/services are stopped.

## Tests and evidence

- ClientCore **14/14**, native **10/10**, including seven handshake cases and
  seven reconnect-policy cases. Optional runtime MultiUse assertions were not
  exercised by the harness's no-objects-srv native run; outside this task.
- Unreal automation **10/10**, zero test errors/warnings (`automation/index.json`).
- Fresh REAL33DEditor Win64 Development build **PASS**, MSVC sanity **PASS**.
- Full harness **11 PASS / 0 FAIL / 0 SKIP** (`harness/`); its initial protocol
  scan covered 11 existing logs. It is not substituted for live verification.
- Independent final live audit: **125 files**, residual_bytes **0**,
  unsupported_opcodes **0**, protocol_anomalies **0**, no decoder-error text.
  Expected unavailable-service/RemoteClosed messages are connection-test results,
  not protocol decode errors.
- Git Bash secret_check **6/6** in the harness and final staged-evidence scan
  (`secret-check-final.txt`). No production/test/reference diffs from c7b3c3d.

Raw logs, screenshots, per-gesture before/after snapshots, journals, launch
metadata, server trace, automation/build output and reproducing helpers are
retained here. The prior independent failure at `16cfa3d` is not hidden: that
test found the reconnect loop on 41e9225 alone; this certification tests the
combined chain and closes that specific blocker.

## Explicit limits

- **Known behavior:** server death after an established session closes/resets
  the socket and is also classified RemoteClosed, so it will not auto-reconnect.
  The client cannot infer displacement intent from that transport classification.
  EOF and Windows reset/aborted/not-connected errors share this path. Normal
  server shutdown was exercised; abrupt process death was not separately injected.
- **UNVERIFIED:** silent network loss. No packet blackhole, cable loss or long
  timeout experiment was staged, and no claim is made about eventual detection.
- **Out of scope / UNVERIFIED follow-up:** ParseGameInitialMessage positional
  recognition. Correction to implementation prose: ReadNextMessage DOES call it
  and preserves message_bytes; the bridge walks those bytes independently of the
  classification. The parser is unchanged and is not certified by this result.
- Other pre-init cleanup combinations, classic-client takeover, and hours-long
  displaced-client holds were not exercised. These do not extend this matrix.
