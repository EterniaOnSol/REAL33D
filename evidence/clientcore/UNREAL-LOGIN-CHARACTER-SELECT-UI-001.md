# UNREAL-LOGIN-CHARACTER-SELECT-UI-001 — in-client login and character select

State: **PASS** for the scope observed below. Audit, implementation and live QA
on 2026-10-24, from `origin/main` @ `1ac7245`, branch
`milestone/unreal-login-character-select-ui-001`. No merge of main.

The client can now be launched, logged into, played, logged out of and logged
back into **without restarting the application**. Before this it could not: the
world actor connected in `BeginPlay`, the account came out of a file, the first
character was taken without asking, and when the session ended the worker
returned and the window stood over an empty scene with no way back in.

---

## 1. Audit of what already existed

Read before writing anything. Nothing below was replaced.

| Existing, reused unchanged | Where |
| --- | --- |
| Account login request/response, character list, MOTD, premium days, login error text | `clientcore/{include/fusion32/protocol772,src}/login.*` |
| Game login request, `GameLoginSession`, `ParseGameInitialMessage` (incl. `LoginError`/`WaitingList`/`PremiumNotice` text) | `clientcore/.../gamelogin.*` |
| Framing, XTEA/RSA, transport | `clientcore/.../framing.*`, `crypto.*`, `tcp_transport.*`, `framed_connection.*` |
| Reconnect-ownership rule | `unreal/.../Public/Real33DReconnectPolicy.h` + `tests/real33d_reconnect_policy_tests.cpp` |
| World teardown on a dead session | `AReal33DWorld::ClearWorld` |
| Panel visibility with one owner | `FReal33DPanelRegistry` |
| Logout command on the wire | `p772::BuildLogoutCommand`, sent by the worker as it stops |

**No wire or server change was needed and none was made.** Every byte this
milestone puts on a socket was already built by ClientCore and already sent by
the bridge; what changed is *when* and *how many times*.

Gaps the audit found, all of them above the protocol:

1. `FReal33DWorker::LoginAndEnterGame` did both phases in one shot and took
   `Response.characters.front()` with no way to choose.
2. Credentials came only from `secrets/credentials.env`; there was no input.
3. `Bridge::Connect` was the only entry point and refused while a worker
   existed — and a worker *object* outlives its thread, so after any failure
   every later attempt was a silent no-op.
4. `Disconnect()` drained the stopped worker's events into a throwaway array,
   which discarded the orderly `Disconnected` a logout produces.
5. The retry decision lived in `AReal33DWorld` while the worker lived in the
   bridge, so neither could answer "is this session over for good".
6. Exit called `Disconnect()` and left the application in an unusable state.

---

## 2. What was built

* `Real33DSessionFlow.h` — the phase model and the ending rule, free of Unreal
  types, composed with the **unchanged** `Real33DReconnectPolicy.h`.
  `tests/real33d_session_flow_tests.cpp`, 12 cases.
* `UReal33DBridge` — `PrepareSession`, `BeginAccountLogin`, `BeginAutoLogin`,
  `EnterGame`, `Logout`, `ReturnToAccountScreen`, `TickSession`, the phase, the
  character list, and the reconnect bookkeeping moved here from the world actor.
* `FReal33DWorker` — split into `AskForCharacterList` (login service, publishes
  `CharacterList` or `LoginFailed`) and `EnterGame` (game service). Same
  ClientCore calls, same framing.
* `SReal33DLoginScreen` — account form and character list, one visible at a
  time via its own `FReal33DPanelRegistry` under `PanelIds::Account` and
  `PanelIds::CharacterSelect`.
* `AReal33DPlayerController` — hosts the screen, and collapses the HUD for
  exactly the phases the screen shows, from one rule.
* `SReal33DControlPanel::HandleLogout` — `Logout()` instead of `Disconnect()`.
* `FReal33DLoginQA` — opt-in live driver (`-real33d-login-qa`) that **types
  into the real boxes and presses the real buttons** with
  `SButton::SimulateClick`. It has no second path to the bridge.

### Defects this milestone's own live QA found and fixed

* **A second login did nothing.** The finished account-login worker was never
  reaped, so `StartWorker` refused every later attempt — exactly what a player
  hits after mistyping a password. Fixed by `ReapFinishedWorker`.
* **The session's counters were lost from its own evidence file.** A logout
  reaps the worker before the snapshot is written, so the file documenting a
  finished session reported zero frames, zero commands and zero anomalies.
  Fixed by `LastStats`.
* **A refused entry read as a socket error.** Reworded from "Fusion32 closed
  this connection" to what it is.

---

## 3. Live QA — `login_qa.jsonl`

Live Fusion32 (`querymanager` 7173, `game` 7172, `login` 7171, all verified
ALIVE; see `server-status.txt`). Account B of the sanitized runtime, one
character. Driver: `-real33d-login-qa -real33d-login-qa-cycles=4`.

Result: **PASS**, 5 sessions entered of 5 expected.

| QA item | Observed |
| --- | --- |
| Cold login | Launch → `phase offline`, account form showing, character list not. |
| Wrong credentials | Fusion32 refused with **its own words**, verbatim: `Accountnumber or password is not correct.` 0 characters offered; still on the account form. |
| Character list | `1 offered [Test Player B@Fusion Test:7172]`; 1 row drawn; rows drawn == characters offered; **password box cleared** after submit. |
| Select / enter | Row 0 picked, Enter Game pressed → in game at `32110,32202,6`. |
| Game entry | 555 WorldState tiles, 555 tile actors, login screen collapsed. |
| Logout → UI | HUD Exit pressed → back on the character list in 0.07–0.10 s, list still offered. |
| Relogin, no restart | 4 further entries in the same process, all from the list. |
| Repeated switching | 5 enter/logout cycles. **One character on this account**, so these are repeated re-entries, not switches between two names — see UNVERIFIED. |
| Exactly one INIT_GAME | `init_games 1` on every one of the 5 entries; a second would have failed the run. |
| World teardown | `tile_actors 0`, `creature_actors 0`, local player gone, after every logout. |
| World rebuild | 555 / 568 tiles **and** the same number of actors on every re-entry. |
| Immediate movement | Fusion32 accepted a step in every session (`accepted 1`). |
| Protocol errors | `unsupported_opcodes 0`, `protocol_anomalies 0`, `residual_bytes 0`. |
| Reconnect | `reconnect attempts across the whole run 0`. |

### Fusion32 refuses a too-fast re-entry, and that is the server's ruling

Entering again immediately after a logout is turned away. The server says so
(`server-game-session-lines.log`):

```
game.log: Spieler Test Player B loggt gerade aus - Einloggen gescheitert.
game.log: Einloggen gescheitert.
```

`connections.cc::JoinGame` refuses a character whose `LoggingOut` has not
finished, and `receiving.cc` answers with `SendResult(Connection, LOGINERROR)`
followed immediately by `Connection->Disconnect()` — so the
`TALK_FAILURE_MESSAGE` that would have carried *"An error occured while logging
in."* does not survive the close. **Observed live: every refusal arrived as a
bare orderly close with no decodable message.**

The client's behaviour is the one being tested and it is correct: it does not
reconnect, it tears the world down, and it returns to the list. The QA driver
then presses Enter Game again — a *player* action, taken only after the refusal
has been **observed**, never on a timer — and records each one. 3–5 refusals
per cycle were recorded before the server accepted.

7.72 does define a game login-error command and ClientCore decodes its text;
the worker now surfaces it (and `WaitingList`, reported rather than queued).
That path did not fire on this runtime, so it is listed as UNVERIFIED.

---

## 4. Death → UI, and the handover/reconnect policy

A death ends as an **orderly close**: `TCreature`'s destructor calls
`Logout(30, true)` and Fusion32 closes the socket (certified under
UNREAL-PLAYER-DEATH-RESPAWN-001). `ResolveSessionEnd` is a rule about the *kind*
of ending, so a death, a logout and a displacement take one path, and
`tests/real33d_session_flow_tests.cpp::ADeathReturnsToTheCharacterList` asserts
the death and logout outcomes are identical.

A **live server-initiated orderly close** was observed for this milestone by
displacing a client (`displacement/`): client A in game, client B logged the
same character in, Fusion32 closed A's socket silently.

```
session phase: entering-game -> in-game
session phase: in-game -> character-select
Warning: session ended: Fusion32 closed this connection; not reconnecting automatically
ownership released: this client will not reconnect by itself
```

A's `AfterCleanup` snapshot: `init_games 1`, `reconnect_attempts 0`,
`tile_actors 0`, `creature_actors 0`, `protocol_anomalies 0`, phase
`character-select`. Observed for 20 s after the drop — well past the 5 s
reconnect delay — with **zero** retries. The ping-pong
UNREAL-LOGIN-RECONNECT-OWNERSHIP-001 fixed cannot recur: returning to the list
is a surface change, and nothing enters a game without the player.

**A literal death was not re-observed in this run** — see UNVERIFIED.

---

## 5. Credential safety

* `FReal33DCredentials::Clear()` **overwrites** the characters before emptying
  the string; called on logout-to-account-screen and in `Deinitialize`.
* The password is never placed in an `FReal33DEvent`. Events are copied,
  queued, drained and written into evidence files; a credential in one would
  reach all of that.
* No format string in the bridge, the worker, the screen or the QA driver takes
  the password.
* **CORRECTED.** This section used to read "The account *number* is logged; the
  password is not", and that was the defect the independent certification
  (`certification/login-ui-independent-20261025`) failed this milestone on: the
  raw runtime account id appeared four times in the retained raw text. The
  account number is the long-lived half of the same credential and is now
  treated exactly as the password is — see `Real33DCredentialLog.h` and
  `evidence/clientcore/unreal-login-credential-logging-20261026/`. The log
  lines in the artifacts below are redacted to `[REDACTED_ACCOUNT_B_ID]`; the
  diagnostics around them are untouched.
* **CORRECTED.** `credential-scan.txt` below searched for the two runtime
  *passwords* only, which is why it returned PASS over artifacts that carried
  the account id. Its replacement, `tests/scan_credential_values.py`, searches
  for both ids and both passwords and is run by the harness as
  `credential-value-scan`.
* The password box is `IsPassword(true)` and is cleared the instant the bridge
  has been handed the value — observed live (`password box cleared yes`).
* The QA driver reads the runtime file, types it, and overwrites its own copy
  when the run ends. Its wrong-password literal is `not-the-password-qa`.
* `tests/secret_check.sh`: see the harness report.

Grep of all 27 artifacts this milestone wrote, plus the whole Unreal module
source tree and `tests/`, for **both** runtime passwords returned zero
occurrences; see `credential-scan.txt`. The scan reads the secret from the
runtime file and never prints it.

---

## 6. Files

| File | Purpose |
| --- | --- |
| `login_qa.jsonl` | The scripted live run, one JSON object per event. |
| `login-qa-run.log` | The client's stdout for that run. |
| `unreal_slice_evidence*.json` | Snapshots at the session's end and after cleanup. |
| `movement_journal.jsonl` | Key press → wire → server answer, per step. |
| `screens/01-account-form.png` | The account form at launch. |
| `screens/02-character-list-after-logout.png` | The list after a logout, message `this client logged out`. |
| `displacement/clientA/*` | The displaced client's log and snapshots. |
| `server-login-tail.log`, `server-game-session-lines.log`, `server-status.txt` | Fusion32's own side. |
| `credential-scan.txt` | Proof no artifact here carries the password. Passwords only — see the correction in section 5. |
| `REGRESSION-HARNESS-20261024T225025Z.md` | Strict harness: 11 PASS / 0 FAIL / 0 SKIP over 125 logs. |

## 7. Reproduction

```powershell
wsl -d Ubuntu-26.04 -- bash scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl -d Ubuntu-26.04 -- bash scripts/server/status_wsl.sh /mnt/c/Users/dell/Desktop/fusion32

"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" `
  unreal\REAL33D\REAL33D.uproject -game -windowed -ResX=1280 -ResY=720 `
  "-real33d-runtime=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0" `
  -real33d-account=B -real33d-host=127.0.0.1 -real33d-loginport=7171 `
  "-real33d-evidence=<dir>" -real33d-login-qa -real33d-login-qa-cycles=4 `
  -real33d-login-qa-quit -log -stdout -unattended=0
```

Launch **without** `-real33d-account=` to get the account form and type into it.
`-real33d-account=` keeps every existing QA script working: it drives the same
two phases through the same workers and only supplies the two answers a human
would otherwise give.

Put each client's `-abslog=` somewhere of its own when running two at once, or
they fight over one log file.

## 8. UNVERIFIED

* **A literal death in this run.** Death reaches the UI by the same orderly
  close a displacement does, proven live here and asserted in the flow tests,
  and the death itself is certified under UNREAL-PLAYER-DEATH-RESPAWN-001 — but
  no character was killed on 2026-10-24.
* **Switching between two different characters.** This account has one. The
  row-rotation code runs every cycle (`row 0 of 1`) and the screen's selection
  is tested against a two-row list in the automation suite, but a live switch
  between two names was not observed.
* **An account with no characters**, and **more than one world** in a list.
  Both are handled and unit-tested; neither exists on this runtime.
* **The game service's `SV_CMD_LOGIN_ERROR` and waiting-list paths.** Decoded
  by ClientCore, now surfaced by the worker, not exercised by this server.
* **A lost-link reconnect.** Unchanged and still covered by its own suites; no
  link was broken during this run, so the retry did not fire (0 attempts, which
  is the correct observation for a run with no losses).
* **Premium days other than 0**, and a **multi-line MOTD beyond the two lines**
  this runtime sends.
