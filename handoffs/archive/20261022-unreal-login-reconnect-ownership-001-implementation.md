# HANDOFF - UNREAL-LOGIN-RECONNECT-OWNERSHIP-001

Date/time: 2026-10-22, America/Guatemala
Agent / role: Claude; diagnosis, fix and live QA
Starting commit: `41e9225ce47ed7dd4148bac065c56db12411a54c`
Certification that raised the blocker: `16cfa3d`
Ending commit: the commit carrying this handoff on
`milestone/unreal-login-reconnect-ownership-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261022-unreal-login-handover-001.md`

OWNERSHIP_001_STATUS = **PASS**
MAIN_MERGE = NONE. PUSH = NONE.
SERVER CHANGE = **NONE**. PROTOCOL CHANGE = **NONE**.
The handshake fix of `41e9225` is preserved untouched.

## Reproduced first

Two clients on one character, both left open, on the base commit:

```
R_A  connects=4  inits=4  reconnects=3
R_B  connects=4  inits=4  reconnects=3
```

Eight logins for two clients. Protocol diagnostics zero throughout, so this
was an ownership defect, never a decoding one.

## Root cause

`connections.cc` zeroes the old connection's `CharacterID` **before** calling
`TConnection::Logout`, so Logout does not even reach `StartLogout`:
**Fusion32 sends the displaced client nothing**. The socket just closes.

There is no protocol message to key on — but there is a distinguishable
*transport* ending, and ClientCore already had it
(`ConnectionState::RemoteClosed` vs `Failed`). The client discarded it: the
bridge collapsed every non-timeout failure into one generic `Disconnected`,
and `Real33DWorldActor::Tick` retried any dead session with no idea why it
died. So a displaced client logged back in and took the character off whoever
had just been given it, which displaced them, which retried in turn. That
retry is pre-existing on main.

## Fix

The reason now survives from the transport to the retry:
`GameLoginSession::connection_state()` (read-only accessor; the state is
sticky, the failing read's status is not), carried on
`FReal33DEvent::bOrderlyClose`, decided by `Real33DReconnectPolicy.h` — which
the shipped code calls, so there is no second copy of the rule.

```
Closed -> never reconnect, at any attempt count, after any delay
Lost   -> reconnect, unchanged: 3 attempts, 5 seconds apart
```

Reconnect is not disabled and nothing is timed differently.

## Live QA

Cold login, then four intentional chained handovers: every client
`connects=1 inits=1 reconnects=0`, movement 4 sent / 4 accepted in every
round, 336 tiles on the final owner, all protocol counters 0. In the
two-client hold the new owner logged **0** session ends across 75 s.

Legitimate reconnect proven twice — it still tries (full 3-attempt budget
against a down server, each classified as a loss) and it still **works**
(login service restarted inside the window: `reconnecting, attempt 1` ->
`connected` -> `local player is creature 1002`).

## Tests

ClientCore 14/14 · native 10/10 · Unreal automation 10/10, 0 errors,
0 warnings · REAL33DEditor built · MSVC archive · harness **11 PASS / 0 FAIL /
0 SKIP** over 102 logs · secret_check 6/6.

## Remaining UNVERIFIED

- **A server that dies is now treated as a deliberate close.** A process death
  closes its sockets and latches `RemoteClosed` exactly as a displacement
  does; the two are not distinguishable from the client. A client will no
  longer retry across a server restart. Small in practice — this Fusion32
  needs about a minute to reload sectors, far beyond the unchanged 3x5s
  budget, so those retries failed anyway — but it is a real change, stated
  rather than hidden.
- A silent network loss was not staged; it surfaces as reads timing out, which
  the bridge treats as "nothing to read", so no reconnect happens today either
  way. Unchanged and unexamined.
- Displacement was always by a second REAL33D client; the classic 2D client
  was not used to take over.
- Displaced clients were held for about forty-five seconds each, not for hours.
- `ParseGameInitialMessage`'s positional assumption is unchanged and still not
  reached on this path.

## Exact next task

Independent certification of this branch against the same two-client
condition. If a server-restart retry is wanted back, that needs a signal the
client can actually distinguish, which does not exist today on this protocol.

## Environment

Fusion32 started and **stopped cleanly**; all three ports confirmed clear
afterwards. One login service was started by hand for the recovery test and
was stopped again. No server data edited.
