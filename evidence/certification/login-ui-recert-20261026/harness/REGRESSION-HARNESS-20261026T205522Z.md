# REAL33D 3D regression harness

**Overall: PASS** — 12 PASS, 0 FAIL, 0 SKIP.

A SKIP is not a pass. It records a check this run could not execute, and why.

## Provenance

| | |
|---|---|
| Run (UTC) | `20261026T205522Z` |
| Repository | `/c/Users/dell/Desktop/fusion32/build/cert-login-ui-recert-20261026` |
| Branch | `certification/login-ui-recert-20261026` |
| HEAD | `444a89e268289cad31fd64260b04762bef73dfdc` |
| HEAD subject | docs(unreal): record the harness run taken against the committed fix |
| origin/main | `1ac7245da68d602697c59f3241277ed43577737b` |
| Merge base | `1ac7245da68d602697c59f3241277ed43577737b` |
| Ahead / behind origin/main | 4 / 0 |
| Uncommitted paths | 0 |
| Toolchain | WSL Ubuntu-26.04, g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |

Worktrees at run time:

```
C:/Users/dell/Desktop/fusion32                                                          444a89e [milestone/unreal-login-character-select-ui-001]
C:/Users/dell/Desktop/fusion32/build/cert-chat-channels-20261018                        18b587d [certification/unreal-chat-channels-001-20261018]
C:/Users/dell/Desktop/fusion32/build/cert-container-controls-20261022                   2136e6f [certification/unreal-container-controls-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-container-independent-20261022                119dc2e [certification/unreal-container-controls-independent-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-death-loot-20261023                           28b5524 [certification/unreal-death-loot-independent-20261023]
C:/Users/dell/Desktop/fusion32/build/cert-diagonal-input-20261022                       73c01e3 [certification/unreal-diagonal-input-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-editors-ui-20261019                           76a2619 [certification/unreal-editors-ui-001-20261019]
C:/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022                       af251c9 [certification/unreal-login-combined-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-login-handover-20261022                       16cfa3d [certification/unreal-login-handover-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-login-ui-20261025                             bf54da4 [certification/unreal-login-character-select-ui-20261025]
C:/Users/dell/Desktop/fusion32/build/cert-login-ui-independent-20261025                 3b82a9a [certification/login-ui-independent-20261025]
C:/Users/dell/Desktop/fusion32/build/cert-login-ui-recert-20261026                      444a89e [certification/login-ui-recert-20261026]
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

## Results

| Check | Status | Detail | Log |
|---|---|---|---|
| `provenance` | **PASS** | branch certification/login-ui-recert-20261026 at 444a89e2, 4 ahead / 0 behind origin/main, 0 uncommitted path(s) | `logs/provenance-status.txt` |
| `branch-isolation` | **PASS** | on certification/login-ui-recert-20261026, not on main | — |
| `clientcore-tests` | **PASS** | 14 CTest suite(s) passed via wsl | `logs/clientcore-tests.txt` |
| `real33d-native-tests` | **PASS** | 12 passed, 0 failed via wsl | `logs/real33d-native-tests.txt` |
| `outfit-color-mapping` | **PASS** | 43 byte assertion(s) hold; Colors[0..3] = Head, Body, Legs, Feet | `logs/outfit-color-mapping.txt` |
| `outfit-palette` | **PASS** | 51 assertion(s) hold; 133 colours, 7x19, derived not tabulated | `logs/outfit-palette.txt` |
| `secret-check` | **PASS** | 6 sub-check(s) ok | `logs/secret-check.txt` |
| `build-sanity-msvc` | **PASS** | C++17 suites and the C++20 archive Unreal links both built | `logs/build-sanity-msvc.txt` |
| `unreal-module-build` | **PASS** | REAL33DEditor Win64 Development built | `logs/unreal-module-build.txt` |
| `protocol-log-check` | **PASS** | 49 log(s) scanned, no unsupported opcodes or residual bytes | `logs/protocol-log-check.txt` |
| `credential-value-scan` | **PASS** | 0 account-id and 0 password occurrences in 60 produced file(s) (patterns self-tested) | `logs/credential-value-scan.txt` |
| `output-secret-scan` | **PASS** | logs carry no key material (pattern self-tested) | `logs/output-secret-scan.txt` |

## What each check covers

- `provenance` — branch, HEAD, origin/main, merge base, ahead/behind, dirty paths and the full worktree list, captured before anything else runs.
- `branch-isolation` — refuses to certify a run sitting on `main`.
- `clientcore-tests` — the Protocol772Core CTest suites: transport, crypto, login, gamelogin, initial_world, movement, player_state, worldview, minimap.
- `real33d-native-tests` — the header-only 3D unit tests under `tests/real33d_*.cpp`: mouse gestures, minimap view, item-use/MultiUse classification, stackable sprites, chat views, outfit selection, stack-move controls, session flow, and the credential-log rule that no string this client produces about an account carries a decimal digit.
- `secret-check` — `tests/secret_check.sh` over the tracked tree and the whole reachable history.
- `build-sanity-msvc` — `tests/build_clientcore_windows.cmd`: the C++17 suites plus the C++20 archive the Unreal module links.
- `unreal-module-build` — UnrealBuildTool on REAL33DEditor. Opt-in with `--unreal`.
- `protocol-log-check` — `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, plus the bridge diagnostics that say a command was left unwalked.
- `credential-value-scan` — `tests/scan_credential_values.py`: this runtime's own account ids and passwords, searched for literally in the logs and evidence in scope. Needs `--runtime DIR`; needles come from the gitignored secrets file and only counts and key names are printed.
- `output-secret-scan` — the harness re-reads everything it just wrote and fails rather than leave key material or a credential assignment on disk.

## Scope and limits

- Nothing here proves live behaviour. No Fusion32 server is started, no client is launched, no gameplay and no protocol is modified. Live certification stays a separate, operator-driven step.
- Read-only against the repository. Everything written goes under `build/recert/harness`, inside the gitignored `build/` tree.
- No environment values are printed, and `output-secret-scan` is the last word on what was left on disk.
