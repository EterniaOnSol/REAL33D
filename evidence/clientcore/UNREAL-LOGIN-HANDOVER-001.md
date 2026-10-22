# UNREAL-LOGIN-HANDOVER-001

2026-10-22, America/Guatemala. Branch `milestone/unreal-login-handover-001`
from `origin/main` at `73c01e330ef35cb3ba8b1fa65bec75596a8f183c`.
No merge to main. No push. **No server change. No protocol layout change.**

STATUS = **PASS**. Root cause identified exactly, fixed in the layer that held
the wrong assumption, and verified live on both paths.

## Reproducing it first

A plain kill-and-relaunch **does not** reproduce this, and finding that out was
the first useful result. Killing the process closes the socket, Fusion32 logs
the character out within a second, and the ~45-second engine boot means the
next login is an ordinary cold one. Two such attempts produced clean sessions
and clean server logs — `loggt ein` / `loggt aus` pairs, no handover.

The defect needs a **second login for a character that is still online**.
`connections.cc` then takes its other branch: it logs the old connection out
and calls `TPlayer::TakeOver`. Reproduced by launching a second REAL33D on the
same account while the first was in game, which the server confirms with
`Spieler Test Player B übernimmt Verbindung.`

Both clients then reported, verbatim:

```
SV_CMD_INIT_GAME is not decoded at offset 1 of 1921; 1920 bytes of the payload
left unwalked; head=A3 0A EA 03 00 00 32 00 00 64 58 7D
```

The offset and the `head=` window are not in the shipped diagnostic; they were
added to find this. **The offset was kept**, because "is not decoded" with no
position cannot distinguish "the server sent something new" from "this client
lost its place earlier in the same frame", and that distinction is the whole
diagnosis here.

## Root cause

Reading the head: `A3` is 163, `SV_CMD_CLEAR_TARGET`, a zero-body command.
`0A` is `SV_CMD_INIT_GAME` — at **offset 1**. `EA 03 00 00` is creature 1002,
`64` is `SV_CMD_FULLSCREEN`.

So the first game payload of a handover is

```
[CLEAR_TARGET][INIT_GAME ...][FULLSCREEN ...]
```

and `TPlayer::TakeOver` says why: it assigns the new connection **first**
(`this->Connection = Connection`) and only then does its tidying.
`this->Combat.StopAttack(0)` emits `SV_CMD_CLEAR_TARGET` onto that new
connection, several lines **before** `SendInitGame` is reached. A cold login
has no such prologue — `crplayer.cc` constructs the player and goes straight to
`SendInitGame`, `SendRights`, `SendFullScreen` — which is why this was invisible
until now.

`Real33DBridge.cpp::ProcessPayload` recognised the two handshake commands **by
position**:

```cpp
if (Opcode == kGameInitOpcode  && At == 0) { ... }
if (Opcode == kGameRightsOpcode && At == 8 && ...) { ... }
```

Offset 0 and offset 8 are where a cold login puts them. They are not a protocol
rule, and nothing in `sending.cc` promises them. With INIT_GAME at offset 1 the
test missed, the opcode fell through to `DecodeServerUpdate`, which has no case
for a login-phase opcode, and the walker stopped — by design, because an
unsized command means the next one cannot be located.

**Stopping there discarded the remaining 1920 bytes: the entire
`SV_CMD_FULLSCREEN`.** That is why the affected session came up with no map and
why every walk it sent was refused or expired unanswered. The protocol counters
were telling the exact truth; the client had genuinely lost the stream.

## The fix, and why this layer

The defect is a positional assumption in the payload walker, so that is what
changed. Opcodes 10 and 11 are recognised **by identity at a command
boundary**, wherever they appear.

This is safe and unambiguous: `connections.hh` gives no in-game command a value
below 100 apart from the login errors (20..22) and ping (30), so neither 10 nor
11 can be confused with anything the in-game decoder handles. The check is only
ever applied where the walker has already established a command begins.

The rule and its bounds live in
`unreal/REAL33D/Source/REAL33D/Public/Real33DHandshakeFrame.h`, free of Unreal
types, with `tests/real33d_handshake_frame_tests.cpp` proving it on a plain
toolchain — including the real observed bytes.

What was **not** done, deliberately:

- no sleep, no retry, no reconnect backoff — the stream was never late, it was
  misread;
- nothing is dropped — the walker now consumes the handshake commands and
  continues into the FULLSCREEN instead of abandoning it;
- no server change, and no change to any wire layout. `SendInitGame`'s 8 bytes
  and the 33-byte rights block are read exactly as `gamelogin.cpp` already
  reads them;
- ClientCore's decoder was left alone. Teaching `DecodeServerUpdate` about
  login-phase opcodes would be a larger change to a certified component for a
  defect that is entirely in the bridge's walker.

A second INIT_GAME is adopted rather than refused: a handover legitimately
re-states the same creature id and follows it with a fresh FULLSCREEN that
replaces the world wholesale. The client does not re-login, and the server log
confirms one login per client process.

## Live QA

Fusion32 from `scripts/server/start_wsl.sh`, all services `VERIFIED`; stopped
cleanly afterwards. Account B, *Test Player B*, creature 1002.

| Row | Result |
|---|---|
| Cold launch | in game, 0 diagnostics, 336 tiles, 47 commands, counters 0 |
| Handover (TakeOver) | in game, 0 diagnostics, 336 tiles, 51 commands, counters 0 |
| Repeated handovers | four chained rounds, every one in game, **0 diagnostics in all four** |
| INIT_GAME decoded | every session; no `left unwalked` anywhere after the fix |
| Initial world | `worldstate_tiles` 336 on cold and on handover alike |
| Movement immediately | four cardinal walks straight after login: **sent 4, accepted 4**, on both the cold and the handover client |
| Duplicate login/init | server log shows exactly one `loggt ein` per client process and one `übernimmt Verbindung` per handover |
| Protocol | `residual_bytes` 0, `unsupported_opcodes` 0, `protocol_anomalies` 0, `walks_unanswered` 0, `last_diagnostic` `none` |

Before the fix the handover client never reached the world at all; after it,
the handover session is indistinguishable from a cold one.

## Tests

ClientCore **14/14** · native **9/9** (`real33d_handshake_frame_tests`, 7 cases)
· Unreal automation **10/10**, 0 errors, 0 warnings · REAL33DEditor Win64
Development built · MSVC C++17 suites and the C++20 archive · harness
**11 PASS / 0 FAIL / 0 SKIP** over 97 logs · secret_check 6/6.

## Remaining UNVERIFIED

- `ParseGameInitialMessage` in ClientCore carries the **same** positional
  assumption: it accepts INIT_GAME only as the first opcode of a payload. It is
  not reached on this path — the bridge walks `message_bytes` itself — so the
  defect does not manifest through it today, and it was left alone rather than
  changed speculatively. If a future caller drives login through that function,
  it will need the same correction.
- Only the `StopAttack` prologue was observed. `TakeOver` also calls
  `ClearRequest`, `LeaveAllChannels`, `CloseAllContainers` and `RejectTrade`
  before `SendInitGame`; any of those could prepend further commands under the
  right state (a player in a channel, with containers open, mid-trade). The fix
  is indifferent to how many commands precede INIT_GAME, but only the
  one-byte CLEAR_TARGET case was seen on the wire.
- The handover was always driven by a second client on the same account. A
  genuine network-drop-then-reconnect, where the server has not yet noticed the
  old socket, was not staged.
- Movement was verified with the four cardinals. Diagonals were not re-exercised
  here; they are covered by `UNREAL-DIAGONAL-INPUT-001`.
