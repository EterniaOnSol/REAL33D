# UNREAL-INVENTORY-CONTAINERS-001 — live certification

Date: 2026-09-23, America/Guatemala
Branch: `milestone/unreal-inventory-containers-001`
Starting commit: `723b557`
Server: Fusion32 baseline runtime, WSL Ubuntu-26.04, ports 7171/7172/7173
Client: `unreal/REAL33D`, uncooked, 1280x720 windowed, account B

`723b557` said of itself: *"NOT YET CERTIFIED LIVE. The container round trip this
milestone exists to unblock … has not been exercised against the running
server."* This is that run.

```
RIGHT_CLICK_OPENS_CONTAINER      PASS
DISPLAYED_CONTENTS_MATCH_SERVER  PASS
MOVE_INSIDE_CONTAINER_LIVE       PASS
CLOSE_REMOVES_STATE_AND_PANEL    PASS
NESTED_CONTAINER_OWN_WINDOW      PASS
USE_WITH_ONE_VALID_TARGET        PASS
INVENTORY_EQUIPMENT_CORRECT      PASS
PROTOCOL_ERRORS                  0

residual_bytes 0   unsupported_opcodes 0   protocol_anomalies 0
88 commands, 5 client commands sent, 0 failure messages
```

## How to repeat it

```bat
wsl -d Ubuntu-26.04 -e bash scripts/server/stop_wsl.sh  /mnt/c/Users/dell/Desktop/fusion32
rem install the acceptance inventory below into game/state/usr/02/1002.usr
wsl -d Ubuntu-26.04 -e bash scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32

python scripts\client\extract_item_sprites.py
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
  REAL33DEditor Win64 Development -Project="%CD%\unreal\REAL33D\REAL33D.uproject" -WaitMutex
scripts\client\run_unreal_slice.cmd B
```

Then, in the client: right-click the backpack in the body; right-click the bag
inside it; drag the apple from the bag to the backpack; shift+right-click the
flour and left-click the bucket; right-click the bag again. `F9` writes a
snapshot at any point.

### The acceptance inventory

Test Player B's save was given one backpack, so that a container can be opened
*inside* a container, and the one MultiUse pair in `dat/moveuse.dat` whose two
ends both fit in a bag. Everything else is what the character already had.

```
Inventory   = {3 Content={2854 Content={2853 Content={2920, 3270, 3585 Amount=1}, 3603 Amount=5, 2873 ContainerLiquidType=1}},
               4 Content={3562}}
```

`2854` backpack, `2853` bag, `2920` torch, `3270` club, `3585` red apple,
`3603` flour, `2873` bucket with `ContainerLiquidType=1`, `3562` coat. The
previous save is kept beside it as
`1002.usr.bak-before-inventory-containers-<stamp>`.

## The eight criteria

Snapshots are in `evidence/clientcore/unreal-inventory-containers/step-*.json`,
written by `F9`. The command log is
`evidence/clientcore/UNREAL-INVENTORY-CONTAINERS-001-session.log`.

### 1. Right-click a container, it opens

```
use 1: object 2854 at body 0 slot 3, would open as container 0
```

`step-1` / `step-2` show `open_containers` going from 0 to 1 with container
number 0 named `backpack`, capacity 20.

The last byte of `CL_CMD_USE_OBJECT` is the open-container slot, and the client
picked 0 because nothing was open. The server accepted it and answered with
`SV_CMD_CONTAINER`.

### 2. What is drawn is what the server sent

`step-2-open-nested-bag.json`, against the save file above:

| | server save | client |
| --- | --- | --- |
| container 0 | `2854`, capacity 20 | `backpack`, `type_id` 2854, `capacity` 20, `has_parent` false |
| its objects | `2853`, `3603 Amount=5`, `2873 ContainerLiquidType=1` | `2853`, `3603 amount 5`, `2873 liquid 1` |
| container 1 | `2853`, capacity 8 | `bag`, `type_id` 2853, `capacity` 8, `has_parent` true |
| its objects | `2920`, `3270`, `3585 Amount=1` | `2920`, `3270`, `3585 amount 1` |

Order, counts and the liquid byte all match, and the names are the server's own
(`SendContainer` carries the name; the client never looks them up).

This also live-covers two of the classes `DUAL_CLIENT_LIVE_CAPTURE` left as
`NOT_LIVE_COVERED`: a cumulative object with an amount byte (`3603`) and a
liquid container with a colour byte (`2873`).

### 3. Move an object inside a container, the panel follows

```
slot drop: valid=1 onto kind=2 slot=3
move 3: object 3585 from container 1 slot 2 to container 0 slot 3, count 1
```

`step-3`: the apple has left container 1 and is at **index 0** of container 0,
not index 3. `SendCreateInContainer` prepends to the server's own object list
and the client mirrors that rather than putting the object where the cursor
dropped it. Nothing moved on screen until the server said so.

### 4. Close, and the state and panel go with it

```
use 5: object 2853 at container 0 slot 2, would open as container 2
```

`UseContainer` in `moveuse.cc` closes a container that is already open rather
than opening it twice, which is the 7.72 way to close one — there is no
client-initiated `CL_CMD_CLOSE_CONTAINER` in this client and none is needed for
this path. `step-6` shows `open_containers` 2 → 1, container 1 gone from the
list entirely, container 0 untouched.

### 5. A container inside a container gets its own window

```
use 2: object 2853 at container 0 slot 0, would open as container 1
```

`step-2`: two entries, numbers 0 and 1, the second with `has_parent: true`. The
client chose 1 because 0 was taken; reusing 0 would have replaced the open
window instead of opening a second one.

### 6. Use-with, on a target Fusion32 has a rule for

```
use-with begun with object 3603; waiting for a target      (nothing sent)
use-with 4: object 3603 on object 2873 in a slot           (CL_CMD_USE_TWO_OBJECTS)
```

`dat/moveuse.dat`:

```
MultiUse, IsType (Obj1,3603), IsType (Obj2,2873),
  HasInstanceAttribute (Obj2,ContainerLiquidType,= ,1)
  -> SetAttribute(Obj2,ContainerLiquidType,0), Change(Obj1,3604,0)
```

`step-5` against `step-4`, in container 0:

- `3603 amount 5` → `3603 amount 4` plus a new `3604 amount 1`. Flour is
  cumulative, so one of the five became a lump of dough and the rest stayed
  flour. The client did not compute that; it drew what the server sent.
- `2873 liquid 1` → `2873 liquid 0`. The bucket was emptied.
- no failure message, where an unmatched pair answers `Sorry, not possible.`

### 7. Inventory and equipment stay right

All eight snapshots, start to finish: `slot 3 → 2854`, `slot 4 → 3562`, and
nothing else occupied. Opening, moving inside, using and closing containers
never disturbed the body.

### 8. Protocol errors

Zero in every snapshot: `residual_bytes 0`, `unsupported_opcodes 0`,
`protocol_anomalies 0`, over 88 commands. No `SV_CMD_*` went unparsed and no
frame left bytes behind.

## A live failure, and what it was

The operator could not tell a bag from a barrel in the container window: the
picture drawn for type 2853 was a stack of plates and 2854 was a desk, while
the server was plainly calling them `bag` and `backpack` on the wire.

`scripts/client/extract_item_sprites.py` indexed the sprite offset table with
the sprite id. Sprite ids are one-based and zero means "no picture", so every
object was drawn with the sprite *after* the one it meant. That is invisible
when the next sprite is another frame of the same object — a torch still looked
like a torch — and glaring when it belongs to the following object.

Fixed by indexing `offsets[sprite_id - 1]`, which is what
`visual/tools/tibia772.py::SpriteFile.decode` has always done. That reader is
the one whose format was established by four independent checks in
`VISUAL-REFERENCE-PACK-001`, and the repository now holds the two to each
other:

```
> python tests/verify_item_sprite_extraction.py
items compared:            4990
geometry + ids agree:      4990
sprite pixels compared:    8163
failures:                  0
RESULT: PASS
```

The same test against the pre-fix indexing reports 8163 of 8163 sprites
mismatching, so it is not a test that would have passed either way.

## Changes in this commit

- `Real33DWorldActor.cpp`: the evidence snapshot now carries the inventory by
  slot and every open container with its objects, plus the say/move/use request
  counters the stats block already held. Without it, "what is displayed matches
  the server" rests on reading type ids off 32-pixel pictures in a screenshot.
- `extract_item_sprites.py`: the one-based sprite id fix above.
- `tests/verify_item_sprite_extraction.py`: new; holds that extractor to the
  validated reader.
- `Real33DContainersPanel.cpp`: a container draws its whole capacity, as a 7.72
  client does, instead of only its contents plus one empty square. Operator
  request: a backpack holding four read as "5 slots" with no sense of how much
  room was left.
- `Real33DHUD.cpp`: the containers panel is 420 tall rather than 200, so a
  backpack's twenty squares and a bag opened inside it both fit without
  scrolling now that the grid is capacity-sized.

Not touched: ClientCore, the protocol, the server, its runtime data, V08,
WideWorld, REAL33D2D.

## Limits of this run

- One run, one operator, one machine. No independent repetition.
- The acceptance inventory was installed into the QA character's save rather
  than acquired in game. The save file is quoted above and the previous one is
  kept beside it.
- Screenshots were taken at every step and are **not** committed: they render
  item pictures cut from CipSoft client data. They are gitignored next to the
  pictures themselves. The JSON snapshots carry the type ids, which is what the
  claims above actually rest on.
- The close path exercised is the 7.72 toggle (`UseContainer` on an open
  container). `CL_CMD_CLOSE_CONTAINER` and `CL_CMD_UP_CONTAINER` are still not
  built; the parent arrow in the window header is drawn dithered and inert.
- Container windows are fixed panels stacked in a column. The 2D client opens
  them minimised, resizable and movable between columns. That is the next
  milestone, not a defect of this one.
