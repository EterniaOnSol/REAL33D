# UNREAL-LOGIN-CHARACTER-SELECT-UI-001 independent review

Status: FAILED for complete certification; functional observations PASS. Task scope: independent review, fresh builds, tests, live
observations, and evidence/documentation only. No gameplay, protocol, server
source, character records, or production client changes authorized by this audit.

Main: `1ac7245da68d602697c59f3241277ed43577737b`.
Implementation: `bf54da4c576859fdd7ac7188d7a9f925e966d421`, exactly one parent,
the specified main. `git push origin <implementation>:refs/heads/milestone/unreal-login-character-select-ui-001`
returned `Everything up-to-date`; remote branch and main matched independently.
Fresh worktree: `build/cert-login-ui-independent-20261025`, branch
`certification/login-ui-independent-20261025`. An existing differently named
certification worktree was discovered and preserved; it is not this run.

## Inspected boundaries

- `Real33DBridge.cpp`: `AskForCharacterList` parses the existing ClientCore
  login response and publishes its character entries. `EnterGame` uses the
  selected name and existing game-login builder. `ProcessPayload` counts
  INIT_GAME per worker; `StartGameWorker`, `ReapFinishedWorker`, `Disconnect`,
  `DrainEvents`, `NoteSession`, and `TickSession` govern session transitions.
- `Real33DSessionFlow.h`: `MayEnterGame` admits CharacterSelect only;
  `ResolveSessionEnd` returns orderly closes to the retained character list.
  Lost connections still use the unchanged reconnect policy and budget.
- `Real33DLoginScreen.cpp`: `HandleLogin` clears the password box after
  handing credentials to the bridge; `HandleEnterGame` submits the selected
  row; `RefreshControls` disables busy controls.
- `Real33DWorldActor.cpp::ClearWorld`: destroys registered tiles, creatures,
  static sectors, and resets player identity, stats, equipment and combat.
  `HandleEvent` captures pre-cleanup and AfterCleanup evidence.
- `Real33DLoginQA.cpp`: the opt-in driver uses real Slate controls for login,
  selection/entry and logout. Movement uses RequestWalk directly; therefore
  its movement evidence does not by itself certify physical keyboard input.
  Its wrong-password step accepts any nonempty refusal; independent review
  must verify the actual server message and subsequent usable form.

## Server authority

Source provenance is `docs/protocol772/SOURCE_TRUTH.md`: selected game revision
`386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, derived/manual decompilation.
`communication.cc` selects version 772 under `TIBIA772`; none of the following
shared lifecycle functions has a separate 772 branch.

- `reference/game/src/connections.cc::JoinGame`: takeover clears the old
  connection's character ID, calls Logout(0,true), then assigns the new owner.
- `receiving.cc::ReceiveData`: failed JoinGame sends LOGINERROR then immediately
  disconnects. A bare close must not be treated as a proven decoded error text.
- `crplayer.cc::TPlayer::Death`: sends player data and the exact event
  `You are dead.\n`, then calls connection Die.
- `connections.cc::Die`: sets CONNECTION_DEAD; it does not itself respawn.
- `crmain.cc::TCreature` destructor calls Logout(30,true); Process later closes
  the connection. A takeover or test of ResolveSessionEnd is not a literal death.

## Credential finding

The requested absence of credentials includes the login account identifier.
`Real33DBridge.cpp::BeginAccountLogin` logs the numeric account; the QA driver's
`Start` records it in both the application log and login_qa.jsonl. Password
absence and account-ID absence must be evaluated separately. This report does
not reproduce either runtime account identifier or password. A successful
secret_check does not override this source finding: its checks do not prohibit
numeric account identifiers in logs.

## Environment preparation

All three QA services were initially DEAD. First start failed because
`game/state/save/game.pid` contained a stale PID. No game process or listening
QA port existed. The exact stale lock was archived, with escalation approval,
and the unchanged QA runtime restarted successfully. No character/map edits.

The initial harness used an incorrect objects.srv path and lacked the ignored
classic executable used for static byte checks. Preserve that failed run;
corrected runs use a read-only copy of runtime `game/reference/dat/objects.srv`
and the existing classic binary, which is never executed.
