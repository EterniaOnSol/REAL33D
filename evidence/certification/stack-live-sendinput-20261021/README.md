# UNREAL-STACK-MOVE-CONTROLS-001 independent live certification

Status: CERTIFIED for the exercised scope below. READY_TO_MERGE_MAIN=YES,
with the explicitly listed unverified cases preserved. No main merge performed.

Reviewer: Codex. Local date 2026-10-21 America/Guatemala; live evidence UTC
2026-10-22. Implementation 1abb013bf711978b9439b6abe836672b5f4148ac.
Starting certification infrastructure HEAD b8b7f3252afaacc193948c686d85ac5f3f582da7.
Branch certification/stack-live-sendinput-20261021. Worktree
C:/Users/dell/Desktop/fusion32/build/cert-stack-move-controls-20261021.
Main remains fbc0de992142e26cab0a80f813d2c09b10dc191a.

## Method and provenance

The existing exact-implementation DLL was reused without rebuilding or changing
production code. SHA256 of UnrealEditor-REAL33D.dll:
62FD3AA4AE610C35AD8D3FFEC7141B105375882E1EB3E30E020EC47F9F6E6076.
The production diff against 1abb013 for unreal/clientcore/scripts/tests/reference
is empty. Earlier independent deterministic checks/builds are retained in
../../clientcore/unreal-stack-move-controls-independent-20261021/README.md:
ClientCore 14/14, native 7/7, Unreal automation 10/10, Unreal build PASS,
Git Bash secret_check 6/6, harness 0 FAIL. They were not repeated in this live run.

The certification-only native_ui_driver.py extends the previously proven external
Win32 helper. It discovers the current PID's window, checks foreground ownership,
and uses SetCursorPos plus SendInput mouse and scan-code keyboard events. No
Slate callbacks, game API calls, protocol commands, memory attachment, or server
data edits were used. F9 snapshots are requested through actual keyboard input.
Screenshots are captures of the actual foreground application's client area.
Input records preserve PID/session/desktop/HWND, coordinates and modifiers.

Normal Fusion32 services were restored using the existing start_wsl.sh. Their
PID/executable/cwd/start/port checks passed. The unchanged Unreal client launched
with the existing runtime, account B, -game -windowed 1600x900 and t.MaxFPS 30;
see run2/launch.json. Unreal PID 6812, player 1002 at 32097,32210,7. Real objects
already present were used. No inventory, world or server fixtures were fabricated.

## Results and exact evidence

run2/*-state.json are captured production snapshots; matching *-input.json and
PNGs record the real actions and resulting UI. Counts below are received state,
not an optimistic client animation. World amounts are additionally evidenced by
the received source amount offered in the dialog, server Look responses and
subsequent authoritative pickup counts. A second observing client was not used.

| Criterion | Executed behavior and evidence | Result |
| --- | --- | --- |
| Plain drag / full stack | run1 05 moved both coins from bag to body. run2 13 recombined to body=2; 14 moved 2 to world; 34/35 moved 2 bag->world->bag. | PASS |
| Shift held before press | run1 06 split body 2 into body=1/bag=1. run2 19 split world pile 2 into bag=1/world remainder=1, then 20 picked up the remainder. run2 22 split bag 2 into two separate 1 stacks. | PASS |
| Ctrl amount dialog | run1 08 body->bag; run2 03/08 bag->body, 17 world->bag, 32 bag->world. Each offers the actual received amount 2. | PASS |
| Clamp | run2 03 opens 2 of 2; 04 clicks >> and remains 2 of 2. 05 clicks << and reaches 1 of 2; 06 repeats and stays 1. | PASS at authoritative stack=2 |
| Chosen partial | 09 selects One; 10 confirms 1 of 2: exactly one move, body=1 and bag=1. | PASS |
| Cancel sends nothing | 03..07: moves stays 0. 32..33: stays 17. Escape at 18 abandons world dialog; 17->19 increases by only the one actual Shift move at 19. | PASS |
| Inventory<->world | 11/12 move coin body->world->body. 14 sends full body stack=2. 27 sends rope body->world. | PASS |
| Inventory<->container | 10/13/21 coins both directions; 26 rope bag->body; run1 04 rope body->bag with Ctrl. | PASS |
| Container<->container | 22 uses two slots in container 0, authoritative 2 becomes 1+1; 23 merges them; 30/31 move the full stack via real container coordinates. | PASS within one bag; distinct bags UNVERIFIED |
| World destinations | 12/20 world->body; 19 partial world->bag and 35 full world->bag. 28 non-stackable world->bag. | PASS |
| Non-stackables | Rope type 3003: run1 04 Ctrl body->bag; run2 26 Shift bag->body, 27 Ctrl body->world, 28 Ctrl world->bag. Each count=1 without amount dialog. | PASS |
| Authoritative counts | Every step's F9 inventory/container state retained. Split and recombination 2->1+1->2 and world pickup conservation confirmed. Final inventory/container equals run2 02 exactly. | PASS for one client's server-received state |
| No duplicate/stray moves | 34 state snapshots, expected per-action deltas; 19 actual move gestures yield exactly 19 unique move requests. Dialog/cancel/Look/Use/UseWith add none. | PASS for controlled run2 |
| Stack slot tier loading | Visible one-coin and two-coin art changes in run1 05/06 and run2 02/10/22/23. Delivered 3031.png and 3031_p1.png render in real body/container slots. This independently exercises the filename fix. | PASS for amounts 1 and 2 |
| Look regression | Shift with no pointer displacement at 29 gives server "You see a rope" and weight; moves stays 15, looks rises 4->5. Shift drags also preserve their initial Look. | PASS |
| Use regression | Right-click existing backpack opens actual server container 0 at 02; uses=1, inventory unchanged. | PASS |
| UseWith regression | Right-click rope at 24 displays target banner without sending; click sword at 25 sends one use-with, clears banner and displays Fusion32's "You cannot use this object." Moves unchanged. Successful rope gameplay on a suitable environmental target was not exercised. | PASS for targeting/transport/refusal flow |
| Protocol | EndPlay: 664 seconds, 355 frames, 664 commands; residual_bytes=0, unsupported_opcodes=0, protocol_anomalies=0, last_diagnostic=none. All retained controlled snapshots have the same zero counters. | PASS |

audit_evidence.py reads only retained snapshots/logs and checks the 34 expected
move totals, 19 unique production log requests, zero protocol counters, connected
state, and exact inventory/container restoration. audit.json retains its output.
It passed. This audit does not replace the real gestures or visual inspection.

## Retained attempts and limits

Run1 captures 01..08 are the independently performed early gestures. During the
long interval between user continuations its log also records additional actions
and reconnections without matching agent input records. Their provenance is not
established here; they are excluded from independent gesture/count certification.
The first session eventually disconnected after an idle warning. Its full filtered
production log is retained honestly in run1/application-state.txt. Run2 starts a
new client session and counter baseline. No later manual activity is used as PASS.

At run2 15, the initial helper released Ctrl immediately after mouse-up; the world
controller processed a full move rather than opening a dialog. The record and
result are retained. Certification tooling was corrected to hold the modifier
for 300 ms after mouse-up, across the game tick processing release. The repeated
world gesture at 17 opens the expected 2-of-2 dialog without a move, and Shift
at 19 sends one from a pile of two. This is a disclosed driver timing correction,
not a production change; step 15 is not claimed as successful Ctrl behavior.

UNVERIFIED:
- Two distinct containers: only one backpack exists among the received inventory
  and open container's objects; none was created or inserted.
- Concurrent authoritative source shrink while the amount dialog is open: no
  second interacting client was available; no source mutation was fabricated.
- Amount-dependent world 3D appearance: world uses generic delivered placeholders;
  the observed square is not evidence of amount-specific world art.
- Classic binary 10/25/50 tier thresholds; higher-than-2 live slot tier rendering.
  Available art paths and prior deterministic tests are not new live rendering
  proof at amounts the server did not provide.
- A second client independently observing source and destination counts; successful
  UseWith environmental gameplay beyond the tested targeting/refusal regression.
- Ground->ground movement was not part of this resumed route matrix and is not
  promoted to independently live-certified behavior by these results.

## Closure

All run2 moved objects were recovered; inventory and backpack contents/order are
identical to the captured run2 baseline. This does not claim to restore unknown
intervening run1 actions. No server data files were edited. Unreal closed normally
while connected; EndPlay retained. The three services initially started by this
certification were stopped cleanly. Production and main unchanged. No push or
merge performed in this resume. Tracked changes are certification helper, evidence,
report and status/handoff documentation only.
