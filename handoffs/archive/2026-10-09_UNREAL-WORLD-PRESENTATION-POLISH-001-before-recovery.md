# CURRENT - Presentation polish recovered; live QA continuing

UPDATE 2026-10-07 18:15: first fresh session ended normally at
32091,32176,6 (1373 commands, protocol errors0). Operator rejected underground
appearance and reported missing inventory icons. Source masked_ground/collapse
proves centered borders lost XY offsets. AssetRegistry now restores offsets for
4785..4792 and tests actual bounds for4785..4796; build and three automation
tests PASS. Inventory4913 PNGs generated into this worktree's ignored
Resources/UI/Items; existing comparison PASS4990 layouts/8163 sprites.
QA launcher now generates icons before launch and requires brother creature
catalog. User requested his brother's monsters: catalog already enabled, all16
folders present, live rat21/snake28 loaded; no reimport or substitute needed.
Latest details and failure captures are in the recovery report/compact folder
below. Runtime source is now changed by this bounded border correction,
superseding the earlier note that recovered runtime source was untouched.
Corrected client is being relaunched for live comparison; recheck latest PID.

Date/time: 2026-10-07 17:53 America/Guatemala (initial recovery checkpoint).
Agent/role: Codex, recovery and local verification.
Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IMPLEMENTED_UNVERIFIED.
Branch: milestone/unreal-world-presentation-polish-001.
Starting/ending HEAD: 4305d23738454c2530007b16e759cbcaeddbca55 (uncommitted work retained).
Main and origin/main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.

## Recovery and scope

Verified worktree list, branch/HEAD/status, live remote heads, both merge bases,
ancestor main -> 393e7e8 -> 4305d23, commits outside main and complete diff.
Main is clean; polish is 0 behind/2 ahead and remote matches local HEAD.
No new branch/reset/rebase/main merge/push. Frozen agent checkout is untouched;
four modified evidence files there predated this run. Preserve them.

Read startup contract/docs, SOURCE_TRUTH, current/prior handoffs and all polish
reports. Inspected TileActor::ApplyStack, StaticSectorActor::Build, AssetRegistry
TryResolveExperimental/ResolveInstancedMaterial, presentation policy/tests,
launchers and geometry/evidence. Previous handoff archived verbatim at
archive/2026-10-07_UNREAL-WORLD-PRESENTATION-POLISH-001-before-recovery.md.

Recovered eight modified source/test files, render-fixes report and artifacts.
Terrain layering, wall orientation and transient HISM material adapter already
existed. Preserved them rather than reimplementing. Newly recovered raw results
prove the material adapter had built and passed three tests. October 3 screenshot
shows distant textures and its EndPlay contains real movement, but aggregate
Use/Follow counters do not prove their visual success or complete acceptance.

Changes this run: test_unreal_presentation.ps1 rejects existing/missing reports
and requires every named test to pass; evidence report/compact artifacts,
PROJECT_STATUS.md, PARITY_MATRIX.md and this handoff. Recovered runtime source
is unchanged. Only Unreal presentation/test/evidence scope; no gameplay/protocol,
ClientCore, V08 source, server configuration, 2D or autonomous-agent edits.

## Tests and evidence

PASS: native nine-suite ClientCore (C++17/C++20 builds); Unreal build retry
BUILD_EXIT=0; CameraAndFloors, AuthoritativeWindow and ExperimentalV08 automation
Success/errors0, one expected catalog warning; stale-report refusal exit1 before
engine launch; git diff --check; secret_check tracked tree/reachable history.
Restricted Unreal build stopped at UBT exit1. Tool server interrupted the first
fresh automation before results; process absence verified before separate retry.
Do not credit either incomplete attempt. Build/test success is not live parity.

Report: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RECOVERY-20261007.md.
Compact: evidence/clientcore/unreal-world-presentation-polish-recovery-20261007/.
Raw: build/unreal-world-presentation-polish-001/recovery-20261007/.
Recovered earlier raw session: build/unreal-world-presentation-polish-001/live-20261003T155044/.
Untracked September29 render-fixes report/artifacts remain preserved too.

## Runtime and exact next step

Services were stopped. Installed executable hashes matched prepare_wsl.sh.
Prior logs/PID/start records preserved with .preserved-20261007T1745 suffix;
internal game lock absent. Existing certified startup ran; querymanager860,
game873, login1183 passed identity/cwd/start/ports/readiness checks. No account,
map or config reset. Hidden bounded WSL host Windows PID5568; recheck liveness.

Fresh normal Account B Unreal PID14100, launched after all targeted tests:
build/unreal-world-presentation-polish-001/live-20261007T175204/.
Operator agreed to provide ordinary gameplay inputs; native UI control unavailable.
Asked for underground water-edge F9, return-outside F9 and speech F9 during its
six-second lifetime. Inspect new JSON/PNG and journal before attributing success.
Then validate successful Use With, Follow/Stop, minimap-click arrival, sector
travel and camera clearance return using ordinary observed client state only.

Pending/risks: underground edge fix and full live acceptance IMPLEMENTED_UNVERIFIED;
previous FAILED overlap must not be silently cleared; material adapter WITH_EDITOR
only, cooked shader preparation still unverified; outfit/lamp art gaps remain.
The material adapter copies graphs transiently, never saves original packages.
No independent certification or main integration until full local acceptance.

Reproduction commands (from this worktree):
- build/unreal-world-presentation-polish-001/recovery-20261007/run-checks.cmd
- powershell -ExecutionPolicy Bypass -File scripts/client/test_unreal_presentation.ps1 -ExperimentalCatalog visual/qa/full_catalog_v08/experimental_catalog_runtime.json
- powershell -ExecutionPolicy Bypass -File scripts/client/run_unreal_presentation_qa.ps1 -Account B
Use fresh evidence directories. Before launching check existing PID/session.
Do not rerun startup while services are already alive. F9 captures screenshot,
semantic state and live geometry. Normal window close records EndPlay.
