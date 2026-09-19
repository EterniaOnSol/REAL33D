# HANDOFF

Date/time: 2026-09-19
Agent: Claude
Role: MINIMAL PLAYABLE CHAT SURFACE
Branch: `main`
Milestone: `UNREAL-CHAT-AREA-001`
Status: **PASS** — proven live, record `evidence/clientcore/unreal-slice/chat_area_live.md`.

A distant yell is readable. A yelled from eleven fields away with no actor to
draw text above, `ResolveTalkSpeaker` returned `NoMatch`, and the operator read
it in the chat panel. B's own yell resolved to creature `1002` in the same run,
so the resolved path still works. Zero refusals; both characters were level 2.

All six steps the previous handoff listed are done: `Slate`/`SlateCore` in
`REAL33D.Build.cs`, `EReal33DTalkMode` with the wire mapping confined to
`Real33DBridge.cpp`, `EReal33DEventKind::ServerMessage`, `SReal33DChatPanel`,
deterministic focus gating, and the `AddOnScreenDebugMessage` speech fallback
removed. The counters overlay is untouched.

## Two problems behind the milestone, both fixed at the root

**The runtime was in RAM.** `/tmp` is `tmpfs` on this distribution, so the whole
runtime died whenever WSL stopped the distribution on its own — taking the
SQLite database, the generated passwords and the compiled binaries with it. The
level-2 bump applied on 2026-09-18 was gone by 2026-09-19 for this reason and
nothing else, along with the backup taken to protect it. The runtime now lives
at `/var/lib/fusion32-server-baseline-772-$UID`, on the distribution's disk.
Nine scripts and three documents were updated; see `docs/SERVER_RUNTIME.md`,
section "Why `/var/lib` and not `/tmp`".

**The characters were born at level 1.** `receiving.cc::CTalk` refuses
`TALK_YELL` below level 2, so every fresh runtime silently reopened the same
defect and the bump had to be redone by hand each time. `prepare_wsl.sh` now
seeds `tests/fixtures/usr/{1001,1002}.usr`, which are level 2.
`scripts/server/bump_level2_wsl.sh` documents where the values come from
(`crplayer.cc:162-168`, the GM first-login path) and how the 15-field Skill line
maps (`crplayer.cc:2492-2522`); it refuses to run while the server is listening,
because Game caches players in memory and writes them only on shutdown.

Seeding has a second effect worth knowing: a seeded character does not go
through Outfitwahl on first login, which the Unreal client does not implement
and which used to drop it.

## Shipped but unproven: the orbit camera

The camera used to be pinned at one hardcoded angle, so there was no way to look
at a speech tag from anywhere else. It now orbits: right mouse button held plus
mouse movement, wheel to zoom, bound in code like every other input. Yaw, pitch
and distance live in `AReal33DWorld`, which owns the limits (pitch -85 to -5,
distance 400 to 3000); the controller only converts a gesture into degrees. The
defaults reproduce the previous fixed transform exactly, so an operator who
never right-drags sees the same framing as before.

It compiles clean, zero warnings. **No live run has exercised it.** Do not claim
anything about it without one.

## Open: in-world speech, and why the fix is not obvious

    IN_WORLD_SPEECH_READABLE = NOT_PROVEN

The operator read the yells in the chat panel only. Two independent things work
against reading speech above a creature, and they pull opposite ways, which is
why this needs measuring rather than another guess:

1. The speech tag billboards in **yaw only**
   (`Real33DCreatureActor.cpp:190-198`), so it stands upright and a downward
   camera sees it foreshortened. Lowering the camera improves it.
2. There is **no horizon to lower towards**. `kTerminalWidth = 18`,
   `kTerminalHeight = 14` (`worldstate.h:19-20`) is the entire world this client
   is given; the scene is a slab about nine fields in each direction that
   travels with the player, and a shallow camera looks off its edge into empty
   space. This is the 7.72 protocol, not a loading delay, and no renderer change
   addresses it.

Giving the client a horizon means reading the server's `.sec` sector files from
`<runtime>/game/state/map` rather than relying on the protocol window. That is a
separate milestone and it is the real prerequisite for judging in-world speech
at a shallow angle.

## Environment notes

- Runtime: `/var/lib/fusion32-server-baseline-772-0`. Survives WSL shutdown.
  Only `reset_wsl.sh` discards it.
- Every `prepare_wsl.sh` mints **new random passwords** and a new RSA key, so
  `scripts/client/prepare_fusion32_ipchanger_wsl.sh` must be rerun afterwards or
  the classic client cannot connect. Player A's password must be retyped by hand.
- `run_unreal_slice.cmd` takes an account letter and logs in unattended, so both
  characters can be created without touching the classic client.
- Close the Unreal client before compiling; Live Coding holds the build.

## Qualifications that must not be quietly dropped

    IN_WORLD_SPEECH_READABLE = NOT_PROVEN
    SPEECH_LIFETIME_PARITY = NOT_PROVEN
    CONSECUTIVE_WORLD_SPEECH_PARITY = NOT_PROVEN
    EXACT_SPEECH_COLOR_PARITY = NOT_PROVEN
    TALK_MODE_PERSISTENCE = REAL33D_UI_BEHAVIOUR, not proven parity
    CHAT_HISTORY_CAPACITY = REAL33D_UI_BEHAVIOUR, not 7.72 parity
    Seeded level 2 is TEST DATA, not 7.72 parity.
