# REAL33D 3D regression harness

**Overall: PASS_WITH_SKIPS** — 9 PASS, 0 FAIL, 1 SKIP.

A SKIP is not a pass. It records a check this run could not execute, and why.

## Provenance

| | |
|---|---|
| Run (UTC) | `20261021T042050Z` |
| Repository | `/c/Users/dell/Desktop/fusion32/build/cert-outfit-dialog-20261020` |
| Branch | `certification/unreal-outfit-dialog-001-20261020` |
| HEAD | `61ceaa955933672c526864b5ed24dca0e4c8de84` |
| HEAD subject | fix(unreal): keep outfit stepper arrows visible within button width |
| origin/main | `b8e7433baeddc2875943f519d33a5cb9380f3eaa` |
| Merge base | `b8e7433baeddc2875943f519d33a5cb9380f3eaa` |
| Ahead / behind origin/main | 2 / 0 |
| Uncommitted paths | 91 |
| Toolchain | WSL Ubuntu-26.04, g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |

Worktrees at run time:

```
C:/Users/dell/Desktop/fusion32                                                          56bf508 [milestone/unreal-outfit-dialog-001]
C:/Users/dell/Desktop/fusion32/build/cert-chat-channels-20261018                        18b587d [certification/unreal-chat-channels-001-20261018]
C:/Users/dell/Desktop/fusion32/build/cert-editors-ui-20261019                           76a2619 [certification/unreal-editors-ui-001-20261019]
C:/Users/dell/Desktop/fusion32/build/cert-outfit-dialog-20261020                        61ceaa9 [certification/unreal-outfit-dialog-001-20261020]
C:/Users/dell/Desktop/fusion32/build/cert-skills-panel-20261017                         ca3121c [certification/unreal-skills-panel-001-20261017]
C:/Users/dell/Desktop/fusion32/build/cert-trade-ui-20261020                             2428a14 [certification/unreal-trade-ui-001-20261020]
C:/Users/dell/Desktop/fusion32/build/cert-unreal-world-presentation-polish-001-20261015 bef902c [certification/unreal-world-presentation-polish-001-20261015]
C:/Users/dell/Desktop/fusion32/build/clientcore-channels-772-001                        9b950af [milestone/clientcore-channels-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-editors-772-001                         842fd16 [milestone/clientcore-editors-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-movement-actions-772-001                c666b9b [milestone/clientcore-movement-actions-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-outfit-772-001                          b8e7433 [milestone/clientcore-outfit-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-trade-772-001                           fa6c6f6 [milestone/clientcore-trade-772-001]
C:/Users/dell/Desktop/fusion32/build/integration-item-use                               b8e7433 [main]
C:/Users/dell/Desktop/fusion32/build/real33d-3d-regression-harness-001                  3443e1b [milestone/real33d-3d-regression-harness-001]
C:/Users/dell/Desktop/fusion32/build/unreal-item-use-interaction-001                    a97cf25 [milestone/unreal-item-use-interaction-001]
C:/Users/dell/Desktop/fusion32/build/unreal-minimap-navigation-001                      ea53ecc [milestone/unreal-minimap-navigation-001]
C:/Users/dell/Desktop/fusion32/build/unreal-skills-panel-001                            63a62dd [milestone/unreal-skills-panel-001]
C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001               bc9ce8f [milestone/unreal-world-presentation-polish-001]
```

Uncommitted paths at run time:

```
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/README.md
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/REGRESSION-HARNESS-20261020T213720Z.md
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/automation-probe-console.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/build-sanity-msvc.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/clientcore-tests.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/outfit-color-mapping.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/output-secret-scan.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/protocol-log-check.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/provenance-status.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/provenance-worktrees.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/real33d-native-tests.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/secret-check.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-build-fixed-probe.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-build-probe-2.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-build-probe-cursor.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-build-probe-menu.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-build-probe-wire.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-build-probe.txt
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/automated/unreal-probe-results.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/Real33DOutfitBridgeCertification.inl
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/Real33DOutfitCertification.cpp
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/audit_live.py
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/bridge-diagnostic.patch
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/build-unreal.cmd
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/launch-live.ps1
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/original-live-driver.cpp
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/run-automation.ps1
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/run-harness-final.cmd
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/run-harness.cmd
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/run-original-automation.ps1
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-cursor/REAL33D.log
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-cursor/movement_journal.jsonl
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-cursor/unreal_slice_evidence.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-cursor/unreal_slice_evidence_OutfitCertFailed.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/REAL33D.log
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/movement_journal.jsonl
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_MinimapInitial.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_MinimapInitial.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_MinimapSettled.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_MinimapSettled.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_OutfitCertBeforeInput.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_OutfitCertBeforeInput.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-failed-menu/unreal_slice_evidence_OutfitCertFailed.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/REAL33D.log
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/audit.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/movement_journal.jsonl
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_MinimapInitial.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_MinimapInitial.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_MinimapSettled.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_MinimapSettled.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertAuthoritative.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertAuthoritative.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertBeforeInput.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertBeforeInput.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertChannels.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertChannels.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertComplete.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertOpened.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertOpened.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertRegression.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertRegression.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertReopened.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertReopened.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertSelected.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-fixed/unreal_slice_evidence_OutfitCertSelected.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/REAL33D.log
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/audit.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/movement_journal.jsonl
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_MinimapInitial.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_MinimapInitial.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_MinimapSettled.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_MinimapSettled.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertAuthoritative.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertAuthoritative.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertBeforeInput.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertBeforeInput.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertChannels.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertChannels.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertComplete.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertOpened.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertOpened.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertRegression.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertRegression.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertReopened.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertReopened.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertSelected.json
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-original/unreal_slice_evidence_OutfitCertSelected.png
A  evidence/clientcore/unreal-outfit-dialog-independent-20261020/live-startup-blocked/REAL33D.log
```

## Results

| Check | Status | Detail | Log |
|---|---|---|---|
| `provenance` | **PASS** | branch certification/unreal-outfit-dialog-001-20261020 at 61ceaa95, 2 ahead / 0 behind origin/main, 91 uncommitted path(s) | `logs/provenance-status.txt` |
| `branch-isolation` | **PASS** | on certification/unreal-outfit-dialog-001-20261020, not on main | — |
| `clientcore-tests` | **PASS** | 14 CTest suite(s) passed via wsl | `logs/clientcore-tests.txt` |
| `real33d-native-tests` | **PASS** | 6 passed, 0 failed via wsl; no --objects-srv, so the runtime MultiUse assertions were not exercised | `logs/real33d-native-tests.txt` |
| `outfit-color-mapping` | **PASS** | 43 byte assertion(s) hold; Colors[0..3] = Head, Body, Legs, Feet | `logs/outfit-color-mapping.txt` |
| `secret-check` | **PASS** | 6 sub-check(s) ok | `logs/secret-check.txt` |
| `build-sanity-msvc` | **PASS** | C++17 suites and the C++20 archive Unreal links both built | `logs/build-sanity-msvc.txt` |
| `unreal-module-build` | **SKIP** | engine present but --unreal not passed; a full module build is not a CI default | — |
| `protocol-log-check` | **PASS** | 17 log(s) scanned, no unsupported opcodes or residual bytes | `logs/protocol-log-check.txt` |
| `output-secret-scan` | **PASS** | logs carry no key material (pattern self-tested) | `logs/output-secret-scan.txt` |

## What each check covers

- `provenance` — branch, HEAD, origin/main, merge base, ahead/behind, dirty paths and the full worktree list, captured before anything else runs.
- `branch-isolation` — refuses to certify a run sitting on `main`.
- `clientcore-tests` — the Protocol772Core CTest suites: transport, crypto, login, gamelogin, initial_world, movement, player_state, worldview, minimap.
- `real33d-native-tests` — the header-only 3D unit tests under `tests/real33d_*.cpp`: mouse gestures, minimap view, item-use/MultiUse classification.
- `secret-check` — `tests/secret_check.sh` over the tracked tree and the whole reachable history.
- `build-sanity-msvc` — `tests/build_clientcore_windows.cmd`: the C++17 suites plus the C++20 archive the Unreal module links.
- `unreal-module-build` — UnrealBuildTool on REAL33DEditor. Opt-in with `--unreal`.
- `protocol-log-check` — `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, plus the bridge diagnostics that say a command was left unwalked.
- `output-secret-scan` — the harness re-reads everything it just wrote and fails rather than leave key material or a credential assignment on disk.

## Scope and limits

- Nothing here proves live behaviour. No Fusion32 server is started, no client is launched, no gameplay and no protocol is modified. Live certification stays a separate, operator-driven step.
- Read-only against the repository. Everything written goes under `build/certification/harness-final`, inside the gitignored `build/` tree.
- No environment values are printed, and `output-secret-scan` is the last word on what was left on disk.
