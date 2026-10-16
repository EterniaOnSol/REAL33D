# CLIENTCORE-CHANNELS-772-001

Status: `PASS` for the deterministic protocol layer. **Not** `CERTIFIED`: no
live session has joined a channel, and nothing here renders one.

ClientCore only. No Unreal, no HUD, no input, no gameplay rule, no Fusion32
server or protocol change, no 2D, no agent, no V08 asset, and no document
another milestone owns. Base `origin/main` at
`fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, on
`milestone/clientcore-channels-772-001` in its own worktree, concurrent with
and disjoint from `milestone/unreal-world-presentation-polish-001`. No main
merge.

This layer opens, lists and closes channels. The speech that travels inside
one is `SV_CMD_TALK` / `CL_CMD_TALK`, already covered by `CHAT-772-001`.

## Layouts, traced

| Command | Source | Layout |
|---|---|---|
| `CL_CMD_GET_CHANNELS` (151) | `receiving.cc::CGetChannels` | no payload; answers `SendChannels` immediately |
| `CL_CMD_JOIN_CHANNEL` (152) | `receiving.cc::CJoinChannel` | `word Channel` |
| `CL_CMD_LEAVE_CHANNEL` (153) | `receiving.cc::CLeaveChannel` | `word Channel`; nothing is sent back |
| `CL_CMD_PRIVATE_CHANNEL` (154) | `receiving.cc::CPrivateChannel` | `string Name` |
| `SV_CMD_CHANNELS` (171) | `sending.cc::SendChannels` | `byte NumAvailable`, then that many `word Channel` + `string Name` |
| `SV_CMD_OPEN_CHANNEL` (172) | `sending.cc::SendOpenChannel` | `word Channel`, `string Name` |
| `SV_CMD_PRIVATE_CHANNEL` (173) | `sending.cc::SendPrivateChannel` | `string Name` — **no channel number** |
| `SV_CMD_OPEN_OWN_CHANNEL` (178) | `sending.cc::SendOpenOwnChannel` | `word Channel`, `string Name` — identical body to 172 |
| `SV_CMD_CLOSE_CHANNEL` (179) | `sending.cc::SendCloseChannel` | `word Channel` |

Four details the source settles, each pinned by a test:

1. **The channel list can begin with a row that is not a channel.** When
   `MayOpenChannel(Player->ID)` is true, `SendChannels` writes
   `SendWord(0xFFFF)` and the literal name `"Private Chat Channel"` as the
   first row. It is an offer to create a channel, not one that exists; every
   real channel satisfies `Channel < GetNumberOfChannels()`. The decoder
   flags it as `own_channel_offer` instead of reporting channel 65535. A
   test asserts `0xFFFE` is *not* flagged, so the boundary is exact.

2. **`SV_CMD_PRIVATE_CHANNEL` carries a name and no number.** A private
   conversation is addressed by character, not by channel, which is the one
   asymmetry in the family.

3. **`SendString` has no `0xFFFF` escape; `readString` does.** The game
   protocol's `sending.cc::SendString` always writes a plain word length, so
   the decoder reading a plain word is correct for every server string. The
   generic `TReadStream::readString` the server uses for *incoming* strings
   does honour a `0xFFFF` word followed by a quad length. The private-channel
   builder therefore clamps to `0xFFFE` bytes so it can never emit a length
   that the server would read as an escape and desynchronise on. No real
   name comes close; the clamp is asserted rather than assumed.

4. **Names are truncated by the server, not refused.** `CPrivateChannel`
   reads into `char Name[30]` via `readString(Name, sizeof(Name))`, and
   `TReadStream::readString` keeps `MaxLength - 1` bytes and skips the rest,
   so a long name silently becomes 29 characters. The builder sends what it
   was given rather than inventing a local rule.

Two server behaviours recorded but deliberately **not** decoded:

- Joining `CHANNEL_RULEVIOLATIONS` (`operate.hh:29`, value 3) does not answer
  with `SV_CMD_OPEN_CHANNEL`. `CJoinChannel` branches to
  `SendOpenRequestQueue` (174) and `SendExistingRequests`. The request-queue
  family stays undecoded, and a test asserts 174–177 still report
  `Unsupported`, so this milestone cannot be read as covering them.
- `CJoinChannel` silently ignores a channel at or beyond
  `GetNumberOfChannels()`. That count is runtime server state, so no range is
  enforced client-side.

## Architecture

Ordinary ClientCore builders and `MapScanner` parsing; no raw packets.
`channels.h` sits at the `player_state.h` layer and does not include
`movement.h`, which includes it. Every builder takes scalars, so unlike
trade none had to be declared elsewhere.

**Channels store nothing in WorldState.** Fusion32 keeps the lists itself and
resends them on request, and what travels inside a channel is `SV_CMD_TALK`,
which stores nothing either. Surfaced as a typed event and ends there. A test
asserts WorldState is untouched.

## Tests

`clientcore/tests/channels_tests.cpp`, registered as
`protocol772_channels_tests` in `clientcore/CMakeLists.txt` and in
`tests/build_clientcore_windows.cmd`. The emitter in the suite is a literal
port of the five `Send*` functions, asserted against hand-computed golden hex
before it is used for anything else.

Golden byte vectors:

- `97` — get channels, no payload
- `980500` / `990300` — join 5, leave 3; plus `980000`, `98ffff`, `983412`
  for endianness and range
- `9a0300426f62` / `9a0000` — private channel "Bob", and an empty name
- `ab02ffff1400507269766174652043686174204368616e6e656c0300040048656c70`
  — the list with the `0xFFFF` "Private Chat Channel" offer first, then
  channel 3 "Help"
- `ac0300040048656c70` — open channel 3 "Help"
- `b207000300426f62` — open own channel 7 "Bob"
- `ad0300426f62` — private channel "Bob"
- `b30700` — close channel 7
- `ab00` — the empty list
- `ab0101000000` — one channel with an empty name

Decoding runs through the real `DecodeServerUpdate` dispatcher, so opcode
routing is proven and not just the body readers.

Negative and malformed cases:

- **55 truncated prefixes**: every proper prefix of all five well-formed
  messages fails as `MapDecodeError::Truncated` with `bytes_consumed == 0`
- a count promising three rows when one is present
- a channel name length that outruns the payload
- a private-channel name length that outruns the payload
- a close carrying half of its word
- five wrong opcodes handed to `DecodeChannel` (110, 170, 174, 177, 180),
  each `UnexpectedCommand`
- 174–177 still decode as `Unsupported` through the dispatcher
- output reuse: a close after a list leaves no rows; a private open after a
  close leaves no channel number

Edge cases: empty list, the `0xFFFE`/`0xFFFF` sentinel boundary, an empty
channel name, a 255-row list at the byte ceiling `SendChannels` clamps to,
and the builder clamp at `0xFFFE`.

## One existing suite changed

`player_state_tests.cpp` listed 171 among the opcodes asserted to be
unsupported, with a comment saying the list exists "so that removing an
opcode from this list cannot pass unnoticed". It did not pass unnoticed. 171
leaves the list, which becomes `{125, 126, 150, 151, 174}` — trade, the two
text editors and the request queue.

## Results

- `ctest`: **10/10 suites PASS** via WSL `Ubuntu-26.04`, g++ 15.2.0,
  `-Wall -Wextra -Wpedantic -Werror`
- ASan + UBSan rebuild of all ten: **10/10 PASS**
- `tests/regression_harness.sh --repo-root <this worktree>`:
  `PASS_WITH_SKIPS`, 6 PASS / 0 FAIL / 3 SKIP
- `build-sanity-msvc` is `SKIP`: no MSVC environment this session, so the
  Windows `/W4 /WX /permissive-` build of `channels.cpp` and its suite is
  **not claimed**. Both are registered for the next run that has one.

## Audit: how NPC shops actually work in Fusion32 7.72

Asked alongside this milestone, implemented nowhere.

**NPC buying and selling uses neither the 125–128 player-trade path nor any
shop opcode. There are no shop opcodes. It runs entirely over ordinary
chat.**

Evidence, in order:

1. **The trade path refuses non-players.** `cract.cc::TCreature::Trade`
   resolves the partner and rejects anything that is not a player before
   doing anything else:
   ```
   TPlayer *Partner = GetPlayer(PartnerID);
   if(Partner == NULL || Partner->Type != PLAYER){
       throw PLAYERNOTONLINE;
   }
   ```
   (`reference/game/src/cract.cc:612-615`.) An NPC can never be a
   `TradePartner`.

2. **Every caller of `SendTradeOffer` is player-to-player.** The only call
   sites are `cract.cc:662-664` and `:668`, inside that same `Trade`, and
   they write to `Partner->Connection` and `this->Connection` — two player
   connections. `SendCloseTrade`'s callers are `cract.cc:1435`,
   `crplayer.cc:972-998` and `operate.cc:1020`, all player lifecycle.

3. **The protocol has no shop command at all.** A case-insensitive search of
   `reference/game/src/connections.hh` for `shop`, `buy` and `sell` across
   both `ClientCommand` and `ServerCommand` returns nothing. The shop window
   that later Tibia protocols carry does not exist in 7.72.

4. **Shops are a chat-driven behaviour script.** `crnonpl.cc` implements a
   behaviour engine entered through
   `TNPC::TalkStimulus(uint32 SpeakerID, const char *Text)`
   (`crnonpl.cc:1664`), which dispatches to
   `TBehaviourDatabase::react(TNPC *Npc, const char *Text, SITUATION)`
   (`crnonpl.cc:973`, called at `:1678`, `:1681`, `:1689`). That engine
   matches words in the player's speech and carries exactly the nodes a shop
   needs: `price` and `amount` variables (`:357-362`, `:534-537`,
   `:760-766`), `countmoney` (`:557`, `:791`), `createmoney` and
   `deletemoney` (`:402-407`).

**What this means for the client.** Buying from an NPC needs no new protocol
whatsoever. The player says "hi", "trade", "buy sword"; that is `CL_CMD_TALK`
(150), which is built and certified. The NPC answers with `SV_CMD_TALK`
(170), decoded and certified by `CHAT-772-001`. The resulting item and money
changes arrive as `SV_CMD_SET_INVENTORY` / `SV_CMD_DELETE_INVENTORY` and the
container commands, all decoded and live-proven by
`UNREAL-INVENTORY-CONTAINERS-001`.

**This corrects the earlier backlog audit**, which placed NPC shopping behind
trade and rated it P0 partly on that basis. Player-to-player trade still
needs the 125–128 path and a window; NPC shopping does not, and may already
work end to end in the current client, since a chat input exists. That is a
hypothesis from source reading, **not** a live result: nobody has bought
anything from an NPC in the 3D client. Verifying it is cheap — one operator
session in Rookgaard, no code — and it should be done before any further
trade UI work is scheduled, because it changes that work's priority.

## Limits

- No live session has listed, joined, left or closed a channel, and none has
  opened a private conversation.
- `TALK_CHANNEL` speech itself is covered by `CHAT-772-001`'s deterministic
  cases only; its live coverage was blocked because the sanitized runtime has
  no channel and no gamemaster. This milestone supplies the commands that
  would create one but proves nothing new about the speech.
- The request-queue family (174–177) remains undecoded, so channel 3 has no
  working client path.
- No UI exists. The Unreal half is a separate milestone, blocked on
  `Real33DHUD.*`.
