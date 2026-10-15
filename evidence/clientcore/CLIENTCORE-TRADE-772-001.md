# CLIENTCORE-TRADE-772-001

Status: `PASS` for the deterministic protocol layer. **Not** `CERTIFIED`: no
live session has exercised a trade, and nothing in this milestone renders one.

ClientCore only. No Unreal, no HUD, no input, no gameplay rule, no Fusion32
server or protocol change, no 2D, no agent, no V08 asset. Base `origin/main`
at `fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, developed on
`milestone/clientcore-trade-772-001` in its own worktree, concurrent with and
disjoint from `milestone/unreal-world-presentation-polish-001`. No main merge.

## Scope

The four client commands and three server commands that carry a 7.72 trade,
whether between two players or with an NPC shop. This closes two thirds of
the "recognized by name but unparsed" set that `PLAYERSTATE-772-001` left
open; the text editors and the quest/channel commands remain.

## Layouts, traced

Every field below comes from the server's own reader or writer, not from a
7.72 protocol description.

| Command | Source | Layout |
|---|---|---|
| `CL_CMD_TRADE_OBJECT` (125) | `receiving.cc::CTradeObject` | `word x`, `word y`, `byte z`, `word TypeID`, `byte RNum`, `quad TradePartner` — 13 bytes with the opcode |
| `CL_CMD_INSPECT_TRADE` (126) | `receiving.cc::CInspectTrade` | `byte OwnOffer`, `byte Position` — 3 bytes |
| `CL_CMD_ACCEPT_TRADE` (127) | `receiving.cc::CAcceptTrade` | no payload — 1 byte |
| `CL_CMD_REJECT_TRADE` (128) | `receiving.cc::CRejectTrade` | no payload — 1 byte |
| `SV_CMD_TRADE_OFFER_OWN` (125) | `sending.cc::SendTradeOffer` | `string Name`, `byte ObjCount`, `ObjCount ×` `SendItem` |
| `SV_CMD_TRADE_OFFER_PARTNER` (126) | same function, other branch | identical body |
| `SV_CMD_CLOSE_TRADE` (127) | `sending.cc::SendCloseTrade` | no payload |

Three details the source settles, and which the tests pin:

1. **The inspect byte is inverted.** `CInspectTrade` reads
   `bool OwnOffer = (Buffer->readByte() == 0)`, so **zero means your own
   offer**. A `TradeOfferSide` enum exists so no caller has to remember that,
   and `TradeOfferSideByte` states it in one place.

2. **The offer object list is flat, and carries no nesting.**
   `SendTradeObjects` emits the offered object and then recurses through the
   contents of any container it finds, one `SendItem` per object in
   pre-order, while `ObjCount` is the total `CountObjects` returns for the
   same walk. A bag of three items therefore arrives as four consecutive
   objects with nothing marking where the bag ends. The decoder reports
   exactly that and reconstructs nothing; inferring a tree would mean
   guessing which types are containers and how many children each consumed.

3. **Two server-side rules are not second-guessed by the builder**, for the
   same reason `BuildUseObjectCommand` does not test MULTIUSE — the caller
   states what it means and Fusion32 rules on it:
   - `CTradeObject` returns without answering when the type
     `isMapContainer()`.
   - For a map object, `if(ObjX != 0xFFFF){ RNum = 1; }`: the server
     overwrites the stack index before resolving the object.

## Architecture

Ordinary ClientCore builders and parser. No raw packets anywhere: the
builders return byte vectors the existing session sends, and the decoder runs
through `MapScanner`'s bounds-checked reads and the shared
`ReadScannerString` / `ReadScannerByte` / `ReadScannerItem` helpers.

`trade.h` sits at the `player_state.h` layer and deliberately does not
include `movement.h`, which includes it. `BuildTradeObjectCommand` is
declared in `movement.h` beside the other builders that take a
`MoveEndpoint`, exactly as `BuildMoveObjectCommand` is, and is implemented in
`src/trade.cpp` with the rest of trade.

**Trade stores nothing in WorldState.** Fusion32 owns the trade outright: it
resends each offer in full whenever it changes and sends one close when it
ends, and never asks the client what the trade currently is. Mirroring it
would create a second copy of state the server already resends, and the flat
object list is a presentation list rather than world state. Trade is
surfaced as a typed event and ends there, exactly like the effects. A test
asserts WorldState is untouched.

## Tests

`clientcore/tests/trade_tests.cpp`, registered as
`protocol772_trade_tests` in `clientcore/CMakeLists.txt` and in
`tests/build_clientcore_windows.cmd`.

The server emitter in the suite is a literal port of `SendTradeOffer`,
`SendTradeObjects` and `SendCloseTrade`, asserted against hand-computed
golden hex **before** it is used to build anything else, because a generator
that agrees with a decoder written from the same misreading proves nothing.

Golden byte vectors:

- `7d617ddb7d07260b02e9030000` — trade a map object at (32097, 32219, 7),
  type 2854, stack 2, to partner 1001
- `7dffff410003480803ea030000` — the same from open container 1, slot 3
- `7dffff050000260b00e9030000` — from body slot 5
- `7e0000` / `7e0105` — inspect own position 0, inspect partner position 5
- `7f` / `80` — accept, reject
- `7d0300426f62026500c80007` — own offer, "Bob", a plain object and a
  cumulative one carrying its amount byte
- `7e0200416c012c0102` — partner offer, "Al", one liquid carrying its colour
- `7f` — close

Decoding runs through the real `DecodeServerUpdate` dispatcher, not just the
body reader, so the opcode routing is proven too.

Negative and malformed cases:

- **32 truncated prefixes**: every proper prefix of three well-formed
  messages must fail as `MapDecodeError::Truncated`, with
  `bytes_consumed == 0` so nothing is partially applied
- a count promising two objects when one is present
- a name length longer than the bytes that follow it
- a cumulative object whose amount byte never arrives
- a liquid object whose colour byte never arrives
- an undeclared object type, which must refuse rather than guess its length
  and desynchronise the stream (`UnknownObjectTypeId`)
- four wrong opcodes handed to `DecodeTrade` (110, 124, 128, 170), each
  `UnexpectedCommand`
- a close with an empty buffer, which is correct and not truncated — the one
  trade command that cannot be short
- output reuse: a close after an offer must leave no name or objects behind

Edge cases: an empty offer (count zero), an offer at the 255-object ceiling
`SendTradeOffer` enforces, and an empty name, which is what `SendString`
writes for a zero-length string and is not the NULL the server refuses.

## Two existing suites changed, deliberately

`movement_tests.cpp` and `player_state_tests.cpp` both used trade as their
canonical example of a named-but-unparsed opcode, and both were written to
break when that stopped being true — `player_state_tests.cpp` says so in a
comment: "so that removing an opcode from this list cannot pass unnoticed".
They did break, which is the mechanism working.

- `movement_tests.cpp`: the unsupported-opcode check moves from 125 to 150
  (`SV_CMD_EDIT_TEXT`), and gains a positive assertion that 127 now decodes
  as `Trade` — otherwise the edit would have quietly removed coverage.
- `player_state_tests.cpp`: 125 and 126 leave the `still_unsupported` list,
  which becomes `{150, 151, 171, 174}`.

## Results

- `ctest`: **10/10 suites PASS** (was 9 before this milestone added one), via
  WSL `Ubuntu-26.04`, g++ 15.2.0, `-Wall -Wextra -Wpedantic -Werror`
- ASan + UBSan rebuild of all ten: **10/10 PASS**, trade suite clean
- `tests/regression_harness.sh --repo-root <this worktree>`:
  `PASS_WITH_SKIPS`, 6 PASS / 0 FAIL / 3 SKIP. `clientcore-tests` PASS with
  10 suites, `real33d-native-tests` PASS 3/3, `secret-check` PASS
- `build-sanity-msvc` is `SKIP` in this session: no MSVC environment was
  available, so the Windows `/W4 /WX /permissive-` build of `trade.cpp` and
  its suite is **not claimed**. `tests/build_clientcore_windows.cmd` now
  lists `trade` in both its source set and its suite set, so the next run
  from an x64 Native Tools prompt covers it.

## Limits

- No live trade has been performed. Nothing here proves a trade completes,
  that an NPC shop answers, or that the flat object list is enough to draw a
  trade window.
- `CL_CMD_INSPECT_TRADE` is answered through the ordinary Look path, so its
  reply is a `TALK_INFO_MESSAGE` already covered elsewhere; this milestone
  adds no evidence about that reply.
- No UI exists. The Unreal half of trade is a separate milestone and is
  blocked on `Real33DHUD.*`, which another milestone currently holds.
