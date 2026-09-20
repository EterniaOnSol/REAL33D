# HANDOFF

Date/time: 2026-09-20, America/Guatemala
Agent: Codex
Role: Unreal V08 presentation and QA
Task: VISUAL-FULL-CATALOG-INGEST-TEST-001, operator-directed presentation corrections, and Thais temple live session
Branch: main; checkpoint based on 78c2797988295d396fe907ae0cb0298d4a1c4d89; ending commit: this handoff's commit
Worktree: C:/Users/dell/Desktop/fusion32
Status: IN_PROGRESS. This commit is authorized by the operator; visual certification and artistic approval remain open.

## Scope and decisions

- Unreal-only presentation corrections: floor spacing, selected rotations, counter placement, roof visibility, camera-relative WASD, faster orbit, continuous visual movement, and V08 in-world ID notes.
- Fusion32 server source, protocol, ClientCore semantics, WorldState semantics, combat, chat, containers, quests, and houses were not modified.
- Experimental `.uasset` binaries remain ignored by the narrow `/unreal/REAL33D/Content/Experimental/V08/` rule. The editor-generated `Config/DefaultInput.ini` change is excluded from this commit.
- Source GLBs, imported asset identity, and logical Fusion32 TypeIds remain distinct. TypeIds 3497 through 3500 display mesh 3502, and wall TypeIds 1295, 1301, and 1303 display mesh 1294, as explicit V08 visual aliases while retaining their server identities. TypeId 429 remains a stone tile.

## Changes

- Set presentation floor spacing to 220 Unreal units and extend short structural V08 wall meshes vertically to that height.
- Rotate only operator-reviewed IDs 429, 870, 1270, 1271, 1281, 1282, 1294, 1295, 1301, 1303, 1734, 1735, 2154, 2156, 2162, 2164, 2173, 2174, 4461, 4464, and 4465 by yaw 90 degrees. The 379-wall global rule was abandoned because the operator found mixed orientations. Roofs 1158/1161 and wooden floor 408 remain unchanged.
- Lift archways 1626 and 1627 to the top of the 220-unit room span, following the operator notes.
- Place eight V08 counter TypeIds (2317, 2318, 2320, 2321, 2342 through 2345) and the twelve plain-table TypeIds (2322 through 2333) horizontally, centered and resting on the tile floor.
- When a nonvoid tile covers the local player's position, hide tiles and creatures above the player floor. The live log records `covered=true` and hidden upper tiles; the final visual review is pending.
- Make WASD follow camera yaw and repeat movement intent while held. Arrow keys remain cardinal. Increase orbit sensitivity to 0.40 degrees per mouse unit. Creature rendering advances at constant 220 units/second with a small walking cue; Fusion32 remains movement authority.
- Add an in-world V08 TypeId inspector and TSV note panel. `wall_inspector_notes.tsv` contains operator annotations; excluded roof/floor notes are preserved as history.
- Correct the earlier milestone overclaim: 4,913 imports succeeded, but `VISUAL-FULL-CATALOG-INGEST-TEST-001` remains incomplete.

## Tests and evidence

- `python scripts/client/validate_v08_mapping.py`: 4,913 manifest/import/runtime rows checked, 25 deterministic source GLB hashes, zero identity mismatches. The script produces `visual/qa/full_catalog_v08/MAPPING_VALIDATION.md` and includes 3501 and 3508. This is static mapping evidence, not live actor proof.
- UE 5.8.2 build succeeded and `REAL33D.AssetRegistry.ExperimentalV08` completed with Result=Success and exit code 0 after the latest rotation, table, and visual-alias changes. The test checks depot and wall aliases, reviewed rotations, unchanged roof/floor, horizontal counters, and plain tables.
- `scripts/client/run_v08_gallery.cmd` opened and loaded the V08 catalog. Gallery screenshot, all requested category/warning examples, and FPS are still missing; gallery certification remains open.
- `scripts/client/run_unreal_v08_experimental.cmd B` connected live to Fusion32. The V08 floor visibility log and operator's 57 inspector notes show in-world use. No claim of live 3508 actor proof is made.
- The attempted experimental water-edge/material change was fully reverted at the operator's request. Water is back to the pre-experiment V08 presentation; no water material, script, or report is included in this checkpoint.
- Test Player B's sanitized runtime account was moved to Thais temple coordinate (32369,32241,7) while the services were stopped. After restart, REAL33D's log recorded `connected: Test Player B`, `local player is creature 1002`, and `creature 1002 ... appeared at 32369,32241,7`. The runtime save is outside this repository and is not included in Git. The operator subsequently moved; after the final test and restart, the live client reconnected at (32330,32226,7). UE and the three Fusion32 services remained running at handoff.
- QA warnings remain pending visual assessment: 4,397 degenerate UV, 2,272 zero-area triangle, 1,253 pivot, and 19 very-flat entries. No harmless/cosmetic conclusion was made.

## Remaining work

- Recheck latest floor occlusion, wall selection, counters and tables, depot and wall appearance aliases, camera speed, and held movement in the running world.
- Capture real gallery screenshots for 3501, 3508, each refinement state, and warning examples; report startup time, visible chunk size, peak memory, FPS/range, and crashes/asserts.
- Prove the full live identity chain for 3501 and 3508 and visually inspect representative warnings before changing the milestone to PASS.
- Keep TEST_IMPORTED=YES, APPROVED=NO, READY=NO, PRODUCTION_INTEGRATED=NO for every V08 asset until human review.

## Next commands

- `scripts/client/run_v08_gallery.cmd`
- `scripts/client/run_unreal_v08_experimental.cmd B`
- `python scripts/client/validate_v08_mapping.py`
- Review `evidence/clientcore/unreal-slice/v08-experimental/wall_inspector_notes.tsv` before any new orientation changes. The live client was closed to create a stable checkpoint.
