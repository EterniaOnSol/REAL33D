# UNREAL-HUD-PANEL-REGISTRY-001 independent certification

Status: IN_PROGRESS. No certification or main integration claimed yet.

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

## Execution record (pending completion)

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
  exists yet, so UI compatibility remains unverified.

Remaining: fresh UI/stackable compatibility
evidence, corrected harness with 13 CTest suites and
4 native suites, direct Git Bash six-check secret scan, clean committed closeout
and remote certification publication only if all requirements pass.
