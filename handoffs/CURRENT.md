# HANDOFF

Date/time: 2026-09-23, America/Guatemala
Task: `UNREAL-INVENTORY-CONTAINERS-001`
Agent: Claude Opus 5, driving the client and the sanitized runtime directly
Role: live certification of the milestone `723b557` left uncertified
Branch: `milestone/unreal-inventory-containers-001`
Starting commit: `723b557`
Worktree: `C:\Users\dell\Desktop\fusion32`, no worktrees, no submodules

## Objective

`723b557` ended with: *"NOT YET CERTIFIED LIVE. The container round trip this
milestone exists to unblock — use a bag, see it open, move an item inside it,
watch the update arrive, close it, open a nested one — has not been exercised
against the running server."* The brief was to exercise exactly that and
certify eight named criteria, implementing nothing unless a live failure
demonstrated a bug.

## Result

`UNREAL-INVENTORY-CONTAINERS-001 = CERTIFIED_PASS`, all eight.

```
RIGHT_CLICK_OPENS_CONTAINER      PASS   DISPLAYED_CONTENTS_MATCH_SERVER  PASS
MOVE_INSIDE_CONTAINER_LIVE       PASS   CLOSE_REMOVES_STATE_AND_PANEL    PASS
NESTED_CONTAINER_OWN_WINDOW      PASS   USE_WITH_ONE_VALID_TARGET        PASS
INVENTORY_EQUIPMENT_CORRECT      PASS   PROTOCOL_ERRORS                  0
```

Full detail, the acceptance inventory, and the repeat procedure:
`evidence/clientcore/UNREAL-INVENTORY-CONTAINERS-001.md`. Eight `F9` snapshots
in `evidence/clientcore/unreal-inventory-containers/step-*.json`; the command
log in `evidence/clientcore/UNREAL-INVENTORY-CONTAINERS-001-session.log`.

## Discoveries

- **Closing a container in 7.72 is the use toggle.** `moveuse.cc::UseContainer`
  closes a container that is already open rather than opening it twice. There is
  no client-initiated `CL_CMD_CLOSE_CONTAINER` in this client and the criterion
  does not need one. `cract.cc::NotifyGo` is the other close path: walking out
  of reach of an open container closes it.
- **A live failure: the item pictures were one sprite off.**
  `scripts/client/extract_item_sprites.py` indexed the sprite offset table with
  the sprite id. Sprite ids are one-based and zero means "no picture", so every
  object drew the sprite *after* the one it meant — invisible when the neighbour
  was another frame of the same object, glaring when it belonged to the next
  object. A bag drew as a barrel and a backpack as a desk while the server was
  plainly calling them `bag` and `backpack` on the wire. Fixed; see below.
- **The two liquid/cumulative wire classes are now live-covered.** The
  acceptance inventory carries `3603 Amount=5` and `2873 ContainerLiquidType=1`,
  so the amount byte and the liquid byte both crossed the wire and both were
  drawn. `DUAL_CLIENT_LIVE_CAPTURE` had to leave those `NOT_LIVE_COVERED`
  because every TypeId it saw was zero-extra-byte.
- **`dat/moveuse.dat` is where a valid use-with lives.** Only 98 distinct object
  types appear as `Obj2` in a `MultiUse` rule. The one pair whose two ends both
  fit inside a bag is flour on a bucket of water.

## Changes

| File | Why |
| --- | --- |
| `unreal/.../Real33DWorldActor.cpp` | The evidence snapshot now carries the inventory by slot and every open container with its objects, plus the say/move/use counters the stats block already held. Criterion 2 is unprovable from a screenshot of 32-pixel pictures. |
| `scripts/client/extract_item_sprites.py` | The one-based sprite id fix. |
| `tests/verify_item_sprite_extraction.py` | New. Holds that extractor to `visual/tools/tibia772.py`. |
| `unreal/.../Real33DContainersPanel.cpp` | Operator request: a container draws its whole capacity, as a 7.72 client does. |
| `unreal/.../Real33DHUD.cpp` | Operator request follow-on: the containers panel is 420 rather than 200, so a backpack's twenty squares and a bag opened inside it both fit. |
| `PROJECT_STATUS.md`, `PARITY_MATRIX.md`, `.gitignore` | Status, the Inventory / Containers / Use rows, and keeping the acceptance screenshots out of the repository. |

Not touched: ClientCore, the protocol, the server, its runtime data, V08,
WideWorld, REAL33D2D.

## Tests

```
> python tests/verify_item_sprite_extraction.py
items compared: 4990   geometry + ids agree: 4990
sprite pixels compared: 8163   failures: 0   RESULT: PASS
```

Against the pre-fix indexing the same comparison reports 8163 of 8163
mismatching, so it is not a test that would have passed either way.

Unreal build: `Result: Succeeded` after each of the three edits.

## Remaining UNVERIFIED / not built

- `CL_CMD_CLOSE_CONTAINER` and `CL_CMD_UP_CONTAINER`. The parent arrow in a
  container header is drawn dithered and inert.
- `CL_CMD_USE_ON_CREATURE` and the map-field target of `CL_CMD_USE_TWO_OBJECTS`
  are built and byte-tested but were not exercised live.
- One run, one operator, one machine. No independent repetition.

## Risks

- The QA character's save (`game/state/usr/02/1002.usr`) still holds the
  acceptance inventory. The original is beside it as
  `1002.usr.bak-before-inventory-containers-<stamp>`. Restore it if the next
  task needs the original loadout.
- The acceptance run was driven with synthetic mouse and keyboard input against
  the live window. Three earlier attempts were discarded because the operator
  was clicking in the same window at the same time; the certified session is the
  one whose log contains exactly the five commands the script sent and nothing
  else.

## Next task

**Container mini-window behaviour**, requested by the operator on 2026-09-23.
The 2D client opens container windows **minimised**, **resizable**, and
**movable between columns**. None of that exists here: `SReal33DContainersPanel`
stacks fixed-height windows in one column, and `SReal33DMiniWindow` has a
collapse toggle but no size or dock state.

Files and functions to start from:

- `unreal/REAL33D/Source/REAL33D/Private/Real33DContainersPanel.cpp`,
  `SReal33DContainersPanel::MakeContainer` and `::SetContainers`
- `unreal/REAL33D/Source/REAL33D/Private/Real33DPanelChrome.cpp`,
  `SReal33DMiniWindow::Construct` and `::ToggleCollapsed`
- `unreal/REAL33D/Source/REAL33D/Private/Real33DHUD.cpp`, the two
  `MakeSideColumn` calls and `ContainersHeight`

Per-window state (rows shown, collapsed, which column) has to survive
`SetContainers` rebuilding the panel, which currently discards everything on any
content change. None of it is protocol: Fusion32 is told nothing about window
geometry.

`REAL33D-2D-BOOTSTRAP-001` remains available and unstarted; its scope is
unchanged and still recorded in
`handoffs/archive/2026-09-21_DUAL-CLIENT-ARCHITECTURE-CLOSEOUT-001.md`.

## Critical context

- Start the server with `scripts/server/start_wsl.sh`. If the game server
  refuses to start with *"Game-Server is already running, PID file exists"*
  after an unclean stop, the stale file is
  `game/state/save/game.pid` inside the runtime; delete it only after confirming
  no game process is alive. Do **not** run `reset_wsl.sh`: it wipes the
  characters and their items.
- `unreal/REAL33D/Resources/UI/Items/` is gitignored and regenerated by
  `python scripts/client/extract_item_sprites.py`. A clone with no local client
  data draws numeric type ids instead, which is the intended fallback.
- The acceptance screenshots in
  `evidence/clientcore/unreal-inventory-containers/shots/` are gitignored for
  the same reason as the pictures they contain.
