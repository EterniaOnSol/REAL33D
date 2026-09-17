# UNREAL-SLICE-001, corrective certification

Supersedes the criteria 6 and 7 assessment in `UNREAL-SLICE-001.md`, which
remains in place as the historical record.

## What was wrong

The original report credited the outgoing input path — Unreal input reaching
Fusion32 — on the strength of a live session in which Player B moved. Two
separate things were wrong with that.

**The operator never controlled B from Unreal.** B moved because Player A,
driven from the original `Tibia.exe`, pushed him. In 7.72 a creature displaces
whoever stands on its destination field
(`reference/game/src/cract.cc::TCreature::Move`), so Fusion32 relocated B
authoritatively and the client followed. On the wire that arrives as an ordinary
`SV_CMD_MOVE_CREATURE` naming this client's own creature, indistinguishable
from the answer to a walk the client asked for.

That is real evidence, but of the *incoming* chain:

```text
Tibia.exe (A) -> Fusion32 push -> authoritative B position
    -> Protocol772Core -> WorldState -> Unreal presentation
```

It says nothing about whether a key press in Unreal reaches Fusion32.

**The cited numbers had no retained artifact.** Independently of the operator's
correction, the original document quoted "6 intents produced 4 accepted and 2
refused" from a snapshot that was read mid-session and then overwritten by the
next one. Every snapshot actually committed shows `steps_requested: 0`:

| Snapshot | `steps_requested` |
| --- | --- |
| `snapshot_1_baseline_both_players.json` | 0 |
| `snapshot_2_player_a_out_of_view.json` | 0 |
| `snapshot_3_player_a_returned.json` | 0 |
| `snapshot_4_after_reconnect.json` | 0, with 8 under `steps_accepted` |
| `snapshot_5_passability.json` | 0 |

`snapshot_4` is the confusion in one line: zero requests, eight moves, and a
field named `steps_accepted` reporting eight. Read plainly, that says Fusion32
accepted eight requests the client never made.

Walk commands *were* sent in two earlier sessions, by synthetic key messages
posted to the window by the agent rather than by an operator pressing a key.
Those logs live in `unreal/REAL33D/Saved/Logs/`, which is gitignored, so they
are not evidence this repository retains.

## What changed so this cannot recur

The client could not previously tell its own step from a push, because both
arrive as the same command naming the same creature. It can now, and the
distinction is enforced where it is testable rather than in a Tick function.

`clientcore/src/movement_ledger.cpp` keeps the walks this client actually put on
the wire. A local-player move is counted as this client's own only when a walk
is outstanding **and** the field the player landed on is exactly the field that
walk asked for. Everything else is an external relocation: a push, a teleport,
or anything unexplained. A push arriving while a walk is outstanding does not
consume that walk.

Five deterministic tests in `movement_tests.cpp` cover it, including
`TestLedgerNeverCountsAPushAsOurOwnStep`, which replays the exact eight-field
push against zero requests and asserts `accepted == 0` and `external == 8`.

The counters the client publishes now separate:

| Field | Meaning |
| --- | --- |
| `walks_requested_from_unreal` | commands this client put on the wire |
| `walks_accepted` | requests answered by a move to the requested field |
| `walks_rejected` | requests refused, counted from `SV_CMD_SNAPBACK` |
| `walks_unanswered` | requests that expired with no answer |
| `external_relocations` | position changes that were not ours |
| `local_player_moves_total` | every local-player move, whatever the cause |

A key press is numbered on the game thread the moment it happens, and that
`input_id` follows the request through the wire and back to the authoritative
answer. `movement_journal.jsonl` records each phase as one line, so the chain is
read rather than assumed.

## The corrective run

2026-09-17, roughly 17:23 to 17:32 local. Player A driven by the operator on
the original `Tibia.exe`; Player B driven by the operator **from the Unreal
client**, which is the thing the previous session never tested.

Artifacts, all under `evidence/clientcore/unreal-slice/`:

| File | What it holds |
| --- | --- |
| `corrective_movement_journal.jsonl` | the acceptance session: self-walks and pushes side by side |
| `corrective_rejection_journal.jsonl` | the session that walked B into a wall repeatedly |
| `corrective_confirmation_journal.jsonl` | a third session after the no-op fix, verifying the counts |
| `corrective_disconnect_at_drop.json`, `corrective_disconnect_after_cleanup.json` | criterion 10, retained this time |

Each journal is one JSON object per phase, appended as that phase happened.

Totals over the session:

| Phase | Count |
| --- | --- |
| `sent` | 19 |
| `accepted` | 18 |
| `rejected` | 1 |
| `external_relocation` | 6 |
| `unanswered` | 0 |

19 requests resolved as 18 accepted plus 1 refused, with nothing left hanging.
The 6 external relocations are counted apart and belong to no request.

### The chain, observed end to end

Every accepted step appears as three journal lines sharing one `input_id`, and
the same three lines appear in the client log. Taking input 9 as written:

```text
input 9: walk south requested from Unreal      <- the input handler ran
input 9: walk command 9 sent to Fusion32       <- ClientCore put it on the wire
input 9: Fusion32 accepted request 9,
         32096,32201,7 -> 32096,32202,7        <- the server moved us, +1 on y
```

```json
{"phase":"sent","input_id":9,"request_id":9,"direction":"south"}
{"phase":"accepted","input_id":9,"request_id":9,"direction":"south",
 "from":{"x":32096,"y":32201,"z":7},"to":{"x":32096,"y":32202,"z":7}}
```

The `from` and `to` are WorldState positions, not Actor transforms. The Actor
follows them; it never sources them.

### Accepted movement

Three consecutive south steps, pressed by the operator in the Unreal window:

| Input | Request | From | To |
| --- | --- | --- | --- |
| 8 | 8 | `32096,32200,7` | `32096,32201,7` |
| 9 | 9 | `32096,32201,7` | `32096,32202,7` |
| 10 | 10 | `32096,32202,7` | `32096,32203,7` |

Each is exactly one field on the requested axis, in the requested direction.

### Rejected movement

Input 1, north, was refused:

```text
input 1: walk north requested from Unreal
input 1: walk command 1 sent to Fusion32
input 1: Fusion32 refused request 1; position unchanged
```

`WorldState` did not advance and no `accepted` line exists for request 1. The
Actor could not have been left a field ahead, because no code path moves it on
input: the presentation only ever follows a confirmed position.

A second session pressed the point, with the operator walking B repeatedly into
a wall. `corrective_rejection_journal.jsonl`:

| Phase | Count |
| --- | --- |
| `sent` | 29 |
| `accepted` | 8 |
| `rejected` | 21 |
| `external_relocation` | 1 |
| `unanswered` | 0 |

29 requests, 8 accepted plus 21 refused, nothing unaccounted. Every refusal
leaves `from` and `to` on the same field, and the position after twenty-one
consecutive refusals is the position before the first.

### External relocation, kept separate

Six position changes carry `input_id: 0` and `request_id: 0`. Two of them are
decisive:

```json
{"phase":"external_relocation","input_id":0,
 "from":{"x":32096,"y":32200,"z":7},"to":{"x":32095,"y":32201,"z":7}}
```

That is a **diagonal** displacement. This client exposes no diagonal walk at
all, so it cannot have been requested; it can only be Fusion32 moving the
player. The ledger classified it as external without being told.

This is the same phenomenon the previous report mistook for accepted movement,
now counted in its own column.

### Criterion 7

`HUMAN-OBSERVED`, with machine corroboration.

The operator pressed south three times in the Unreal window and reported that
`Tibia.exe` showed B move. The machine record shows exactly three Unreal-
originated south walks accepted by Fusion32 in that window (inputs 8, 9, 10
above).

The Tibia side remains human observation. Nothing in this repository can
machine-certify what the original client rendered, and no artifact was invented
to suggest otherwise.

### Criterion 10, re-evidenced

The same audit that found the criterion 6 gap found a smaller one: the original
document's file table cited `unreal_slice_evidence_Disconnected.json` and
`unreal_slice_evidence_AfterCleanup.json`, which are written per run and
overwritten by the next. Neither was ever committed, so the figures quoted for
criterion 10 were also read live rather than retained. The claim was not wrong,
but nothing in the repository backed it.

The test was repeated and the artifacts kept. A real server-side drop was
induced by freezing the client past Fusion32's 90-round keepalive timeout,
which drops that client alone:

| Moment | Tile actors | WorldState tiles | Creature actors | Visible creatures |
| --- | --- | --- | --- | --- |
| at the drop, `corrective_disconnect_at_drop.json` | 445 | 445 | 4 | 4 |
| after teardown, `corrective_disconnect_after_cleanup.json` | 0 | — | 0 | — |

After cleanup the origin is unset and `connected_at_close` is false. Reconnect
attempt 1 succeeded and rebuilt the scene from nothing.

## A defect the corrective run found

The rejection session produced one `external_relocation` whose `from` and `to`
were the same field:

```json
{"phase":"external_relocation","input_id":0,"request_id":0,"direction":"none",
 "from":{"x":32096,"y":32200,"z":7},"to":{"x":32096,"y":32200,"z":7}}
```

After refusing a step the server re-announces the position, and that arrives as
a move from a field to itself. The ledger was counting it as a relocation,
inflating exactly the number this correction relies on to say how often
something other than the client moved the player.

A move that does not move is now `LocalMoveCause::NoMovement`: it is not
counted, it publishes no event, and it does not consume an outstanding request.
`TestLedgerIgnoresAMoveThatDoesNotMove` covers both halves.

Confirmed live afterwards. A third session repeated the wall-bumping with the
fix in place, `corrective_confirmation_journal.jsonl`:

| Phase | Count |
| --- | --- |
| `sent` | 19 |
| `accepted` | 8 |
| `rejected` | 11 |
| `external_relocation` | **0** |
| `unanswered` | 0 |

19 = 8 + 11, and the phantom relocations the same exercise produced before are
gone.

The same species of mistake produced two more fixes in this pass. A refusal
used to be journalled with `direction` defaulting to `0`, which reads as
`north`, so a rejection could be recorded against a direction nobody asked for;
`NoteSnapback` now returns the direction of the request it matched, and events
with no direction say `none` via an explicit `kNoDirection` sentinel rather
than defaulting into a real heading. And `sent` lines carried `0,0,0` for
position; they now carry where the player stood when the request was made.

## Corrected criteria

Only the criteria this corrective run bears on are restated. The rest stand as
recorded in `UNREAL-SLICE-001.md`.

| # | Criterion | Result | Kind | Evidence |
| --- | --- | --- | --- | --- |
| 6 | B's movement from Unreal reaches Fusion32 | `PASS` | MACHINE | 19 requests, 18 accepted, 1 refused, each joined by `input_id` from key press to authoritative position, in `corrective_movement_journal.jsonl` |
| 7 | The original client observes B's accepted movement | `PASS` | HUMAN, machine-corroborated | Operator observation of `Tibia.exe`, alongside inputs 8, 9, 10 accepted by Fusion32 |
| 8 | A refused move leaves no false actor position | `PASS` | MACHINE | Request 1 refused; no `accepted` line, `WorldState` unchanged |
| 10 (movement accounting) | A push is never counted as a self-walk | `PASS` | MACHINE | 6 external relocations at `input_id: 0`, two of them diagonal; `walks_accepted` counts only requested steps |

Criterion 12 was unchanged and still qualified at the time of this correction:
`SV_CMD_TALK` remained undecoded, was not worked on here, and was not claimed
to be solved.

**Superseded by `CHAT-772-001`**, which decoded talk from Fusion32 source truth
and demonstrated a live session using chat with zero unsupported opcodes, zero
protocol anomalies and zero residual bytes. See `CHAT-772-001.md` for what that
removal does and does not cover.

