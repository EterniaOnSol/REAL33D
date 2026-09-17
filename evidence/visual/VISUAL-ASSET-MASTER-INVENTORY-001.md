# VISUAL-ASSET-MASTER-INVENTORY-001

Status: `PASS` for the datasets available. The inventory is **not complete in
the visual sense** and does not claim to be; see "What could not be extracted".

## What was built

A `visual/` area with a master inventory regenerated entirely by tooling from
Fusion32 source truth, a tracker that survives regeneration, and the P0
Rookgaard subset derived from map evidence.

No ClientCore, protocol or gameplay file was touched. No Unreal work and no 3D
models were produced.

## Reproduction

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/extract_visual_inventory.py --archive /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz --source /mnt/c/Users/dell/Desktop/fusion32/reference/game/src --out /mnt/c/Users/dell/Desktop/fusion32/visual/manifests
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/sync_tracker.py --manifests /mnt/c/Users/dell/Desktop/fusion32/visual/manifests --tracker /mnt/c/Users/dell/Desktop/fusion32/visual/tracker/VISUAL_TRACKER.csv --p0-out /mnt/c/Users/dell/Desktop/fusion32/visual/rookgaard_p0/P0_ASSETS.csv
```

The extractor runs in about 21 seconds and reads 9,873 map sectors covering
7,846,045 tiles. Both tools are idempotent.

## Sources used

| Dataset | What it provided |
| --- | --- |
| `./dat/objects.srv` | 5,003 declared object types with names, flags and attributes |
| `./dat/map.dat` | sector bounds, 50 named marks, `NewbieStart = [32097,32219,7]` |
| `./origmap/*.sec` | 9,873 sectors, which type ids occur where |
| `./dat/monster.db` | 9,615 monster spawn points |
| `./mon/*.mon` | 159 monster races |
| `./npc/*.npc` | 337 npcs |
| `reference/game/src/enums.hh` | 26 graphical effects from `EffectType` |

The archive also contains `usr/`, `usr.bak/`, `.ssh/`, `.lftp/` and similar
operational material. The extractor matches only the six paths above and reads
none of it.

## Validation counts

### Visual ids found

| Metric | Count |
| --- | --- |
| Object types declared | 5,003 |
| Monster races | 159 |
| NPCs | 337 |
| Outfit identities referenced | 152 |
| Graphical effects | 26 |
| Projectile ids | 13 |
| **Tracker rows total** | **5,690** |

### Objects by behaviour category

| Category | Count |
| --- | --- |
| terrain | 1,455 |
| decoration | 1,241 |
| structure | 423 |
| usable | 378 |
| item | 347 |
| creature_remains | 341 |
| equipment | 307 |
| container | 122 |
| vegetation | 119 |
| furniture | 118 |
| readable | 117 |
| effect_object | 31 |
| traversal | 4 |

Behaviour categories come from the object's own flags, so 4,694 are
`DEMONSTRATED` and 309 `INFERRED`. None are `UNRESOLVED`.

### Objects by art class

A second, independent axis, read from the object's own `Name` because flags
cannot separate a tree from a wall when both are merely `Unpass` and `Unmove`.
Always `INFERRED` when a keyword matches.

| Art class | Count |
| --- | --- |
| vegetation | 568 |
| wall | 533 |
| rock | 422 |
| water | 306 |
| furniture | 183 |
| stairs | 75 |
| roof | 53 |
| statue | 53 |
| bone | 50 |
| sign | 8 |
| **UNRESOLVED** | **2,752** |

### Names

| Metric | Count |
| --- | --- |
| Object types with a known name | 4,991 |
| Object types with no name | 12 |
| Art class UNRESOLVED | 2,752 (2,740 named but no keyword match, 12 unnamed) |

### Reuse and grouping

| Metric | Count |
| --- | --- |
| Visual groups | 1,351 |
| Multi-member groups | 489 |
| Object ids inside a multi-member group | 4,141 |
| Demonstrated disguise aliases | 53 groups, 21 tracker rows |
| Rows needing their own static mesh | 1,351 |
| Rows flagged as a shared candidate | 3,964 |
| Rows needing a skeletal mesh | 315 |
| Rows that are VFX only | 39 |

5,003 object ids collapse into 1,351 visual groups. Roughly three quarters of
the object inventory is expected to reuse an asset rather than receive one, which
is the single most useful number here for planning production.

`ALIAS_OF` is settled by the data: `reference/game/src/objects.hh::getDisguise`
shows the client is told to draw the target type. `SHARED_CANDIDATE` is a
proposal an artist confirms or splits.

### Priorities

| Priority | Objects | Basis |
| --- | --- | --- |
| P0 | 456 | sectors within one ring of `NewbieStart`, 48 sectors |
| P1 | 139 | sectors whose nearest named mark is `Rookgaard` |
| P2 | 2,377 | occur elsewhere in `origmap`, labelled by region |
| UNPRIORITIZED | 2,031 | declared but never placed in `origmap` |

Creatures and npcs are prioritised from their own data rather than the P0 box:
20 monster races and 18 npcs belong to Rookgaard by nearest named mark, both P1.

The 2,031 unprioritized object types are real inventory entries, not errors.
Many are created at runtime, dropped as loot or produced by events.

## What could not be extracted, and why

**Appearance.** Per-thing sprite dimensions, layer counts, animation frame
counts, draw offsets and the sprite pixels live in the client's `Tibia.dat` and
`Tibia.spr`. Those files are not part of this repository, are covered by
`.gitignore`, and their provenance is recorded as `UNKNOWN` in
`docs/CLASSIC_CLIENT_772.md`. Building the inventory on them would make it
non-reproducible for anyone else and would tie it to an artifact with no chain
of custody.

Every manifest therefore carries a `sprite_geometry` column fixed at
`UNRESOLVED`, and `summary.json` records the gap under `sources_unavailable`
with the hook for adding a reader later. Nothing else in the schema changes when
that source arrives.

**Art class for 2,752 object types.** Reported as `UNRESOLVED` rather than
bucketed by guess.

**Floor height in Unreal units.** The protocol addresses floors 0..15 and
`SendFullScreen` shifts x and y by one field per floor of depth, which fixes the
horizontal offset but not a vertical distance. Left `UNRESOLVED` in
`visual/docs/TECHNICAL_STANDARD.md` as a presentation choice.

## Two findings worth recording

**Stairs are not teleports in 7.72.** Exactly one object type carries
`TeleportRelative` and one carries `TeleportAbsolute`. Level changes are decided
by the height and climbing logic in
`reference/game/src/cract.cc::TCreature::Go`, not by a flag. The 75 objects the
inventory lists as `art_class=stairs` are found by name alone and marked
`INFERRED`. This is why the behaviour axis reports only 4 `traversal` objects
while the art axis reports 75 stairs; both numbers are correct for what they
measure.

**Flag spellings.** `objects.srv` writes flags in CamelCase (`Bank`,
`LiquidContainer`), resolved by name in
`reference/game/src/objects.cc::LoadObjects` against the uppercase `enum FLAG`.
A first version of the extractor matched the C++ spelling and silently
classified all 5,003 types as `INFERRED`. Caught by the summary counters showing
zero `DEMONSTRATED` rows, which is what those counters are for.

## Git LFS

`.gitattributes` now routes `.blend`, `.fbx`, `.glb`, `.gltf`, `.png`, `.jpg`,
`.jpeg`, `.tga`, `.psd`, `.exr`, `.wav` and `.ogg` to Git LFS, and
`.gitignore` excludes Blender autosaves and local cache and bake directories.
Manifests, the tracker and the tools stay in plain Git so they remain diffable.

The LFS entries are inert until `git lfs install` is run in a clone. **No binary
asset is committed by this task.** Declaring the policy now avoids a history
rewrite when the first asset lands.

## Tracker

`visual/tracker/VISUAL_TRACKER.csv`, 5,690 rows, all `TODO`.

```text
ID | Name | Category | Subcategory | Source | Priority | Status |
Mockup Version | Approved Version | Production Version |
Representation | Rig | Animation | VFX | Notes
```

Derived columns are refreshed on every sync. `Status`, the three version
columns, `Rig`, `Animation`, `VFX` and `Notes` are human-owned and never
overwritten. Entities that disappear from a regenerated manifest are kept and
annotated `ORPHANED` rather than deleted.

Verified by marking a row `MOCKUP` with a note, regenerating, and confirming the
sync reported `added 0, refreshed 5690` with the human state intact.

## Art direction

`visual/docs/ART_DIRECTION.md` records that there is no fidelity percentage. The
original data fixes identity and context; how closely a 3D asset should resemble
a 32-pixel sprite is a judgement, not arithmetic. The artist proposes mockups,
the project director decides `APPROVED` or `REJECTED`, and rejected work is kept
in `visual/rejected/` rather than deleted.

## Checks

- `tests/secret_check.sh`: `PASS`
- `reference/` untouched
- No `clientcore/`, protocol or gameplay file modified
- No binary asset committed
