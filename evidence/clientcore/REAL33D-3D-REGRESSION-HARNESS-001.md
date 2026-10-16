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

## Log scope policy

Recorded in `tests/README.md` under "Log scope policy" and restated here
because run 4 below produces a `FAIL` that must never be read as this
milestone's result:

- Default scope is milestone-scoped: only evidence logs the branch added or
  changed against `origin/main`, machine-written extensions only.
- `--logs-all` is historical audit only. It is never a milestone gate.
- A historical anomaly is not attributable to the current milestone unless
  that milestone's own logs reproduce it. The nine `SV_CMD_TALK` logs found by
  run 4 predate this branch, are already adjudicated, and are not a finding
  of `REAL33D-3D-REGRESSION-HARNESS-001`.
- `PASS` / `FAIL` / `SKIP` semantics do not change with scope. An empty scope
  is `SKIP`, never `PASS`.

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
- `build-sanity-msvc` and `unreal-module-build` were not observed PASS when
  this document was first written: no MSVC environment was available, so both
  were recorded `SKIP` and neither was claimed. That caution was warranted —
  when they were first actually exercised they turned out to be broken. See
  "False PASS in the two Windows checks" below for the defect, the fix and
  the first real `PASS` for each.
- `real33d-native-tests` ran without `--objects-srv`, so the runtime MultiUse
  assertions against a live `objects.srv` were not exercised. The harness says
  so in its own report rather than leaving the omission silent.
- The harness reads logs that already exist. It does not produce sessions.

## False PASS in the two Windows checks, 2026-10-15

A gate that cannot fail is worse than no gate. Both Windows checks were in
that state from the moment they were written until this fix, and two
published reports carried the overstated rows before anyone noticed.

### What happened

Both checks were written as

```bash
( cd "$REPO" && cmd.exe /c "<command>" ) >"$log" 2>&1
```

and from Git Bash that runs nothing at all. MSYS rewrites any argument that
looks like an absolute Unix path, so the `/c` **switch** arrived as the path
`C:/`. `cmd.exe` never saw a switch, started interactively, read EOF from the
redirected stdin, printed its banner and exited **0**. Both checks keyed on
exit status alone, so both recorded `PASS`.

The signature is unmistakable once known: a four-line log containing a
Windows banner and a prompt, and nothing else.

```
Microsoft Windows [Versión 10.0.22631.6199]
(c) Microsoft Corporation. Todos los derechos reservados.

C:\Users\dell\Desktop\fusion32\build\...>
```

It stayed hidden because the `Limits` section above is accurate: until
2026-10-15 nobody had an MSVC environment, so both checks always `SKIP`ped
and the broken branch was never taken. The first two runs that did take it —
the independent certification of UNREAL-WORLD-PRESENTATION-POLISH-001 and the
ClientCore integration rehearsal against the new `origin/main` — both got a
false `PASS`.

### The fix

Two changes, because the invocation and the verdict were both too weak.

**1. `run_windows_command`.** The command line is written to a temporary
`.cmd` and executed by path, with argument conversion disabled and stdin
closed, so an interactive prompt can never wait and cmd's own quote stripping
can never mangle a quoted program path.

The switch stays `/c`, not `//c`. Both forms run under Git Bash, but for
opposite reasons: `//c` works only *because* MSYS collapses it back to `/c`,
so it breaks the moment conversion is turned off and is wrong on a non-MSYS
host. `/c` plus `MSYS_NO_PATHCONV` is correct everywhere, because the
variables are ignored where they mean nothing. Combining both mitigations
cancels them out and silently restores the original bug — which is not a
hypothetical: the first draft of this fix did exactly that, and case 1 of the
self-test caught it.

Absence of `cmd.exe` returns 127, which callers map to `SKIP`. A Linux CI box
is not a broken build.

**2. Exit status is no longer sufficient.** Each check now also requires the
success line the build prints for itself:

| Check | Required evidence |
|---|---|
| `build-sanity-msvc` | `WINDOWS CLIENTCORE: PASS` |
| `unreal-module-build` | `Result: Succeeded` |

Exit 0 without that line is `FAIL`, with the reason named in the report:
*"exit 0 but the log does not contain '…'; the build did not run"*. The three
statuses keep their meanings exactly: missing toolchain `SKIP`, execution
failure `FAIL`, real success `PASS`.

### Regression test

`tests/regression_harness_selftest.sh` drives the real harness against a
throwaway fixture repository whose batch files it writes, so it exercises the
real `cmd.exe` on the real host rather than a mock. Seven assertions, all
passing:

```
ok   : the Unreal check actually executes its command (sentinel written)
ok   : a real build that prints its success line is PASS
ok   : no-op cmd.exe exit 0 is FAIL for unreal-module-build, not PASS
ok   : no-op cmd.exe exit 0 is FAIL for build-sanity-msvc, not PASS
ok   : build-sanity-msvc is PASS when its success line is present
ok   : a non-zero exit is FAIL even when the success line was present
ok   : a missing engine is SKIP, not FAIL and not PASS

7 passed, 0 failed
```

Case 1 is the root-cause assertion: the fixture's batch file writes a
sentinel to disk, so a cmd invocation that runs nothing fails the test
regardless of what it returns. The positive cases matter as much — a gate
pinned to `FAIL` would also pass a naive "does it fail?" test.

### First real PASS for both checks

Full run from an x64 Native Tools environment, `--unreal` and
`--objects-srv` supplied, report
`build/harness-fix-verification/REGRESSION-HARNESS-20261015T233154Z.md`:

```
provenance             PASS
branch-isolation       PASS
clientcore-tests       PASS  9 CTest suite(s) passed via wsl
real33d-native-tests   PASS  3 passed, 0 failed via wsl
secret-check           PASS  6 sub-check(s) ok
build-sanity-msvc      PASS  C++17 suites and the C++20 archive Unreal links both built
unreal-module-build    PASS  REAL33DEditor Win64 Development built
protocol-log-check     SKIP  no logs in scope
output-secret-scan     PASS
OVERALL                PASS_WITH_SKIPS
```

Both rows are now backed by real output rather than an exit code:
`build-sanity-msvc` 63 lines ending in `WINDOWS CLIENTCORE: PASS`;
`unreal-module-build` 60 lines, 29 compile/link actions, ending in
`Result: Succeeded` after 287.94 s. Before the fix each log was four lines of
banner.

### Erratum: UNREAL-WORLD-PRESENTATION-POLISH-001 remains CERTIFIED

The polish certification at `bef902ca` is **unaffected and stands as
CERTIFIED**. Its MSVC and Unreal results were obtained by running those tools
**directly**, not through the harness:

- `tests\build_clientcore_windows.cmd` from a VS Build Tools x64 environment
  produced real output ending `WINDOWS CLIENTCORE: PASS`, with transport
  22/22 and crypto 25/25.
- `Build.bat REAL33DEditor Win64 Development` ran 31 actions in 365.74 s and
  reported `Result: Succeeded`; the retained `unreal-build.txt` and the
  resulting `UnrealEditor-REAL33D.dll` are that build.

What was overstated is narrower and purely a labelling error: two **row
labels** in the harness table of
`UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-20261015.md` read `PASS`
for `build-sanity-msvc` and `unreal-module-build` when those two harness
invocations had in fact executed nothing. The correct reading of that run is
**6 PASS / 0 FAIL / 3 not-executed**, which is identical to what the earlier
ClientCore integration rehearsal honestly reported as `6 PASS / 0 FAIL /
3 SKIP`. No live criterion, no protocol counter and no test result in that
certification depended on either harness row.

The same correction applies to the integration rehearsal report against
`bef902ca`: its `OVERALL PASS_WITH_SKIPS — 8 PASS, 0 FAIL, 1 SKIP` should be
read as 6 PASS / 0 FAIL / 3 not-executed. Its gates were likewise obtained by
running CTest, the sanitizers, MSVC and the Unreal build directly, and the
integrated tree's `REAL33DEditor` build plus all three polish automation
suites passed on their own evidence.

## Second false PASS: clientcore-tests could not fail, 2026-10-16

The first false PASS was found by reading a log. This one was found the same
way, during UNREAL-HUD-PANEL-REGISTRY-001: a run recorded

```
| `clientcore-tests` | **PASS** | 0 CTest suite(s) passed via wsl |
```

beside a log whose first line was

```
CMake Error: The source directory "..." does not exist.
```

"PASS, 0 suites passed" is a sentence that contradicts itself, and it was
counted as success.

### What happened

Worse than the symptom. The string handed to `bash -c` through `wsl.exe` is
expanded **once more** before the inner shell ever sees it, so every `$var`
and `$?` inside it is replaced with the *outer* shell's value. Measured
directly on this host:

```
wsl.exe -- bash -c 'x=5; echo "x=$x"'        ->  x=          (not x=5)
wsl.exe -- bash -c 'false; echo "q=$?"'      ->  q=0         (the outer $?)
wsl.exe -- bash -c 'false && true'           ->  rc=1        (chains are fine)
wsl.exe -- bash -c 'exit 1'                  ->  rc=1        (literals are fine)
```

The runner ended `rc=$?; rm -rf '$wbuild'; exit $rc`, which therefore arrived
as `rc=; rm -rf ...; exit`. A bare `exit` returns the status of the last
command — the `rm`, which always succeeds. **`clientcore-tests` returned 0
unconditionally**, so it could not report `FAIL` for a configure error, a
build error, or a genuinely failing test suite. The "0 suites" case is simply
the one that made it visible.

`run_one_wsl`, which drives the `real33d-native-tests` stems, ended with the
same idiom and had the same hole:

```
wsl.exe -- bash -c '/bin/false; rc=$?; rm -rf /tmp/x; exit $rc'   ->  rc=0
```

Its compile failures still surfaced, because those take a `|| { ...; exit 1; }`
branch with a literal status, which is why the breakage stayed hidden: the
check looked like it worked whenever the build was what broke.

### The fix

Stop depending on a status surviving the hop, in both runners.

- The measured command is now a plain `&&` chain and the **last thing**
  `wsl.exe` runs, so its own status is what propagates — the one form the
  measurements above show is reliable.
- Cleanup moved out of the measured command into a separate `wsl.exe -- rm`,
  so it can no longer overwrite the result.
- `run_one_wsl` names its build directory from the caller instead of
  `$(mktemp -d)`, which was being expanded away too, and ends with the run.

And `clientcore-tests` no longer trusts a status alone. Three things must all
hold, mirroring the marker gating the Windows checks already use:

| condition | otherwise |
|---|---|
| `rc == 0` | FAIL "configure/build/ctest returned N" |
| at least one suite ran | FAIL "exit 0 but no CTest suite ran; configure or build produced no tests" |
| ctest printed `100% tests passed` | FAIL "N suite(s) ran but ctest did not report 100% tests passed" |

`SKIP` is untouched and still means only one thing: no toolchain. `PASS`,
`FAIL` and `SKIP` keep their existing meanings.

### A/B proof

The same fixture — a repository whose `clientcore/` cannot be configured —
through both versions:

```
old harness (ab54527):  clientcore-tests  PASS  0 CTest suite(s) passed via wsl
new harness:            clientcore-tests  FAIL
```

with `CMake Error: The source directory ... does not exist.` in the log both
times.

### Regression test

`tests/regression_harness_selftest.sh` grows to **10 assertions**, three of
them new:

```
ok   : a clientcore that cannot be configured is FAIL, not PASS
ok   : confirmed: a captured exit status does not survive wsl.exe
ok   : a failing && chain does propagate through wsl.exe
```

The second and third are there because the fix *depends* on that asymmetry.
If a future refactor goes back to capturing `$?` across the hop, or if a host
ever stops propagating chain status, the suite says so instead of silently
returning to a gate that cannot fail. The clientcore cases skip cleanly where
there is no WSL distro.

### Scope

`tests/regression_harness.sh` and `tests/regression_harness_selftest.sh`
only. No other file, no protocol, no gameplay. The HUD panel registry branch
was not touched and shares no file with this change.

### Harness run for this change

Default mode, from an x64 Native Tools environment:

```
clientcore-tests       PASS  13 CTest suite(s) passed via wsl
real33d-native-tests   PASS  4 passed, 0 failed via wsl
secret-check           PASS  6 sub-check(s) ok
build-sanity-msvc      PASS  C++17 suites and the C++20 archive Unreal links both built
unreal-module-build    SKIP  engine present but --unreal not passed
protocol-log-check     SKIP  no logs in scope
OVERALL                PASS_WITH_SKIPS          (0 FAIL)
```

`clientcore-tests` reporting 13 and `real33d-native-tests` reporting 4 are
both through the repaired WSL path, so the fix is exercised by the run that
reports it.

One limitation, stated rather than papered over: `--unreal` was attempted
twice and both runs ended `FAIL  UnrealBuildTool exited 6 / 112`, in each case
with `fatal error C1085 ... Espacio en disco insuficiente` while writing the
UnrealEd shared PCH. That is disk exhaustion on this machine, not a result of
this change — the diff is two shell scripts and a document and contains no
C++ at all, so it cannot affect whether the module compiles. The harness
reported those runs as `FAIL` rather than passing them, which is the corrected
behaviour working. The editor build is covered on the branches that actually
change C++.
