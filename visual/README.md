# REAL33D Visual Area

Everything needed to turn Fusion32's 7.72 world into 3D assets: the master
inventory, the tracker that follows each asset through production, and the
reviewed work itself.

This area produces no gameplay and no protocol. `clientcore/` remains the only
authority on what the server says; nothing here may change it.

## Layout

```text
visual/
  docs/            art direction, technical standard, pipeline, sources
  tools/           extractors that regenerate every manifest from source truth
  manifests/       generated inventory, never hand-edited
  tracker/         the live tracker, human-owned status columns
  rookgaard_p0/    the first vertical slice subset
  mockups/         proposals awaiting review, one directory per asset
  approved/        proposals the project director accepted
  rejected/        proposals the project director declined, kept on purpose
  production/      source and export files for assets in production
```

`manifests/` is derived data. Regenerate it, do not edit it. `tracker/` holds
human decisions and is merged rather than overwritten.

## Regenerating the inventory

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/extract_visual_inventory.py --archive /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz --source /mnt/c/Users/dell/Desktop/fusion32/reference/game/src --out /mnt/c/Users/dell/Desktop/fusion32/visual/manifests
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/sync_tracker.py --manifests /mnt/c/Users/dell/Desktop/fusion32/visual/manifests --tracker /mnt/c/Users/dell/Desktop/fusion32/visual/tracker/VISUAL_TRACKER.csv --p0-out /mnt/c/Users/dell/Desktop/fusion32/visual/rookgaard_p0/P0_ASSETS.csv
```

The extractor takes about twenty seconds and reads 9,873 map sectors. The sync
adds rows for new entities, refreshes the derived columns of existing ones, and
flags rows whose entity disappeared instead of deleting them, so approved work
is never lost to a regeneration.

## Three identities, deliberately separate

A Fusion32 type id is not a 3D model. The inventory keeps three layers apart:

1. **Logical identity** — the Fusion32 type id, race number or effect id. This
   is what the protocol speaks and what `clientcore/` decodes.
2. **Visual identity** — the `visual_group`. Many ids depict the same thing and
   should share one asset.
3. **Physical asset** — the file actually produced, tracked in `production/`.

5,003 declared object types collapse into 1,351 visual groups, so most ids are
expected to reuse an asset rather than receive their own. The tracker's
`Representation` column records which: `MESH`, `SKELETAL_MESH`, `VFX`,
`ALIAS_OF:<id>` for a demonstrated disguise alias, or `SHARED_CANDIDATE:<id>`
for a grouping this tool inferred and an artist should confirm.

## Confidence

Every derived row says how it was reached:

- `DEMONSTRATED` — read directly from a dataset field, such as a category taken
  from an object's own flags.
- `INFERRED` — derived from dataset evidence, with the evidence recorded, such
  as an art class taken from the object's own name.
- `UNRESOLVED` — the dataset that would settle it is not available here.

Read `docs/SOURCES.md` for what each dataset can and cannot answer, including
what is still `UNRESOLVED` and why.
