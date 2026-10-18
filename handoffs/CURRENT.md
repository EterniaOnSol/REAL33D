# HANDOFF - UNREAL-CHAT-CHANNELS-001

Date/time: 2026-10-18, America/Guatemala
Task ID: `UNREAL-CHAT-CHANNELS-001`
Agent / role: Claude; implementation and operator-assisted live QA
Starting commit: `ca3121c42e8db7902bd155572bcb42c3c852bb8f` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-chat-channels-001`
Worktree: `C:/Users/dell/Desktop/fusion32`

CHAT_CHANNELS_001_STATUS = PASS LOCAL
MAIN_MERGE = NONE. Branched from origin/main; main untouched.
PUSH = NONE. No remote operation was performed or authorized.

## Objective and allowed scope

Add the real 7.72 channel UI to the Unreal client through the existing
`FReal33DPanelRegistry`: request the channel list, show it, join, leave, one
view per joined channel, send and receive channel messages, and the
private-message flow. Preserve Local Chat. Fusion32 authoritative. No protocol
or server change; use ClientCore's existing channel builders and decoder.
Minimal HUD edits. No invented channels or messages.

## Inspection

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
`PARITY_MATRIX.md` and the prior `handoffs/CURRENT.md` (the agent-project
closure, archived intact at
`handoffs/archive/2026-10-18_REAL33D-AGENT-PROJECT-CLOSURE-001-before-chat-channels.md`).
Git state recorded: the checkout was on
`milestone/real33d-agent-veteran-play-002` at `b9ed335` with four modified
`evidence/clientcore/unreal-slice/` files.

## Discoveries worth keeping

- **ClientCore already had the whole protocol layer.** `channels.cpp` carries
  the four builders and the five-command decoder with its own CTest suite,
  `DecodeServerUpdate` already yields `ServerUpdateKind::Channel`, and
  `ApplyServerUpdate` deliberately stores nothing for it. Channel membership
  is therefore presentation state and belongs on the game thread beside the
  transcripts. No ClientCore file was changed; the suite count stays 13.
- **A channel's name is not derivable from its number.**
  `operate.cc::GetChannelName` returns the player's own guild for channel 0,
  and `ChannelAvailable` filters the list per player. Every name on screen is
  the server's.
- **Two client-side refusals, both reported as this client's own:** channel 3
  (`CHANNEL_RULEVIOLATIONS`) is answered with the undecoded
  `SV_CMD_OPEN_REQUEST_QUEUE` (174), which would stop the payload walk; and
  the own-channel offer row needs `CL_CMD_OPEN_CHANNEL`, for which ClientCore
  has no builder.
- **Leaving is the one effect not waited for.** `CLeaveChannel` answers with
  nothing at all, so the client's own command is the only signal.
- **The sender of a private message gets no echo.** `operate.cc` sends the
  addressee the talk and the sender `"Message sent to X."` via
  `SV_CMD_MESSAGE`. Channel calls, by contrast, are broadcast to every
  subscriber including the speaker, so a channel tab fills from the server.
- **Speaking into a channel needs level 2** and an unsubscribed channel call
  is silently rewritten to `TALK_SAY`. Trade (5) carries a two-minute
  per-offer cooldown.

## Changes

New: `Public/Real33DChatViews.h` (Slate-free `Real33D::ChatViewSet`),
`Public/Real33DChannelsPanel.h` + `Private/Real33DChannelsPanel.cpp`,
`tests/real33d_chat_views_tests.cpp`.

Changed: `Real33DBridge.h/.cpp` (four channel event kinds, four channel
commands and two addressed talk forms as intents, per-surface transcripts
keyed by view id, twelve counters), `Real33DChatPanel.h/.cpp` (a real tab
strip), `Real33DPanelRegistry.h/.cpp` (the `Channels` id),
`Real33DWorldActor.cpp` (the twelve counters in the evidence snapshot),
`Real33DPresentationTests.cpp` (two new suites, registry suite extended),
`Real33DHUD.h/.cpp` (**15 lines total**: one `AddPanel` call, one include, one
member), `tests/regression_harness.sh` (the new native stem),
`PARITY_MATRIX.md`, `PROJECT_STATUS.md`.

## Tests and results

| Check | Result |
| --- | --- |
| `clientcore-tests` | PASS 13/13, GCC via WSL `Ubuntu-26.04` |
| `real33d-native-tests` | PASS 5/5 |
| Unreal automation | PASS 6/6 Success, errors 0, warnings 0 |
| `build-sanity-msvc` | PASS |
| `unreal-module-build` | PASS |
| harness selftest | PASS 10/10 |
| harness overall | PASS, 9 checks, 0 FAIL, 0 SKIP |
| `secret-check` | PASS 6/6 from Git Bash |

Live: two operator-driven sessions, both with 0 unsupported opcodes, 0
protocol anomalies and 0 residual bytes. Full account in
`evidence/clientcore/UNREAL-CHAT-CHANNELS-001.md`.

## The one real defect found, and how

Session 1 ended with two joins and one logged leave. The operator confirmed
they had closed the second tab themselves, so nothing was broken on screen —
but the log could not say whether the close box had put
`CL_CMD_LEAVE_CHANNEL` on the wire or dropped it, because the log line lived
at the Channels panel's Leave button and the tab's close box reaches
`RequestLeaveChannel` by a different route. The line was moved into the
bridge, where both routes pass, and session 2 proved the close-box path:
three joins, three tab-close leaves, 3 = 3. Two further blind spots were
closed at the same time — the channel list now names every row it received,
and the evidence snapshot carries the channel counters.

## Environment facts the next agent will need

- The build toolchain on this host is **VS2022 BuildTools MSVC 14.44.35207**.
  The VS 18 Community install is missing `cl.exe` from its 14.51 toolset and
  `vswhere.exe` and cannot build; `vcvarsall.bat x64` succeeds there but
  leaves no compiler. Source the VS2022 `vcvars64.bat` before the harness.
- **The harness selftest reports 10/10 only with `cl.exe` on PATH.** Without
  it the Unreal and MSVC checks SKIP and four selftest cases fail, because
  they assert on those checks' behaviour. Six passes in a bare shell is not a
  regression.
- **`--objects-srv` must not be a `\\wsl.localhost\...` UNC path** when the
  harness runs its native tests through WSL: `to_wsl_path` passes it through
  unchanged and `real33d_item_use_policy_tests` then fails on `file.good()`.
  Use a local copy.
- **A worktree's `build/clientcore-windows/protocol772core.lib` is its own.**
  The one here was from 23 September and predated channels, minimap and
  look-at-point; the first Unreal link failed with ten unresolved symbols.
  Run `tests\build_clientcore_windows.cmd` after any ClientCore-facing change.
- The game service would not start: a stale `game.pid` for dead PID 21676 sat
  in `state/save/`. Preserved as `game.pid.stale-channels-20261018T005027Z`
  following the convention already in that directory, not deleted.

## Preserved work

`git stash` entry `preserve unrelated unreal-slice evidence edits
(pre-CHANNELS-001)` holds four `evidence/clientcore/unreal-slice/` files
modified on `milestone/real33d-agent-veteran-play-002` before this task began.
It was not dropped and does not belong to this milestone. The untracked
`agent/`, `evidence/agent/`, `evidence/clientcore/unreal-action-bars/` and
`tools/` trees belong to that branch and were not staged.

## Remaining UNVERIFIED

- **Own private channels.** `CL_CMD_OPEN_CHANNEL`, invite and exclude have no
  ClientCore builder, so the offer row Fusion32 sends is shown and refused.
- **Channel 3 and the request queue.** `SV_CMD_OPEN_REQUEST_QUEUE` (174),
  `DELETE_REQUEST` (175), `FINISH_REQUEST` (176) and `CLOSE_REQUEST` (177)
  remain undecoded; joining 3 is refused client-side.
- **A guild channel (0) has never been observed live** — neither test
  character has a guild, so the "name comes from the server" rule is proven by
  source and by unit test, not by a live guild row.
- **`SV_CMD_CLOSE_CHANNEL` (179) has never been observed live.** The decoder
  and the view removal are unit-tested, but no live run has seen Fusion32
  close a channel on its own; that needs a moderator excluding a player.
- **Anonymous and gamemaster channel modes** (10, 12, 14) are decoded but
  unreachable with an ordinary character.
- Tab-strip overflow with many channels open is unstyled; three tabs fit, and
  nothing caps or scrolls the strip beyond that.

## Exact next task

Either certify this milestone independently from a fresh worktree, or take the
request queue (174-177) in ClientCore so channel 3 stops being refused. The
files for the latter are `clientcore/include/fusion32/protocol772/channels.h`,
`clientcore/src/channels.cpp` and `reference/game/src/sending.cc`
(`SendOpenRequestQueue`, `SendExistingRequests`).
