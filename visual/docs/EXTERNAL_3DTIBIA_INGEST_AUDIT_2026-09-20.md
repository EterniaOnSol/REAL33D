# External 3DTIBIA art audit — 2026-09-20

Task: `REAL33D-3DTIBIA-ART-AUDIT-001`. Read-only source and identity audit; no source asset, gameplay, protocol, ClientCore or Unreal import was changed. This **supersedes the source-selection conclusion** of `INGEST_AUDIT_2026-09-20.md`: `fusion32/visual` is REAL33D's own inventory/reference area, not the brother's 3DTIBIA package. Its internal file counts remain valid only for that internal area.

## Source identity and Git

`ART_SOURCE_FOUND = YES`. Primary local path: `C:/Users/dell/3DTIBIA_leo`; `GIT_REPO = YES`; `origin = https://github.com/leodavidsoto/3DTIBIA.git`; branch `carril/ORQUESTADOR`; `HEAD = 21fafb57dd86b594223bab7dbe9076d1fa380640` (2026-09-19 checkpoint). The inspected `motor3d/assets` tree had zero Git status rows and all 96,478 files were tracked in that clone. This is a **source/staging project**, never a REAL33D protocol, gameplay or architecture authority. No fetch was run. Other local copies exist, including `C:/Users/dell/3DTIBIA` on a different, dirty branch; this audit uses the clean `3DTIBIA_leo` asset tree.

REAL33D remains `C:/Users/dell/Desktop/fusion32`, `EterniaOnSol/REAL33D`, `main`, starting `HEAD = origin/main = bf81f039aa7c89afdf6943c4a544f3b0affb3cf7`. Its initial status for this correction consisted only of the previous audit's uncommitted documentation changes, recorded in the handoff. No Git operation modified either repository.

Final REAL33D Git status is documentation only: modified `PROJECT_STATUS.md`, `PARITY_MATRIX.md`, `handoffs/CURRENT.md`; untracked the two audit reports and two archived handoffs. `HEAD` and local `origin/main` are still `bf81f039`. `git diff --check` produced no errors. Nothing was staged or committed.

## Complete `motor3d/assets` inventory

Recursive read-only count: **96,478 files**, **530,913,575 bytes**. Formats sum to the total exactly.

| Format | Count | Ingestion class |
| --- | ---: | --- |
| `.glb` | 4 | A: 3D exports |
| `.blend` | 4 | A: authoring files |
| `.jpg` | 2 | B: sidecar images for SF3D dragon; the GLB also embeds two JPEG images |
| `.png` | 49,240 | C: sprites, UI, outfit/effect/projectile references, plus 5 monster reference/preview images |
| `.import` | 46,951 | D: Godot generated import metadata; do not copy |
| `.json` | 276 | E: mostly 15.25/game/map data; one is the dragon's art metadata |
| `.bin` | 1 | E: map data |
| `.gltf`, `.fbx`, `.obj`, other 3D | 0 | — |
| Godot `.gd` within assets | 0 | F: Godot source is elsewhere in 3DTIBIA and is excluded |

Top-level groups: `items_1525` 88,827 files, `outfits_1525` 2,959, `objetos` 2,399, `items` 949, `efectos_1525` 485, `sprites` 315, `mapa` 254, `proyectiles_1525` 153, `paredes` 80, `modelos` 31, `ui` 15, `juego` 10 and `mapas` 1. The 15.25 indices, flags, automap and map data are explicitly **not** inputs to REAL33D identity or gameplay. No mass copy/import is justified by their presence.

## `motor3d/assets/modelos` inventory and technical inspection

The complete model subtree has **31 files**: four GLB, four BLEND, 15 `.import`, five PNG, two JPG and one JSON. `monstruos/` has exactly **one** creature directory. All four GLB passed basic GLB v2 magic/declared-length/JSON/chunk and buffer-view bounds checks; all contain one or more nonempty triangle primitives. This is a structural check, not an Unreal import, topology validation or visual approval. The four `.blend` files begin with a Zstandard frame header and were not opened/decompressed; their internal rig/geometry state remains `NOT_STARTED` for validation. No GLB has a skin, joint hierarchy or animation clip.

| Source path relative to `motor3d/assets/modelos/` | Size, bytes | Geometry/materials | Texture, rig, animation | Visual identity and quality finding |
| --- | ---: | --- | --- | --- |
| `outfit_022.blend` / `outfit_022.glb` | 424,480 / 1,481,340 | GLB: 1 mesh, 22,064 triangles, 1 material; local bounds X -0.75..0.781, Y 0.406..1.688, Z -0.625..0.781 | Vertex colour; no UV or image texture; no skin/clip | `outfit:22` **EXACT** (cyclops appearance). Pivot/foot clearance roughly 0.406 m requires review. Voxel model is documented as a failed visual experiment. |
| `outfit_034.blend` / `outfit_034.glb` | 752,351 / 3,149,140 | 1 mesh, 43,068 triangles, 1 material; bounds X -0.875..0.875, Y 0.063..1.906, Z -0.906..0.938 | Vertex colour; no UV/image texture; no skin/clip | `outfit:34` **EXACT**; REAL33D `mon:34` is dragon. Failed voxel experiment. |
| `outfit_035.blend` / `outfit_035.glb` | 971,728 / 4,213,004 | 1 mesh, 57,476 triangles, 1 material; bounds X -0.906..0.969, Y 0.063..1.938, Z -0.75..0.938 | Vertex colour; no UV/image texture; no skin/clip | `outfit:35` **EXACT**; race is **AMBIGUOUS** because five 7.72 monster races reuse that outfit. Failed voxel experiment. |
| `monstruos/34_dragon_verde/modelo_base.blend` / `modelo_sf3d.glb` | 101,986 / 800,960 | GLB: 1 mesh, 18,612 triangles, 1 material; bounds X -0.491..0.492, Y -0.487..0.479, Z -0.041..0.039 | GLB embeds base-colour and normal JPEGs; two sidecar JPGs present. UV present; no skin/clip | `outfit:34` **EXACT**, `mon:34` dragon. The BLEND is a guide/template, **not** the GLB's authoring source. The SF3D model was removed from live use by 3DTIBIA after a visual rejection; it has about 0.079 m thickness against 0.983 m width. |

The dragon folder additionally has `_metadatos.json` (`id_outfit: 34`, `nombre: dragon_verde`, `alto_en_tiles: 2.0`, four directions), four directional PNG references, one preview PNG and their `.import` files. The metadata gives a target height of 2 tiles; the SF3D mesh's local Y span is about 0.966 m and its pivot is centred, so scaling/foot placement need explicit import settings if it were ever reconsidered. The GLB's JPEGs are embedded, so no external texture reference is missing in the four GLBs. No separate roughness, metallic, AO or opacity map was found for these models. `glTF` uses Y-up; REAL33D uses Z-up, so axis conversion belongs in import configuration. A reliable creature forward axis and animation behaviour cannot be inferred from these static files. Material appearance in Unreal remains untested.

The voxelizer declares 32 sprite pixels per tile and 1 source unit per field; REAL33D's presentation uses approximately `1 SQM = 100 Unreal Units`. Preserve source geometry and adapt units in the Unreal import pipeline. All four GLBs had different SHA-256 hashes; the outfit-34 voxel mesh and SF3D mesh are distinct visual variants of the same 7.72 appearance, not byte duplicates. Full `.blend` integrity and mesh topology remain to be validated by an appropriate importer before any technical-import claim.

The three outfit exports were generated by `herramientas/voxelizar.py` and `construir_en_blender.py` from `motor3d/assets/sprites/outfit_0XX.png`, according to `docs/EXPERIMENTO_VOXEL.md`. That document explicitly says the technique failed visually and the exports are not connected to the game. `LEEME.md` describes `modelo_sf3d.glb` as an SF3D generation from the old 7.72 dragon reference, also visually rejected and no longer used as `modelo.glb`. The presence of files therefore does **not** mean approved production art.

## Cross-version identity evidence

The three source sprite sheets are the repository's *old 7.72 extraction*, separate from `outfits_1525/` (1,479 15.25 sheets). `LEEME.md` and `herramientas/extraer_sprites.py` state this provenance. Their 12 directional/frame tiles were compared read-only with REAL33D's local 7.72 reference previews, reversing only the preview's 3x nearest-neighbour scale/background. RGB equality on every opaque pixel was **24,104/24,104** for outfit 22, **18,726/18,726** for outfit 34 and **32,319/32,319** for outfit 35; every transparent pixel matched the preview background. This is positive cross-source appearance evidence, not a modern-ID guess.

REAL33D's `visual/manifests/creatures.csv`, `outfits.csv` and local reference pack establish `outfit:22` as the cyclops appearance (shared by `mon:22` and NPC A Sweaty Cyclops), `outfit:34` as the dragon appearance (`mon:34`), and `outfit:35` as the demon appearance reused by demon, Apocalypse, Bazir, Infernatil and Morgaroth. The folder prefix `34` was **not** accepted as a race by itself; the 7.72 manifest independently supplies race/outfit/name evidence. Where multiple semantic entities share an outfit, the physical mesh maps to the outfit only.

Mapping unit = **four GLB candidates**, with their BLEND files paired as sources/templates rather than counted again. Visual-identity confidence: `EXACT 4`, `PROBABLE 0`, `AMBIGUOUS 0`, `UNMAPPED 0`. Secondary **race** mapping remains ambiguous for the 22 and 35 appearances. `READY_FOR_IMPORT = NO` for all four because source-project quality findings, absent rig/animation and unreviewed redistribution provenance remain. None is a P0 Rookgaard object or an approved P0 creature sample. Cyclops has P1 Rookgaard-nearest spawn attribution; dragon and demon are P2 or unprioritized, and P1 is not P0.

## Provenance and publication

The model authoring chain is partly recorded, but the appearance input is CipSoft sprite art. `LEEME.md` explicitly identifies the 15.25 sprites as official client art and cautions against public redistribution; its 7.72 extraction also does not establish redistribution rights. `herramientas/sf3d/LICENSE.md` carries the Stability AI Community License for the generator, which alone does not settle rights in a model generated from a third-party dragon sprite. No per-model license or artist rights declaration was present in `motor3d/assets/modelos`. Classification for publishing these four meshes or their texture derivatives to REAL33D: `THIRD_PARTY_UNKNOWN`. Do not commit them, the 49,240 PNGs, the 15.25 datasets or Godot generated files to REAL33D at this stage. REAL33D's `.gitattributes` already routes `.glb`, `.blend`, `.png`, `.jpg` through LFS, but LFS is storage policy, not rights clearance.

## Small vertical sample decision

**No 3–5 asset integration set is ready from the primary `motor3d/assets` tree.** It contains only the four rejected/experimental creature meshes and no ground or static prop GLB. Choosing them to meet a count would falsely imply quality and P0 readiness. Keep `IDENTITY_MAPPING = PASS` only for the narrow outfit identities; `TECHNICAL_IMPORT`, `IN_ENGINE_RENDER` and `VISUAL_APPROVAL` remain `NOT_STARTED` for REAL33D.

A **separate** local, non-Git package exists at `C:/Users/dell/Desktop/3DTIBIA_para_hermano/arte/`: 65 GLB and 431 PNG. Its README says it was prepared for another project and names three props as passing its own quality pilot. It is **not counted** in `motor3d/assets` or attributed to the clean clone's HEAD. If that package is confirmed as an acceptable artist source, a next scoped audit could consider three P0 static-display candidates: `props/2487_bed.glb` (`obj:2487`, source-project pilot accepted), `props/2370_bench.glb` (`obj:2370`, quality unverified), and `props/2488_bed.glb` (`obj:2488`, quality unverified). Their names, 7.72 reference pictures and REAL33D object rows align, but no automatic import is proposed before separate file-level provenance, dimensions/pivot/material QA and director review. The pilot-accepted `2473_box.glb` and `2523_barrel.glb` are P0 container objects, so they are left out of this task's sample to avoid advancing container work.

For a later verified sample, `UReal33DAssetRegistry::ResolveGround` and `ResolveThing` are the existing lookup seam. Do not hardcode paths in actors or copy Godot code. Creature presentation remains a separate design task: the registry receives a session `CreatureId`, while `WorldState::CreatureRecord` already knows the outfit but current `WorldEvent`/Unreal events do not carry it. An eventual outfit-to-visual mapping must cross the semantic event boundary without using transient creature IDs as persistent asset identities. No registry redesign was done in this audit.

## Reproducibility and limits

`VISUAL-3DTIBIA-ASSET-INVENTORY-001 = PASS` for the directory/format/model count at the stated clone HEAD, from `Get-ChildItem -Recurse -File -Force`, `git ls-files` and status restricted to `motor3d/assets`. `VISUAL-3DTIBIA-GLB-STRUCTURE-001 = PASS` for four GLB header/chunk/JSON/buffer-view reads with positive mesh/triangle counts; it does not prove complete geometric validity or engine import. `VISUAL-772-OUTFIT-CROSSCHECK-001 = PASS` for the three pixel comparisons above. The source-project's documented failed visual experiment remains failed; no REAL33D render or visual approval was executed.
