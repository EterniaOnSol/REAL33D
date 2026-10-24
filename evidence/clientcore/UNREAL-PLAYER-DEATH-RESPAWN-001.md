> Independent certification follow-up (2026-10-24): observed logical flow
> CERTIFIED; see [evidence](../certification/player-death-20261023/README.md).
> Historical source correction: Die only sets DEAD; the creature destructor
> calls Logout(30,true), then Process performs the delayed close. The audit
> below is retained as the pre-certification record, not the final live verdict.

# UNREAL-PLAYER-DEATH-RESPAWN-001 — audit

2026-10-23, America/Guatemala. Work branch
`milestone/unreal-player-death-respawn-001` from current `origin/main`
(`28b5524`). No merge to main. No push.
**No protocol, no server/data, and no ClientCore/Unreal code change.**

Goal: certify own-player death → session transition → respawn/relogin → world
recovery. This is the player-death half the death-loot milestone explicitly left
out.

STATUS = **AUDIT-COMPLETE. The whole flow is already supported by composition of
landed primitives; no protocol change, no missing decoder/builder, no code
change.** The live kill-own-character-and-relogin certification is **UNVERIFIED
LIVE** here: no Fusion32 server was started and no GUI client was driven, so
nothing below claims an own-death or a respawn was observed.

## Do not invent protocol — there is nothing to invent

There is **no death-dialog command** in 7.72. Own death is a `PLAYER_DATA`
update, a text event message, and an orderly socket close; respawn is an
ordinary fresh login.

### Server (authoritative; `reference/game`, read-only)

`TPlayer::Death` (`crplayer.cc:324-342`), reached when HP≤0:

1. `SendPlayerData(Connection)` — `SV_CMD_PLAYER_DATA 160` with HP at 0.
2. `SendMessage(Connection, TALK_EVENT_MESSAGE, "You are dead.\n")` — this is
   `SV_CMD_MESSAGE 180` + a mode byte + the string (`sending.cc:1618-1641`); the
   mode is `TALK_EVENT_MESSAGE`. There is no separate death screen on the wire.
3. `this->Connection->Die()` (`connections.cc:269-287`): sets the connection
   `CONNECTION_DEAD`, calls `Player->ClearConnection()` then
   `Player->StartLogout(false, StopFight)` — a deliberate, orderly logout that
   closes the socket.
4. `TCreature::Death()` flags `IsDead`/`LoggingOut`; `~TPlayer`
   (`crplayer.cc:243-302`) then applies the death penalty, and on a full reset
   (profession level ≤ 5, or max HP ≤ 0) zeroes `startx/starty/startz` so the
   next login spawns at the temple; the corpse+loot is created by `~TCreature`
   as audited in UNREAL-DEATH-LOOT-001.

Respawn is therefore not an in-session event: the character is logged out and
re-enters the world on the **next login** at the server-chosen position.

### ClientCore (every decoder already present)

- `SV_CMD_PLAYER_DATA 160` → `player_state.cpp` (HP/mana/level; HP 0 at death).
- `SV_CMD_MESSAGE 180` → `movement.cpp:787`, with the mode resolved
  (`movement.cpp:1364-1366`: 18 AdminMessage, **19 EventMessage**, 20 Login),
  so "You are dead." arrives as an EventMessage server message
  (`chat_log.cpp:30-54`).
- The orderly close is surfaced by the transport as
  `ConnectionState::RemoteClosed` → the bridge's `bOrderlyClose = true`.
- Relogin is the ordinary login path already certified by
  UNREAL-LOGIN-HANDOVER-001 and UNREAL-LOGIN-RECONNECT-OWNERSHIP-001:
  `SV_CMD_INIT_GAME 10`, `SV_CMD_FULLSCREEN 100`, `PLAYER_DATA/SKILLS/STATE`,
  `SET_INVENTORY 120`, all decoded and applied to a fresh `WorldState`.

### Unreal (every link already wired)

- `Real33DReconnectPolicy.h` (`ShouldReconnect`): `SessionEnd::Closed` is
  **never** retried. `SessionEndFromOrderlyClose(true) = Closed`, so a death —
  an orderly close — does **not** auto-reconnect. This is the exact rule the
  ownership milestone shipped; player death is the same `Closed` case.
- `Real33DWorldActor` on `Disconnected`/`Failed`
  (`Real33DWorldActor.cpp:349-384`) sets
  `bSessionEndedDeliberately = Event.bOrderlyClose` and **tears down every
  actor** — `Tiles/StaticSectors/Creatures` reset, `LocalCreatureId = 0`,
  `PlayerInventory`, `OpenContainers` and `Combat` reset (`:299-330`) — so a
  later relogin starts from a clean slate with no stale world.
- A fresh login (the client's one `Connect` at `BeginPlay`,
  `Real33DWorldActor.cpp:146`) recreates the local player from
  `LocalPlayerIdentified`, the world from `FULLSCREEN`, vitals from
  `PLAYER_DATA`, inventory from `SET_INVENTORY`, and combat defaults to empty.
  Movement works through the existing walk path.

## Mapping to the requested outcomes

| Outcome | Mechanism (already present) | Code-verified here |
|---|---|---|
| DEATH_SEQUENCE | `PLAYER_DATA` (HP 0) → `MESSAGE` "You are dead." → `Connection->Die()` → `~TCreature` corpse | yes (server + decoders) |
| DEATH_UI | `SV_CMD_MESSAGE 180` mode EventMessage(19); no dedicated dialog command exists; shown as a server message | yes (decoder) |
| SESSION_END | orderly close → `RemoteClosed` → `SessionEnd::Closed` → no reconnect; actors torn down | yes |
| RESPAWN_PATH / RELOGIN | fresh manual login; server places character at respawn/temple | yes (login path) |
| LOCAL_PLAYER | `LocalPlayerIdentified` on the fresh session recreates the local creature | yes |
| PLAYER_STATS | `PLAYER_DATA`/`SKILLS`/`STATE` from the fresh login | yes (decoder) |
| POSITION | `FULLSCREEN` anchor + tiles at the server respawn position | yes (decoder) |
| INITIAL_WORLD | one `INIT_GAME` + `FULLSCREEN`; world rebuilt from the clean slate | yes |
| INVENTORY_EQUIPMENT | `SET_INVENTORY` from the fresh login (post-death penalty applied server-side) | yes (decoder) |
| COMBAT_STATE | `Combat` reset to empty on teardown; fresh session has no target | yes |
| IMMEDIATE_MOVEMENT | existing walk path on the new session | yes (path) |
| RECONNECT_REGRESSION | death is the `Closed` case; ownership fix unchanged; no ping-pong | yes |
| DUPLICATE_INIT | one `Connect` → one `INIT_GAME`; the ownership fix is what prevents a death-triggered relogin loop | yes |

## Why no code was written

Every server command in the own-death and respawn/relogin flow is already
decoded, the orderly-close classification and no-reconnect rule are already
shipped and are exactly what player death needs, and the world actor already
resets on session end so a relogin is clean. No decoder or builder is missing.
Adding a bespoke "death screen" would be inventing a dialog the 7.72 protocol
does not carry; the faithful behaviour is the event message this client already
shows. So the correct outcome is **no protocol/code change**; the only edits are
this evidence file and the PARITY_MATRIX note.

One honest UX observation, not a defect: because a deliberate close is final,
relogin after death is a fresh client login (the operator relaunches/logs in
again), not an in-place respawn — which is correct, since Fusion32 closes the
socket and the player must log in again exactly as the classic client requires.

## Offline verification (this environment)

- ClientCore CTest: **14/14** (WSL g++).
- REAL33D native unit tests: **10/10** (includes `real33d_reconnect_policy_tests`).
- `regression_harness.sh`: **0 FAIL**.
- `secret_check.sh`: **6/6**.
- Unreal module build (`REAL33DEditor Win64 Development`): built clean — the
  branch is `origin/main` + this documentation only, unchanged from `28b5524`.

## Remaining UNVERIFIED LIVE

No Fusion32 server was started and no REAL33D GUI client was driven here, so none
of the following was observed; each needs the operator-driven live step
(`scripts/server/start_wsl.sh` + a human or input automation at the GUI):

- Killing one's own character legitimately and the exact death/session sequence.
- The "You are dead." message appearing and the socket closing without a
  reconnect attempt.
- The respawn/relogin recreating the local player with authoritative
  HP/mana/position, restored initial world, matching inventory/equipment, a
  cleared combat target, immediate movement, and exactly one `INIT_GAME`.
- That no reconnect/ownership regression occurs across a real death.

What is proven here is that the flow needs no new protocol and that every
decoder, the orderly-close/no-reconnect rule, and the world-reset-on-relogin
already exist and build; what is missing is a live own-death to certify, which is
the operator-driven step.
