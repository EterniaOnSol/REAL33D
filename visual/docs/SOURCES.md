# Visual Inventory Sources

What each dataset answers, what it cannot, and what remains `UNRESOLVED`.

## Datasets used

All paths are inside the historical runtime archive `tibia-game.tarball.tar.gz`
unless noted. The archive's SHA-256 is pinned by
`scripts/server/prepare_wsl.sh`.

| Dataset | Answers | Used for |
| --- | --- | --- |
| `./dat/objects.srv` | every declared object type, its name, description, flags and attributes | the object inventory, behaviour categories, wire-extra bytes, disguise aliases, missile ids |
| `./dat/map.dat` | sector bounds, named marks, newbie and veteran start | region attribution, P0 anchor |
| `./origmap/*.sec` | which type ids actually occur, per sector | map usage, priorities, absence from the world |
| `./dat/monster.db` | monster spawn points with race, position, radius and amount | which races exist in which region |
| `./mon/*.mon` | monster race number, name, article, outfit, corpse type, blood | the creature inventory |
| `./npc/*.npc` | npc name, sex, race, outfit, home coordinate | the npc inventory and its region |
| `reference/game/src/enums.hh` | `EffectType` | the graphical effect inventory |

The archive also contains `usr/`, `usr.bak/`, `.ssh/`, `.lftp/`,
`ip-address.txt` and similar operational material. The extractor reads none of
it; it matches only `./dat/objects.srv`, `./dat/map.dat`, `./dat/monster.db`,
`./mon/*.mon`, `./npc/*.npc` and `./origmap/*.sec`.

## How categories are derived

Two independent axes, because neither alone is sufficient.

**Behaviour class** comes from the object's own flags, the same names
`reference/game/src/objects.cc::LoadObjects` resolves against `enum FLAG` in
`enums.hh`. A ground is a ground because it carries `Bank`; a door is a door
because it carries `KeyDoor`, `NameDoor`, `LevelDoor` or `QuestDoor`. These rows
are `DEMONSTRATED`.

**Art class** comes from the object's own `Name` string. Flags cannot separate a
tree from a wall when both are merely `Unpass` and `Unmove`, and 7.72 has no
flag for "is a tree". These rows are always `INFERRED`, with the matched
keyword recorded as evidence, or `UNRESOLVED` when no keyword matched.

The two axes are stored in separate columns on purpose. Collapsing them would
either lose the demonstrated flag evidence or hide the fact that the art class
is a guess.

### A worked example of why both are needed

Only one object type carries `TeleportRelative` and one carries
`TeleportAbsolute`. Stairs in 7.72 are therefore not teleports: level changes
are decided by the height and climbing logic in
`reference/game/src/cract.cc::TCreature::Go`, not by a flag. The 75 objects the
inventory lists as `art_class=stairs` are found by name alone, and are marked
`INFERRED` accordingly.

## How priorities are derived

- **P0** — object types occurring in the map sectors within one sector ring of
  the sector containing `NewbieStart`, which `./dat/map.dat` declares as
  `[32097,32219,7]`. That is the field two live clients actually spawned on in
  `TWO-CLIENT-VERTICAL-SLICE-001`. Nine sectors per floor, so roughly a 96x96
  tile neighbourhood of the Rookgaard temple.
- **P1** — object types occurring in sectors whose nearest named mark is
  `Rookgaard`, minus P0.
- **P2** — everything else that occurs anywhere in `origmap`, labelled with the
  regions it occurs in.
- **UNPRIORITIZED** — declared in `objects.srv` but never placed in `origmap`.
  These are real entries in the master inventory, not errors: many are created
  at runtime, dropped as loot or produced by events.

Region attribution assigns each sector to the nearest of the 50 named marks in
`map.dat`. That is a reproducible partition built only from shipped data, but it
is an approximation of a region boundary rather than a declared one, so regions
are `INFERRED`. No P2 ordering is invented beyond that grouping.

## What is UNRESOLVED and why

**Sprite geometry.** Per-thing sprite dimensions, layer counts, animation frame
counts, draw offsets and the pixels themselves live in the client's
`Tibia.dat` and `Tibia.spr`. Those files are not part of this repository, are
covered by `.gitignore`, and their provenance is recorded as `UNKNOWN` in
`docs/CLASSIC_CLIENT_772.md`. Building the inventory on them would make it
non-reproducible for anyone else and would tie it to an artifact with no chain
of custody.

Every manifest therefore carries a `sprite_geometry` column fixed at
`UNRESOLVED`. The extractor is structured so a reader for that data can be
added as one more source without changing any other column, should an artifact
with verifiable provenance become available.

**Art class for 2,752 object types.** 2,740 have a name that matched no art
keyword and 12 have no name at all. Most are items and equipment whose art class
is simply the item itself, but the inventory does not claim to know that, so
they are reported as `UNRESOLVED` rather than bucketed by guess.

**Player outfit range.** `reference/game/src/sending.cc::SendOutfit` derives the
selectable range from the player's sex and premium right: 128..131 for male and
136..139 for female, each extended by three with premium. Those bounds are in
the source and are honoured, but the inventory lists outfit identities actually
referenced by `mon/` and `npc/` data rather than asserting a complete catalogue
of player outfits.

## Consequence for the completeness claim

This inventory is **not complete** in the visual sense, and does not claim to
be. It is complete over the datasets listed above: every declared object type,
every monster race file, every npc file, every graphical effect in the enum and
every missile id referenced by an object attribute. What it cannot see is the
appearance of any of them.
