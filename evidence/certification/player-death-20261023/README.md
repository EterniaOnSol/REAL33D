# UNREAL-PLAYER-DEATH-RESPAWN-001 independent certification

2026-10-24, America/Guatemala. **CERTIFIED for the observed logical flow.**
Audit `20db3a62e567e96595cc554a0092ff0bb30103ab` is the sole child of main
`28b5524c17b98a8e279125c4994e528b6430f380`. The audit branch was pushed normally;
both remote hashes were independently rechecked. This fresh certification
worktree contains evidence/documentation only. No production source, server
gameplay data, protocol or imported artwork was changed. No main merge or
certification push was performed.

The operator performed the legitimate combat that killed Test Player A after
offering to do so. Codex independently recorded server bytes, death/disconnect
breakpoints, UI evidence, packet replay and fresh relogin. This is an
operator-assisted live certification, not a claim of unattended gameplay.

## Observed results

| Check | Result and evidence |
|---|---|
| Death | Real rats/cave rat attacked QA A at `(32138,32241,8)`. HP fell to zero; server `TPlayer::Death` observed at `09:34:31.047729Z`. |
| Exact message | One opcode 180, mode 19 event: `You are dead.\n` at `09:34:31.058584Z`. The newline is present on the wire. Client chat and `death-message.png` show the exact phrase. |
| Session end | Server disconnect `09:35:00.698364Z`, 29.639780 seconds after the message. Client reports an orderly remote close and releases ownership. |
| Cleanup | `AfterCleanup` has zero tile/creature actors, local id zero, empty inventory/containers, neutral combat, unset origin. No locally fabricated respawn. |
| Reconnect | Zero automatic reconnect attempts. Old client remains open and disconnected through the new client's successful movement; final ownership capture `09:48:16.644Z`. |
| Fresh relogin | Explicitly launched `A-relogin`; exactly one INIT_GAME at `09:37:56.777469Z`, followed by authoritative initial map/stats/equipment. Pre-test runs are separate sessions and not duplicate INITs. |
| Player/world | Local id 1001 recreated once; world actors rebuilt and viewport synchronized. Origin agrees with server temple spawn `(32097,32219,7)`. |
| Stats/penalty | Server respawn HP 150/150, mana 0/0, level 1, experience 90; before death level 2, experience 100, HP maximum 155, mana maximum 5. HUD agrees. |
| Equipment | Server and client agree: slots 4=3379, 5=3561, 6=3272, 10=3577 x1 after relogin; backpack slot 3 lost. Later operator Use consumes slot-10 meat, correctly removing it from both server/client. |
| Combat | Neutral target/follow state after cleanup and relogin. An actively selected attack/follow target at the instant of death was not exercised. |
| Movement | Operator's first north step was requested during initial scene loading and accepted from `(32097,32219,7)` to `(32097,32218,7)` when the client became ready. Six operator steps precede the first independent relogin snapshot. Independent south step at `09:46:57` also accepted, ending `(32097,32214,7)`. |
| Ownership regression | Old client stays disconnected while the new client moves and uses an item; no ownership ping-pong, duplicate local spawn or automatic retry observed. |
| Protocol | Complete retained trace replays 3,205 commands, zero decode errors/anomalies. Live snapshot diagnostics all zero; observer errors zero. |

## Validation

`verify.py live` checks the actual recorded evidence using temporal session
boundaries, not reused connection pointers. Final result: **95 PASS / 0 FAIL**.
It compares connected snapshots' equipment to the latest authoritative replay
state, verifies cleanup, one INIT per relevant session, timestamps, stats,
position, movement and ownership overlap. `live/verification.json` records every
assertion. The snapshots named `001-relogin` are after six operator steps;
initial spawn is proved by FULLSCREEN plus presentation origin and movement log.

ClientCore **14/14**, native **10/10** (including reconnect policy and runtime
MultiUse metadata), Unreal automation **10/10**, zero automation errors/warnings.
Fresh initial Unreal build succeeded. Final build succeeded after relinking
the unchanged source with the rebuilt archive (two build actions); it was not
an up-to-date no-op. Build configuration emits a deprecated
`bAllowUBALocalExecutor` notice. Both live launches used identical audit DLL
SHA256 `44994268acdbc82acc103d11240e01c3c8096532fe4db773cf89016223aa7825`.

Final strict harness `20261024T095054Z`: **11 PASS / 0 FAIL / 0 SKIP**;
protocol scan **70 logs / 0 anomalies**; Git Bash secret_check **6/6**.
Initial and final reports/logs, automation results, and raw observation are
preserved. No requirement was replaced by a build-only claim.

## Evidence limitations and interruptions

- One real QA character death, level 2 to 1, rats/cave rat on floor 8 and temple
  respawn on floor 7. Additional levels/professions, death penalties, attackers,
  floors and repeated deaths remain **UNVERIFIED**.
- Active-target-at-death clearing remains **UNVERIFIED**; neutral state reset is
  observed, and existing reconnect/combat tests pass. No dedicated death dialog
  or death animation was added or certified. Chat displays the server event.
- Brother artwork was explicitly requested during the run. Existing 16-form
  creature and V08 world catalogs were enabled via local content junctions.
  Model/world rendering is observed, not comprehensive mesh/animation approval.
  Unsupported outfits still use placeholders. See `ART_LOADING.md`.
- The pre-test client and art sessions are retained separately. The operator
  played the art session; these actions are not claimed as independent gestures.
- WSL restarted between sessions. A verified stale PID lock was archived and
  the existing QA services restarted without editing characters or map data.
  Read-only observation resumed in the same trace with a second TRACE_READY.
- Window-focus failures safely refused input. One post-movement F9 snapshot
  (`A-relogin/002-immediate-move-after.json`) is stale: it is excluded as proof
  of that movement. Server bytes, accepted-movement log and fresh
  `003-move-confirmed` snapshots prove the step. Freshness failure is preserved.
- Copied disk `.usr` files remained the old on-disk save during the server's
  deferred save lifecycle; they are explicitly **not** used to prove post-death
  inventory or stats. Those claims use current server packets and HUD/snapshots.

Audit correction: `TConnection::Die` switches GAME to DEAD; the creature
destructor later calls `Logout(30,true)`, then Process disconnects when due.
The audit's claim that Die itself clears the connection/starts logout is
inaccurate. See `SOURCE_NOTES.md`. No product defect was demonstrated.

Both QA clients were closed normally after verification. Debugger detached;
querymanager 513, game 542 and login 1065 were healthy at final verification.
The user-authorized death penalty remains; it was not reversed with server edits.

## Reproduce and integrity

From this worktree, compile `replay.cpp` against unchanged `clientcore/src/*.cpp`
and include `clientcore/include` and `unreal/REAL33D/Source/REAL33D/Public`, with
`-std=c++17 -lcrypto -pthread`. Run the replay with the runtime's `objects.srv`
and `live/packets.tsv`; direct stdout to `live/replayed.jsonl`, then run
`python verify.py live` from this evidence directory. Runtime metadata is not
replaced by invented fixtures. Observer captures only server outputs and
allowlisted gameplay inputs; login bodies/keys/passwords are never recorded.

`manifest.json` hashes retained evidence bytes. PNGs use repository Git LFS.
Full-desktop screenshots containing unrelated applications are excluded from
the published evidence; only target-window game captures are included. The
occluded `002-immediate-move.png` is likewise excluded; original working files
remain in ignored build/certification/live.
