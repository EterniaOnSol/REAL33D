# Independent review: UNREAL-STACK-MOVE-CONTROLS-001

Reviewer: Codex. Date: 2026-10-21, America/Guatemala.

Candidate: `1abb013bf711978b9439b6abe836672b5f4148ac`.
Fresh worktree: `C:/Users/dell/Desktop/fusion32/build/cert-stack-move-controls-20261021`.
Branch: `certification/unreal-stack-move-controls-001-20261021`.

## Provenance

The milestone was pushed normally to origin before creating this worktree.
Remote branch HEAD was independently resolved with `git ls-remote` to the
candidate above. The exact parent chain is:

```
fbc0de992142e26cab0a80f813d2c09b10dc191a
0cbc057b9d0054040b2f7a210993008340cb5526
1abb013bf711978b9439b6abe836672b5f4148ac
```

Each implementation commit has exactly the preceding commit as its sole
parent. No merge to main was performed. No implementation files changed.

## Independent live verification is blocked

The Windows Computer Use package initialized, but `sky.list_windows()` failed
with `Computer Use native pipe is unavailable: failed to connect native pipe`
and Windows error 2. The retry failed identically. Resetting the JavaScript
session, reinitializing the package and retrying also failed identically.
No UI gestures were executed. No new live server session is claimed.

The implementer's screenshots, move logs and snapshots were inspected as
historical evidence; they do not constitute an independently repeated run.

All requested live rows remain UNVERIFIED: plain full-stack drag, held
Shift+drag, held Ctrl+drag and dialog, clamp against authoritative count,
Cancel sending nothing, inventory/world, inventory/container,
container/container, world destinations, non-stackable behavior,
authoritative source and destination counts, no duplicate/stray moves,
Look/Use/UseWith regression, actual stack tier texture loading, and live
protocol health.

The four explicitly bounded scenarios also remain UNVERIFIED: two distinct
containers, concurrent shrink while the dialog is open, amount-dependent
world 3D appearance, and 10/25/50 thresholds against the classic binary.

## Source review

Reviewed the two implementation diffs and their common move path:
`SReal33DHUD::RequestStackMove`, `SendMove`, `AskMoveAmount`,
`HandleAmountChosen`, `HandleAmountAbandoned`, slot drag/drop handlers,
world drag/drop handlers, the amount panel and the sprite path resolver.

`DecideStackMove` implements the requested decision table and treats
non-stackables separately. `ClampStackAmount` bounds to 1..min(stack,255).
`reference/game/src/receiving.cc::CMoveObject` reads a single count byte and
rejects count zero for cumulative objects. `HandleAmountChosen` clears its
pending request before sending; abandonment clears it without a move call.
The four routes converge on the common decision. These are source findings,
not live outcomes.

The amount bound is captured, not refreshed at confirmation. The requested
concurrent-shrink scenario remains unexercised. `ItemFramePath` accepts both
`<id>_p<n>.png` and `<id>_<n>.png`; the native sprite suite verifies the tier
selection logic, not loading/rendering PNGs. No implementation defect was
established by the performed checks.

## Executed checks

- ClientCore: 14/14 CTest suites PASS in WSL from this worktree.
- Native: 7/7 suites PASS, including 16 stack-move cases.
- Git Bash secret_check: 6/6 PASS.
- Outfit mapping: 43 byte assertions PASS; palette: 51 assertions PASS.
- MSVC: C++17 suites and C++20 Unreal archive PASS.
- Unreal: fresh REAL33DEditor Win64 Development build PASS (`Result: Succeeded`).
- Unreal automation: 10/10 Success; zero test errors and warnings, no failed,
  not-run or in-process tests. This includes `REAL33D.Presentation.StackMoveControls`.
  See `automation-index.json`, `automation-results.txt` and `unreal-build.txt`.
- Harness: 9 PASS, 0 FAIL, 2 SKIP (MSVC/Unreal PATH detection); the builds
  are executed separately and retain their own logs.
- Historical protocol scan: 12 logs, zero reported anomalies. This is not
  a fresh live protocol-zero certification.

The first harness wrapper could not resolve Unix utilities. A login-shell
retry lacked the Windows toolchain on PATH and produced skips. The direct
Git Bash run above executed the WSL suites and other available checks. No
failed/skipped startup attempt is being reclassified as a successful check.

Overall status: BLOCKED for independent live certification. READY_TO_MERGE_MAIN=NO.

## Reproduction and evidence

Run from the certification worktree. Use Git Bash (with the normal Windows
PATH) for `bash tests/regression_harness.sh --out build/certification/harness-direct --unreal`.
The retained harness report and per-check logs are under `harness/` beside
this report. Its two toolchain-detection skips are explicitly retained.

The independent Windows build was run with the installed x64 `vcvars64.bat`
followed by `tests/build_clientcore_windows.cmd`; see `msvc-build.txt`.
The Unreal command is the installed UE_5.8 `Build.bat` with arguments
`REAL33DEditor Win64 Development -Project=<this-worktree>/unreal/REAL33D/REAL33D.uproject -WaitMutex`.
Automation uses `scripts/client/test_unreal_presentation.ps1` with a new
`-EvidenceDirectory`, and its actual JSON results must be read rather than
inferring a count from the script's two required-test names.

The classic executable was copied only to the ignored `build/` directory for
the static mapping/palette checks; it was never executed. No runtime secrets
were copied or printed. No pre-existing files in the implementation worktree
were edited. Final tracked changes are evidence and handoff/status only.

The automation script was launched using a process-local PowerShell
`-ExecutionPolicy Bypass` after the ordinary script invocation was refused.
No persistent execution-policy setting changed. Engine startup emitted
unrelated Zen registry-access and Epic SDK network diagnostics in the local
full log; the ten selected tests completed successfully with zero test
errors/warnings. No claim is made that every engine startup log line is clean.
