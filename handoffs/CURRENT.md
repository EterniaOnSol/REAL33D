# HANDOFF

Date/time: 2026-09-21, America/Guatemala
Agent: Codex
Role: Unreal wide-world static streamer
Task: UNREAL-WIDE-WORLD-001
Branch: main
Certification base commit: 4637437a495f22134af5206b93c342c97589be49
Worktree: C:/Users/dell/Desktop/fusion32
Status: CERTIFIED_PASS. Four radii loaded, and a live round-trip across the 32x32 boundary passed. V08 visual certification remains STANDBY.

## Objective and boundaries

- Stream clean Fusion32 static .sec baseline beyond the 18×14 authoritative live window.
- WorldState owns the whole live rectangle, including empty fields.
- Outside live authority, render STATIC_BASELINE_ONLY. No creatures, players, combat, effects, moving items, door changes, or other runtime mutations are synthesized.
- Preserve visible Fusion32 TypeId identity and keep missing V08 physical meshes non-blocking.
- Keep V08 assets, aliases, mappings, materials, rotations, pivots, imported files, gallery, notes, and approval state frozen.
- Keep ClientCore, protocol, server, and reference sources unchanged.

## Implementation

- Added scripts/client/build_wide_world_cache.py, a strict .sec parser and local cache compiler. It handles flags, string and numeric attributes, recursive Content, exact stack order, and objects.srv disguise targets.
- Generated cache rows store world coordinates, stack, visible TypeId, and raw source TypeId. Output goes to Unreal Saved/WideWorldCache and remains gitignored.
- Added AReal33DStaticSector. It groups V08 meshes in HISM components, uses the local classic preview as a billboard for unresolved V08 identities, and uses a separately counted placeholder only if the classic preview is also absent.
- Added async .wws read/parse and game-thread-only actor/component installation, suppression, and destruction.
- Added configurable -real33d-visual-radius, default 64, accepted range 32–512.
- Added parsed sector cache and reload without disk IO.
- Added complete 18×14, floor-offset-aware static suppression and exact radius filtering per tile.
- Added desired-sector calculation and distant actor unload.

## TypeId 469

- Source .sec TypeId: 451.
- objects.srv visible DisguiseTarget: 469.
- Identity: known stairs.
- V08 physical mesh: missing.
- Wide-world render: CLASSIC_SPRITE_FALLBACK.
- All 17 exact radius-128 occurrences around the audited Thais center retain visible TypeId 469.
- No physical equivalence or V08 alias was invented.

## Tests and evidence

- Strict radius-128 cache compilation: PASS, 199 sectors.
- Exact occurrence counts at radii 32/64/96/128 are in evidence/clientcore/unreal-wide-world-radius-counts.json.
- UE 5.8 REAL33DEditor build: Result Succeeded.
- REAL33D.WideWorld.AuthoritativeWindow automation: Success. Covers live bounds, floor offset, visual radius, static-to-live transition suppression, and load/unload candidates.
- Live headless run connected Player B at 32094,32203 and loaded 62 cached sectors. Raw sector contents: V08_RESOLVED=41,506; CLASSIC_SPRITE_FALLBACK=14; MISSING_PHYSICAL_ASSET=0.
- Frozen runtime catalogue SHA-256 remains 635b9c74a8a889be4764d87e27f742e4934f4a0e738100134fcf5ad0a2ea7a01.
- No changed file under visual/qa/full_catalog_v08, Unreal Content/Experimental/V08, clientcore, or reference.
- Four stationary headless performance runs loaded 25/62/104/141 sector Actors at radii 32/64/96/128. Full counts and unmeasured metrics: evidence/clientcore/UNREAL-WIDE-WORLD-001.md.
- Initial TypeId-469 billboard visibility mismatches were diagnosed as Unreal billboard hidden-in-game defaults; the one-line WideWorld fix passed three consecutive stationary audits with zero mismatches. A real east/west round-trip crossed x=32096. Nine sectors loaded and nine unloaded in each direction; six audits returned zero mismatches and zero static/live overlaps. The live player tile remained present.
- Full evidence: evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

## Remaining limits

- Classic sprite fallbacks are provisional 2.5D presentation. Final artistic and physical 3D fidelity are outside this milestone.
- The local classic reference pack and generated cache must not be published.
- V08 certification remains STANDBY and retains its previous approval state.

## Subsequent operation

- For a live visual run, regenerate or extend Saved/WideWorldCache around the chosen account position, then launch Unreal with the frozen experimental catalogue, cache path, preview path, and desired radius.
- Review wide-world cold-start cost separately if a performance budget is later defined. The live radius-64 run loaded successfully; no FPS target belongs to this milestone.
