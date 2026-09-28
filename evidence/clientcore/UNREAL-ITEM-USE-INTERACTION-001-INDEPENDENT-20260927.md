# UNREAL-ITEM-USE-INTERACTION-001 - independent run and mouse correction

Date: 2026-09-27 22:13 America/Guatemala (2026-09-28 04:13 UTC).
Candidate branch: `milestone/unreal-item-use-interaction-001`.
Candidate local/remote HEAD: `35284595c44957600615f9cce6b78b9e9b761b86`.
Remote main: `bd15cc0a49d8182dc1cc3732b8487859f1662044`.
Tracked worktree was clean at start. No main checkout, REAL33D2D, agent,
Fusion32 gameplay rule, protocol definition or V08 art was changed.

## Decision

Independent certification: `CERTIFIED` for the corrected interaction revision
documented in the final decision below. The first automated
attempt was `BLOCKED` by the Windows UI helper. A subsequent operator session
reproduced descriptions but exposed unwanted world actions from slot releases.
The operator then explicitly requested mouse corrections within this milestone.
These edits supersede the original no-implementation instruction for this narrow
repair only. During repair the published candidate stayed unchanged on origin
and the corrected worktree held uncommitted edits. The final certification
commit publishes those reviewed changes. No new milestone or main merge started.

Latest gate completed: subsequent fresh sessions reproduce world/slot Look, normal
Say, world/inventory/container Use and valid Use With; operator confirmed the
visible results. User-requested automatic MultiUse entry from normal Use,
including rune classification, has tests/build PASS and fresh live confirmation
with the rapier. Rune spell effects themselves remain IMPLEMENTED_UNVERIFIED.
This current gate supersedes older pending/failed route states below; those
sections preserve the exact chronological failures. Rune spell effects and
complete art coverage are not claimed. Original models accepted with defects
for now. Publication is the final closeout step on the isolated milestone branch.

## Fresh checks and session

- `tests/build_clientcore_windows.cmd`, VS 2022 BuildTools x64: C++17 and
  C++20 libraries built; all eight suites `PASS`, including movement,
  player_state and worldview. Transport 22/22 and crypto 25/25 passed.
- `Build.bat REAL33DEditor Win64 Development
  -Project=<worktree>/unreal/REAL33D/REAL33D.uproject -WaitMutex`:
  `Result: Succeeded`, two incremental actions, 4.89 seconds.
- `tests/secret_check.sh`: `PASS` with explicit WSL `GIT_DIR` and
  `GIT_WORK_TREE` for the Windows-created worktree; all six output checks
  completed without Git errors.
- No established game connection was present before restart. Existing
  services stopped cleanly. Fresh services were identity/start/cwd/exe/port
  verified: Query Manager PID 1820 / 7173, Game PID 1835 / 7172, Login
  PID 2040 / 7171.
- A new REAL33D process was launched against that runtime as account A,
  with evidence directed to ignored `build/cert-item-use-20260927`.
  It entered Game and received a server LoginMessage at 00:35:07.726 UTC.
  It later closed cleanly at 00:38:41 UTC, with no requested interactions.
- `unreal-item-use-independent-20260927/startup-only.json` is that fresh
  session's EndPlay export at 00:38:38.905 UTC: 170 decoded commands,
  residual 0, unsupported opcodes 0, protocol anomalies 0, uses requested 0,
  looks requested 0, synchronized viewport. This proves startup traffic
  only. `startup-session.txt` retains the matching bounded log lines.

The already existing character A inventory was read only to plan actions:
backpack containing a bag containing an apple, plus body armour and a weapon.
Character B had only a club. No character save or inventory fixture was
changed to manufacture a Use With success.

## Exact blocker

The computer-use skill was read with its Windows guidance and confirmation
rules. `@oai/sky` imported, but `sky.list_windows()` returned:

```text
Computer Use native pipe is unavailable: failed to connect native pipe:
El sistema no puede encontrar el archivo especificado. (os error 2)
```

The lightweight call was retried after two seconds, then the node_repl
kernel was reset, the package imported again and discovery retried. All
three window discovery attempts returned the same native-pipe error.
The skill requires stopping Windows app input after recovery fails and
forbids mixing direct PowerShell UI automation in the same turn. No click,
key injection, local success state or protocol bypass was used. A second
REAL33D process (PID 10140) was launched for recovery, but no input was sent;
at the final inspection it was still in startup. The UI-helper failure is
an execution-environment blocker, not observed evidence of a client regression.

## First automated attempt: routes and comparison

| Route | This independent attempt | Published state retained |
| --- | --- | --- |
| World Look, three distinct descriptions | NOT_STARTED - no clicks | PASS local from three previous live pairs |
| World Use | NOT_STARTED - no clicks | PASS from prior live corpse/container opening |
| Inventory Use | NOT_STARTED - no clicks | PASS from prior live backpack opening |
| Container Use | NOT_STARTED - no clicks | PASS from prior live nested-bag opening |
| Use With | NOT_STARTED - no clicks | PASS for prior flour-on-water-bucket example only |
| Inventory-slot Look | IMPLEMENTED_UNVERIFIED - no live click | IMPLEMENTED_UNVERIFIED |
| Container-slot Look | IMPLEMENTED_UNVERIFIED - no live click | IMPLEMENTED_UNVERIFIED |

No fresh description or gameplay effect was observed. No contradiction was
found in startup state, but the required interaction comparison is incomplete.
Zero startup protocol errors do not establish zero errors after interaction.

## Original blocked-attempt resumption

Restore the Windows computer-use native helper, select the returned REAL33D
window, and execute the published core route procedure in a fresh session.
Capture three distinct world Look request/server InfoMessage pairs, a valid
world Use result, body backpack opening, nested-container opening, and a
valid Use With result through normal client input. Attempt both slot Look
clicks, retain post-action F9 exports and authoritative server results.
Use only objects acquired or already available through normal gameplay;
do not alter character saves or rules. Only a completed live run can decide
whether the milestone moves to `CERTIFIED`. No main merge.

## Operator presentation correction during the attempt

The operator requested retaining the family V08 art and reported missing
inventory pictures. The original fresh certification launch did not enable
the V08 catalog. A subsequent launch explicitly enabled the existing local
4,913-entry catalog and WideWorld radius 64; it entered Game at
04:19:23.890 UTC and loaded 56 sectors. No V08 mesh or material was edited.

For inventory pictures, `FReal33DUIStyle::ItemBrush` reads
`<project>/Resources/UI/Items/<server TypeId>.png` and caches an absent file
for the session. That ignored directory was absent in the isolated worktree.
The principal checkout already held 5,259 extracted PNGs. They were copied
unchanged into the isolated worktree's ignored `Resources/UI/Items` directory;
counts matched and SHA-256 comparisons passed for backpack 2854, bag 2853,
armour 3561 and weapon 3272. No REAL33D2D file or source code was modified.
REAL33D was relaunched with V08/WideWorld enabled and a separate evidence
directory `build/cert-item-use-20260927/v08-icons`, because missing pictures
are cached for the lifetime of the preceding process. The operator confirmed
that the inventory pictures became visible. This dependency correction alone
does not constitute live item-use or Look certification.

## Fresh operator session, before mouse correction

Session: 2026-09-28 04:24:17 to 04:32:54 UTC, account A, Unreal PID 4192,
V08 4,913 entries and WideWorld radius 64. These are new manual operator
actions; the agent independently read their fresh requests and decoded Fusion32
responses. They are not automated agent clicks or reused published evidence.
The requested isolated backpack/F9 test did not yield an identifiable F9
snapshot; do not claim that isolated test passed. Closing the viewport did
produce `operator-pre-correction.json`, with 967 decoded commands, 49 Uses,
25 Looks, zero residual bytes, zero unsupported opcodes and zero anomalies.
The bounded matching log is `operator-pre-correction-actions.txt`.

Distinct world request/description pairs include grass, small fir tree, sign,
blueberry bush and cobbled pavement. Slot requests also received real backpack,
bag, jacket and rapier descriptions. Representative exact pairs:

- 04:27:38.434: Look 86, body slot 6 -> 04:27:38.521 InfoMessage rapier.
- 04:27:39.816: Look 90, body slot 3 -> 04:27:39.908 InfoMessage backpack.
- 04:27:41.333: Look 92, container 0 slot 0 -> 04:27:41.425 InfoMessage bag.

However, slot Look/Use frequently also produced a world request on button-up:
Look 92 was followed by world Look 93 and a framework-wall description;
container Uses 96/98 were followed by world Uses 97/99 and server refusals.
Source inspection found `EndOrbit` called world interaction on any right
release, without proving that the controller owned the press. A Slate-handled
slot press therefore could leak its release into world interaction. The left
world binding only selected V08 art; no left-click walk path existed. These
findings contradict clean input isolation and block certification of this
candidate as the operator expects it. Server descriptions are real, but they
do not prove correct gesture routing. No valid fresh Use With effect was
recorded; server refusals are not successful world Use effects.

## Narrow operator-requested correction

Allowed files: REAL33D mouse/controller/slot/world adapter, one portable mouse
test and status/evidence/handoff. No ClientCore or protocol definitions, Fusion32
gameplay rules/save fixtures, 2D/agent files, V08 meshes/materials or principal
checkout edits. Starting HEAD remains `35284595c44957600615f9cce6b78b9e9b761b86`.

- Left-click world release plans cardinal steps through current server-described
  tiles only. It reuses `RequestWalk` -> existing ClientCore `BuildWalkCommand`
  -> Fusion32 `CGoDirection` (`reference/game/src/receiving.cc`, unguarded
  function in the selected 7.72 build). It sends one step, waits for the
  authoritative accepted position, and stops on refusal, timeout, relocation,
  invalidated tile or manual movement. It never moves an Actor or writes state.
  Scope is the known same-floor live tiles, not the distant static map cache.
- Shift-left-click and the overlapping left+right gesture, in either press
  order, dispatch the existing Look request. Both individual release actions
  are suppressed for a chord. Alt-right remains a compatibility Look binding.
- Right-click Use waits for release, permitting a Look chord before sending
  Use. Slots consume both press and release; releases without an owned press
  do not become world Use. Shift-right on a slot still begins Use With; normal
  left click still selects its target, and slot drag/drop remains available.
- Ctrl-left selects the existing local V08 inspector. Right drag remains camera
  orbit. No artwork or gameplay state is changed.

Verification after stable edits:

- `tests/real33d_mouse_input_tests.cpp`: native C++17 /W4 `PASS` for left walk,
  Shift-left Look, both chord press orders and release orders, UI-owned release
  isolation, released-outside-slot recovery, Use With, orbit and reset.
  Compile with VS x64: `cl /nologo /EHsc /std:c++17 /W4
  /Iunreal\REAL33D\Source\REAL33D\Public tests\real33d_mouse_input_tests.cpp
  /Fobuild\real33d_mouse_input_tests.obj /Febuild\real33d_mouse_input_tests.exe`;
  run `build\real33d_mouse_input_tests.exe`. This is input testing, not live proof.
- Relevant native movement, player_state and worldview executables rerun: `PASS`.
  All eight suites had already passed at the published candidate; ClientCore
  was not edited by this repair.
- Unreal stable-source rebuild: `PASS`, 8 actions, `Result: Succeeded`, 33.90 s.
  A previous build ran during the last header edit and failed on the missing
  newly added helper; it is not accepted as the final build result.
- Explicit-worktree WSL `tests/secret_check.sh` rerun: `PASS`, six checks.

Fresh corrected session launched as PID 18128 with V08 and icons enabled,
evidence directory `build/cert-item-use-20260927/corrected-mouse`. Corrected
mouse routes remain `IMPLEMENTED_UNVERIFIED` until actual live clicks and
matching authoritative effects are retained. Tests/build do not upgrade them.
The complete independent milestone still needs successful world Use, inventory
Use, container Use and one valid Use With example in the corrected context.
Do not certify, commit certification or push until that gate is met.

## Mouse-only live run and chat presentation regression

PID 18128 closed normally at 04:47:44.933 UTC. The operator explicitly
confirmed left-click walking and Use, but reported that Look descriptions
were invisible in Local Chat and that sending chat did not work. This report
is decisive: decoded descriptions alone are not visible Look PASS.
Retained `mouse-only-endplay.json`: 512 commands, 38 walks requested,
37 accepted, 1 refused, 0 unanswered, 6 Uses, 15 Looks, 0 residual bytes,
0 unsupported opcodes, 0 protocol anomalies, synchronized viewport.
`mouse-only-actions.txt` and `mouse-only-movement.jsonl` retain matching traces.

Observed authoritative effects:

- Left click at 04:45:19.803 targets 32101,32212,7; inputs 27/28/29 correspond
  to journal accepted requests 13/14/15, from 32098,32212,7 through each
  eastward tile to the target. No client-side relocation is used.
- Body backpack Use 30 at 04:45:21.123 and nested bag Use 31 at 04:45:22.605
  are followed by server-owned open containers 0/1 in EndPlay, with the bag
  marked as having a parent. No extra world Use accompanies those clicks.
- World Use 55 at 04:47:27.262 identifies object 435, tile 32097,32205,7,
  stack 2; at 04:47:29.182 Fusion32 moves the player to the same x/y, floor 8,
  with no walk request outstanding. World Use 56 on object 1948 at
  04:47:35.650 is followed by the authoritative return to 32097,32206,7.
  These are server-owned Use relocations, not accepted left-click steps.
- World Looks 8/10/17 produce blueberry bush, fir tree and small fir tree
  descriptions. Body Look 32 produces a backpack description. All are
  retained in the server transcript but were not visible to the operator.
  Their presentation therefore remains FAILED for this run.

Exact root cause of the newly exposed presentation regression:
`SReal33DHUD::MakeBottomPanel` constructed `SReal33DChatPanel` with only
`Embedded(true)`, omitting both the existing Bridge and OnTypingChanged
arguments. The panel's transcript Tick returned immediately on null Bridge;
Send could not route to the server, and typing did not notify the controller.
No protocol parser defect was found. Narrow repair in Real33DHUD.cpp/.h:
forward those two existing arguments. No popup system, fabricated description,
optimistic speech or new protocol path was added.

HUD correction Unreal rebuild: `PASS`, 5 actions, 29.94 seconds,
`Result: Succeeded`. Fresh PID 2356 launched with V08/radius 64/icons and
evidence in `build/cert-item-use-20260927/corrected-chat`. Operator was asked
to perform Shift-left Look and a normal Say line, then export/close. Visual
Look/chat validation and one valid independent Use With remain open. The
published certification candidate is still unchanged on origin and the
uncommitted corrected worktree is not CERTIFIED.

## Visible Look/chat confirmation and requested presentation assets

Corrected-chat PID 2356 ended normally at 04:51:57.488 UTC. Fresh exports:
87 commands, 6 Looks, 2 Uses, 1 Say; zero unsupported opcodes, residual bytes
or protocol anomalies. Operator confirmed "funciona bien" after the visible
Look/chat check. Distinct world Look request/response pairs: sewer grate
04:51:21.803/21.880, pavement 04:51:28.227/28.326, street lamp
04:51:30.571/30.666. Say 9 at 04:51:44.862 received Fusion32 Talk[Say] for
TestPlayerA at 04:51:44.974. This supersedes invisible Look for these world
examples; it does not certify slot Look after the HUD repair or Use With.

Operator requested the existing target cursor and brother's monsters/NPC art.
Original 23x23 targetcursor.png copied unchanged from 3DTIBIA client assets;
9,9 hotspot retained. SHA256:
`3dab6378d5df27bb4a0c4f907b5d1127d712ef9d79c4db678e62736265680936`.
Controller/Slate use it only while normal Use With target selection is pending.
Cursor build PASS (5 actions, 44.25 s); live cursor check pending.

Resolved 3DTIBIA carril/ARTE-CRIATURAS at
`37a758df0fab94080ea7b8128db5321f5225618c`. All 16 GLBs in delivered
3DTIBIA_para_hermano/arte/criaturas match that revision's Git blob IDs.
visual/qa/brother_creatures/source.json pins paths, Git blobs and SHA256.
Initial import FAILED due to an incorrect one-mesh assumption. Corrected
importer preserves every skinned part and verifies a shared skeleton with
idle/caminar/atacar/morir clips. Second commandlet PASS 16/16, 0 errors,
0 warnings. Separate ignored generated content: Experimental/BrotherCreatures.
Creature adapter build PASS, 22 actions, 196.81 s. No original art/V08,
Fusion32 rule/save or protocol definition edited. Import/build are not live
art certification. Only actual decoded server outfits map to models; no
name-based substitution or catalog-spawned creatures. Idle/walk follow
server-confirmed placement; attack/death clips have no invented triggers.

Coverage clarification requested by operator: monsters3DISH/assets/indice.json
contains 156 outfit sprite references and refs contains 155 outfit folders.
That source has 3 .blend files and no .glb files; the 16 delivered GLBs are
not the complete outfit catalog. Outfit references include character
appearances; do not label all 156 as monsters or completed 3D models.
Available delivered models: 5,15,16,21,25,26,27,28,30,31,33,36,45,50,56,128.
Unknown outfits retain the current placeholder. No separate finished NPC
catalog found; Rookie 128 applies only to actual server outfit 128.

Fresh creature/cursor session PID 18128 (new process, distinct from the earlier
mouse-run PID reuse): startup 05:24 UTC, server-presented creatures at
05:25:45.343 and 05:25:56.282 UTC. Test Player A/1001 and Seymour/1073742035
both report outfit 128 and load all eight original parts (height 104 uu).
Dixi/1073742106 reports outfit 136 and explicitly retains a placeholder.
Catalog enabled 16 exact mappings; original target cursor loaded. Operator
visual confirmation and pending live actions are not yet received.

An earlier launcher argument concatenation inserted a space after
-real33d-evidence=, causing Unreal to try that path as a map. This startup is
FAILED/excluded from certification; full log preserved as
build/cert-item-use-20260927/creatures-cursor/launch-argument-error.log.
Operator closed it; corrected fresh startup has the exact joined argument.
Do not confuse this launch error with a protocol anomaly or successful test.

After creature changes, explicit-worktree WSL secret_check PASS, all six
checks. Remote verification remains milestone 35284595c44957600615f9cce6b78b9e9b761b86
and main bd15cc0a49d8182dc1cc3732b8487859f1662044. No commit/push/merge.

## Closed creature/cursor run and automatic Use targeting correction

Operator confirmed the interaction check "si funciona todo" and the cursor/
valid Use With check "si esta bien". Art defects were reported, then accepted
"esta bien por ahora"; preserve the original delivery, do not claim polished
art certification. Closed normally at 05:31:41 UTC. Fresh EndPlay retained in
creatures-cursor-endplay.json: 614 frames, 798 commands, 24 Uses, 6 Looks,
0 residual bytes/unsupported opcodes/protocol anomalies, synchronized viewport,
0 duplicate spawns/orphan events. Movement separately records 67 accepted,
6 refused and 7 unanswered requests; these are not relabelled as successful
walks or hidden by the zero protocol error result.

Live slot Look: backpack request at 05:27:24.437 received authoritative
description; container 0 slot 0 requests 73/74/75 at 05:28:20.115,
05:28:21.064 and 05:28:21.734 received bag descriptions at 20.165,
21.103 and 21.834. Operator confirmed they worked visibly. These are live
clicks in the restored HUD, not byte tests.

Valid live Use With includes rapier 3272 against table 2319 at 32106,32200,6
(requests 100/101/105) and drawers 2434 at 32103,32200,6 (request 110,
05:29:23.580). Frame 153 immediately loads server-presented visual 3136,
05:29:23.779/786. Public runtime objects.srv declares drawers DestroyTarget
3136, wooden trash. Fusion32 operate.cc::Use dispatches a close weapon on a
DESTROY target to moveuse.cc::UseWeapon, which sends EFFECT_POFF and may
Change to DestroyTarget. The transformation correlation follows that code
and the incoming-state-driven render path; no local Use code changes items.
Exact action/render lines retained in creatures-cursor-actions.txt. Refused
earlier attempts on object 2520 remain refusals, never successful Use With.

Operator then requested that normal Use initiate targeting automatically for
appropriate items, including runes, without Shift. Inspected authoritative
receiving.cc::CUseObject (384/403) and CUseTwoObjects (430/454): normal use
refuses MULTIUSE, targeted use requires it. These handlers have no local
version guard; the selected runtime is the established 772 baseline.
No protocol definitions/ClientCore/Fusion32 files changed. A separate Unreal
UI metadata reader extracts exact MultiUse flags from the same validated
objects.srv already loaded by the worker. Mutex-protected semantic getter
drives normal slot/world Use into the existing pending-target UI. Pending
source preserves the existing full map/body/container endpoint and stack.
Nothing is sent until a target is selected; commands still use existing
ClientCore builders. Non-MultiUse containers retain normal Use.

Portable metadata tests PASS, including rejection of name/substr/invalid-ID
false positives. Same-runtime table test PASS: 204 MultiUse types, rapier
classified, backpack/bag not classified. First sandboxed runtime read could
not open UNC path; rerun with allowed access passed. Automatic normal-Use
entry and rune effects remain IMPLEMENTED_UNVERIFIED live pending rebuild
and fresh normal clicks. No previous live result certifies this new entry.

Normal-Use correction final build PASS, 4 actions, 15.81 s. First attempt FAILED
on missing owning log-category header; added REAL33D.h and rebuilt successfully.
Expanded same-runtime metadata test PASS: spell runes 3148/3149 require a target,
blank rune 3147 does not. No rune was placed in inventory to fake a test.
Fresh normal-use client PID 8288 launched with unchanged V08/icons/original
creature/cursor assets, evidence build/cert-item-use-20260927/normal-use.

## Final independent decision - 2026-09-27 23:49 America/Guatemala

UNREAL-ITEM-USE-INTERACTION-001: CERTIFIED for this corrected branch revision.
The published candidate exposed real input/chat regressions and is not
retroactively called certified. Repairs and presentation requests were
explicitly directed by the operator. Fresh manual clicks were independently
matched to requests, Fusion32 responses and closed-session exports. Native
UI automation was unavailable; no alternate input injection, raw packets,
fabricated local state or server fixtures were substituted.

Final normal-Use PID 8288: operator confirmed "it works"; Manual/EndPlay
retained. EndPlay 05:43:36 UTC: 110 frames, 302 commands, 1 Use, 1 Look,
residual/unsupported/anomalies all 0, viewport synchronized, diagnostic none.
Normal Use on rapier began targeting at 05:42:56.612. Use With 7 at
05:42:57.609 targeted trough 2524 at 32103,32203,6 stack 1. Frame 280 loaded
the incoming-state-derived 3135 visual at 05:42:57.797/810. Public objects.srv
defines that trough as DESTROY with DestroyTarget=3135, matching UseWeapon's
authoritative transformation. Operator confirmed the visible flow. No simple
Use packet on rapier preceded selection. Jacket Look 22 at 05:43:19.326 received
its description at 05:43:19.413. Later "Sorry, not possible" messages belong
to refused walks, not that successful Use With; they remain refusals.

| Certified route | Independent live evidence |
|---|---|
| WORLD_LOOK | Sewer grate, pavement and street lamp request/response pairs, visible confirmation in restored chat |
| WORLD_USE | Mouse-only Use 55/56 produced authoritative floor 7->8->7 relocations |
| INVENTORY_USE | Mouse-only backpack Use 30 opened server container 0 |
| CONTAINER_USE | Nested bag Use 31 opened server container 1 with parent |
| USE_WITH | Rapier -> drawers 2434/3136, then final normal-Use rapier -> trough 2524/3135 |
| INVENTORY_SLOT_LOOK | Fresh backpack description and final jacket Look 22, visible confirmation |
| CONTAINER_SLOT_LOOK | Fresh bag Looks 73/74/75 at container 0 slot 0, visible confirmation |
| AUTHORITATIVE_RESULT | Existing client builders -> 772 server -> decoded WorldState/messages; no local success mutation |

Certification uses these independently recorded fresh sessions; the source
correction applicable to each is identified above. Published prior evidence
and byte tests are not substitutes for live evidence. Final delta selects the
existing target route using exact MultiUse metadata; non-MultiUse backpack/bag
retain the identical normal Use route. Automatic world MultiUse source entry
and actual rune spell effects are not separately live-certified. One valid
targeted example is the contract's scope, not every item/target combination.

Final relevant movement/player_state/worldview tests PASS, retained in
final-clientcore-tests.txt. Initial clean candidate also passed all eight
suites. Native mouse/policy tests PASS; rune classification checked against
the public table. Final Unreal build PASS, 4 actions/15.81 s. Explicit-worktree
secret_check PASS, six checks. All retained independent session exports have
zero residual/unsupported/anomaly counts. Art defects/unmatched outfits and
untested spells remain limited; no evidence contradiction or main merge.

Reproduce with the build commands above and established 772 runtime, without
editing saves/rules. Launch normal REAL33D with existing V08/icons and optional
pinned creature catalog. Perform left walking, Shift-left or left+right Look,
right backpack/nested bag Use, normal right rapier then a nearby DESTROY target,
and normal Say. Press F9 or close normally; compare requests/descriptions,
server effects and all zero protocol counters. Asset provenance/preconditions
are above and in visual/qa/brother_creatures/README.md. Rune spell testing must
obtain a rune through normal gameplay; none was manufactured for this run.
