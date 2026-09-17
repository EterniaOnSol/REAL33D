# HANDOFF

Date/time: 2026-09-17
Agent: Claude
Role: CHAT DECODING
Branch: `main`
Starting commit: `e5add9c`
Implementation commit: `e756bb7`
Worktree: clean after the focused chat commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, `HEAD == origin/main`

Previous handoff archived at `handoffs/archive/2026-09-17_UNREAL-SLICE-001.md`.

## Objective

`CHAT-772-001`: decode `SV_CMD_TALK` properly, from Fusion32 source truth, so
an ordinary live session may use chat without an unsupported opcode, residual
bytes or a parser desynchronisation. Not a byte skip, and not a chat UI.

## Result

`PASS`, and `UNREAL-SLICE-001` criterion 12's qualification is removed for the
tested scope.

Full record in `evidence/clientcore/CHAT-772-001.md`. Packet structure and the
source trail in `docs/protocol772/TALK.md`.

## What the source actually says

Three `SendTalk` overloads in `reference/game/src/sending.cc` all emit opcode
170 and share a head of `quad StatementID`, `string Sender`, `byte Mode`. The
tail then differs, and the mode is the only thing that says which tail it is:
the server picks an overload by argument types at the call site, and the client
has to recover that choice from the mode alone.

Two details would each have desynchronised a decoder written from assumption:

- `TALK_ANONYMOUS_CHANNELCALL` sends an **empty** sender rather than omitting
  the field. Treating anonymity as an absent field loses two bytes.
- `TALK_GAMEMASTER_REQUEST` is the one mode that inserts a quad between the
  mode and the text.

`enums.hh` also declares `ANONYMOUS_BROADCAST` 13 and `ANONYMOUS_MESSAGE` 15,
which no overload accepts, and 18..23, which belong to `SendMessage` under a
different opcode. An unrecognised mode is refused with `UnknownTalkMode` and
consumes nothing, because the tail is unlocatable and guessing would corrupt
everything after it.

## Ownership decision

Talk stores nothing in `WorldState`. Fusion32 keeps no per-connection chat
history: `SendTalk` serialises and forgets. A client-side transcript would be a
feature this client does not have, and holding one in `WorldState` would make
it look like server state comparable against a fresh `FULLSCREEN`. Talk reaches
the caller as an event and ends there, exactly like the effects.

## Tests and sanitizers

Discovered and executed by `tests/build_clientcore_windows.cmd`; these are the
totals that run reported, not figures carried forward.

| Suite | Result |
| --- | --- |
| transport | 22/22 |
| crypto | 25/25 |
| login, gamelogin, initial_world, movement, player_state, worldview | `PASS` |
| Windows MSVC `/W4 /WX /permissive-`, C++17 and C++20 | `WINDOWS CLIENTCORE: PASS` |
| WSL GCC + ASan/UBSan via CTest | 8/8 |
| `tests/secret_check.sh` | `PASS` |

Seven talk cases in `movement_tests.cpp`, including golden bytes per form,
every mode each overload accepts, all truncation lengths, and three talks
followed by a move and a ping decoded to exactly zero residual bytes.

## Two defects the tests caught

1. A hand-computed golden encoded `y = 32218` as `0xBA` where it is `0xDA`.
   The emitter was right and the golden was wrong, which is exactly why this
   suite asserts goldens before letting structural tests depend on the emitter.
2. `player_state_tests` listed opcode 170 among the commands that are *still
   unsupported* — true until this milestone. Removing it silently would have
   left nothing checking the claim, so the list now has a matching positive
   assertion that a well-formed talk decodes whole.

## Live validation

`Tibia.exe` 7.72 (Player A, operator-driven) + Fusion32 + Unreal running
Protocol772Core (Player B). Three talk commands decoded:

```text
talk [Say]     at 32097,32205,7, Test Player A: "asdasdsa"
talk [Whisper] at 32097,32205,7, Test Player A: "asadasd"
talk [Whisper] at 32097,32205,7, Test Player A: "asdasd"
```

The decoded position cross-checks against the same snapshot's creature list,
which places Player A at exactly `32097,32205,7` through an entirely separate
path. Two unrelated decoders agreeing is stronger than any assertion inside the
talk tests.

Protocol health over 221 frames and 235 commands, with chat used:
`unsupported_opcodes 0`, `protocol_anomalies 0`, `residual_bytes 0`,
`last_diagnostic none`. Talk is decoded, not filtered out to reach those zeros.

Parser continuity: two Unreal-driven walks after the chat were sent and
accepted, each exactly one field on the requested axis.

## What was not exercised live, and why

`TALK_YELL` was attempted and refused by the server: yelling is level-gated in
7.72 and both characters are level 1. That is Fusion32 being correct, not a
decoder limitation.

The channel and plain forms need a channel or a gamemaster, neither of which
this sanitized two-account runtime has. They are covered deterministically and
are not claimed as live-proven.

## Regression

Nothing from a previous milestone changed behaviour. The movement accounting
introduced after the `UNREAL-SLICE-001` correction is intact, including the
self-walk versus external-relocation distinction, which the live snapshot
exercises directly: `walks_requested 2` = `accepted 2` + `rejected 0`, with
`external_relocations 0`.

## Not done, deliberately

No chat UI, no chat window, no text above creatures, no client-side talk
sending, no channel management, no private-message handling. The Unreal module
still contains no protocol knowledge: the bridge resolves the mode to a name so
nothing above it sees a mode number.

## Suggested next milestone, not started

`CONTAINERS-772-001` or `TRADE-772-001` are the next undecoded command groups
and would continue closing the ordinary-session surface.

Alternatives: `ROOKGAARD-P0-MOCKUPS-001`, unblocked because the asset registry
can adopt approved art without touching ClientCore or any Actor; or a
floor-transition slice, which is the one movement case the 3D client has never
exercised and the one place the presentation still uses a number
(`UnitsPerFloor`) that the protocol never states.
