# Presentation polish recovery - 2026-10-07

Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IMPLEMENTED_UNVERIFIED.
Agent: Codex, recovery and local verification; no independent certification.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.

## Verified recovery

Both local origin/main and live `git ls-remote origin refs/heads/main` returned
fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. Main worktree is clean (read-only
per-command safe.directory exception used for the sandbox account). Current
polish HEAD and remote polish HEAD are 4305d23738454c2530007b16e759cbcaeddbca55,
newer than the requested checkpoint 393e7e8570319a35caef7072065d29cf67448475.
Both ancestor checks exited 0: main -> 393e7e8 -> HEAD. Both merge bases with
main equal fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. Symmetric difference is
0 main-only / 2 polish-only commits:

- 393e7e8: preserve presentation polish with live defects documented.
- 4305d23: preserve polish recovery and live rendering findings.

No branch creation, reset, merge, rebase, push or change to the frozen agent
checkout. Its four pre-existing modified unreal-slice evidence files were
observed, not cleaned. Their hashes are retained in before-qa-manifest.json.

Recovered eight modified source/test files and the untracked September 29
render-fixes report/artifacts. The report describes terrain layer separation,
wall alias orientation and a transient instanced-material adapter. All remain
preserved. Inspected ApplyStack, StaticSector::Build, TryResolveExperimental,
ResolveInstancedMaterial, presentation policy/tests, launch/test scripts, prior
reports/handoff, project contract and source authority inventory.

## Previously unreported completed work

The raw material-fix build finished successfully. September 30 02:52 UTC
automation reports Success for ExperimentalV08, CameraAndFloors and
AuthoritativeWindow, errors 0, one expected experimental-catalog warning.
This supersedes the render-fixes report's final "not yet PASS" statement only
for that build and automation, not the overall live milestone.

Recovered session live-20261003T155044 has a settled screenshot at
(32060,32190,7) showing textured distant terrain instead of the previous gray
default-material region. Camera/scene framing differs from the earlier capture;
this is bounded visual evidence, not a pixel-identical comparison. No
InstancedStaticMeshes/default-material substitution warning was found in its
log. The adapter still requires WITH_EDITOR; cooked packaging is unverified.

Its EndPlay records (32090,32195,7), 143 commands, zero residual bytes,
unsupported opcodes and anomalies; synchronized viewport, 88 sector loads and
26 unloads. It records 55 requested walks, 51 accepted, 13 rejected, 0 unanswered,
2 external relocations, 2 Uses and 2 Follows. These aggregate categories are not
a one-to-one action ledger and do not establish successful Use/Stop Follow or
the entire gameplay regression. No new acceptance is inferred from counters.

## Changes and fresh verification

The test runner now refuses an existing report directory and requires each
named test to return Success with zero errors. Missing reports/tests, skipped
or unfinished tests and nonzero engine exits fail the run. This prevents stale
or empty runs from looking successful. Runtime source changes recovered above
were not rewritten during this recovery.

- Native ClientCore: PASS, all nine suites; C++17 and C++20 libraries built.
- Unreal build: PASS, normal-permission retry returned BUILD_EXIT=0 and
  Result Succeeded. Restricted attempt stopped at UnrealBuildTool with exit 1;
  retain both logs, credit only the retry.
- secret_check: PASS for tracked tree/reachable history. New untracked
  artifacts are not implied to be covered by that script.
- git diff --check: PASS.
- Fresh automation: PASS. CameraAndFloors, AuthoritativeWindow and
  ExperimentalV08 all Success, errors 0; expected experimental-catalog warning
  only. Runner returned 0. First attempt was interrupted by the tool server
  restart before a report existed; process absence verified before retry.
- Stale-report refusal: PASS. Reusing the completed evidence directory returned
  1 with the expected fresh-directory error before starting another engine.

Compact artifacts: unreal-world-presentation-polish-recovery-20261007/.
Raw checks: build/unreal-world-presentation-polish-001/recovery-20261007/.

## Runtime and remaining acceptance

Initial status reported all three Fusion32 services DEAD. Installed executable
SHA256 values matched scripts/server/prepare_wsl.sh's three certified hashes.
No internal game lock was present. Prior log/PID/start files were copied beside
the originals with suffix .preserved-20261007T1745 before the existing startup
script ran. Querymanager860, game873 and login1183 passed executable/cwd/start/
port ownership and readiness checks. No account/config/map/source reset.
Bounded hidden WSL host PID5568 was started for QA; recheck before reusing.

Fresh normal Account B session launched as PID14100, evidence directory
build/unreal-world-presentation-polish-001/live-20261007T175204/.
Operator agreed to provide normal gameplay input after verification. Pending:
underground water-edge readability, wall/camera return review, speech captured
within its six-second lifetime, successful Use With, Stop Follow, minimap-click
and sector coexistence, and the full presentation regression. Existing art
coverage/lamp defects remain separate. Compilation and tests do not certify
these live behaviors. No main integration is performed.

## Operator findings and corrective work

First fresh session closed normally with EndPlay. Operator verdict:
"underground looks awefull.. also is not displaying the sprites on the inventory".
Manual 23:53:04.511 and 23:53:06.933 UTC captures confirm disconnected water/dirt
strips and corners on floor8. Underground readability remains FAILED for that
revision; two-unit vertical layer separation alone did not fix the layout.

Inventory: this worktree lacked Resources/UI/Items. UIStyle::ItemBrush looks up
that exact location and caches absent IDs as null per session. The existing
extract_item_sprites.py generated 4913 PNGs (77 blank appearances); the existing
verify_item_sprite_extraction.py compared all 4990 layouts and 8163 sprite pixels
with zero failures. The QA launcher now generates local item images from the
same validated Tibia.dat/Tibia.spr pair before launch and fails on extraction
error. Generated images remain ignored; no artwork is committed.

Water-border source trace (read-only clone C:/Users/dell/3DTIBIA_leo):
produccion_sprites_v4/geometria.py::masked_ground builds alpha-mask contours at
x/w-.5,y/h-.5. piloto_3d_v3/generar.py::collapse unconditionally subtracts the
XY bounding center, even for ground; it only exempts ground's Z origin.
The GLB files under worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/
models match the manifest's centered border bounds. UE5.8 GLTFCore
Private/GLTF/ConversionUtilities.h::ConvertVec3 maps X,Z,Y; Blender's bottom-up
pixels/export therefore match Unreal's east/south axes without additional yaw.
No protocol or server version guard applies to these presentation transforms.

The eight partial masks 4785..4792 need their lost XY offsets restored; full-span
triangles4793..4796 keep zero offset. Offsets derive from exclusive alpha bounds:
((min+max)/2/32-.5)*100 per axis. terrain_reference.py and
terrain-reference-offsets.json reproduce all12 using the validated extractor.
TryResolveExperimental now restores only those eight offsets for both live and
static consumers. Original mesh/material/source packages stay unchanged.
The catalog automation checks all12 actual transformed mesh bounds against
independent sprite-pixel bounds, including unchanged full-span triangles.

Operator additionally requested his brother's monsters. The existing launch
already supplies brother_creatures/runtime.json, all16 imported outfit folders
exist, and this fresh log confirms rat21 and snake28 multipart models loaded.
The catalog includes 15 creature forms plus Rookie128. Missing outfits136/57/
129/131 were explicitly logged as placeholders; no NPC-name substitution.
The launcher now requires the catalog file to exist. No new creature import or
source-art modification was needed to enable this delivery. Complete artistic
approval and attack/death animation triggers are not inferred.

Corrected border build PASS (BUILD_EXIT=0). Fresh automation-borders returned
PASS for all three named tests, including all12 actual mesh footprints; report
errors0, one expected experimental-catalog warning. Native suites were not
repeated for this rendering-only delta. Sprite generation and catalog loading
are included in the fresh normal-client launcher.

Corrected live session: live-20261007T181511, Unreal PID9048. Operator asked to
repeat the underground F9 and check inventory sprites. Comparison IN_PROGRESS.
git diff --check PASS; main still clean and frozen evidence hashes unchanged.
