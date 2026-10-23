# UNREAL-DEATH-LOOT-001 — audit

2026-10-23, America/Guatemala. Work branch `milestone/unreal-death-loot-001`
from current `origin/main` (`eda5cc2`). No merge to main. No push.
**No protocol, no server/data, and no ClientCore/Unreal code change.**

Scope: creature death → corpse → open corpse → loot. **Player death is out of
scope** and nothing here touches it.

STATUS = **AUDIT-COMPLETE. The whole flow is already supported by composition
of landed primitives; no protocol change and no missing decoder/builder.** The
end-to-end LIVE kill-and-loot QA is **UNVERIFIED LIVE** here for the same
environmental reason as the prior milestones: no Fusion32 server was started and
no GUI client was driven, so nothing below claims a kill or a loot was observed.

## The audit: do NOT invent protocol — there is nothing to invent

A 7.72 corpse is **not a special protocol concept**. It is an ordinary
container-type object the server places on the tile, and loot is ordinary
objects the server moves into it. The whole flow therefore rides on commands and
client code that already exist and already ship.

### Server (authoritative; `reference/game`, read-only)

`TCreature::~TCreature` (`crmain.cc:173-259`), reached from `Death()`
(`crmain.cc:789`, `846`):

- Chooses `CorpseType` from `RaceData[Race].Male/FemaleCorpse` (`crmain.cc:197`).
- Places the corpse on the tile's map container:
  `Object Corpse = Create(Con, CorpseType, 0)` (`crmain.cc:217-218`).
- Moves inventory into the corpse: `::Move(0, Item, Corpse, -1, false, NONE)`
  for every item when `LOSE_INVENTORY_ALL`, every CONTAINER item, or `random(0,9)==0`
  (`crmain.cc:235-250`).

`Create`/`Move`/`Delete` on the map and in containers notify spectators through
the ordinary map and container commands — there is **no `SV_CMD_DEATH`**. The
command set (`connections.hh:77-137`) carries exactly what is used:
`CREATURE_HEALTH 140`, `ADD_FIELD 106` / `CHANGE_FIELD 107` / `DELETE_FIELD 108`
/ `MOVE_CREATURE 109`, `CONTAINER 110`, `CLOSE_CONTAINER 111`,
`CREATE/CHANGE/DELETE_IN_CONTAINER 112/113/114`. Corpses carry `EXPIRE` /
`TOTALEXPIRETIME` (`objects.cc:152,199-200`), so decay is the server deleting the
field on expiry — i.e. `DELETE_FIELD`, not a new command.

### ClientCore (every decoder already present)

- `CREATURE_HEALTH 140` → `player_state.cpp` (health reaching 0 on the victim).
- `ADD_FIELD`/`CHANGE_FIELD`/`DELETE_FIELD` → `movement.cpp:645,657,679` (the
  tile stack losing the creature and gaining the corpse).
- `CONTAINER`/`CREATE`/`CHANGE`/`DELETE_IN_CONTAINER`/`CLOSE_CONTAINER` →
  `movement.cpp:889-892` and `player_state.cpp:140-175` (open corpse, loot
  contents, and the update after each item is removed; `CreateInContainer`
  prepends at slot 0 exactly as `SendCreateInContainer` does).
- `WorldView::Diff` turns WorldState changes into events with **no local
  invention**: a changed tile stack emits `TileUpserted` with the full stack
  (`worldview.cpp:108-119`) — the corpse appearing — and a creature no longer on
  any stored tile emits `CreatureVanished` (`worldview.cpp:172-181`) — the dead
  creature's removal.

### Unreal (every link already wired)

- Bridge translates `TileUpserted`/`TileRemoved`/`CreatureVanished`/
  `ContainerChanged` (`Real33DBridge.cpp:1347,1410-1424`).
- `Real33DWorldActor` rebuilds the tile on `TileUpserted` (`:557`) — rendering
  the corpse — destroys the creature actor on `CreatureVanished` (`:672`), and
  drives the container panel on `ContainerChanged` (`:440`).
- `AReal33DTile::GetTopObject` (`Real33DTileActor.cpp:31-47`) skips creatures and
  returns the top object, so a right-click lands on the corpse.
- Right-click **Use** on a map object → `InteractUnderCursor`
  (`Real33DPlayerController.cpp:1092`) → `SReal33DHUD::UseWorldObject` →
  `RequestUseObject(..., FirstFreeContainerNumber())`, which is how any map
  container is opened; the server answers `SV_CMD_CONTAINER` and the loot panel
  fills. Moving loot out is the existing drag → `RequestMoveObject`
  (corpse container → inventory/another container).

## Mapping to the requested outcomes

| Outcome | Mechanism (already present) | Code-verified here |
|---|---|---|
| CREATURE_DEATH | `CREATURE_HEALTH 140` decode; death is server-side | yes (decoder) |
| CREATURE_REMOVAL | tile loses the creature → `CreatureVanished` → actor destroyed | yes |
| CORPSE_SPAWN | corpse added to tile → `TileUpserted` full stack → tile rebuilt; corpse is `GetTopObject` | yes |
| CORPSE_USE | right-click Use → `RequestUseObject(OpenAsContainer)` → `SV_CMD_CONTAINER` | yes (path) |
| LOOT_CONTENTS | `SV_CMD_CONTAINER` + `CREATE_IN_CONTAINER` decoded into the container's objects/counts | yes (decoder) |
| LOOT_MOVE | existing drag → `RequestMoveObject` corpse→inventory/container | yes (path) |
| CORPSE_UPDATE | `CHANGE/DELETE_IN_CONTAINER` → `ContainerChanged` after each removal | yes (decoder) |
| EMPTY_CORPSE | container with zero objects after the last `DELETE_IN_CONTAINER` | yes (decoder) |
| CORPSE_DECAY | `EXPIRE` object → server `DELETE_FIELD` → `TileRemoved`/`TileUpserted` | yes (decoder) |
| AUTHORITATIVE_STATE | WorldState-diff model; no local corpse/loot synthesised | yes (by construction) |

## Why no code was written

The task forbids inventing protocol and permits a change only if "a proven
missing decoder/builder requires it." The audit proves none is missing: every
server command in the flow is decoded, every event is produced, and every UI
link (tile render, creature removal, Use-opens-container, container panel, drag
→ move) already ships and was exercised by the container-controls and
turn-object milestones. Adding corpse-specific client code — e.g. labelling an
object "corpse" or predicting it is lootable — would require the client to judge
object flags locally, which is exactly the invention this architecture forbids
(the server rules; the client draws what it is told). So the correct engineering
outcome is **no protocol/code change**; the only edits are this evidence file
and the PARITY_MATRIX row.

## Offline verification (this environment)

- ClientCore CTest: **14/14** (WSL g++).
- REAL33D native unit tests: **10/10** (via harness).
- `regression_harness.sh`: **0 FAIL**.
- `secret_check.sh`: **6/6**.
- Unreal module build (`REAL33DEditor Win64 Development`): built clean — see the
  status block. The branch is `origin/main` + this documentation only, so the
  module is unchanged from the certified `eda5cc2`.

## Remaining UNVERIFIED LIVE

No Fusion32 server was started and no REAL33D GUI client was launched or driven
here, so none of the following was observed and none may be read as such; each
needs the operator-driven live step (`scripts/server/start_wsl.sh` + a human or
input automation at the GUI):

- Killing a real creature and seeing authoritative removal.
- The corpse appearing on the correct tile/stack.
- Right-click Use opening the corpse as a container.
- Loot contents/counts matching the server.
- Moving loot corpse → inventory/container and the corpse updating after.
- Empty-corpse behaviour and corpse decay/removal.
- That combat, containers, stacks and Use/Look/drag remain intact under this flow
  (they are unchanged in code; the claim is behavioural and needs a live run).

What is proven here is that the flow requires no new protocol and that every
decoder, event and UI link it depends on already exists and builds; what is
missing is a live creature to kill and loot, which is the certification step.
