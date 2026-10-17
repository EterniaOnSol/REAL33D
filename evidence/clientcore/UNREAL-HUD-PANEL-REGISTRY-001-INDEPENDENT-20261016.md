# UNREAL-HUD-PANEL-REGISTRY-001 independent certification

Status: CERTIFIED. Independent review completed 2026-10-17. No main integration.

## Provenance and scope

- Agent: Codex independent reviewer, 2026-10-16, America/Guatemala.
- Fetched base: `611939af271a3d21fd9e3993a42150c3439cb08d` (`origin/main`).
- Implementation: `64aa86a38d8f15c42cde275401d4942b9e174ab8`.
- Implementation's older base: `1a15ddbd79a23a2841d882584afaa82fabd9f74d`.
- Isolated branch: `certification/unreal-hud-panel-registry-001-20261016`.
- Isolated checkout: `build/cert-hud-panel-registry-20261016` under the repository root.
- Merge: `55d1d4660110ce2ca8e09a8d5e944a16d0750574`, first parent current main,
  second parent implementation. No rebase, cherry-pick, main update, or implementation-branch update.
- Changed-file intersection since the common ancestor: **0**; merge conflicts: **0**.
- Root checkout's four pre-existing unreal-slice evidence changes were preserved.

## Independent source review

The merge changes exactly the six HUD implementation/report files. ClientCore,
protocol commands, scripts, tests/harness, stackable frame selection, UIStyle,
PanelChrome, inventory, containers, chat, vitals, minimap and battle implementation
files retain current main's blobs. The corrected harness still captures WSL exit
status outside its shell and requires a nonzero CTest count plus the 100% marker.

Reviewed `FReal33DPanelRegistry` register/find/list/open/close/toggle/unregister/reset,
`SReal33DHUD::AddPanel`, `Construct`, `HandlePanelToggled`, and unchanged refresh
and interaction paths. The six registry ids are Minimap, Health, Inventory, Skills,
Battle and Containers. Chat remains a separate widget, not a seventh registry id.
Construction retains panel sizes, order, titles, icons, contents and callbacks.
Only the existing Skills and Battle control-strip buttons drive the registry;
this change does not add buttons for the other four panels.

Independent diagnostic `REAL33D.Presentation.CertificationHUDSixPanels` constructs
the actual production HUD without a bridge. It checks all six real mini windows,
their original child types, registration order, individual close/open/toggle,
visibility, independence, original widget identity, and separate visible chat.
It passed, was removed from production Source and retained as evidence, and is
followed by an exact merged-source build (PASS). No implementation defect or
production change has been established by this review.

## Execution record

- Harness selftest: PASS, **10 passed / 0 failed**, MSVC on PATH.
- Fresh MSVC ClientCore build: PASS, C++17 suites and C++20 Unreal archive.
- Independent CTest run: PASS, **13/13**, 100% tests passed, zero failures;
  fresh GCC Debug configure/build in `/tmp/real33d-hud-cert-20261016-clientcore`,
  exact suite count separately asserted. Raw output: clientcore-direct.txt.
- Native suites: PASS, **4/4** (mouse input, minimap view, item use policy,
  stackable sprites), GCC17 with warnings as errors. Runtime objects.srv read-only
  input verified204 MultiUse types. Raw output: native-direct.txt.
- Direct Git Bash secret_check: PASS, **6/6**, including reachable history and
  archived-template presence; raw output: secret-check-direct.txt.
- Initial Unreal build: FAILED, C1085 insufficient disk while writing the shared PCH.
- Removed only this session's failed 2,232,680,448-byte PCH after validating its
  absolute path under this checkout's Intermediate directory; free space rose
  to 6,961,278,976 bytes. No other session's caches were removed.
- A `-NoPCH` trial exposed existing reliance on transitive PCH declarations
  (FSlateBrush and UPackage, in unchanged main files); stopped this alternate
  configuration without modifying those files.
- Standard PCH retry generated the full 2,488,532,992-byte PCH. Compressing that
  owned file reclaimed space (896,901,120 stored bytes), but temporarily locked
  it while compiler actions read it. That attempt's C1083 permission failures
  are setup errors, not an accepted build. The following build after compression
  PASSED (`unreal-diagnostic-final.txt`, 179.17 seconds).
- Exported Unreal results at 2026-10-16 21:29:22 UTC: **5 Success / 0 failed /
  0 notRun / 0 inProcess**. PanelRegistry, CertificationHUDSixPanels,
  CameraAndFloors, WideWorld.AuthoritativeWindow and AssetRegistry.ExperimentalV08
  all passed with zero errors. The V08 suite's one warning is its intentional
  TEST_IMPORTED/not-APPROVED catalogue notice; all other suites have zero warnings.
- Removed the diagnostic source after preserving an identical evidence copy.
  Cleared only this checkout's generated SourceFileCache.bin and Makefile.bin so
  UBT detects the removed source. Exact merged-source rebuild PASSED in 53.37s;
  `git diff HEAD -- unreal/REAL33D/Source clientcore tests scripts` is empty.
- First graphical launch (PID6780, live-20261016T153111) had not connected when
  its five shader workers exhausted free disk through memory/pagefile pressure;
  Zen reported 507 Insufficient Storage. Stopped only the verified owned parent
  and its five workers before gameplay. Operator's early F9 produced no capture
  and is not accepted as evidence. No character/server change was made.
- Retried the same binary and gameplay arguments in live-20261016T153651-single-worker,
  PID16324, with per-process Engine ini overrides reserving seven shader threads
  and threshold64. Log explicitly confirms one shader worker; free disk recovered
  to approximately9.5GB. No checked-in config or global cache was changed.
- Single-worker preparation was stopped before login for a two-worker retry,
  live-20261016T154238-two-workers, PID28912. The new log confirms TWO workers.
  Free disk remains approximately7.8GB. Shader cache file count keeps growing
  (977 at21:44UTC,1599 at21:49,2969 at21:51,3479 at21:56); this is slow cold
  compilation, not an observed HUD/gameplay defect. No server connection/capture
  existed at that checkpoint, so UI compatibility was still unverified then.

## Completed live compatibility and final gates

The final graphical run was `live-20261016T160844-two-workers`, PID10080, using
the independently built exact merge. Fusion32's game service had shut down
normally while the cold engine was loading; its previous log was preserved and
only that selected service restarted. No server rules, saved character contents,
reference material or production source were edited. Details and scope are in
`unreal-hud-panel-registry-independent-20261016/live/OBSERVATIONS.md`.

Two fresh F9 screenshots and received-state JSON show all six panels plus chat,
then the normally opened backpack. Inventory gold3031 amount5 and container
worms3492 amount6 display their cumulative frame4 images and unchanged counts.
Shovel/rope and equipment remain present. Minimap/player coordinates agree;
vitals and battle entries display normally. The operator confirmed the existing
Skills button alternates visibility. All six panels' actual register/open/close/
toggle behavior was independently exercised in the real HUD automation probe;
a live Battle-button toggle is not claimed. Chat retained the server login and
idle-warning messages. This is bounded HUD compatibility, not full gameplay
recertification or new asset approval.

Normal EndPlay at2026-10-16T22:27:33.882Z records1137commands/581frames,
26accepted walks/1server rejection/0unanswered, one normal Use and one open
backpack. All five retained snapshots have residual_bytes0, unsupported_opcodes0
and protocol_anomalies0. No duplicate spawns or orphan events; viewport remains
synchronized. Compact raw events and SHA256 provenance accompany the snapshots.

The corrected strict harness completed at2026-10-17 with **9 PASS / 0 FAIL /
0 SKIP**, including a fresh real Unreal build (44.47s), MSVC C++17 suites/C++20
archive, native4/4, exactly13 CTest suites, Git Bash secret6/6 and33 protocol logs
with zero anomalies. Raw report and every gate log are retained under `harness/`.
The wrapper independently asserted13 Passed lines and the exact100%/13 marker;
its exit was0. Earlier selftest10/10 and independent Unreal5/5 remain valid:
production implementation did not change. Final direct Git Bash secret output is
retained as `secret-check-final.txt` after all final evidence is staged.

| Criterion | Result | Evidence |
| --- | --- | --- |
| File overlap / merge conflicts | PASS: 0 / 0 | provenance.json and two-parent merge55d1d466 |
| Panel registry and six actual panels | PASS | automation-index.json, retained independent probe |
| Inventory/containers/chat/vitals/minimap/battle compatibility | PASS | two fresh screenshots, JSON, unchanged content implementations |
| Stackable compatibility | PASS | gold5 and worms6 live; generic frame code unchanged; native suite PASS |
| Harness selftest | PASS: 10/10 | selftest.txt |
| ClientCore | PASS: 13/13 | direct and final harness CTest logs |
| Native suites | PASS: 4/4 | direct and harness native logs |
| Unreal tests | PASS: 5/5 | PanelRegistry, actual six-panel HUD, CameraAndFloors, AuthoritativeWindow, ExperimentalV08 |
| MSVC / actual Unreal build | PASS / PASS | final harness build logs |
| Corrected harness | PASS: 9/0/0 | REGRESSION-HARNESS-20261017T092251Z.md |
| Secret check | PASS: Git Bash 6/6 | direct/final secret logs |
| Protocol | PASS: all counters0 | five snapshots and33-log harness scan |

No certification requirement remains unverified. Known art/3D presentation
boundaries remain unchanged, including amount-invariant world meshes; this HUD
review does not change their acceptance. Root's four unrelated dirty evidence
files are preserved. Main and implementation checkouts remained clean at their
expected611939af/64aa86a heads. Only the certification branch is published.

## Reproduction

From the isolated certification checkout with the documented engine/MSVC tools:

1. Run `tests/regression_harness_selftest.sh` from Git Bash with MSVC on PATH;
   require10passed/0failed.
2. Run `tests/regression_harness.sh --out build/new-cert-harness --strict --unreal
   --objects-srv /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv
   --logs evidence/clientcore/unreal-hud-panel-registry-independent-20261016/live`.
   Require9PASS/0FAIL/0SKIP and independently count13 CTest Passed lines and4 native
   results. Use the Visual Studio developer environment for the MSVC gate.
3. For the independent six-panel probe, copy its retained `.cpp` into a new
   disposable certification checkout's `unreal/REAL33D/Source/REAL33D/Private`,
   build REAL33DEditor Win64 Development, then run
   `scripts/client/test_unreal_presentation.ps1 -EvidenceDirectory <fresh-dir>
   -ExperimentalCatalog visual/qa/full_catalog_v08/experimental_catalog_runtime.json`.
   Inspect all five named tests in automation-report/index.json, requiring
   Success/errors0 and failed/notRun/inProcess0. Remove the temporary probe from
   that disposable source tree and rebuild the unchanged implementation afterward.
4. Launch `scripts/client/run_unreal_presentation_qa.ps1 -Account B` against the
   existing authoritative runtime; capture the HUD, open the backpack normally,
   capture again, operate the existing Skills button, and close normally. Confirm
   actual counts/images, registry/API results and zero protocol counters. No
   synthetic local state or new items are needed.
