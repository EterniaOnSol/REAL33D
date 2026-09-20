# V08 experimental full catalog QA

Milestone: `VISUAL-FULL-CATALOG-INGEST-TEST-001`

This is a local visual test surface. `TEST_IMPORTED` does not mean `APPROVED`,
`READY`, or production `INTEGRATED`. The normal client does not load this
catalog unless an operator explicitly enables it.

## Source authority

- Repository: `leodavidsoto/3DTIBIA`
- Branch: `carril/ORQUESTADOR`
- Commit: `21fafb57dd86b594223bab7dbe9076d1fa380640`
- Read-only catalog: `worklog/ORQUESTADOR/generaciones_assets/refinamiento_en_curso_v8/manifest.json`
- Expected records: 4,913

The catalog's relative links are resolved from its own directory. This is
required because active V08 rows can point into V3 through V8 production
folders. Absolute macOS paths retained in upstream metadata are treated as
provenance text and never used as local paths.

## Reproduce

From the REAL33D repository root:

```powershell
& 'C:\Users\dell\AppData\Local\Programs\Python\Python311\python.exe' `
  visual\tools\build_v08_test_manifest.py `
  --source C:\Users\dell\3DTIBIA_leo

powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\client\import_v08_catalog_all.ps1 `
  -Source C:\Users\dell\3DTIBIA_leo -BatchSize 250
```

The importer is resumable. Successful IDs are skipped. Failed IDs are retained
with their reason and are retried only when `-RetryFailed` is supplied. Each
attempt is fsynced to `visual/qa/full_catalog_v08/import_results.jsonl` before
the next model begins.

For a bounded batch:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\client\import_v08_catalog_unreal.ps1 `
  -Source C:\Users\dell\3DTIBIA_leo -Start 0 -Limit 250
```

Generate the final accounting after the pass:

```powershell
& 'C:\Users\dell\AppData\Local\Programs\Python\Python311\python.exe' `
  visual\tools\summarize_v08_catalog.py
```

## Transform policy

The source GLB remains unchanged in the external read-only checkout. Unreal's
glTF Interchange importer performs the deterministic conversion:

- glTF: right handed, metres, `+Y` up
- Unreal: left handed, centimetres, `+Z` up
- REAL33D: one source tile/metre becomes 100 Unreal Units

The pipeline records source and imported bounds. It does not move a pivot to
the floor or normalize a model to fit the viewer. Nonzero source floor,
floating, underground, very flat, tiny, and gigantic results remain visible
and are warnings. This is diagnostic evidence, not an art fix.

## Runtime isolation

Generated `.uasset` files live under
`unreal/REAL33D/Content/Experimental/V08/`. That directory is ignored by Git
until the project owner separately authorizes its binary and Git LFS cost.

`UReal33DAssetRegistry` remains the only TypeId-to-asset resolver. With the
experimental option off it does not read the QA manifest. With it on, a row
whose import status is `IMPORTED_OK` resolves to its deterministic mesh path;
any missing, invalid, failed, or not-yet-imported row resolves to the existing
placeholder. Materials imported from the GLB stay assigned to the mesh.

No lookup was added to protocol, ClientCore, WorldState, tile actors, movement,
gameplay, or the Fusion32 server.

## Open the gallery

```cmd
scripts\client\run_v08_gallery.cmd
```

The gallery shows one unnormalized 3D asset at a time above a 100 x 100 UU tile
reference. Its panel supports ID/name search and `ALL` plus exact filters for
category, refinement state, and geometry quality. Previous/next buttons page
through 40 rows at a time. The details panel shows ID, name, category,
refinement state, geometry quality, generation, import result, and warnings.

## Test in the real Fusion32 world

Start the sanitized runtime normally, then run:

```cmd
scripts\client\run_unreal_v08_experimental.cmd B
```

The command-line flag enables the experimental manifest for that session only.
Fusion32 still supplies the authoritative TypeId and all semantics. Types with
no successfully imported experimental mesh use the placeholder.

## Outputs

- `full_catalog_manifest.json`: normalized 4,913-row source and technical inventory
- `import_results.jsonl`: append-only attempt log with success/failure/warnings
- `experimental_catalog_runtime.json`: gallery/registry view of current results
- `ingest_report.json`: final machine-readable counts and failure list
- `INGEST_REPORT.md`: compact operator report

