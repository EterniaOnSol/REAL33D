# REAL33D-3D-REGRESSION-HARNESS-001

Status: `IMPLEMENTED`. Tooling only. No gameplay, no protocol, no Unreal
runtime and no live claim is touched or made by this milestone. No main merge.

Base `origin/main` at `fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, developed on
`milestone/real33d-3d-regression-harness-001` in its own worktree, concurrent
with and disjoint from `milestone/unreal-world-presentation-polish-001`.

## What was added

- `tests/regression_harness.sh` — one command a 3D milestone runs before it
  claims anything.
- `tests/README.md` — a section describing it, next to the checks it drives.

Nothing else. No file carried by the polish worktree was read for content or
written: that worktree holds `Real33DHUD.*`, `Real33DPlayerController.*`,
`Real33DAssetRegistry.cpp`, `Real33DPresentationTests.cpp`,
`Real33DStaticSectorActor.*`, `Real33DTileActor.*`, `Real33DWorldActor.*`,
`Real33DPresentationPolicy.h`, `PARITY_MATRIX.md`, `PROJECT_STATUS.md` and
`handoffs/CURRENT.md`, and this milestone touches none of them.

## Three statuses, never two

`PASS`, `FAIL`, `SKIP`. A check whose toolchain is absent is `SKIP` with the
reason printed, never `PASS`. Overall is `FAIL` if anything failed,
`PASS_WITH_SKIPS` if anything was skipped, `PASS` only when every check ran and
passed. Exit `1` on any `FAIL`, or on any `SKIP` under `--strict`.

This is the property the harness exists for, so it is the property that was
tested hardest below.

## Checks

| Check | Covers |
|---|---|
| `provenance` | branch, HEAD, origin/main, merge base, ahead/behind, dirty paths, full worktree list |
| `branch-isolation` | refuses a run sitting on `main` |
| `clientcore-tests` | the nine Protocol772Core CTest suites |
| `real33d-native-tests` | the header-only `tests/real33d_*.cpp` |
| `secret-check` | `tests/secret_check.sh` over tree and reachable history |
| `build-sanity-msvc` | `tests/build_clientcore_windows.cmd`, incl. the C++20 archive Unreal links |
| `unreal-module-build` | UnrealBuildTool on REAL33DEditor, opt-in with `--unreal` |
| `protocol-log-check` | `unsupported_opcodes` / `protocol_anomalies` / `residual_bytes` above zero, and the bridge diagnostics naming a command left unwalked |
| `output-secret-scan` | re-reads everything the run wrote; the report is discarded rather than published if it matches |

## Runs executed

All from Git Bash on Windows 11, repository at the harness worktree. Reports
land under the gitignored `build/` tree; the run directory is named in each
line below only where it mattered.

1. **Default run** — `bash tests/regression_harness.sh`.
   `PASS_WITH_SKIPS`, 6 PASS / 0 FAIL / 3 SKIP, 39 s wall clock.
   `clientcore-tests` PASS with 9 CTest suites through WSL `Ubuntu-26.04`,
   g++ 15.2.0. `real33d-native-tests` PASS, 3 of 3. `secret-check` PASS, 6
   sub-checks. Skips were `build-sanity-msvc` (no `cl.exe` on PATH),
   `unreal-module-build` (`--unreal` not passed) and `protocol-log-check`
   (no evidence log added or changed against `origin/main`).

2. **Anomaly detector, negative control** — `--logs <fixture>/clean`, a
   snapshot with all three counters at zero. `protocol-log-check` PASS,
   1 log scanned.

3. **Anomaly detector, positive control** — `--logs <fixture>/dirty`, a
   snapshot with `residual_bytes` 55 and `unsupported_opcodes` 2, plus a text
   log carrying `SV_CMD_TALK is not decoded; 41 bytes of the payload left
   unwalked`. `protocol-log-check` FAIL, 2 of 2 logs, each anomaly named with
   its file and value. Overall `FAIL`, exit `1`.

   A detector that cannot fail is not a detector, which is why both directions
   are recorded rather than only the clean one.

4. **Historical census** — `--logs-all`. FAIL, 9 of 136 logs under `evidence/`
   carry anomalies, every one of them the already-known `SV_CMD_TALK` chat gap
   with its matching residual byte count. This is why the default scope is the
   current branch: scanning history re-reports adjudicated numbers on every
   run until nobody reads the check.

5. **`--strict`** — a run whose only non-PASS results were skips exited `1`.

6. **Graceful degradation** — `--no-wsl` on a host with no native C++
   compiler. `clientcore-tests` and `real33d-native-tests` both `SKIP` with
   `no C++ compiler and no usable WSL distro found`. No check invented a
   result; `secret-check`, `provenance` and `output-secret-scan` still ran and
   passed.

## One defect the harness found in itself

Adding this very document made run 1 turn `FAIL`. The default scope collects
whatever git reports as added or changed under `evidence/`, and the extension
filter was only being applied when walking a directory, not to the paths git
returned. A milestone write-up is prose that quotes the exact diagnostics the
check greps for, so the document reporting an anomaly was flagged as the
anomaly.

Fixed by filtering git-derived paths to `.json`, `.jsonl`, `.txt` and `.log`
— machine-written logs — while leaving explicit `--logs` paths unfiltered,
because naming a file is the caller saying they meant that file. Re-verified
afterwards: default scope back to `SKIP`, the dirty directory still `FAIL`
2 of 2, an explicitly named dirty file still `FAIL` 1 of 1, the clean fixture
still `PASS`.

## Limits

- Nothing here is live proof. No Fusion32 server is started, no client is
  launched. Live certification remains a separate, operator-driven step.
- `build-sanity-msvc` and `unreal-module-build` have not been observed PASS
  from this session: no MSVC environment was available, so both are recorded
  `SKIP` above and neither is claimed. They remain unexercised code paths
  until a run from an x64 Native Tools prompt says otherwise.
- `real33d-native-tests` ran without `--objects-srv`, so the runtime MultiUse
  assertions against a live `objects.srv` were not exercised. The harness says
  so in its own report rather than leaving the omission silent.
- The harness reads logs that already exist. It does not produce sessions.
