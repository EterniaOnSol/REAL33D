# HANDOFF

Date/time: 2026-09-20 03:38 -06:00 (America/Guatemala)
Agent: Codex
Role: external art source auditor
Task: `REAL33D-3DTIBIA-ART-AUDIT-001`
Branch: REAL33D `main`; external 3DTIBIA `carril/ORQUESTADOR`
Starting/ending REAL33D commit: `bf81f039aa7c89afdf6943c4a544f3b0affb3cf7` (no commit)
External 3DTIBIA HEAD: `21fafb57dd86b594223bab7dbe9076d1fa380640`
Worktree: `C:/Users/dell/Desktop/fusion32`; documentation edits uncommitted, no asset or Unreal code edits. External source `C:/Users/dell/3DTIBIA_leo` was read only.

## Objective and source correction

The previous audit inspected `fusion32/visual`, which is REAL33D's internal reference area. The user corrected the source to `leodavidsoto/3DTIBIA`. Before any new inspection, `git status --short` and `git diff --stat` for REAL33D showed only the previous audit's uncommitted documentation. The actual Git clone was found at `C:/Users/dell/3DTIBIA_leo`, origin `https://github.com/leodavidsoto/3DTIBIA.git`; the specific `motor3d/assets` subtree had zero status rows and 96,478 tracked files. The former handoff was archived byte-for-byte at `handoffs/archive/2026-09-20_REAL33D-VISUAL-INGEST-AUDIT-001.md`; the prior chat handoff remains at `handoffs/archive/2026-09-19_UNREAL-CHAT-AREA-001.md`.

## Inspection and findings

Read the REAL33D startup documents and registry in the preceding turn; inspected `motor3d/assets`, all 31 files under `modelos`, the one monster metadata JSON, 3DTIBIA's source/provenance and failed-model documentation, REAL33D's 7.72 object/creature/outfit/P0 manifests and local reference previews. The external project is Canary/Tibia 15.25 overall, but its three voxel outfit GLBs were generated from its older 7.72 sprite sheets, not `outfits_1525`. Pixel comparison of all 12 source frames per outfit to REAL33D's 7.72 previews was exact on all opaque pixels. No number from the 15.25 item universe was mapped to a 7.72 object by coincidence.

`motor3d/assets`: 96,478 files = 4 GLB, 4 BLEND, 2 JPG, 49,240 PNG, 46,951 Godot `.import`, 276 JSON, 1 BIN. All other 3D formats absent. Model identities: `outfit_022` -> REAL33D `outfit:22` cyclops appearance; `outfit_034` -> `outfit:34` dragon; `outfit_035` -> `outfit:35` demon appearance reused by five 7.72 races; `monstruos/34_dragon_verde/modelo_sf3d.glb` -> `outfit:34` dragon. Confidence per GLB visual identity: `EXACT 4`, `PROBABLE 0`, `AMBIGUOUS 0`, `UNMAPPED 0`; race remains ambiguous for the 22/35 shared outfits. None is P0 or ready to import: the three voxel meshes are documented as visually failed experiments and the SF3D dragon was removed from use after being judged too flat. Four GLBs structurally parse and have no skins or animations; no Unreal rendering or visual approval was performed. These assets derive from 7.72 CipSoft imagery and have no per-model publication grant, so do not commit them to REAL33D.

A separate older, non-Git package at `C:/Users/dell/Desktop/3DTIBIA_para_hermano/arte/` contains 65 GLB and 431 PNG and may supply P0 props, but it is **not** the primary clone and was not merged into its file counts or HEAD. Three conditional, non-container P0 candidates for a later scoped audit are `2370_bench.glb`, `2487_bed.glb`, `2488_bed.glb`; only the bed 2487 was called acceptable in that package's own pilot. Source ownership, exact file-level mapping, quality and technical import remain to be checked before any REAL33D integration.

The existing Unreal design issue remains: `UReal33DAssetRegistry::ResolveCreature` receives a session `CreatureId`, not a persistent outfit/race visual identity. `WorldState::CreatureRecord` holds outfit data but current semantic/Unreal events do not carry it; creature presentation is a static mesh. No redesign was undertaken.

## Changes and evidence

- Added `visual/docs/EXTERNAL_3DTIBIA_INGEST_AUDIT_2026-09-20.md` with exact counts, four per-model rows, cross-version pixel evidence, technical/provenance findings, P0/sample decision and reproducible limits.
- Marked `visual/docs/INGEST_AUDIT_2026-09-20.md` as a superseded source-selection finding while preserving valid internal directory counts.
- Corrected only the previous audit paragraphs in `PROJECT_STATUS.md` and `PARITY_MATRIX.md`; prior functional parity claims remain unchanged.
- Archived the previous handoff and wrote this one. No copy/import of art, no code change, no commit, no push, no source modification.

`VISUAL-3DTIBIA-ASSET-INVENTORY-001 = PASS`: recursive/file-format counts at the external HEAD, all tracked and the asset subtree clean. `VISUAL-3DTIBIA-GLB-STRUCTURE-001 = PASS`: four GLB v2 headers, JSON/chunks and buffer-view ranges parsed with positive geometry counts; no complete topology/import claim. `VISUAL-772-OUTFIT-CROSSCHECK-001 = PASS`: opaque RGB matches 24,104/24,104 (22), 18,726/18,726 (34), 32,319/32,319 (35), with transparent pixels matching the local preview background. Detailed commands, preconditions and limits are in the audit report. `TECHNICAL_IMPORT`, `IN_ENGINE_RENDER` and `VISUAL_APPROVAL` are `NOT_STARTED`; no visual parity cell changes.

## Next task and risks

The user asked for this inventory before deciding on vertical integration. For a first static sample, conduct a separate read-only audit of the three P0 prop files in `C:/Users/dell/Desktop/3DTIBIA_para_hermano/arte/props`, including source notes, texture comparison to REAL33D's local reference pack, scale/pivot/material/UV validation and rights. Keep the non-Git package provenance distinct from the clean 3DTIBIA clone. Identity authority: `visual/manifests/objects.csv`, `creatures.csv`, `outfits.csv`, `visual/rookgaard_p0/P0_ASSETS.csv`; presentation seam: `unreal/REAL33D/Source/REAL33D/Public/Real33DAssetRegistry.h` and `Private/Real33DAssetRegistry.cpp`. Do not change creature events or the registry until an approved sample requires it. Preserve all source files, avoid 15.25 ID equivalence, and do not copy Godot/generated/data files.

Critical context: source project quality documentation rejects all four primary model GLBs as production visual replacements even though their 7.72 outfit identities are exact. `EXACT` means identity only, never import or director approval. No P0-ready 3D asset was found in the primary `motor3d/assets` tree. The separate package's GLBs are potential follow-up sources, not audited and not approved by this report.
