# Independent UNREAL-TURN-OBJECT-001 certification

Status: **CERTIFIED** for the requested logical interaction matrix, resumed on
2026-10-23 from the existing worktree and captures. This is not visual mesh
orientation certification. Implementation remains `2827c6c82f711ec356f65db213e59c3894b597f2`.

## Final live matrix

| Check | Result and retained evidence |
|---|---|
| First rotatable object | PASS: goblin statue at 32081,32203,7, stack 1; 2030 -> 2048 -> 2046 -> 2047 -> 2030. B/010 through B/019. |
| Second rotatable object | PASS: chest at 32110,32203,6, stack 1; 2472 -> 2482 in B2/043; restored through 2480,2481 to 2472 in B3/047,049,051. |
| Opcode and duplicate sends | PASS for observed test activations: 9 UI activations correlate one-to-one with 9 decoded server TURN_OBJECT 133 calls. Zero duplicates observed. Ten total UI rotations include one explicitly untraced cleanup step (B3 action 1, described below). |
| Target routing / stack | PASS: exact x/y/z/type/stack in server input and authoritative CHANGE_FIELD 107; floors 7 and 6, object stack 1 above ground, nonrotatable ground stack 0. No arbitrary buried-stack selection claim. |
| Authority | PASS: server holds first Turn for 12 seconds; client menu still resolves type 2030 during the hold, then resolves 2048 after server change. Server field bytes are retained. Screenshots use placeholders and do not prove visible orientation changes. |
| Non-rotatable | PASS: ground 408 at 32081,32205,7 stack 0 -> NOTTURNABLE 57, displayed refusal, no field mutation. B/022-023. |
| Range | PASS: statue request from 32084,32204,7 causes server approach to 32082,32204,7 then rotation. Blocked chest approach returns NOWAY 44; adjacent request succeeds. Different-floor refusal is not separately exercised. |
| Normal right-click Use | PASS: backpack opens via one ordinary right click; B/025, B3/052. Door Use changes 1632 -> 1633 in B2/038. |
| Ctrl-right-click Rotate | PASS: opens world menu without a send; selecting Rotate routes one request with captured object reference. Reopening resolves updated type. |
| Look | PASS: dustbin and ground descriptions returned by Fusion32; B/009 and B/020 logs plus subsequent fresh snapshots. |
| UseWith | PASS for the regression: right-click rope 3003 arms target mode with no send; Ctrl-right-click chest completes one UseWith, bypasses the new menu, then Fusion32 returns CANNOTUSE 29. B3/053-054. Successful rope gameplay is not claimed. |
| Drag/drop and Ctrl amount | PASS: one move reorders rope; next ordinary move restores bag order. Ctrl drag opens amount dialog before a move is sent. B2/040-041 cancel sends nothing. B/027-029 and B2/040-041. |
| Restoration | PASS: statue 2030, chest 2472, door 1632; original inventory and bag contents/order unchanged in final B3 close snapshot. Character remains at 32109,32203,6. |
| Protocol | PASS: all 122 valid retained F9/close snapshots have residual bytes, unsupported opcodes and anomalies = 0. |

Run `python evidence/certification/turn-object-20261022/audit.py` for **38
assertions** over published captures. `audit-result.json` records the counts and
limitations. `raw-manifest.json` hashes all 247 original artifacts; 203 compact
review copies (all structured captures/logs and 14 selected screenshots) live
under `live/`. Every original remains at `build/certification/live/`.

## Resume and instrumentation limits

The server was already healthy (querymanager 754, game 763, login 1504), with
executable/cwd/start/port identities verified. No service restart was needed.
The old client had ended, so the unchanged DLL was launched once as B3 (PID
11324). Startup hash matched the earlier certified build. No source, tests,
server memory, runtime files or inventory data were edited.

The debugger attached successfully. Its first resumed batch invocation detached
when normal SIGUSR1 arrived during login. Thus cleanup rotation B3 action 1
(2482 -> 2480) has one UI activation and a subsequent server-derived client type,
but **no independent decoded send count**. This gap is retained, not silently
counted as an observed opcode. Reattaching with `handle SIGUSR1 nostop noprint
pass` and the same setting for SIGUSR2 allowed the final two rotations, door
restoration and UseWith refusal to be observed. Server PID 763 never changed.

The earlier attach interruption and this signal behavior are tooling issues;
neither supplies evidence of a product defect. Hardware breakpoints were
read-only and detached after the final live checks; no inferior calls or writes.

Twenty-eight `*-after.json` files copied an older F9 snapshot because the UI did
not emit a fresh snapshot for those presses. Their original timestamps are
preserved and listed by the audit. They are not evidence of immediate counter
changes. In particular, authority during the hold is proved by the timestamped
client menu log resolving 2030, the screenshot and server trace, not the stale
F9 copies. Final state uses `B3/unreal_slice_evidence.json` from graceful close
(07:55:58.900 UTC), with uses=3 for backpack, UseWith, and door restoration.

## Reproduction

Use this implementation and the existing normal QA account. Find the two object
coordinates above through the actual world, observe current type and stack,
Ctrl-right-click then select Rotate once. Compare client log activation with
server `TURN_133`, then its field update. Complete each rotation cycle using
normal input to restore orientation. Test ground refusal and approach/blocked
path without editing map data. Open the backpack, exercise Look/drag/amount
Cancel, right-click rope and Ctrl-right-click the chest; expect a single use
request and server refusal. Preserve inventory and close the client normally.

For the read-only observer on this verified binary:

```text
gdb -q -batch -p <verified game PID>
  -ex 'set pagination off'
  -ex 'handle SIGUSR1 nostop noprint pass'
  -ex 'handle SIGUSR2 nostop noprint pass'
  -ex 'source /mnt/c/Users/dell/Desktop/fusion32/build/cert-turn-object-independent-20261022/evidence/certification/turn-object-20261022/server_observer.py'
  -ex continue
```

These offsets are binary-specific; reverify disassembly before a different build.
The held-request marker is optional and only delays a request; it changes no
server data. `launch.py` and `probe.py` retain one native gesture per named
capture, with foreground/PID guards. A new `--run` preserves existing evidence.

## Provenance and scope

Expected main: `119dc2e19ea3465518075a93dcdc07fb10dabd5b`.
Implementation: `2827c6c82f711ec356f65db213e59c3894b597f2`.
The implementation's sole parent is the expected main. The implementation
milestone `milestone/unreal-turn-object-001-rebased` was pushed normally before
creating `build/cert-turn-object-independent-20261022` on branch
`certification/unreal-turn-object-independent-20261022`. No main merge.
Production source and tests remain unchanged. Server source, data and inventory
are read-only; live gameplay will use ordinary client input only.

## Independent source inspection

Selected Fusion32 game revision: `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`,
protocol 7.72, as recorded in `docs/protocol772/SOURCE_TRUTH.md`.

- `reference/game/src/receiving.cc::CTurnObject`: reads x/y as words, z as a
  byte, type as a word, stack as a byte, then schedules ToDoTurn. It does not
  replace the supplied stack index. Special map-container types are ignored.
- `reference/game/src/cract.cc::TCreature::ToDoTurn`: resolves that object,
  checks floor, schedules ToDoGo when outside distance one, then TDTurn.
  A non-adjacent request can therefore cause authoritative approach and turn.
- `reference/game/src/operate.cc::Turn`: checks accessibility and ROTATE,
  otherwise throws NOTTURNABLE; accepted rotation calls Change with RotateTarget.
- `reference/game/src/sending.cc::SendChangeField`: field-change opcode 107,
  position, stack and the changed object. Live observer offsets were checked
  against disassembly of the actual running binary, retained under build/.
- Unreal `RequestTurnObject` queues one Turn intent. `DrainUses` passes it to
  ClientCore's existing builder and increments TurnsRequested on send. Neither
  path changes a tile locally. The new world menu captures position/type/stack;
  `GetTopObject` preserves the index in the ordered tile things. Plain right
  click retains Use; pending UseWith bypasses the new context menu.

Observation limitation: TurnsRequested is not exported by this implementation's
F9 JSON writer. The independent server trace, menu/rotate logs and input records
must establish activation/send counts; absence of an F9 turn counter is not
evidence of zero sends.

## Legitimate candidate discovery (not live proof)

`find_objects.py` reads existing runtime map sectors and object metadata. It
found these existing world objects, without creating or moving any:

| Object | Position | Initial file type | Rotation cycle | File stack |
|---|---|---|---|---|
| Goblin statue | 32081,32203,7 | 2030 | 2030 → 2048 → 2046 → 2047 → 2030 | 1 |
| Chest | 32063,32226,7 | 2472 | 2472 → 2482 → 2480 → 2481 → 2472 | 1 |

File stack is only a planning hint. Current live type and stack must be observed
through the actual client and server before any targeting claim.

## Instrumentation

`native_ui_driver.py` uses real Win32 input, checks the selected process owns
the foreground window, and captures only that client area. `probe.py` records
before/after F9 state and one input gesture. The computer-use skill was inspected;
its node_repl runtime was unavailable, so the existing native certification
driver was used as the fallback. It does not call a game API or send packets.

`server_observer.py` uses four hardware breakpoints and reads decoded Turn input,
outgoing field-change bytes and result codes. It makes no inferior function
calls or memory writes. An optional marker pauses one request before mutation
for 12 seconds, then automatically releases it. Existing server services must
remain running; detach only this certification debugger after the test.

## Build history

The first strict harness passed ClientCore 14/14, native 10/10, secret_check 6/6
and the other non-Unreal checks, but failed the Unreal build due to disk space.
A second PCH attempt also exhausted space. Both failure logs are retained.
Generated caches from earlier worktrees were compressed without changing their
contents, and parallel actions reduced to one in ignored local build config.
The third attempt completed all 36 actions successfully (1263.46 seconds).
Unreal automation then passed 10/10, with zero test warnings/errors, at
2026-10-23 02:56:20 UTC. The compiled DLL SHA256 is
`fcbaad1cd68c6c884fd3f29fca58e6922cda018f53a0c3a0880f8e1a6ccc60d7`.

The initial automation script invocation was blocked by PowerShell execution
policy; the explicit, approved process-scoped invocation ran successfully.
Client B launched as PID 21296 and reached the world. Two guarded attempts to
activate its window failed (including elevated execution), so the driver sent
no gameplay input. User focus assistance was requested before proceeding.

## Final validation and remaining scope

The resumed strict harness uses `validation/final-harness-resume.cmd`; the first
resumed invocation lacked Python/WSL access under the sandbox and hit denied UBA
cache-directory writes. It was stopped and retained under build/, including its
successful MSVC suites. The corrected runner supplies explicit tool paths and
runs with the required filesystem access. This changed only certification
tooling. Final reports are retained under `validation/final-harness-resume/`.

Final result: **11 PASS / 0 FAIL / 0 SKIP**. ClientCore 14/14, native 10/10,
MSVC C++17 suites and C++20 archive PASS, Unreal module build PASS, secret_check
6/6, protocol scan 392 files / 0 anomalies, output-secret scan clean. Retained
Unreal automation is 10/10 with zero errors/warnings. The final build relinked
unchanged sources; both the live and final DLL hashes are in `provenance.json`.

No requested logical interaction remains pending. Not claimed: visible mesh
orientation, successful rope gameplay, all possible stack positions, inventory
rotation, and different-floor refusal. The untraced cleanup send and stale F9
copies are explicit limits above. No merge to main, no new push, and no
production changes. Resolve the evidence-only certification HEAD with
`git rev-parse certification/unreal-turn-object-independent-20261022`.
