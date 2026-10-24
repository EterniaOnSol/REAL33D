# REAL33D 3D regression harness

**Overall: PASS** — 11 PASS, 0 FAIL, 0 SKIP.

A SKIP is not a pass. It records a check this run could not execute, and why.

## Provenance

| | |
|---|---|
| Run (UTC) | `20261024T225025Z` |
| Repository | `/c/Users/dell/Desktop/fusion32` |
| Branch | `milestone/unreal-login-character-select-ui-001` |
| HEAD | `1ac7245da68d602697c59f3241277ed43577737b` |
| HEAD subject | docs(unreal): repair UNREAL-PARITY-AUDIT-001 — reconcile certified scope |
| origin/main | `1ac7245da68d602697c59f3241277ed43577737b` |
| Merge base | `1ac7245da68d602697c59f3241277ed43577737b` |
| Ahead / behind origin/main | 0 / 0 |
| Uncommitted paths | 25 |
| Toolchain | WSL Ubuntu-26.04, g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |

Worktrees at run time:

```
C:/Users/dell/Desktop/fusion32                                                          1ac7245 [milestone/unreal-login-character-select-ui-001]
C:/Users/dell/Desktop/fusion32/build/cert-chat-channels-20261018                        18b587d [certification/unreal-chat-channels-001-20261018]
C:/Users/dell/Desktop/fusion32/build/cert-container-controls-20261022                   2136e6f [certification/unreal-container-controls-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-container-independent-20261022                119dc2e [certification/unreal-container-controls-independent-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-death-loot-20261023                           28b5524 [certification/unreal-death-loot-independent-20261023]
C:/Users/dell/Desktop/fusion32/build/cert-diagonal-input-20261022                       73c01e3 [certification/unreal-diagonal-input-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-editors-ui-20261019                           76a2619 [certification/unreal-editors-ui-001-20261019]
C:/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022                       af251c9 [certification/unreal-login-combined-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-login-handover-20261022                       16cfa3d [certification/unreal-login-handover-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-outfit-dialog-20261020                        25c4448 [certification/unreal-outfit-dialog-001-20261020]
C:/Users/dell/Desktop/fusion32/build/cert-player-death-20261023                         0b0a6c0 [certification/unreal-player-death-respawn-independent-20261023]
C:/Users/dell/Desktop/fusion32/build/cert-skills-panel-20261017                         ca3121c [certification/unreal-skills-panel-001-20261017]
C:/Users/dell/Desktop/fusion32/build/cert-stack-move-controls-20261021                  a868572 [certification/stack-live-sendinput-20261021]
C:/Users/dell/Desktop/fusion32/build/cert-trade-ui-20261020                             2428a14 [certification/unreal-trade-ui-001-20261020]
C:/Users/dell/Desktop/fusion32/build/cert-turn-object-independent-20261022              eda5cc2 [certification/unreal-turn-object-independent-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-unreal-world-presentation-polish-001-20261015 bef902c [certification/unreal-world-presentation-polish-001-20261015]
C:/Users/dell/Desktop/fusion32/build/clientcore-channels-772-001                        9b950af [milestone/clientcore-channels-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-editors-772-001                         842fd16 [milestone/clientcore-editors-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-movement-actions-772-001                c666b9b [milestone/clientcore-movement-actions-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-outfit-772-001                          b8e7433 [milestone/clientcore-outfit-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-trade-772-001                           fa6c6f6 [milestone/clientcore-trade-772-001]
C:/Users/dell/Desktop/fusion32/build/integration-item-use                               1ac7245 [main]
C:/Users/dell/Desktop/fusion32/build/real33d-3d-regression-harness-001                  3443e1b [milestone/real33d-3d-regression-harness-001]
C:/Users/dell/Desktop/fusion32/build/unreal-item-use-interaction-001                    a97cf25 [milestone/unreal-item-use-interaction-001]
C:/Users/dell/Desktop/fusion32/build/unreal-minimap-navigation-001                      ea53ecc [milestone/unreal-minimap-navigation-001]
C:/Users/dell/Desktop/fusion32/build/unreal-skills-panel-001                            63a62dd [milestone/unreal-skills-panel-001]
C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001               bc9ce8f [milestone/unreal-world-presentation-polish-001]
```

Uncommitted paths at run time:

```
 M tests/regression_harness.sh
 M unreal/REAL33D/Source/REAL33D/Private/Real33DBridge.cpp
 M unreal/REAL33D/Source/REAL33D/Private/Real33DControlPanel.cpp
 M unreal/REAL33D/Source/REAL33D/Private/Real33DPanelRegistry.cpp
 M unreal/REAL33D/Source/REAL33D/Private/Real33DPlayerController.cpp
 M unreal/REAL33D/Source/REAL33D/Private/Real33DPresentationTests.cpp
 M unreal/REAL33D/Source/REAL33D/Private/Real33DWorldActor.cpp
 M unreal/REAL33D/Source/REAL33D/Public/Real33DBridge.h
 M unreal/REAL33D/Source/REAL33D/Public/Real33DControlPanel.h
 M unreal/REAL33D/Source/REAL33D/Public/Real33DHUD.h
 M unreal/REAL33D/Source/REAL33D/Public/Real33DPanelRegistry.h
 M unreal/REAL33D/Source/REAL33D/Public/Real33DPlayerController.h
 M unreal/REAL33D/Source/REAL33D/Public/Real33DWorldActor.h
?? agent/
?? dev/
?? evidence/agent/
?? evidence/clientcore/unreal-action-bars/
?? evidence/clientcore/unreal-login-character-select-ui-001/
?? tests/real33d_session_flow_tests.cpp
?? tools/
?? unreal/REAL33D/Source/REAL33D/Private/Real33DLoginQA.cpp
?? unreal/REAL33D/Source/REAL33D/Private/Real33DLoginScreen.cpp
?? unreal/REAL33D/Source/REAL33D/Public/Real33DLoginQA.h
?? unreal/REAL33D/Source/REAL33D/Public/Real33DLoginScreen.h
?? unreal/REAL33D/Source/REAL33D/Public/Real33DSessionFlow.h
```

## Results

| Check | Status | Detail | Log |
|---|---|---|---|
| `provenance` | **PASS** | branch milestone/unreal-login-character-select-ui-001 at 1ac7245d, 0 ahead / 0 behind origin/main, 25 uncommitted path(s) | `logs/provenance-status.txt` |
| `branch-isolation` | **PASS** | on milestone/unreal-login-character-select-ui-001, not on main | — |
| `clientcore-tests` | **PASS** | 14 CTest suite(s) passed via wsl | `logs/clientcore-tests.txt` |
| `real33d-native-tests` | **PASS** | 11 passed, 0 failed via wsl; no --objects-srv, so the runtime MultiUse assertions were not exercised | `logs/real33d-native-tests.txt` |
| `outfit-color-mapping` | **PASS** | 43 byte assertion(s) hold; Colors[0..3] = Head, Body, Legs, Feet | `logs/outfit-color-mapping.txt` |
| `outfit-palette` | **PASS** | 51 assertion(s) hold; 133 colours, 7x19, derived not tabulated | `logs/outfit-palette.txt` |
| `secret-check` | **PASS** | 6 sub-check(s) ok | `logs/secret-check.txt` |
| `build-sanity-msvc` | **PASS** | C++17 suites and the C++20 archive Unreal links both built | `logs/build-sanity-msvc.txt` |
| `unreal-module-build` | **PASS** | REAL33DEditor Win64 Development built | `logs/unreal-module-build.txt` |
| `protocol-log-check` | **PASS** | 125 log(s) scanned, no unsupported opcodes or residual bytes | `logs/protocol-log-check.txt` |
| `output-secret-scan` | **PASS** | logs carry no key material (pattern self-tested) | `logs/output-secret-scan.txt` |

## What each check covers

- `provenance` — branch, HEAD, origin/main, merge base, ahead/behind, dirty paths and the full worktree list, captured before anything else runs.
- `branch-isolation` — refuses to certify a run sitting on `main`.
- `clientcore-tests` — the Protocol772Core CTest suites: transport, crypto, login, gamelogin, initial_world, movement, player_state, worldview, minimap.
- `real33d-native-tests` — the header-only 3D unit tests under `tests/real33d_*.cpp`: mouse gestures, minimap view, item-use/MultiUse classification, stackable sprites, chat views, outfit selection, stack-move controls.
- `secret-check` — `tests/secret_check.sh` over the tracked tree and the whole reachable history.
- `build-sanity-msvc` — `tests/build_clientcore_windows.cmd`: the C++17 suites plus the C++20 archive the Unreal module links.
- `unreal-module-build` — UnrealBuildTool on REAL33DEditor. Opt-in with `--unreal`.
- `protocol-log-check` — `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, plus the bridge diagnostics that say a command was left unwalked.
- `output-secret-scan` — the harness re-reads everything it just wrote and fails rather than leave key material or a credential assignment on disk.

## Scope and limits

- Nothing here proves live behaviour. No Fusion32 server is started, no client is launched, no gameplay and no protocol is modified. Live certification stays a separate, operator-driven step.
- Read-only against the repository. Everything written goes under `build/regression-harness/20261024T225025Z`, inside the gitignored `build/` tree.
- No environment values are printed, and `output-secret-scan` is the last word on what was left on disk.
