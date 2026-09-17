# Rookgaard P0

The asset subset for the first Rookgaard vertical slice, in `P0_ASSETS.csv`.

## How the subset is chosen

`./dat/map.dat` declares `NewbieStart = [32097,32219,7]`. That is the field two
live clients actually spawned on during `TWO-CLIENT-VERTICAL-SLICE-001`, so it
is evidence rather than a guess.

P0 is every object type occurring in the map sectors within one sector ring of
the sector containing that position, across every floor those sectors exist on:
48 sectors, roughly a 96x96 field neighbourhood of the Rookgaard temple.

That radius is a production decision, not something the data declares. It is a
parameter:

```powershell
--p0-sector-radius 1
```

Widen it and regenerate if the first slice needs more ground. The tracker merge
keeps any work already approved.

## What is in it

456 object types. Everything else in Rookgaard is P1, the rest of the world is
P2 grouped by region, and object types that never appear anywhere in `origmap`
are `UNPRIORITIZED`.

## Creatures and npcs

Creature and npc priority is derived separately, from where they actually
belong rather than from the P0 sector box:

- 20 monster races have spawn points whose nearest named mark is `Rookgaard`,
  from `./dat/monster.db`.
- 18 npcs have a `Home` coordinate whose nearest named mark is `Rookgaard`,
  from `./npc/*.npc`.

Both are P1. They are not forced into P0 because a spawn point inside the P0 box
is not the same claim as a race belonging to the slice, and the inventory does
not make claims the data does not support. Promote individual rows to P0 in the
tracker when the slice's content is decided; the `Priority` column is derived,
so record such a decision in `Notes` and adjust the extractor if it becomes a
rule.

## What P0 does not tell you

Nothing about appearance. The `sprite_geometry` column is `UNRESOLVED` for every
row here, for the reason given in `visual/docs/SOURCES.md`.
