# UNREAL-LOGIN-RECONNECT-OWNERSHIP-001

2026-10-22, America/Guatemala. Branch
`milestone/unreal-login-reconnect-ownership-001` from
`41e9225ce47ed7dd4148bac065c56db12411a54c`.
Certification that raised the blocker: `16cfa3d`.
No merge to main. No push. **No server change. No protocol change.**

STATUS = **PASS**. The handshake fix of `41e9225` is preserved untouched.

## Reproduced first

Two real clients on one character, both left open — the certification's own
condition, and the reason it could see this at all. On the base commit:

```
R_A  connects=4  inits=4  reconnects=3
R_B  connects=4  inits=4  reconnects=3
```

Eight logins for what should have been two: six uncontrolled handovers,
matching `16cfa3d`. Protocol diagnostics were zero throughout, so this is an
ownership defect and not a decoding one — the handshake fix was working the
whole time.

The last session end on every client read `the session ended`, which is the
information loss at the centre of this.

## The trace

`connections.cc` routes a second login for an online character to
`TPlayer::TakeOver`, and the old connection is dealt with by:

```cpp
OldConnection->CharacterID = 0;
OldConnection->Logout(0, true);
```

`CharacterID` is zeroed **before** the call, so `TConnection::Logout` does not
even reach `StartLogout`. **Fusion32 sends the displaced client nothing at
all.** It sets the connection to `CONNECTION_LOGOUT` and the socket closes.

So, to the audit question *"whether Fusion32 provides any distinguishable
close/message"*: **there is no protocol message.** What there is, is a
distinguishable *transport* ending. ClientCore already separates them —
`ConnectionState::RemoteClosed` for a close performed by the peer,
`Failed`/`Disconnected` for a link that broke — and the state is latched.

The client threw that away. `Real33DBridge.cpp` tested only for `TimedOut` and
collapsed every other failure into one generic `Disconnected`, and
`Real33DWorldActor::Tick` then retried any dead session after five seconds, up
to three times, with nothing to tell it why the session had died. That retry
is pre-existing on main; `41e9225` did not introduce it.

The loop follows: A is displaced → A retries → A displaces B → B retries → B
displaces A, until both exhaust three attempts. Whoever retried last owned the
character.

## Root cause

The client could not distinguish a session that was **ended** from one that was
**lost**, and treated both as grounds to reconnect. Reconnecting after a
deliberate close is how a displaced client reclaimed a character that had just
been handed to someone else.

## Fix

The reason now survives from the transport to the retry.

- `GameLoginSession::connection_state()` — a read-only accessor in ClientCore,
  exposing the latched `ConnectionState`. It is read from the transport rather
  than from the failing read's status because the state is sticky and the
  status is not: the read that surfaces the failure is usually the one *after*
  the close and reports `NotConnected`.
- `FReal33DEvent::bOrderlyClose` carries it, set by the bridge, which also now
  says which happened: `Fusion32 closed this connection; not reconnecting
  automatically`, or `the link was lost (<status>)`.
- `Real33DReconnectPolicy.h` holds the decision, free of Unreal types, proven
  by `tests/real33d_reconnect_policy_tests.cpp`. The shipped code calls that
  function; there is no second copy of the rule.

```
Closed  -> never reconnect, at any attempt count, after any delay
Lost    -> reconnect, unchanged: 3 attempts, 5 seconds apart
```

The latch clears only on a connection that succeeds, so a deliberate close
cannot be forgotten by a later unrelated drop.

**Reconnect is not disabled and nothing is timed differently.** The attempt cap
and the delay are untouched; a refused or unreachable connect and a transport
error are still losses and still retried. Waiting longer is not made to fix
anything — reclaiming someone else's character does not become correct after a
backoff, which is why this is a rule about the *kind* of ending.

## Live QA

Fusion32 from `scripts/server/start_wsl.sh`; stopped cleanly afterwards, all
ports confirmed clear. Account B, *Test Player B*, creature 1002.

| Row | Result |
|---|---|
| Cold login | PASS — T0 in game, movement 4 sent / 4 accepted |
| Controlled takeover | PASS — each new client takes over once |
| Four chained handovers | PASS — T1..T4, every one intentional |
| Automatic ownership ping-pong | **zero** — `reconnects=0` on all five clients |
| INIT_GAME + FULLSCREEN every takeover | PASS — one `init` each, 336 tiles on the final owner |
| Immediate movement by current owner | PASS — 4 sent / 4 accepted in every round |
| Displaced client stays non-owning | PASS — `connects=1`, `reconnects=0`, ends with the orderly-close reason |
| New owner stability | PASS — in the two-client hold, the owner logged **0** session ends across 75 s |
| Legitimate reconnect | PASS — see below |
| Protocol | `residual_bytes` 0, `unsupported_opcodes` 0, `protocol_anomalies` 0, `walks_unanswered` 0 |

```
## AFTER the fix: cold login plus four intentional handovers
  T0  connects=1 inits=1 reconnects=0
  T1  connects=1 inits=1 reconnects=0
  T2  connects=1 inits=1 reconnects=0
  T3  connects=1 inits=1 reconnects=0
  T4  connects=1 inits=1 reconnects=0
```

### Legitimate reconnect still works, and recovers

Two separate checks, because "it still tries" and "it still works" are
different claims.

1. Server down at launch: the client retried its full, unchanged budget —
   `reconnecting, attempt 1`, `2`, `3` — each classified as a loss
   (`could not reach the login service`), never as a close.
2. Recovery: with the login service stopped and then restarted inside the
   retry window, the client's own log reads

   ```
   session ended: could not reach the login service
   reconnecting, attempt 1
   connected: Test Player B
   local player is creature 1002
   ```

   A lost link was retried and the session came back.

## Tests

ClientCore **14/14** · native **10/10** (`real33d_reconnect_policy_tests`,
7 cases) · Unreal automation **10/10**, 0 errors, 0 warnings · REAL33DEditor
Win64 Development built · MSVC C++17 suites and the C++20 archive · harness
**11 PASS / 0 FAIL / 0 SKIP** over 102 logs · secret_check 6/6.

## Remaining UNVERIFIED

- **A server that dies is now treated as a deliberate close**, because a
  process death closes its sockets and the transport latches `RemoteClosed`
  exactly as a displacement does. Those two are not distinguishable from the
  client, and this is a real behaviour change: a client will no longer retry
  across a server restart. It is a small loss in practice — this Fusion32 takes
  roughly a minute to reload its sectors, far beyond the unchanged 3x5s budget,
  so those retries failed anyway — but it is a change and is not hidden here.
- A genuine silent network loss (cable pulled, link black-holed) was not
  staged. Such a loss surfaces as reads timing out, which the bridge treats as
  "nothing to read" rather than as a disconnect, so no reconnect happens today
  either way. That behaviour is unchanged by this fix and remains unexamined.
- Displacement was always driven by a second REAL33D client on the same
  account. A takeover by the classic 2D client was not tried.
- The four chained handovers each kept the previous client alive for the whole
  of the following round — about forty-five seconds, far past the five-second
  retry window — but not indefinitely. A displaced client left running for
  hours was not observed.
- `ParseGameInitialMessage`'s positional assumption, recorded as UNVERIFIED in
  `UNREAL-LOGIN-HANDOVER-001`, is unchanged and still not reached on this path.
