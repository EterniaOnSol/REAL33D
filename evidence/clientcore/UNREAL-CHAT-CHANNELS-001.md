# UNREAL-CHAT-CHANNELS-001 — real 7.72 channel UI

Task ID: `UNREAL-CHAT-CHANNELS-001`.
Branch: `milestone/unreal-chat-channels-001`, created from `origin/main` at
`ca3121c42e8db7902bd155572bcb42c3c852bb8f`. No main merge.
Worktree: `C:/Users/dell/Desktop/fusion32`.

## Scope

Add the channel UI to the Unreal client: request the channel list, show the
real list, join, leave, one view per joined channel, send and receive channel
messages, and the private-message flow where Fusion32 supports it. Local Chat
is preserved. Fusion32 is authoritative. No protocol or server change; the
existing ClientCore builders and decoder are used as they stand.

## Authority trace

Selected Fusion32 game source, protocol 772. Every claim below cites the file
and symbol it came from.

Client commands — `reference/game/src/receiving.cc`:

- `CGetChannels` (151) reads nothing and answers `SendChannels` immediately.
- `CJoinChannel` (152) reads a word and ignores anything at or beyond
  `GetNumberOfChannels()`. It answers `SendOpenOwnChannel` when `JoinChannel`
  reports the channel belongs to this player, `SendOpenChannel` otherwise, and
  for `CHANNEL_RULEVIOLATIONS` it answers `SendOpenRequestQueue` (174) plus
  `SendExistingRequests` instead.
- `CLeaveChannel` (153) reads a word and **answers nothing at all**.
- `CPrivateChannel` (154) reads `char Name[30]`, returns an empty name unsent,
  answers `PLAYERNOTEXISTING` through `SendResult` for an unknown character and
  a refusal message for the player's own name, and otherwise answers
  `SendPrivateChannel` with `GetCharacterName(Name)`. It calls only
  `GetCharacterID`, so the character must exist but **need not be online**.

Server commands — `reference/game/src/sending.cc`:

- `SendChannels` (171) sends a count then, when `MayOpenChannel(Player->ID)`,
  the id `0xFFFF` and the name `"Private Chat Channel"` as the first row. That
  row is an offer to create a channel, not a channel. The remaining rows are
  the channels for which `ChannelAvailable(Channel, Player->ID)` is true.
- `SendOpenChannel` (172) and `SendOpenOwnChannel` (178) have identical bodies;
  only the opcode says whose channel it is.
- `SendPrivateChannel` (173) writes a name and **no channel number**.
- `SendCloseChannel` (179) writes a number and nothing else.

Which channels exist, and what they are called — `reference/game/src/operate.cc`:

- `ChannelAvailable` gates `CHANNEL_GUILD` on the player having a guild and
  `CHANNEL_GAMEMASTER`, `CHANNEL_TUTOR` and `CHANNEL_RULEVIOLATIONS` on
  gamemaster/tutor rights; `CHANNEL_GAMECHAT`, `CHANNEL_TRADE`,
  `CHANNEL_RLCHAT` and `CHANNEL_HELP` are `return true`.
- `GetChannelName` returns `Player->Guild` for channel 0. **A channel's name is
  therefore not derivable from its number**, which is why no name anywhere in
  this milestone comes from a local table.

Speech inside a channel — `receiving.cc::CTalk` and `operate.cc`:

- `CTalk` accepts `TALK_CHANNEL_CALL` (5) with a `word Channel` tail and
  `TALK_PRIVATE_MESSAGE` (4) with a `string Addressee` tail.
- `CTalk` refuses a channel call below level 2:
  `"You may not speak into channels as long as you are on level 1."`
- `CTalk` silently rewrites a channel call to `TALK_SAY` with no addressee when
  `!ChannelSubscribed(Channel, Player->ID)`. Speaking into a channel this
  client has not joined therefore says it out loud instead.
- `operate.cc` channel delivery iterates `GetFirstSubscriber`/`GetNextSubscriber`
  and calls `SendTalk` for every subscriber — **the speaker included**, because
  the speaker is a subscriber. The line a channel tab shows is the server's own
  broadcast, not a local echo.
- `operate.cc` private delivery sends the **addressee** the talk and the
  **sender** `SendMessage(TALK_FAILURE_MESSAGE, "Message sent to %s.")`. The
  sender gets no copy of their own line. `IdentifyPlayer` throws
  `PLAYERNOTONLINE`, so a private message needs the addressee online even
  though opening the conversation does not.
- `CHANNEL_TRADE` (5) carries a two-minute cooldown per offer
  (`EarliestTradeChannelRound`), so it is a poor choice for a send/receive check.

## What was implemented

No ClientCore change. `clientcore/src/channels.cpp` already carried the four
builders and the five-command decoder, `DecodeServerUpdate` already produced
`ServerUpdateKind::Channel`, and `ApplyServerUpdate` already stores nothing for
it — Fusion32 owns the channel lists and resends them on request. Channel
membership is therefore presentation state and lives on the game thread beside
the transcripts, which is where this milestone put it.

New, Slate-free and testable without a renderer:

- `unreal/REAL33D/Source/REAL33D/Public/Real33DChatViews.h` —
  `Real33D::ChatViewSet`: the open chat surfaces, their monotonic never-reused
  ids, and the routing rule. Local Chat exists from construction and cannot be
  removed. Routing never drops a line: a channel this client is not in and an
  unnamed private line both fall back to Local Chat.

New Slate:

- `Real33DChannelsPanel.h/.cpp` — the channel list panel, registered in the HUD
  through `FReal33DPanelRegistry` under `Real33D::PanelIds::Channels`. Shows
  nothing until `SV_CMD_CHANNELS` answers, then the server's own rows in the
  server's own order. Join/Leave per row; a name box opens a private
  conversation. Three distinct empty states, because "not asked", "not yet
  answered" and "the server listed none" are three different facts.

Changed:

- `Real33DBridge.h/.cpp` — four channel event kinds, the four channel commands
  and the two addressed talk forms as intents, per-surface transcripts keyed by
  view id, and twelve channel counters. The worker turns the five decoded
  channel commands into events and flags private talk where the mode is still a
  number; the game thread opens, closes and routes.
- `Real33DChatPanel.h/.cpp` — the `console.otui` tab strip is now real: Local
  Chat first and never closable, then one tab per joined channel or private
  conversation. The transcript shown is the selected surface's own. A line is
  addressed by the tab it is typed in, so the Say/Whisper/Yell selector belongs
  to Local Chat alone and is replaced elsewhere by the destination.
- `Real33DPanelRegistry.h/.cpp` — the `Channels` panel id.
- `Real33DHUD.h/.cpp` — **two lines of layout**: one `AddPanel` call and its
  include, plus the member that holds the panel. That is the whole HUD edit,
  which is what the registry exists for.
- `Real33DPresentationTests.cpp` — two new suites and the registry suite
  extended over the new panel id.
- `tests/real33d_chat_views_tests.cpp`, added to the harness's native list.

### Two refusals this client makes, and says so

Both are client limitations, reported as such rather than as server rulings:

1. **Channel 3, `CHANNEL_RULEVIOLATIONS`.** `CJoinChannel` answers it with
   `SV_CMD_OPEN_REQUEST_QUEUE` (174), which this project does not decode. An
   undecoded command stops the payload walk and costs the rest of that packet,
   so `RequestJoinChannel` refuses the number and the row is drawn unavailable
   with the reason. Pinned to `p772::kChannelRuleViolations` by `static_assert`.
2. **The own-channel offer row.** Creating a channel takes
   `CL_CMD_OPEN_CHANNEL`, for which ClientCore has no builder. The row is shown
   because the server sent it, labelled, and not joinable.

### Deliberate boundaries

- Nothing is rendered optimistically. Join opens no tab; the tab appears when
  `SV_CMD_OPEN_CHANNEL` arrives. A sent line appears when Fusion32 broadcasts
  it back.
- **Leaving is the one exception, and it is forced:** `CLeaveChannel` answers
  nothing, so this client's own command is the only signal. The tab closes
  locally and a line already in flight lands in Local Chat.
- `SV_CMD_MESSAGE` stays in Local Chat, which is where it always went. It names
  no conversation, so routing "Message sent to X." to a private tab would mean
  guessing from English prose which tab it was about.
- An incoming private message opens its conversation
  (`PRIVATE_TAB_ON_ARRIVAL = REAL33D_UI_BEHAVIOUR`). Fusion32 announces one
  with nothing but the message, so the receiving client has to decide; the name
  used is the one the server put in the line.

## Offline results

| Check | Result |
| --- | --- |
| `clientcore-tests` | PASS, 13/13 CTest suites, GCC via WSL `Ubuntu-26.04` |
| `real33d-native-tests` | PASS, 5/5, including the new `real33d_chat_views_tests` |
| `build-sanity-msvc` | PASS, C++17 suites and the C++20 archive Unreal links |
| `unreal-module-build` | PASS, `REAL33DEditor Win64 Development` |
| Unreal automation | PASS, 6/6 Success, errors 0, warnings 0, failed/notRun/inProcess 0 |
| harness selftest | PASS, 10/10 |
| harness overall | PASS, 9 checks, 0 FAIL, 0 SKIP |
| `secret-check` | PASS, 6/6 sub-checks, from Git Bash |

Unreal automation suites: `REAL33D.Presentation.CameraAndFloors`,
`REAL33D.Presentation.ChannelsPanel`, `REAL33D.Presentation.ChatTabs`,
`REAL33D.Presentation.PanelRegistry`, `REAL33D.Presentation.SkillsPanel`,
`REAL33D.WideWorld.AuthoritativeWindow`.

### Two setup faults found and corrected, not worked around

1. The prebuilt `build/clientcore-windows/protocol772core.lib` this worktree
   carried was from 23 September and predated the channels, minimap and
   look-at-point work. The first Unreal link failed with ten unresolved
   symbols, four of them the channel builders. `tests/build_clientcore_windows.cmd`
   regenerated it and the link succeeded. The stale archive is the real reason,
   recorded rather than hidden: a worktree's `build/` is its own.
2. The harness run first reported `real33d-native-tests FAIL 4 passed, 1 failed`.
   The failure was `real33d_item_use_policy_tests` asserting on `file.good()`
   because `--objects-srv` had been given the `\\wsl.localhost\...` UNC path,
   which `to_wsl_path` passes through unchanged and which does not exist inside
   WSL. Corrected by pointing `--objects-srv` at a local copy; the rerun is the
   PASS above. The harness was not changed for this.

Also recorded: the harness selftest reports 10/10 only when `cl.exe` is on
PATH. Without it the Unreal and MSVC checks SKIP and four selftest cases fail,
because they assert on those checks' behaviour. The toolchain this machine
actually builds with is **VS2022 BuildTools MSVC 14.44.35207**; the VS 18
Community install present is missing `cl.exe` from its 14.51 toolset and
`vswhere.exe`, and cannot build. Not a code defect; a host observation.

## Live QA

Fusion32 `baseline-772-0` in WSL `Ubuntu-26.04`, all three services verified
ALIVE with pid/exe/cwd/start/port checks. Two accounts: Test Player B on the
REAL33D Unreal client (creature 1002, level 4) and Test Player A on the
original 7.72 `Tibia.exe` through the IP Changer (creature 1001, level 1). The
operator drove the GUI throughout; nothing was clicked or typed by software.

### Session 1 — 2026-10-18T09:08:47Z to 09:20:09Z, 686 s, 406 commands

Log: `unreal-chat-channels-001/session1_REAL33D.log`.
Snapshots: `session1_endplay.json`, `session1_manual.json`.
Captures: `channels_live_20261018T031330.png` (the joined state),
`channels_after_leave_20261018T031450.png` (the 2D client's Help tab).

| Step | Evidence |
| --- | --- |
| `CHANNEL_LIST` | `channel list 1 requested`, and the capture shows the four rows Fusion32 returned: `4 Game-Chat`, `5 Trade`, `6 RL-Chat`, `7 Help`. Exactly the four `ChannelAvailable` answers true for a character with no guild and no rights, and no own-channel offer row, so `MayOpenChannel` is false for this character. Nothing invented |
| `JOIN` | `join of channel 5 handed to Fusion32 as 17` then `channel 5 (Trade) opened by Fusion32`, 41 ms later. Repeated for 7: `join ... as 20` then `channel 7 (Help) opened by Fusion32`, 34 ms later. Two joins, two server confirmations, 1:1. The name in each is the server's |
| `CHANNEL_TABS` | The capture shows `Local Chat` with no close box, then `Test Player A ×` and `Trade ×` — three surfaces at once, Local Chat first. The input row reads `to Trade (channel 5)` where Say/Whisper/Yell sits on Local Chat |
| `SEND_RECEIVE` | `channel 5 talk 18: "hola" handed to Fusion32` then `talk [ChannelCall] channel 5, Test Player B: "hola"` 63 ms later — the server's own broadcast, not a local echo. Repeated on 7: `"asdasd"` sent, returned 68 ms later. 2 sent, 2 received |
| `PRIVATE_MESSAGE` | Both directions. A→B: `talk [PrivateMessage] Test Player A: "hola mi amado"` and a second, `"faltas cosas por hacer"`. B→A: `server message [FailureMessage]: Message sent to Test Player A.` — the confirmation `operate.cc` sends the sender instead of an echo, exactly as predicted |
| `LEAVE` | `leave of channel 5 handed to Fusion32 as 19`, via the Channels panel's Leave button. The row then reads Join again |
| Local Chat preserved | `talk [Say] at 32095,32204,7, Test Player B: "fasad" speaker Resolved creature 1002` — the positional path still resolves its speaker. `chat_transcript_lines: 3` in the snapshot: the login message, the private-send confirmation and the Say. The four channel and private lines are **not** there, which is the routing working |
| Cross-client | The 2D `Tibia.exe` shows its own `Trade` and `Help` tabs carrying `Test Player B: hola` and `asdasd`. Both clients received the same broadcast |

EndPlay counters: `residual_bytes 0`, `unsupported_opcodes 0`,
`protocol_anomalies 0` over 406 commands; `talk_messages_decoded 5`
(2 channel + 2 private + 1 say); `speech_shown_on_creature 1`,
`speech_shown_in_chat_area_only 4`; 28 walks requested, 28 accepted,
0 rejected, 0 unanswered. Derived from the log: 0 undecoded commands,
0 decode failures, 0 locally refused lines, 0 locally refused joins.

### A defect in the evidence, found in session 1 and fixed

Session 1 closed with **two joins and one logged leave**, and the operator
confirmed they had closed the second channel's tab themselves. The cause was
not a dropped command but a blind spot: the line
`leave of channel %d handed to Fusion32` lived in
`SReal33DChannelsPanel::HandleLeaveClicked`, so the panel's Leave button was
logged and the tab's close box — which reaches the same
`UReal33DBridge::RequestLeaveChannel` by a different route — was not.

That is exactly the kind of gap this project's evidence rules exist to catch:
the log could not distinguish "the command went out" from "the command was
dropped". Rather than reason about which it was, the log line was moved into
the bridge, where both routes pass, and session 2 re-proved the close-box path
first-hand. Two further observability gaps found at the same time were closed:
`SV_CMD_CHANNELS` now names every row it received, and the evidence snapshot
carries the twelve channel counters instead of leaving them to a log grep.

### Session 2 — 2026-10-18T09:30:47Z to 09:35Z, 287 s, 296 commands

Log: `session2_REAL33D.log`. Timeline: `session2_channel_timeline.txt`.
Snapshots: `session2_endplay.json`, `session2_manual.json`.
Capture: `session2_after_tab_close.png`.

The list is now named on arrival, so `CHANNEL_LIST` no longer rests on a
screenshot:

```
channel list: 4 "Game-Chat"
channel list: 5 "Trade"
channel list: 6 "RL-Chat"
channel list: 7 "Help"
channel list: 4 row(s) from Fusion32
```

Three channels joined and each confirmed by the server within 100 ms; one
message sent into each and each broadcast back on its own channel (58 ms,
97 ms, 112 ms); then **all three closed with the tab's close box**, and all
three logged their leave:

```
leave of channel 6 handed to Fusion32 as 9
leave of channel 4 handed to Fusion32 as 10
leave of channel 5 handed to Fusion32 as 11
```

Three joins, three leaves. The close-box path does put
`CL_CMD_LEAVE_CHANNEL` on the wire; session 1's ambiguity was the missing log
line and nothing else.

EndPlay counters:

| Counter | Value |
| --- | --- |
| `channel_lists_requested` / `_received` | 1 / 1 |
| `channel_joins_requested` / `channels_opened` | 3 / 3 |
| `channel_leaves_requested` / `channels_closed` | **3 / 0** |
| `channel_says_requested` / `channel_talk_received` | 3 / 3 |
| `talk_messages_decoded` | 5 (3 channel + 2 say) |
| `speech_shown_on_creature` / `..._in_chat_area_only` | 2 / 3 |
| `chat_transcript_lines` | 5 |
| `residual_bytes` / `unsupported_opcodes` / `protocol_anomalies` | **0 / 0 / 0** |

`channel_leaves_requested 3` against `channels_closed 0` is worth its own
line: it is the live confirmation that `CLeaveChannel` answers with nothing,
which is precisely why leaving is the one effect this client applies without
waiting. Had `SV_CMD_CLOSE_CHANNEL` been arriving, that second number would
not be zero.

The capture shows the end state: all four rows back to `Join`, the tab strip
back to `Local Chat` alone, and Local Chat holding only the two Says and the
server's messages — no channel line leaked into it across either session.

## Remaining UNVERIFIED

- **Own private channels.** `CL_CMD_OPEN_CHANNEL`, invite and exclude have no
  ClientCore builder. The offer row Fusion32 sends is drawn and refused with
  its reason, never silently hidden.
- **Channel 3 and the request queue.** `SV_CMD_OPEN_REQUEST_QUEUE` (174) and
  commands 175-177 are undecoded; joining 3 is refused client-side.
- **A guild channel (0) has never been observed live.** Neither test character
  has a guild, so "the name comes from the server" is proven by source and by
  unit test, not by a live guild row.
- **`SV_CMD_CLOSE_CHANNEL` (179) has never been observed live.** Its decode
  and the view removal are unit-tested; no live run has seen Fusion32 close a
  channel on its own, which needs a moderator excluding a player.
- **Anonymous and gamemaster channel modes** (10, 12, 14) decode but are
  unreachable with an ordinary character.
- **A second message into Trade inside two minutes** was never attempted, so
  the `EarliestTradeChannelRound` refusal path is unobserved.
- Tab-strip overflow is unstyled. Three tabs fit; nothing caps or scrolls the
  strip beyond that.
- Display lifetime and colour of a channel line remain client presentation and
  `NOT_PROVEN` against 7.72, exactly as `CHAT-772-001` left them.
