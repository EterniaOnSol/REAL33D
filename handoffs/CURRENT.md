# HANDOFF - UNREAL-CONTAINER-CONTROLS-001

Date/time: 2026-10-22, America/Guatemala
Agent / role: Claude; implementation and live QA
Starting commit: `af251c9b2de0372c97ecbd128af0ada0c92bc166` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-container-controls-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261022-unreal-login-reconnect-ownership-001.md`

CONTAINER_CONTROLS_001_STATUS = **PASS** for Close and everything reachable.
Up / nesting / multiple windows **UNVERIFIED LIVE**, for one concrete reason.
MAIN_MERGE = NONE. PUSH = NONE. PROTOCOL/SERVER CHANGE = **NONE**.

## Done

`CL_CMD_CLOSE_CONTAINER` (135) and `CL_CMD_UP_CONTAINER` (136) existed in
ClientCore and were used nowhere; the parent arrow was drawn dithered and
inert. Both are now wired through `RequestCloseContainer` /
`RequestUpContainer`, with counters so a live run can count what went out.

**Routing is by container number, never by window position.** Closing a
container out of the middle moves every window below it, and a button bound to
a position would close the wrong container.

## UI corrections asked for mid-task, and done

- The **"Containers" frame is gone**: each bag is its own window, as 7.72 has
  it. Still registered under the same id so the control strip toggle works.
- Bags are **minimizable**, per container, purely local — a collapsed bag is
  still open on the server. Keyed by number, survives rebuilds, forgotten when
  the container really closes.
- **Trade** shows only while `FReal33DTrade::bOpen`. Still docked (the prose-
  only first request is why), but `bOpen` is set on the first offer of either
  side, so the recipient still gets a window.
- **Channels** moved beside the console, out of the side column.

## Live QA

Open a real backpack, Close (`close container 0: request 2`,
`open_containers` 1 -> 0), reopen, minimise/expand (`closes 0, ups 0` — nothing
sent), drag/drop (`move 4 ... count 1`), stack controls (`move amount asked:
1..2`, Cancel sent nothing). Protocol counters 0 throughout.

## The one gap, and why

Up / nesting / multiple windows need a second container object. This character
has exactly one. `sending.cc::SendContainer` sets the parent flag only when a
container sits inside another **non-body** container, so the body-slot backpack
correctly has no parent and correctly shows no arrow. The server's own
`dat/objects.srv` says 2854 is the **only** Container-flagged object the
character holds, and the 7.72 use toggle means re-using the same object closes
it instead of opening a second window (observed: `use 4 ... would open as
container 1` then `open_containers 0`).

## Tests

ClientCore 14/14 · native 10/10 · Unreal automation 10/10, 0 errors,
0 warnings · REAL33DEditor built · MSVC archive · harness **11 PASS / 0 FAIL /
0 SKIP** over 98 logs · secret_check 6/6.

## Exact next task

`UNREAL-CONTAINER-CONTROLS-002`: get one more container into the character's
hands — a bag from an NPC, or nest the backpack into a corpse or chest on the
ground — and exercise Up, nested navigation and two open windows. The code
path is in place and routes by number; only a live case is missing.

## Environment

Fusion32 started and **stopped cleanly**. No server data edited.
