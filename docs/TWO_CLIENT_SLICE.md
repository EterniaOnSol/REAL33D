# Two-Client Vertical Slice

Task: `TWO-CLIENT-VERTICAL-SLICE-001`

Status: `PASS` for one bounded live run with two distinct synthetic characters
in the same sanitized Fusion32 world, plus the deterministic fixtures that cover
the same transitions offline. Unreal, combat, chat, inventory and containers
remain out of scope.

## What the slice proves

```text
Tibia 7.72 original (Player A) <-> Fusion32 <-> Protocol772Core (Player B)
```

Both clients were connected to the same world at the same time. Player A ran the
selected local `Tibia.exe` patched by the Fusion32 IP Changer; Player B ran
Protocol772Core. Neither side was simulated and no second `ClientCore` stood in
for the original client.

## Protocol surface

No new opcode was needed, and none was added. Every transition the slice
requires was already decoded:

| Transition | Server path | Command reaching B |
| --- | --- | --- |
| A enters the world | `cract.cc::SetOnMap` -> `operate.cc::Create` -> `AnnounceChangedObject(OBJECT_CREATED)` -> `AnnounceChangedField` | `SV_CMD_ADD_FIELD` carrying a word-97 creature descriptor |
| A walks | `operate.cc::Move` -> `AnnounceMovingCreature` | `SV_CMD_MOVE_CREATURE` |
| A turns | `cract.cc::TCreature::Rotate` -> `AnnounceChangedObject(OBJECT_CHANGED)` | `SV_CMD_CHANGE_FIELD` |
| A leaves the viewport or logs out | `sending.cc::SendMoveCreature` third branch, or `operate.cc::Delete` -> `AnnounceChangedObject(OBJECT_DELETED)` | `SV_CMD_DELETE_FIELD` |
| B walks | `receiving.cc::CGoDirection` -> `TCreature::Go` -> `Move` | `SV_CMD_MOVE_CREATURE` plus `SV_CMD_ROW_*` to B, and `SV_CMD_MOVE_CREATURE` to A |
| B's step is refused | `TCreature::Go` throws `MOVENOTPOSSIBLE` -> `sending.cc::SendResult` | `SV_CMD_MESSAGE` plus `SV_CMD_SNAPBACK` |

## Two corrections the slice forced

Both were found by running against the real server, not by reading, and both are
covered by fixtures now.

### The known-creature mirror must survive a creature leaving the viewport

`WorldState::known_creatures` mirrors
`reference/game/src/connections.hh::TConnection::KnownCreatureTable`. The client
used to erase a creature from that mirror when `SV_CMD_DELETE_FIELD` removed it
from a tile. That is wrong: the server sends the same command whether a creature
scrolled out of view or was destroyed, and its own table only frees a slot in
`~TCreature` or when `NewKnownCreature` reuses it.

The consequence was visible within seconds of connecting. A rabbit wandered out
of view and back:

```text
EVENT  creature GONE     id=1073743181 name=[] last seen at (32094,32227,6)
EVENT  creature APPEARED id=1073743181 name=[] at (32094,32227,6) stack=1
ANOMALY UnknownCreatureReference creature=1073743181
```

The reappearance arrived as a word-98 or word-99 descriptor, which carries no
name because the server assumes the client still knows the creature. Having
dropped it, the client no longer did. `SV_CMD_DELETE_FIELD` now removes the
creature from the map and leaves the mirror alone;
`WorldState::visible_creature_ids()` is what answers "what is actually on the
map".

### A word-97 that evicts the id it introduces is not an eviction

`~TCreature` sets every knowing connection's slot to `KNOWNCREATURE_FREE` but
leaves its `CreatureID` in place, and a player keeps its `CreatureID` across
logins because `TCreature::SetID` assigns `CreatureID = CharacterID`. On a relog
`NewKnownCreature` therefore finds and reuses that very slot, and the word-97
descriptor reports `removed_creature_id == creature_id`.

That is the server refreshing its own entry, not evicting another creature.
Reporting `UnknownEvictedCreature` for it was a false positive on every relog.

## Keepalive is mandatory for a sustained session

`reference/game/src/connections.cc::TConnection::Process` disconnects a client
whose last command is 90 rounds old, and `reference/game/src/main.cc` advances
one round per second. The server sends `SV_CMD_PING` at 30 and 60 seconds of
silence but does not require a reply; what resets the timer is any client
command, and `TConnection::ResetTimer` explicitly accepts `CL_CMD_PING`.

A client that only listens is therefore dropped after 90 seconds. Player B sent
`CL_CMD_PING` every 20 seconds and stayed connected for the whole 19-minute run.

Note that `ResetTimer` does not refresh `TimeStampAction` for a ping, so pings
keep the connection alive without defeating the 15-minute idle warning and
16-minute idle logout.

## Live run

Full transcript and per-transition analysis in
`evidence/clientcore/TWO-CLIENT-VERTICAL-SLICE-001.md`. Summary:

| Transition | Evidence |
| --- | --- |
| A appears | `APPEARED id=1001 name=[Test Player A] at (32098,32219,7)` one SQM east of B |
| A walks | 11 consecutive `MOVED` events, each exactly one SQM on one axis |
| Viewport edge | A vanished stepping from `y=32213` to `y=32212`, the exact top row of B's window (`anchor.y - 6`) |
| A returns to view | reannounced without a name and still resolved as `Test Player A` |
| B walks | two `SV_CMD_GO_*` commands accepted, anchor advanced exactly as predicted, movement seen on A's screen |
| B blocked | three refusals, each because A physically occupied the destination SQM; anchor unchanged and `synchronized=1` throughout |
| A disconnects | `GONE id=1001`, `visible_creatures` dropped to 2, no ghost |
| A reconnects | `APPEARED id=1001 name=[Test Player A]`, exactly one entry, no anomaly |

Totals: 689 commands consumed across 657 frames with **zero residual bytes,
zero anomalies and zero unsupported opcodes**.

## Player blocking is part of the result

Three of B's walk requests were refused by the server because Player A was
standing on the destination field. That is the 7.72 rule that a creature
occupies one SQM and cannot be walked through, and it is the strongest available
evidence that both clients are in one authoritative world rather than two
views of a shared map: one client's position physically constrained the other's
movement, decided entirely by Fusion32.

The refusals arrive as `SV_CMD_MESSAGE` plus `SV_CMD_SNAPBACK`, neither of which
changes map state. B's anchor and tile set were unchanged across all three, and
`viewport_synchronized()` held.

## Verification

`clientcore/tests/movement_tests.cpp::TestSecondPlayerLifecycle` covers the
whole sequence offline and deterministically: another player appears through
`ADD_FIELD`, walks through `MOVE_CREATURE`, scrolls out of view and returns
through a word-99 descriptor, disconnects through `DELETE_FIELD` and relogs
through a self-evicting word-97 — asserting at each step that this client's own
anchor never moves, that no ghost remains on the map, that the mirror keeps what
the server keeps, and that a genuine eviction of a different creature is still
reported.

CTest 7/7 `PASS` normally and under ASan/UBSan.

## Limits

One run, one operator, no independent repetition and no retained screenshots.
Both characters were on floor 7 in the Rookgaard temple area for the whole run,
so no floor transition was exercised with two clients. The operator was asked
not to use the chat, because `SV_CMD_TALK` is still undecoded and would stop the
frame walk. Nothing here proves rendering or any 2D-to-3D parity: Player B is a
protocol client with no presentation at all.
