# HANDOFF - UNREAL-LOGIN-HANDOVER-001

Date/time: 2026-10-22, America/Guatemala
Agent / role: Claude; diagnosis, fix and live QA
Starting commit: `73c01e330ef35cb3ba8b1fa65bec75596a8f183c` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-login-handover-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261022-unreal-diagonal-input-001.md`

LOGIN_HANDOVER_001_STATUS = **PASS**
MAIN_MERGE = NONE. PUSH = NONE.
SERVER CHANGE = **NONE**. PROTOCOL LAYOUT CHANGE = **NONE**.

## Reproducing it was the first result

A plain kill-and-relaunch **does not** reproduce this. Killing the process
closes the socket, Fusion32 logs the character out within a second, and the
~45s engine boot means the next login is an ordinary cold one. Two such
attempts gave clean sessions and clean `loggt ein` / `loggt aus` server pairs.

It needs a **second login for a character still online**, which
`connections.cc` routes to `TPlayer::TakeOver` — confirmed by the server's own
`übernimmt Verbindung`. Reproduced by launching a second REAL33D on the same
account while the first was in game.

## Root cause

`TakeOver` assigns the new connection first and only then tidies up.
`Combat.StopAttack(0)` emits `SV_CMD_CLEAR_TARGET` (163, zero body) onto that
new connection **before** `SendInitGame` is reached, so a handover's first game
payload is `[CLEAR_TARGET][INIT_GAME][FULLSCREEN]` — INIT_GAME at **offset 1**.
A cold login has no such prologue, which is why this was invisible until now.

`ProcessPayload` recognised the handshake commands **by position** (`At == 0`
for INIT_GAME, `At == 8` for RIGHTS). Those offsets are where a cold login puts
them and are not a protocol rule. The test missed, the opcode fell through to
`DecodeServerUpdate`, and the walker stopped — discarding the remaining 1920
bytes, the whole `SV_CMD_FULLSCREEN`. Hence no map and every walk refused.

Captured verbatim:
`head=A3 0A EA 03 00 00 32 00 00 64 58 7D` — 163, INIT_GAME, creature 1002,
FULLSCREEN.

## Fix

Opcodes 10 and 11 are recognised **by identity at a command boundary**,
wherever they appear. Unambiguous: `connections.hh` gives no in-game command a
value below 100 apart from login errors 20..22 and ping 30.

The rule is in `Real33DHandshakeFrame.h`, free of Unreal types, proven by
`tests/real33d_handshake_frame_tests.cpp` on a plain toolchain with the real
observed bytes. No sleep, no retry, nothing dropped, no server or layout
change. ClientCore's decoder was deliberately left alone.

The `not decoded` diagnostic now reports the **offset** too — without it,
"the server sent something new" and "this client lost its place earlier in the
same frame" are indistinguishable, and that distinction was the diagnosis.

## Live QA

Cold launch: in game, 0 diagnostics, 336 tiles, 47 commands.
Handover: in game, 0 diagnostics, 336 tiles, 51 commands.
Four chained handovers: all in game, **0 diagnostics in all four**.
Movement immediately after login on both the cold and the handover client:
four cardinals, **sent 4, accepted 4**.
Server log: one `loggt ein` per client, one `übernimmt Verbindung` per handover.
All protocol counters 0, `last_diagnostic` `none`.

## Tests

ClientCore 14/14 · native 9/9 · Unreal automation 10/10, 0 errors, 0 warnings ·
REAL33DEditor built · MSVC archive · harness **11 PASS / 0 FAIL / 0 SKIP** over
97 logs · secret_check 6/6.

## Remaining UNVERIFIED

- `ParseGameInitialMessage` in ClientCore holds the **same** positional
  assumption. It is not reached on this path, so it was left alone rather than
  changed speculatively; a future caller driving login through it needs the
  same correction.
- Only the `StopAttack` prologue was seen. `TakeOver` also calls
  `ClearRequest`, `LeaveAllChannels`, `CloseAllContainers` and `RejectTrade`
  before `SendInitGame`; under the right state any could prepend more commands.
  The fix is indifferent to how many, but only CLEAR_TARGET was observed.
- A genuine network-drop-then-reconnect was not staged; every handover was
  driven by a second client on the same account.
- Movement was verified with the four cardinals only.

## Exact next task

`CLIENTCORE-LOGIN-HANDOVER-002` if wanted: give `ParseGameInitialMessage` the
same identity-based recognition, so a caller that drives login through
ClientCore rather than walking `message_bytes` cannot hit this.

## Environment

Fusion32 started with `scripts/server/start_wsl.sh` and **stopped cleanly**
(`All services stopped cleanly`). No server data was edited.
