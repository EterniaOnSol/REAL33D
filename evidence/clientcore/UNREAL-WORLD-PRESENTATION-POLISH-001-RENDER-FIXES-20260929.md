# Presentation rendering fixes, 2026-09-29

Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IN_PROGRESS.
Agent: Codex, implementation and local QA (not independent certification).
Worktree: build/unreal-world-presentation-polish-001.
Branch: milestone/unreal-world-presentation-polish-001.
Starting local and remote HEAD: 4305d23738454c2530007b16e759cbcaeddbca55.
Certified base/main: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.

## Recovery

Remote heads were read with git ls-remote; main matches the requested base.
Merge-base equals that base; git merge-base --is-ancestor base 393e7e8 exited 0.
The clean existing branch has two commits outside main: 393e7e8 and 4305d23.
The latter is a newer preservation checkpoint than the request identified.
Both are preserved; no new branch, reset, rebase, cherry-pick or main merge.
Main and frozen agent checkout were clean when inspected and remain outside
the implementation scope. Existing reports, captures and handoffs are retained.

393e7e8 already implemented camera/floor/cutaway policies, screen entity labels,
speech/health/target/follow presentation, interaction feedback, the area audit,
launchers and tests. 4305d23 preserved the label paint-space correction,
geometry diagnostics and further live evidence. This run does not redo them.

## Runtime defects and changes

- BUG: live TileActor::ApplyStack and StaticSectorActor::Build gave the first
  nonground item no height separation. The retained 22:56:37.342 capture shows
  severe underground water-edge overlap, with ground622 and border4785 at the
  same projected center and each 4.50 units thick. Both consumers now classify
  thin transformed mesh bounds (at most 10 units high) as flat surfaces and add
  2 units per item layer, including the first border. Ground remains at layer0.
  This is draw placement only; the ordered logical stack is unchanged. Tall
  geometry retains its previous placement policy.
- BUG: AssetRegistry::TryResolveExperimental applied yaw90 to all four reviewed
  wall aliases. Local classic previews1294/1301 show north-south walls;
  1295/1303 show east-west walls. The retained live row at y32200,z6 has1295/1303
  along X but bounds29/100/220, producing perpendicular fins. Those two IDs now
  keep the shared1294 mesh at yaw0 (long axis X). 1294/1301 retain yaw90. Meshes,
  visual source IDs, materials and V08 production content are unchanged.
- Targeted tests check terrain separation and unchanged ground/tall geometry;
  the existing real-catalog registry test now checks corrected yaw and actual
  transformed east-west wall bounds. The presentation runner accepts an explicit
  catalog to run that gated test along with camera/floors and WideWorld tests.

Preview provenance (local read-only input, not published art):
visual/reference_pack/previews/item/ under the existing asset workspace.
1294.png and1301.png SHA256:
725ccf34e210db6e0f05b9692f0aa1da7bb6f13cc0219a0855d755593c97912c.
1295.png and1303.png SHA256:
a9e30e0754585bd1fc09c48fad377f8359dbba5dd61d95a5f702fca3f14bdf3d.
Historical classic-client provenance remains UNKNOWN per SOURCE_TRUTH.md.

Raw new results: build/unreal-world-presentation-polish-001/render-fixes-20260929/.
Live and final check results will be recorded below. Compilation is not gameplay
parity. Previous live failures remain open until a new comparison closes them.

## Live access

Computer-use initialization succeeded but list_apps failed: native pipe missing,
OS error2. Operator agreed to provide normal gameplay inputs and F9 captures.
Existing Fusion32 querymanager813/game830/login999 each passed the certified
status script's executable, cwd, start-time and port checks. No server restart,
config/account/map edit, legacy executable or hidden gameplay state was used.

## Separate art findings

V08_ART_DEFECT: retained missing outfit coverage, lamp2108/2109 assembly/UV
warnings and provisional mountain1112/1113 source-shape limitations. None is
silently repaired by replacing assets. Art completeness is not a runtime PASS
requirement. New live findings will be classified individually.

## Executed checks

- CLIENTCORE_TESTS: PASS. VS2022 vcvars64 + tests/build_clientcore_windows.cmd;
  C++17 and C++20 libraries built, all nine native suites passed.
- UNREAL_BUILD: PASS. UE5.8 Build.bat REAL33DEditor Win64 Development with this
  worktree's explicit project path, -WaitMutex -NoUBA; eight actions completed,
  Result Succeeded. The raw wrapper was edited while running to restore cwd
  after ClientCore's script, then returned a trailing command error. A direct
  Build.bat invocation resolved this wrapper ambiguity: Target is up to date,
  Result Succeeded, BUILD_EXIT=0. Both engine logs are retained.
- Unreal automation: PASS. REAL33D.Presentation.CameraAndFloors,
  REAL33D.WideWorld.AuthoritativeWindow and REAL33D.AssetRegistry.ExperimentalV08
  all returned Success, zero errors; the catalog test emitted its expected
  experimental-assets warning. This was run with the explicit4913-entry V08
  catalog, not skipped. Engine and wrapper exit0.
- Restricted automation attempt: FAILED to initialize writable engine cache;
  no test result credited. The normal-permission retry above completed. The
  original engine exited before the targeted stop attempt, which reported that
  its PID no longer existed. Raw failure/retry logs are preserved separately.

Compact outputs: unreal-world-presentation-polish-render-fixes-20260929/.
Live session: live-20260929T203943, Unreal PID5600. Automatic initial and settled
captures confirm login and initial map loading; they do not prove movement.

## First new live session

Normal Account B, operator inputs; local time20:40-20:45. Clean EndPlay retained
as first-session-endplay.json. Final position32060,32190,7. Commands713,
unsupported0, anomalies0, residual0, synchronized viewport, duplicate/orphan0.
194 accepted steps,24 refused,38 unanswered,39 external relocations. Do not
relabel the unanswered requests as successful. ClientCore MovementLedger and
Bridge's existing three-second expiry are byte-unchanged from main; attribution
of this rapid-input case is UNVERIFIED, and overall gameplay regression stays
IMPLEMENTED_UNVERIFIED. No protocol or authority changes were made to hide it.

Named captures (UTC20260930, filenames carry full stamps):
- 024314_745: outdoor labels aligned in resized window; hover outlined player;
  server Look responses for floor and Seymour visible in chat. Corrected stone
  wall rows continuous, but WideWorld gray default materials are conspicuous.
- 024400_770 -> 024414_540: normal building entry; upper floors removed and
  player visible behind near geometry, cutaway1. Tom's label/body readable.
- 024446_889: camera obstructed, retracted from requested1053.50 to249.41;
  player remains visible with near geometry. This is real collision evidence,
  not merely a unit-test result. Clearance/follow return still needs review.
- 024553_236: wider outdoor context at32060,32190,7 after sector travel;
  initial56 sector loads grew to189 with118 unloads. Gray material boundary
  clearly remains a BUG. Minimap tracks position but is in browse mode; no
  minimap-click navigation proof from this capture.

Two player speech commands were received and routed to world presentation, but
no retained capture during their visible lifetime; world speech remains
IMPLEMENTED_UNVERIFIED. No underground, Use, Use With, combat or Follow input in
this session. Those acceptance items are not inferred from unrelated activity.

## Newly confirmed WideWorld material defect

BUG, not V08_ART_DEFECT: log repeatedly reports missing InstancedStaticMeshes
usage and default-material substitution. UE5.8 Material.cpp::SetMaterialUsage
explicitly refuses automatic usage changes in -game. Ordinary live mesh
components can use the same material, explaining the exact colored/gray edge.

Candidate fix: AssetRegistry::ResolveInstancedMaterial creates cached transient
copies of the existing material graph/instance hierarchy with the instancing
shader usage enabled. StaticSector applies these to HISM material slots. Source
mesh, textures, parameter overrides and original material usage stay unchanged;
no package is saved. Tests check source usage/material identity, transient
ownership and cache reuse. Build/live verification follows; not yet PASS.
This shader-generation adapter is for the current WITH_EDITOR desktop -game
client. A future cooked build must prepare its instancing permutations at cook
time; that packaging requirement is not claimed solved here.
