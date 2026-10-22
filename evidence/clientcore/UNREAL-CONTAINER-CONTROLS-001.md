# UNREAL-CONTAINER-CONTROLS-001

2026-10-22, America/Guatemala. Branch `milestone/unreal-container-controls-001`
from `origin/main` at `af251c9b2de0372c97ecbd128af0ada0c92bc166`.
No merge to main. No push. **No protocol and no server change.**

STATUS = **PASS** for Close and for everything reachable with this character.
Up, nesting and multiple windows are **UNVERIFIED LIVE** for one concrete
reason given below, not for want of trying.

## What existed before

`CL_CMD_CLOSE_CONTAINER` (135) and `CL_CMD_UP_CONTAINER` (136) were already
built in ClientCore and **used nowhere**. Closing a container went through the
7.72 use toggle, and the parent arrow was drawn dithered and inert with the
comment *"CL_CMD_UP_CONTAINER is not built yet"*.

## Implemented

- `UReal33DBridge::RequestCloseContainer` / `RequestUpContainer`, each queuing
  a `FContainerIntent` the worker drains and hands to the existing ClientCore
  builder. Two new counters, `container_closes_requested` and
  `container_ups_requested`, so a live run can count what actually went out.
- A **close** button on every container header, and the **up** arrow made live
  — still shown only when the server says there is a parent.
- Both are intents. Nothing closes or navigates locally: the panel keeps
  drawing what Fusion32 last described until SV_CMD_CLOSE_CONTAINER or
  SV_CMD_CONTAINER says otherwise.

### Routing is by container NUMBER, never by window position

The number is captured into each button's handler. Closing a container out of
the middle renumbers nothing on the server but moves every window below it, so
a button bound to a position would close the wrong container — the one way
this control can be silently and expensively wrong.

## UI corrections asked for during the work, and done

- **The "Containers" frame is gone.** A 7.72 client has no such window: each
  bag *is* its own window. Wrapping them gave every bag two headers and two
  sets of header buttons and spent a title bar plus a fixed 420px of column on
  a word naming a category rather than a thing. The stack now sizes itself and
  the column scrolls. It is still registered under the same id, so the control
  strip's toggle still works.
- **Bags are minimizable.** A per-container roll-away, keeping the header and
  the "4 of 20" footer. Purely local: a collapsed bag is still open on the
  server and is a drop target again the moment it is rolled back out. The
  collapsed set is keyed by container number and survives content rebuilds, so
  a bag does not pop open under the player's hands; it is forgotten when that
  container actually closes, so reopening gives it back expanded.
- **Trade no longer holds a column slot permanently.** It is shown only while
  `FReal33DTrade::bOpen`. It stays *docked* rather than becoming a modal for
  the reason a previous live run established — Fusion32 answers the first
  *request* with prose only, so a window keyed to that message would leave the
  recipient unable to respond — but `bOpen` is set on the first **offer of
  either side**, so the recipient still gets a window the moment anything is
  actually offered, and the column gets its space back the rest of the time.
- **Channels moved beside the console**, out of the side column. Joining a
  channel opens a tab in the chat box immediately to its left; in a column
  those two were a whole HUD apart.

## Live QA

Fusion32 from `scripts/server/start_wsl.sh`; stopped cleanly afterwards.
Account B, *Test Player B*, creature 1002.

| Row | Result |
|---|---|
| Open a real container | PASS — backpack, container 0, capacity 20 |
| Close -> correct container closes | PASS — `close container 0: request 2`, `open_containers` 1 -> 0, `container_closes_requested` 1 |
| Reopen | PASS — `open_containers` back to 1 |
| Minimise / expand | PASS — grid rolls away and back; `closes 0, ups 0` proves nothing was sent |
| Drag/drop regression | PASS — `move 4: object 3003 from container 0 slot 1 to container 0 slot 5, count 1` |
| Stack controls regression | PASS — Ctrl+drag the coin stack gave `move amount asked: 1..2, opening on 2`; Cancel gave `nothing reached Fusion32` |
| Up offered only where permitted | PASS — no arrow on the parentless backpack, which is what the server's own rule says |
| Protocol | `residual_bytes` 0, `unsupported_opcodes` 0, `protocol_anomalies` 0 on every snapshot |

## Remaining UNVERIFIED

**Up, nested navigation and multiple simultaneous containers were not
exercised live, because this character cannot produce a second container.**

That is not a guess. `sending.cc::SendContainer` sets the parent flag as:

```cpp
bool HasUpContainer = (Con.getContainer() != NONE
        && !Con.getContainer().getObjectType().isBodyContainer());
```

so a parent exists only for a container sitting inside another container that
is **not** a body container. The backpack is in body slot 3, so Fusion32
correctly reports `has_parent: false`, and the client correctly draws no
arrow — the "where server state permits" half of the requirement is confirmed,
the "returns to parent" half is not.

Producing a parent needs a second Container-flagged object to nest one inside.
Reading the server's own `dat/objects.srv` for everything this character
holds — 2854 backpack, 2815 scroll, 2920 torch, 3003 rope, 3031 gold coin,
3294 short sword, 3379 doublet, 3457 shovel, 3607 cheese — **only 2854 carries
the Container flag.** One container object cannot be nested in itself, and the
7.72 use toggle means asking to open the same object again *closes* it rather
than giving a second window: observed directly as
`use 4: ... would open as container 1` followed by `open_containers 0`.

To unblock, any second container reachable by this character would do — a bag
bought from an NPC, or a corpse or chest on the ground to nest the backpack
into. The code path is in place and routes by number; what is missing is a
live case to point it at.

Also unverified:

- A close sent for a container number the server does not have open. Fusion32
  ignores a number past its sixteen-entry table and the client never offers
  one, so this was not provoked.
- The up control's second answer — SV_CMD_CLOSE_CONTAINER when the parent is a
  map or body container — follows from the same missing case.
- Collapsed state across a reconnect was not tested; it is deliberately local
  and not persisted.
