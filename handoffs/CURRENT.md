# HANDOFF - UNREAL-LOGIN-CHARACTER-SELECT-UI-001 implementation

Date: 2026-10-24. Agent: Claude, implementation.
Role: implement the full in-client login/character-select flow.
Branch: `milestone/unreal-login-character-select-ui-001`.
Worktree: `C:/Users/dell/Desktop/fusion32` (the primary tree, not a worktree).
Starting commit: 1ac7245da68d602697c59f3241277ed43577737b (== `origin/main`).
Ending commit: this branch's single implementation commit.
Main / origin/main: 1ac7245, unchanged. No merge, no push.

## Objective

Launch → login → character list → select character → enter game →
logout/death → back to login/character select, without restarting the
application. Audit the existing login/session plumbing first, reuse the
certified protocol and session code, and do not alter the wire or the server
unless proven necessary.

**No wire or server change was needed and none was made.** Every byte this
milestone puts on a socket was already built by ClientCore and already sent by
the bridge. What changed is *when* and *how many times*.

## Inspection

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
`PARITY_MATRIX.md` and the previous `handoffs/CURRENT.md`; ran `git status`,
`git branch --show-current`, `git rev-parse HEAD`, `git remote -v`.

Audited, and reused unchanged: `clientcore/.../login.*` (request/response,
character list, MOTD, premium days, error text), `clientcore/.../gamelogin.*`
(`BuildGameLoginRequest`, `GameLoginSession`, `ParseGameInitialMessage` incl.
the `LoginError`/`WaitingList`/`PremiumNotice` text), the framing and crypto,
`Real33DReconnectPolicy.h` and its suite, `AReal33DWorld::ClearWorld`,
`FReal33DPanelRegistry`, and `p772::BuildLogoutCommand`.

## Discoveries

Six gaps, all **above** the protocol:

1. `FReal33DWorker::LoginAndEnterGame` fused both phases and took
   `Response.characters.front()` with no way to choose.
2. Credentials came only from `secrets/credentials.env`.
3. `Bridge::Connect` was the only entry point and refused while a worker
   existed — and a worker *object* outlives its thread, so after any failure
   every later attempt was a silent no-op.
4. `Disconnect()` drained the stopped worker's events into a throwaway array,
   discarding the orderly `Disconnected` a logout produces.
5. The retry decision lived in `AReal33DWorld` while the worker lived in the
   bridge, so neither could answer "is this session over for good".
6. Exit called `Disconnect()` and left the application unusable.

Server behaviour worth carrying forward: **Fusion32 refuses a re-entry while
the character is still logging out.** `connections.cc::JoinGame` turns it away
and `receiving.cc` answers `SendResult(Connection, LOGINERROR)` then
`Connection->Disconnect()` **immediately**, so the TALK_FAILURE_MESSAGE does
not survive the close. Observed live: every refusal was a bare orderly close
with no decodable message. The client's response is correct — no reconnect,
world torn down, back to the list — and pressing Enter Game again works after
the server finishes.

## Changes

New: `Public/Real33DSessionFlow.h` (six phases + `ResolveSessionEnd`, free of
Unreal types, **composed with** the unchanged reconnect policy),
`Public/Real33DLoginScreen.h` + `Private/Real33DLoginScreen.cpp`,
`Public/Real33DLoginQA.h` + `Private/Real33DLoginQA.cpp` (opt-in live driver),
`tests/real33d_session_flow_tests.cpp` (12 cases).

Modified: `Real33DBridge.h/.cpp` (session phase, credentials, character list,
`PrepareSession`/`BeginAccountLogin`/`BeginAutoLogin`/`EnterGame`/`Logout`/
`ReturnToAccountScreen`/`TickSession`, `ReapFinishedWorker`, `LastStats`,
`PendingEvents`, worker split into `AskForCharacterList` and `EnterGame`,
`InitGames` stat, login-error surfacing); `Real33DWorldActor.h/.cpp` (no
unconditional connect; reconnect bookkeeping moved to the bridge; `session`
block in the evidence JSON; `GetTileActorCount`); `Real33DPlayerController.h/
.cpp` (hosts the login screen, collapses the HUD from one rule, ticks the QA
driver); `Real33DControlPanel.h/.cpp` (Exit → `Logout()`, exposes the button);
`Real33DHUD.h` (`GetControlPanel`); `Real33DPanelRegistry.h/.cpp`
(`PanelIds::Account`, `PanelIds::CharacterSelect`);
`Real33DPresentationTests.cpp` (+2 suites); `tests/regression_harness.sh`
(+1 native suite); `PROJECT_STATUS.md`; `PARITY_MATRIX.md`.

## Tests and results

- ClientCore CTest **14/14** (unchanged; no new ClientCore suite).
- Native header-only suites **11/11**, including the new
  `real33d_session_flow_tests` (12 cases).
- Unreal automation **14/14**, including the new `REAL33D.Presentation.
  LoginScreen` and `REAL33D.Presentation.SessionPhase`.
- `REAL33DEditor Win64 Development` built; MSVC C++17 suites and the C++20
  archive built.
- **Strict** `tests/regression_harness.sh --unreal --strict`: **11 PASS /
  0 FAIL / 0 SKIP**, 125 logs scanned, OVERALL PASS. Report copied into
  evidence.
- `secret_check.sh` **6/6**; credential scan over all 37 evidence artifacts
  plus the module sources and tests: **0 occurrences** of either runtime
  password.

## Live QA

Live Fusion32 (querymanager 7173, game 7172, login 7171, all verified ALIVE).
`-real33d-login-qa -real33d-login-qa-cycles=4`, account B. **PASS, 5 of 5
sessions entered.** Cold login; wrong password refused by the server in its own
words; `1 offered [Test Player B@Fusion Test:7172]`; entry at 32110,32202,6
with 555 tiles and 555 actors; `init_games` **exactly 1** on all five entries;
Exit returned to the list in 0.07–0.10 s with `tile_actors 0`,
`creature_actors 0`; 555/568 tiles and actors rebuilt each time; a step
accepted every session; unsupported opcodes, anomalies, residual bytes and
reconnect attempts all **0**.

Displacement (a live, server-initiated orderly close, the same path a death
takes): in-game → character-select, "ownership released", `init_games 1`,
`reconnect_attempts 0`, scene emptied, held 20 s past the 5 s reconnect delay
with zero retries.

Two real defects in this milestone's own code were found by that QA and fixed:
`ReapFinishedWorker` (a second login after a refusal did nothing) and
`LastStats` (a finished session documented itself as zero of everything).

## PASS

`UNREAL-LOGIN-CHARACTER-SELECT-UI-001 = PASS` for the scope above.

## Remaining UNVERIFIED

- A literal death this run; it rests on path identity plus
  UNREAL-PLAYER-DEATH-RESPAWN-001.
- A switch between two **different** characters — this account has one, so the
  five cycles are repeated re-entries and the row rotation always read
  `row 0 of 1`.
- An account with no characters, and a list spanning more than one world.
- The game service's `SV_CMD_LOGIN_ERROR` and waiting-list paths: decoded by
  ClientCore, now surfaced by the worker, not exercised by this server.
- A lost-link reconnect: no link was broken, so 0 attempts is the correct
  observation and not evidence that reconnect was disabled.
- Premium days other than 0, and a MOTD longer than this runtime's two lines.

## Blockers and risks

None blocking. Risks: the account form is now the default when
`-real33d-account=` is absent, so any new script that expects an automatic
login must pass `-real33d-account=` or `-real33d-autologin`. Two clients at
once need separate `-abslog=` paths or they fight over one log file.

## Exact next task

Review the single implementation commit on this branch for the observed scope.
Do not merge main and do not push without explicit authorization. A
certification pass should re-run the live QA, and should additionally stage the
two things this run could not: a literal death, and an account with two
characters so a true character switch is observed.

Reproduction, commands and file inventory:
`evidence/clientcore/UNREAL-LOGIN-CHARACTER-SELECT-UI-001.md`.

## Critical context

- Start the server first:
  `wsl -d Ubuntu-26.04 -- bash scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32`,
  then `status_wsl.sh`. At the end of this run querymanager was PID 599,
  game 616 and login 835, all healthy. **Never infer runtime state from these
  old PIDs**; re-check with `status_wsl.sh`.
- Pre-existing untracked trees (`agent/`, `dev/`, `evidence/agent/`,
  `evidence/clientcore/unreal-action-bars/`, `tools/`) were left untouched and
  are not part of this commit.
- PowerShell: do **not** name a `Start-Process` argument array `$args` — it is
  an automatic variable and it silently split `-real33d-host=127.0.0.1` into
  `-real33d-host=127 .0.0.1`, which cost one QA run.
- The QA driver is opt-in and has no second path to the bridge: it presses the
  same controls a player does, and a disabled control stops it.
