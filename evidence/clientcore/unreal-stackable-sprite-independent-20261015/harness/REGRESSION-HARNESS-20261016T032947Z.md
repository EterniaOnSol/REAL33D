# REAL33D 3D regression harness

**Overall: PASS** — 9 PASS, 0 FAIL, 0 SKIP.

A SKIP is not a pass. It records a check this run could not execute, and why.

## Provenance

| | |
|---|---|
| Run (UTC) | `20261016T032947Z` |
| Repository | `/c/Users/dell/Desktop/fusion32/build/cert-stackable-20261015` |
| Branch | `certification/unreal-stackable-sprite-patterns-001-20261015` |
| HEAD | `68f2ffe488627d5da52981bd83a2e95a5846ecc3` |
| HEAD subject | fix(unreal): draw the stackable picture that matches the amount |
| origin/main | `1a15ddbd79a23a2841d882584afaa82fabd9f74d` |
| Merge base | `1a15ddbd79a23a2841d882584afaa82fabd9f74d` |
| Ahead / behind origin/main | 1 / 0 |
| Uncommitted paths | 6 |
| Toolchain | WSL Ubuntu-26.04, g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |

Worktrees at run time:

```
C:/Users/dell/Desktop/fusion32                                                          b9ed335 [milestone/real33d-agent-veteran-play-002]
C:/Users/dell/Desktop/fusion32/build/cert-stackable-20261015                            68f2ffe [certification/unreal-stackable-sprite-patterns-001-20261015]
C:/Users/dell/Desktop/fusion32/build/cert-unreal-world-presentation-polish-001-20261015 bef902c [certification/unreal-world-presentation-polish-001-20261015]
C:/Users/dell/Desktop/fusion32/build/clientcore-channels-772-001                        9b950af [milestone/clientcore-channels-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-editors-772-001                         842fd16 [milestone/clientcore-editors-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-movement-actions-772-001                c666b9b [milestone/clientcore-movement-actions-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-trade-772-001                           fa6c6f6 [milestone/clientcore-trade-772-001]
C:/Users/dell/Desktop/fusion32/build/integration-clientcore-772-batch                   1a15ddb [integration/clientcore-772-batch-20261015]
C:/Users/dell/Desktop/fusion32/build/integration-item-use                               1a15ddb [main]
C:/Users/dell/Desktop/fusion32/build/real33d-3d-regression-harness-001                  3443e1b [milestone/real33d-3d-regression-harness-001]
C:/Users/dell/Desktop/fusion32/build/rehearsal-integration-disposable                   9b822fd [rehearsal/clientcore-integration-disposable]
C:/Users/dell/Desktop/fusion32/build/rehearsal-integration-final-20261015               a5a1751 [rehearsal/clientcore-integration-final-20261015]
C:/Users/dell/Desktop/fusion32/build/unreal-item-use-interaction-001                    a97cf25 [milestone/unreal-item-use-interaction-001]
C:/Users/dell/Desktop/fusion32/build/unreal-minimap-navigation-001                      ea53ecc [milestone/unreal-minimap-navigation-001]
C:/Users/dell/Desktop/fusion32/build/unreal-stackable-sprite-patterns-001               68f2ffe [milestone/unreal-stackable-sprite-patterns-001]
C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001               bc9ce8f [milestone/unreal-world-presentation-polish-001]
```

Uncommitted paths at run time:

```
 M PARITY_MATRIX.md
 M PROJECT_STATUS.md
 M handoffs/CURRENT.md
?? evidence/clientcore/UNREAL-STACKABLE-SPRITE-PATTERNS-001-INDEPENDENT-20261015.md
?? evidence/clientcore/unreal-stackable-sprite-independent-20261015/
?? handoffs/archive/UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-BEFORE-STACKABLE-20261015.md
```

## Results

| Check | Status | Detail | Log |
|---|---|---|---|
| `provenance` | **PASS** | branch certification/unreal-stackable-sprite-patterns-001-20261015 at 68f2ffe4, 1 ahead / 0 behind origin/main, 6 uncommitted path(s) | `logs/provenance-status.txt` |
| `branch-isolation` | **PASS** | on certification/unreal-stackable-sprite-patterns-001-20261015, not on main | — |
| `clientcore-tests` | **PASS** | 13 CTest suite(s) passed via wsl | `logs/clientcore-tests.txt` |
| `real33d-native-tests` | **PASS** | 4 passed, 0 failed via wsl | `logs/real33d-native-tests.txt` |
| `secret-check` | **PASS** | 6 sub-check(s) ok | `logs/secret-check.txt` |
| `build-sanity-msvc` | **PASS** | C++17 suites and the C++20 archive Unreal links both built | `logs/build-sanity-msvc.txt` |
| `unreal-module-build` | **PASS** | REAL33DEditor Win64 Development built | `logs/unreal-module-build.txt` |
| `protocol-log-check` | **PASS** | 175 log(s) scanned, no unsupported opcodes or residual bytes | `logs/protocol-log-check.txt` |
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
- Read-only against the repository. Everything written goes under `build/certification/harness-closeout`, inside the gitignored `build/` tree.
- No environment values are printed, and `output-secret-scan` is the last word on what was left on disk.
