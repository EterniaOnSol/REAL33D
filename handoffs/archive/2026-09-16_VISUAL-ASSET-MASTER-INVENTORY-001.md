# HANDOFF

Date/time: 2026-09-16
Agent: Claude
Role: VISUAL ASSET MASTER INVENTORY
Branch: `main`
Starting commit: `31d4be5`
Implementation commit: `4260d98`
Ending commit: this handoff commit
Worktree: clean after the focused inventory commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, `HEAD == origin/main`

## Objective

Complete `VISUAL-ASSET-MASTER-INVENTORY-001`: create the REAL33D visual area and
build a complete, reproducible master inventory of the visual assets needed to
represent Fusion32 7.72 in 3D, not limited to Rookgaard. Do not modify
ClientCore, protocol or gameplay. Do not implement Unreal. Do not produce models.

## Result

`PASS` over the datasets available, and explicitly **not complete in the visual
sense**, because no appearance data is available to this repository. Full
counts, sources and gaps in
`evidence/visual/VISUAL-ASSET-MASTER-INVENTORY-001.md`.

Headline numbers: 5,690 tracker rows covering 5,003 object types, 159 monster
races, 337 npcs, 152 outfit identities, 26 graphical effects and 13 projectiles.
Those 5,003 object ids collapse into 1,351 visual groups, so roughly three
quarters of the object inventory is expected to reuse an asset rather than
receive its own.

## What was built

```text
visual/
  README.md              how to regenerate, and the three-identity model
  docs/                  SOURCES, ART_DIRECTION, TECHNICAL_STANDARD, PIPELINE
  tools/                 extract_visual_inventory.py, sync_tracker.py
  manifests/             generated: objects, creatures, npcs, outfits, effects,
                         missiles, visual_groups, regions, summary.json
  tracker/               VISUAL_TRACKER.csv, human-owned status columns
  rookgaard_p0/          P0_ASSETS.csv and how the subset is chosen
  mockups/ approved/ rejected/ production/
```

Everything under `manifests/` is derived and regenerable in about 21 seconds.
`sync_tracker.py` merges rather than overwrites, so human decisions survive a
regeneration; verified by marking a row `MOCKUP`, regenerating and confirming
`added 0, refreshed 5690` with the note intact.

## Design decisions worth keeping

**Two classification axes, not one.** Behaviour category comes from the object's
own flags and is `DEMONSTRATED`. Art class comes from the object's own `Name`
and is always `INFERRED`, because flags cannot separate a tree from a wall when
both are merely `Unpass` and `Unmove`. Collapsing them would either lose the
flag evidence or hide that the art class is a guess. This is why the behaviour
axis reports 4 `traversal` objects while the art axis reports 75 stairs; both
are correct for what they measure.

**Three identities kept apart.** Logical id (Fusion32 type id), visual identity
(`visual_group`), physical asset (produced file). `ALIAS_OF` is settled by
`objects.hh::getDisguise`, which shows the client is told to draw the target
type. `SHARED_CANDIDATE` is a proposal for an artist to confirm or split.

**Priorities from map evidence.** P0 is the object types in the sectors within
one ring of `NewbieStart = [32097,32219,7]`, which `map.dat` declares and which
is the field two live clients actually spawned on. P1 is the rest of the
Rookgaard region by nearest named mark. P2 is everything else on the map,
grouped by region, with no invented ordering. 2,031 types declared but never
placed in `origmap` are `UNPRIORITIZED` and remain real inventory entries.

**No fidelity percentage.** `docs/ART_DIRECTION.md` states there is no 50/50 or
any other ratio. The data fixes identity and context; how closely a 3D asset
resembles a 32-pixel sprite is a judgement. The artist proposes, the director
decides, rejected work is kept.

## Two findings

**Stairs are not teleports in 7.72.** Exactly one object type carries
`TeleportRelative` and one carries `TeleportAbsolute`. Level changes are decided
by the height and climbing logic in `cract.cc::TCreature::Go`, not by a flag.

**Flag spellings differ between the data and the enum.** `objects.srv` writes
flags in CamelCase (`Bank`, `LiquidContainer`), resolved by name in
`objects.cc::LoadObjects` against the uppercase `enum FLAG`. The first version
of the extractor matched the C++ spelling and silently classified all 5,003
types as `INFERRED`. Caught by the summary counters showing zero `DEMONSTRATED`
rows, which is exactly what those counters exist for.

## What is UNRESOLVED

**Appearance.** Per-thing sprite dimensions, layer counts, animation frame
counts, draw offsets and the pixels live in the client's `Tibia.dat` and
`Tibia.spr`, which are not in this repository, are gitignored, and have
`UNKNOWN` provenance per `docs/CLASSIC_CLIENT_772.md`. Depending on them would
make the inventory non-reproducible and tie it to an artifact with no chain of
custody. Every manifest carries a `sprite_geometry` column fixed at
`UNRESOLVED`, and `summary.json` records the gap under `sources_unavailable`
with the extractor hook. Nothing else in the schema changes when that source
arrives.

**Art class for 2,752 object types**, reported as `UNRESOLVED` rather than
bucketed by guess.

**Floor height in Unreal units**, left open in `docs/TECHNICAL_STANDARD.md` as a
presentation choice the data does not declare.

## Git LFS

`.gitattributes` routes `.blend`, `.fbx`, `.glb`, `.gltf`, `.png`, `.jpg`,
`.jpeg`, `.tga`, `.psd`, `.exr`, `.wav` and `.ogg` to LFS; `.gitignore` excludes
Blender autosaves and local cache and bake directories. Manifests, tracker and
tools stay in plain Git so they remain diffable. The entries are inert until
`git lfs install` is run in a clone. **No binary asset was committed.**

## Checks

- `tests/secret_check.sh`: `PASS`
- `reference/` untouched
- `clientcore/` untouched, confirmed by `git status --porcelain clientcore/`
- No protocol, gameplay or Unreal work
- No 3D model produced

## Exact next task

Two candidates, neither started automatically.

`ROOKGAARD-P0-MOCKUPS-001`: 456 P0 object types are waiting on mockups. Start
from `visual/rookgaard_p0/P0_ASSETS.csv`, work group representatives first
(`Representation = MESH`) since each covers several ids, and follow
`visual/docs/PIPELINE.md`. The director's `APPROVED`/`REJECTED` decisions are
what unblock modelling.

`UNREAL-SLICE-001`: the protocol side is ready. Create the minimal Unreal
desktop project that consumes `Protocol772Core` through a network-thread event
queue and applies `WorldState` on the game thread, per `ROADMAP.md` step 8.
`visual/docs/TECHNICAL_STANDARD.md` holds the provisional scale, pivots and
naming it should follow, and the floor-height question it will have to settle.

Commands to reproduce this task's results:

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/extract_visual_inventory.py --archive /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz --source /mnt/c/Users/dell/Desktop/fusion32/reference/game/src --out /mnt/c/Users/dell/Desktop/fusion32/visual/manifests
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/sync_tracker.py --manifests /mnt/c/Users/dell/Desktop/fusion32/visual/manifests --tracker /mnt/c/Users/dell/Desktop/fusion32/visual/tracker/VISUAL_TRACKER.csv --p0-out /mnt/c/Users/dell/Desktop/fusion32/visual/rookgaard_p0/P0_ASSETS.csv
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/tests/secret_check.sh /mnt/c/Users/dell/Desktop/fusion32
```
