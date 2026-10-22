# Independent UNREAL-DIAGONAL-INPUT-001 certification

Status: CERTIFIED for the requested live matrix, with the exclusions below.
Reviewer: Codex. Date: 2026-10-22.

Implementation: eb17e26ed79f63bf60e6ec709b9c1540ab008694.
Main: a868572864519b8497af30c5054b78d803b81826.
Branch: certification/unreal-diagonal-input-001-20261022.
Fresh worktree: C:/Users/dell/Desktop/fusion32/build/cert-diagonal-input-20261022.

The milestone branch was pushed normally before certification. Its remote HEAD
was resolved with git ls-remote; the implementation has exactly one parent,
the expected main SHA. No merge, rebase or cherry-pick. The certification worktree
was created from the full remote implementation SHA. Pre-existing untracked work
in the implementation workspace was preserved.

Scope: independently rebuild/test the final camera-relative implementation and
exercise real native input against the normal authoritative Fusion32 runtime.
The old absolute-camera live run is historical evidence, not an independent
confirmation of the final build. No production modification is planned.

The pre-existing login-handover INIT_GAME observation remains separate at
../../clientcore/unreal-diagonal-input-001/OBSERVED-login-handover-initgame.md.
No repair to that path is part of this milestone.

Certification-only native_ui_driver.py extends the external Win32 SendInput
driver with a bounded scan-code key hold, real relative-mouse orbit and F9
snapshot gesture. It requires a current PID and matching foreground HWND.
It invokes no Slate callbacks, game API or protocol methods.

## Independent live result

The fresh REAL33DEditor module built successfully, SHA256
`95D66CC98F7B24CBB01F735D7EE3C60C1AA152C9CC3A5D3B9797EDA3C6880C61`.
The normal launcher is preserved in `commands/launch-live.ps1` and its exact
arguments, PID 17996 and hash in `live/launch.json`. REAL33D connected to the
existing Fusion32 runtime as account B. No credentials, inventory, server saves,
map data, or protocol calls were manufactured. The only gameplay actions were
real OS keyboard/mouse gestures on the foreground Unreal window.

The default camera was actually yaw 45 degrees, not north. Historical prose
calling it north is not used as evidence. Real right-button drags oriented it
to yaw 270.57; W then produced server delta (0,-1,0). Subsequent drags reached
yaw 91.30, a measured 179.27-degree rotation (within one degree of 180), and W
produced (0,+1,0). The two camera states are backed by F9 snapshots and real
window captures, not a forced camera setting.

| Requirement | Independently observed result | Probe / server request |
|---|---|---|
| Q at north | (32087,32227,7) -> (32086,32226,7), NW (-1,-1,0) | 07 / 3 |
| E at north | (32086,32226,7) -> (32087,32225,7), NE (+1,-1,0) | 08 / 4 |
| Z at north | (32087,32225,7) -> (32086,32226,7), SW (-1,+1,0) | 09 / 5 |
| C at north | (32086,32226,7) -> (32087,32227,7), SE (+1,+1,0); loop closes | 10 / 6 |
| Repeated diagonal | Two consecutive Q presses accepted, cumulative (-2,-2,0) | 11-12 / 7-8 |
| Blocked diagonal | Next Q rejected; (32085,32225,7) unchanged | 13 / 9 |
| Cardinal/diagonal alternation | D,C,S,Q,A,C -> E,SE,S,NW,W,SE; one response each | 14-19 / 10-15 |
| WASD | W/N, D/E, S/S, A/W at north; W/S after rotation | 06,14,16,18,37 |
| Arrows | Up/right/down/left -> N/E/S/W; loop closes | 20-23 / 16-19 |
| Chat focus | Real click then Q/E/Z/C displays `qezc`; zero walk/say requests and unchanged position | 24-28; `27-chat-C.png` |
| Mouse walking | Click at (800,380) selects a two-step north path; both steps accepted | 29 / 20-21 |
| Look | Shift+click at (740,474) sends one Look; server says `You see grass.`; no walk | 30 |
| Q after rotation | (32086,32226,7) -> (32087,32227,7), SE (+1,+1,0) | 38 / 23 |
| E after rotation | (32087,32227,7) -> (32086,32228,7), SW (-1,+1,0) | 39 / 24 |
| Return/close | Real east + Q restores starting (32088,32229,7); normal window close | 40-41 / 25-26 |

Each `*-result.json` ties an external gesture to before/after snapshots and the
new journal rows. Every controlled key has exactly one sent request and one
matching accepted/rejected response; the mouse path has exactly two. The
independent read-only audit is reproducible with:

```
python evidence/certification/diagonal-input-20261022/audit_live.py evidence/certification/diagonal-input-20261022/live
```

It checks exact server deltas, fresh snapshot timestamps, request/input identity,
camera rotation, chat inactivity, Look response, and the entire journal for
duplicate sends or terminal responses. `live/audit.json` records PASS for
25 controlled walk requests and 84 snapshots. Entire session: 26 requests,
25 accepted, one rejected, 25 local-player moves, zero unanswered, zero external
relocations, zero duplicate sends/responses. EndPlay: 448 frames, 580 commands,
residual bytes 0, unsupported opcodes 0, protocol anomalies 0, diagnostic `none`.
No item moves, uses, attacks or chat submissions occurred.

## Exclusions and limits

Setup probe 05 contained an unexpected NW walk, request/input 1, while the
helper was only orbiting the camera. Its origin is **UNVERIFIED**. It is retained
in the original journal and in `audit.json`, and excluded from controlled gesture
attribution. It is not labelled user input or a feature defect without evidence.
This does not count as a passing Q test. Controlled requests 2-26 have exact
attribution; no whole-session claim of zero unexplained input is made.

Orbit attempt 33 lost foreground; the helper aborted and released its mouse
button. Its before snapshot and subsequent discovery record are preserved.
There is no successful result for that attempt. Retry 33b completed without
movement. Neither event is hidden or counted as a passing gesture.

East/west-facing camera quadrants, held-key diagonal repetition and modified
diagonals were not live-certified. Repetition here means separate key presses;
the implementation intentionally binds diagonal keys to IE_Pressed only.
The unrelated runtime MultiUse branch of the native item-use suite was not
exercised by this harness (it explicitly reports no `--objects-srv`).

The separate login-handover INIT_GAME issue was not reproduced, fixed, or
certified by this run. Normal close completed, and the Fusion32 services started
for this certification were stopped cleanly afterward.

## Build and deterministic evidence

ClientCore 14/14 and native 8/8 passed. Unreal automation independently executed
all nine REAL33D.Presentation tests and REAL33D.WideWorld.AuthoritativeWindow:
10/10 Success, zero errors; see `tests/unreal-automation-index.json`.
`tests/unreal-build-clean.txt` contains the successful fresh build.

Two earlier build attempts failed for environmental reasons, retained as
`tests/attempt-1-disk-full.txt` and `tests/attempt-2-invalid-pch.txt`. The first
exhausted C: while writing a PCH; the second reused that invalid PCH. Only
untracked, regenerable Intermediate caches were removed after absolute-path
and tracked-file checks: old stack/outfit worktrees to recover disk space, then
this worktree's Intermediate/Build to regenerate the damaged PCH. No source,
evidence, binaries, runtime saves or inventory were deleted. The clean rebuild
took 386.25 seconds and succeeded without implementation edits.

The final harness uses the normal unmodified script with `--unreal` and
`--logs build/certification/live`, including all live observations. Result:
11 PASS, 0 FAIL, 0 SKIP; 367 logs scanned, zero carried protocol anomalies.
Git Bash secret_check: 6/6 PASS. See
`tests/harness-final/REGRESSION-HARNESS-20261022T062730Z.md` and its logs.
Archived text logs normalize line endings and trailing whitespace only;
original tool outputs remain in the ignored `build/certification` directory.

The final harness rebuilt the same-source MSVC library and relinked Unreal;
the resulting DLL SHA256 is
`A754C31726F5BE1AA404CC0F7173E270479EEA0455C629ABC59E1928FA69F56E`.
The live/automation-tested build's hash is the earlier hash above. Both builds
use the exact same implementation commit and source files; this distinction
is retained rather than assigning the later binary hash to the earlier session.
No production, test, protocol or server source differs from implementation HEAD.

Remote verification after live QA: origin/main remains
`a868572864519b8497af30c5054b78d803b81826`; the normally pushed milestone remains
`eb17e26ed79f63bf60e6ec709b9c1540ab008694`. The certification commit adds only
documentation and certification tooling/evidence. Main was not merged.
