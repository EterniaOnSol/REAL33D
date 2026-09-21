# UNREAL-WIDE-WORLD-001 -- sector streamed static baseline

Status: `CERTIFIED_PASS`.

Date: 2026-09-21
Authoritative live viewport: Fusion32 18x14 WorldState
Static sector size: 32x32xZ
Visual radius: configurable with `-real33d-visual-radius=32|64|96|128`

## Result

Unreal now keeps two separate layers:

- AUTHORITATIVE_LIVE_WINDOW: the complete Fusion32 18x14 rectangle. WorldState wins for every coordinate, including empty tiles.
- `STATIC_BASELINE_ONLY`: clean `.sec` content outside that rectangle. It contains no creatures, players, combat, effects, moving items, door mutations, or runtime state.

When the anchor moves, static instances and classic sprites at a coordinate entering the authoritative rectangle are suppressed. They become eligible for static display after leaving it. Live and static content do not share a visible coordinate.

## Source and cache pipeline

`scripts/client/build_wide_world_cache.py` strictly parses the syntax consumed by `reference/game/src/map.cc::LoadSector` and `LoadObjects`. It handles tile flags, object numeric or string attributes, and recursive `Content`. Nested container contents remain attached to the containing item.

Each generated `.wws` row retains world X, world Y, Z, stack index, visible Fusion32 TypeId, and source `.sec` TypeId. Visible identity follows `objects.srv` `DisguiseTarget`. The cache lives in Unreal `Saved/WideWorldCache`, is gitignored, and is reproducible from local sources.

## Rendering and streaming

- Thread pool tasks read and parse `.wws` files.
- Only the game thread creates, updates, or destroys Actors, UObjects, HISM instances, and billboards.
- A physically resolvable frozen V08 entry uses its imported mesh.
- A missing V08 mesh uses the exact local classic preview as a tagged `CLASSIC_SPRITE_FALLBACK_<TypeId>` billboard proxy.
- If neither exists, a separate placeholder is counted as `MISSING_PHYSICAL_ASSET` and sector loading continues.
- V08 meshes use hierarchical instances grouped by mesh.
- Parsed sector data remains in `StaticSectorCache` for IO-free reload.
- Distant sector actors are destroyed on the game thread.
- Exact-radius and authoritative-window filtering operate per tile.

## Radius QA

Counts are static object occurrences after visible-ID resolution, across Z 0-7, inside the exact square XY radius. They exclude extra contents of intersecting edge sectors.

| Radius | V08_RESOLVED | CLASSIC_SPRITE_FALLBACK | MISSING_PHYSICAL_ASSET |
| ---: | ---: | ---: | ---: |
| 32 | 12,286 | 0 | 0 |
| 64 | 38,826 | 3 | 0 |
| 96 | 71,845 | 8 | 0 |
| 128 | 110,073 | 17 | 0 |

Machine-readable result: `evidence/clientcore/unreal-wide-world-radius-counts.json`.

The earlier regular-expression audit reported 9 occurrences of 469 at radius 96 because it counted every item in each intersecting sector. The strict compiler applies per-tile radius filtering and reports 8. Classification: `RADIUS_DEFINITION`. The excluded occurrence is at (32429,32211,6), in intersecting sector `1013-1006-6.sec`: its X distance from the Thais center (32330,32226) is 99, beyond the exact radius 96. The other eight occurrences are inside the square radius.

## TypeId 469

```text
TYPE_ID = 469
IDENTITY = KNOWN
IDENTITY_FIDELITY = PASS
STATIC_MAP_FIDELITY = PASS
V08_PHYSICAL_ASSET = MISSING
WIDE_WORLD_RENDER = CLASSIC_SPRITE_FALLBACK
PHYSICAL_3D_FIDELITY = FALLBACK
```

The `.sec` occurrences use raw TypeId 451. Fusion32's `objects.srv` marks 451 as `Disguise` with `DisguiseTarget=469`; 469 is stairs. The cache stores both values and renders visible TypeId 469. No V08 alias or family substitution was added. All 17 radius-128 occurrences classify as `CLASSIC_SPRITE_FALLBACK`.

## Executed verification

1. Strict cache generation succeeded: 199 source sectors for the Thais radius-128 scan.
2. Unreal build: Build.bat REAL33DEditor Win64 Development ... -WaitMutex -> Result: Succeeded.
3. Unreal automation REAL33D.WideWorld.AuthoritativeWindow -> Success. It checks live bounds, floor XY offset, radius boundary, static-to-live suppression after one anchor step, and load/unload candidates after a sector step.
4. A live headless Fusion32 run at Player B position (32094,32203), radius 64, connected and loaded 62 cached sectors. Raw loaded sector contents totalled 41,506 V08_RESOLVED, 14 CLASSIC_SPRITE_FALLBACK, and 0 MISSING_PHYSICAL_ASSET. Exact visible-radius counts are narrower than raw full-sector totals. A final radius-32 run omitted the catalogue argument: the streamer enabled the frozen 4,913-row catalogue automatically and logged TypeId=469 CLASSIC_SPRITE_FALLBACK at exact map positions while continuing sector loads.
5. Frozen catalogue SHA-256 remains `635b9c74a8a889be4764d87e27f742e4934f4a0e738100134fcf5ad0a2ea7a01`. Git reports no changed V08 catalogue or imported V08 asset.
6. Git reports no change under `clientcore/` or `reference/`.

## Final certification audit

Four stationary headless Unreal runs connected Test Player B at (32094,32203,7), with a local cache regenerated to radius 128 around that position. Sector logs contained one unique load per key and matched the number of nonempty cached sectors requested at each radius. Raw instance totals include full intersecting sectors, including instances suppressed outside the exact visual radius or inside the authoritative window; they differ from the Thais exact-radius occurrence counts above. Memory is whole-process working set at the stable sample, not WideWorld-only allocation. Latency is from the logged player appearance to the last logged sector load, not per-sector IO latency.

| Radius | Loaded sectors | WideWorld Actors | HISM instances | Billboard components | HISM component count | Process working set (MiB) | Initial load completion (ms) | Game thread (ms) | FPS |
| ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- | --- |
| 32 | 25 | 25 | 17,664 | 3 | NOT_MEASURED | 2,049 | 1,635 | NOT_MEASURED | NOT_MEASURED |
| 64 | 62 | 62 | 41,506 | 14 | NOT_MEASURED | 2,136 | 11,939 | NOT_MEASURED | NOT_MEASURED |
| 96 | 104 | 104 | 72,059 | 17 | NOT_MEASURED | 2,202 | 4,302 | NOT_MEASURED | NOT_MEASURED |
| 128 | 141 | 141 | 106,479 | 17 | NOT_MEASURED | 2,233 | 2,814 | NOT_MEASURED | NOT_MEASURED |

The runs used `-NullRHI`, so they provide no valid rendered FPS or game-thread frame cost. `AReal33DStaticSector` uses one Actor per loaded nonempty sector, one root scene component per Actor, HISM components grouped by resolved mesh with one instance per V08 object, and one billboard component per classic fallback. It does not create an Actor per tile or object and does not merge meshes. The 110,073 V08 occurrences at Thais radius 128 are instance candidates, not 110,073 Actors. The exact Thais runtime Actor count was not measured.

## Live visibility forensics and boundary certification

At player (32094,32203,7), normal-RHI Unreal sampled static visibility on sector load, after 1.004 s, and after 3.014 s without movement. Each sample had the same three mismatches, 25 loaded sectors, zero pending sectors, 17,664 HISM refs, and three fallback billboards.

| Mismatch | World tile | Sector | Local tile | Baseline | Floor-specific live X / Y | Expected / actual |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | (32098,32181,5) | (1003,1005,5) | (2,21) | visible TypeId 469; raw 451; stack 0 ground | 32088..32105 / 32199..32212 | visible / hidden |
| 2 | (32098,32197,5) | (1003,1006,5) | (2,5) | visible TypeId 469; raw 451; stack 0 ground | 32088..32105 / 32199..32212 | visible / hidden |
| 3 | (32093,32235,6) | (1002,1007,6) | (29,11) | visible TypeId 469; raw 467; stack 0 ground | 32087..32104 / 32198..32211 | visible / hidden |

Each static tile existed, the live tile was absent, and each coordinate was outside its authoritative 18x14 window, so suppression was not expected. Each registered component was a CLASSIC_SPRITE_FALLBACK_469 billboard, with no HISM component or instance index. Each had hidden_in_game=1. Unreal's UBillboardComponent constructor defaults to that flag; WideWorld only changed SetVisibility. Classification: STABLE_LOGIC_ERROR. The one-line WideWorld fix sets hidden-in-game false when constructing the billboard. Sector and callback generation tokens are not implemented; the forensic log records NOT_IMPLEMENTED for both.

After the fix, three consecutive stationary audits at 0, 1.015, and 3.017 s at the same position returned zero mismatches across all 17,664 HISM refs and three billboards. Automation result: Success.

A separate normal-RHI session sent real east/west walk requests through the Fusion32 bridge. The authoritative path was (32094,32203,7) -> (32095,32203,7) -> (32096,32203,7) -> (32095,32203,7) -> (32094,32203,7). At x=32096, nine sector Actors loaded and nine unloaded. The active count remained 25. Returning loaded nine and unloaded nine, restoring the exact initial sector-key set. Stable audits had zero pending sectors.

| Audit point | X | Sectors | HISM refs | Billboards | Live player tile | Static/live overlaps | Visibility mismatches |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| Before crossing | 32094 | 25 | 17,664 | 3 | yes | 0 | 0 |
| Immediately after crossing | 32096 | 25 | 16,214 | 3 | yes | 0 | 0 |
| After sector-set change | 32096 | 25 | 16,214 | 3 | yes | 0 | 0 |
| Three seconds after crossing | 32096 | 25 | 16,214 | 3 | yes | 0 | 0 |
| After returning | 32094 | 25 | 17,664 | 3 | yes | 0 | 0 |
| Three seconds after returning | 32094 | 25 | 17,664 | 3 | yes | 0 | 0 |

Each audit checked every loaded static HISM transform and billboard visibility against the 18x14 and radius rules, including ground, walls, and objects. The player's live tile existed at both sides of the seam and no visible static reference overlapped a live tile. Every loaded sector key remained desired through the delayed samples, and no stale async resurrection was observed. No stale callback rejection occurred in this run. This is a game-state and render-component audit, not a pixel screenshot. FPS and game-thread ms remain NOT_MEASURED.

Temporary forensic and walk automation source was removed after the run. Local gitignored logs under unreal/REAL33D/Saved/Logs are WWInitialVisibilityForensic.log, WWInitialVisibilityFixed.log, and WWLiveRoundTrip.log; they are not published. The production REAL33D.WideWorld.AuthoritativeWindow automation remains.

UNREAL-WIDE-WORLD-001 = CERTIFIED_PASS. Final artistic and physical 3D fallback fidelity remains outside this milestone. The local reference pack and generated cache remain unpublished.