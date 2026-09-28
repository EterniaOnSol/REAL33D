# HANDOFF

Date/time: 2026-09-27 18:14, America/Guatemala
Task: `UNREAL-ITEM-USE-INTERACTION-001`
Agent / role: Codex, 3D client audit, implementation and local QA
Branch: `milestone/unreal-item-use-interaction-001` (published at closeout;
not merged to main)
Starting commit: `bd15cc0a49d8182dc1cc3732b8487859f1662044`
Ending commit: this handoff commit; resolve with `git rev-parse HEAD`
Worktree: `C:\Users\dell\Desktop\fusion32\build\unreal-item-use-interaction-001`
Principal checkout: `C:\Users\dell\Desktop\fusion32`
Remote main: `bd15cc0a49d8182dc1cc3732b8487859f1662044`

## Objective and state

Complete Tibia-style item Use interaction in REAL33D through the existing
ClientCore/Fusion32 7.72 flow, and audit what had already been implemented.
The new Look route is implemented and deterministic/native build checks pass.
`UNREAL-ITEM-USE-INTERACTION-001 = PASS`, not `CERTIFIED`, for the named live
Use and world Look paths. Three world Look requests and Fusion32 descriptions
were retained together. Independent repetition and live slot Look validation
remain open.
Prior certified world/body/container Use and a valid Use With result remain
certified by their separate milestone evidence.

The operator also requested seeing more map with a family member's V08 art.
The corrected live run enabled the 4,913-entry local V08 catalog and
WideWorld radius 64 with 69 sector-load records. The launch script now
requires a sector cache and passes the paths/radius explicitly. The art and
cache remain local, ignored QA material. A further operator request selected
local presentation-only in-world editing as the next 3D milestone.

## Startup and isolation

Read AGENTS.md, PROJECT_STATUS.md, ARCHITECTURE.md, ROADMAP.md,
PARITY_MATRIX.md, handoffs/CURRENT.md and SOURCE_TRUTH.md. Inspected
`git status`, branch, HEAD, remotes and remote main. The principal checkout
was on `milestone/real33d-agent-veteran-play-002` at `77e35b4` with
pre-existing uncommitted agent/2D changes. Those files were never touched.
Created this clean isolated worktree from main under the principal repo's
ignored `build/` directory. No REAL33D2D, agent, server, runtime data or
reference source was edited. No push or main merge.

## Authority and discoveries

- `reference/game/src/connections.hh::ClientCommand` defines Use 130/131/132
  and Look 140.
- `receiving.cc::CUseObject/CUseTwoObjects/CUseOnCreature` supplied all Use
  shapes already present in ClientCore/Unreal. `UNREAL-INVENTORY-CONTAINERS-001`
  and `UNREAL-COMBAT-FOLLOW-001` retain their live Use evidence.
- `receiving.cc::CLookAtPoint` reads x/y/z only, checks allowed coordinates
  and visibility, then calls `info.cc::GetObject`. `operate.cc::Look` sends
  the description via `SV_CMD_MESSAGE`. No TypeId, stack or description
  belongs in the Look request.
- Existing REAL33D input/HUD/bridge already presents server container and
  message outcomes. No new WorldState or Actor mutation was needed.
- `Real33DWorldActor.cpp::InitialiseWideWorld` enables streaming only if a
  cache directory exists. This isolated worktree initially lacked both local
  V08 imports and that cache, so its first live launch showed placeholders.

## Changes and files

- ClientCore `player_state.h`, `movement.h/.cpp` and
  `player_state_tests.cpp`: six-byte Look builder and exact map/body/container
  vectors.
- REAL33D `Real33DBridge.h/.cpp`, `Real33DHUD.h/.cpp`,
  `Real33DPanelChrome.h/.cpp` and `Real33DPlayerController.cpp`: semantic
  Look intent, worker send, Alt+right-click on world/creature/occupied slot.
  Existing server message presentation owns the response.
- `scripts/client/run_unreal_v08_experimental.cmd`: explicit V08 catalog,
  WideWorld cache/previews and radius arguments; refuses absent cache/import.
- `.gitignore`, `PROJECT_STATUS.md`, `PARITY_MATRIX.md`,
  `evidence/clientcore/UNREAL-ITEM-USE-INTERACTION-001.md` and this handoff.
- Archived the preceding handoff at
  `handoffs/archive/2026-09-23_UNREAL-COMBAT-FOLLOW-001.md`.

## Tests, live result and evidence

`tests/build_clientcore_windows.cmd` under VS 2022 BuildTools:
C++17/C++20 libraries PASS; eight suites PASS. `REAL33DEditor Win64
Development`: `Result: Succeeded`. Exact byte assertions cover Look's
map/body/container addresses. See the evidence report for the commands and
server source chain.

Local Fusion32 services started and identity/ports verified after removing
one verified stale Game PID lock. Game recorded account B entering through
ordinary Login/Game flow. Corrected REAL33D log: V08 catalog 4,913 entries,
WideWorld radius 64, 69 sector-load lines. A local ignored crop at
`build/v08-wideworld-3d.png` visibly shows the wider street/buildings with
the V08 meshes. F9 at 23:37:59 UTC: 532 decoded commands, residual 0,
unsupported 0, anomalies 0, tile/creature actor counts 410/3 equal
WorldState, viewport synchronised. The local snapshot is ignored at
`evidence/clientcore/unreal-item-use-interaction/unreal_slice_evidence_Manual.json`.
That first F9 snapshot contains no Look request/reply and its protocol
counters do not cover Look. A second live run retained three world Look pairs in
`unreal/REAL33D/Saved/Logs/REAL33D.log`:

- 23:59:47.177 `look 1` at (32093,32207,7); 23:59:47.308 server InfoMessage
  `You see a mountain.`
- 00:01:21.985 `look 42` at (32100,32199,7); 00:01:22.027 server InfoMessage
  `You see a framework wall.`
- 00:02:06.278 `look 55` at (32094,32203,7); 00:02:06.354 server InfoMessage
  `You see grass.`

The live HUD showed zero unsupported opcodes and anomalies after these Look
requests; local ignored `build/look-qa3.png` shows the V08/WideWorld window
and counters. No post-Look F9 export was retained. The Unreal process closed
cleanly at 00:02:35 UTC.

## PASS, remaining work and exact next task

`PASS` from prior certified milestones: world Use/open corpse, body Use/open
backpack, nested-container Use/open bag, valid flour-on-bucket Use With and
server-authored inventory/container changes. This branch's new world Look:
`PASS` with three server descriptions. Body/container Look:
`IMPLEMENTED_UNVERIFIED` for live clicks despite exact-byte tests and built
UI path. Use On Creature and Use With on a field have deterministic command
coverage but no separately retained valid live result. Overall milestone is
local `PASS`, not `CERTIFIED` by an independent reviewer.
V08/WideWorld visual load in this branch: live observed, with generated
art/cache local only. Artistic approval of V08 remains unverified.

For stronger certification in this branch, repeat Alt+right-click from the
REAL33D window on an occupied body and container slot; retain
`LogReal33D: look <id>`, the matching server InfoMessage, and a post-Look F9
snapshot with zero protocol counters. Exact functions:
`SReal33DHUD::LookAtWorldPoint`,
`SReal33DHUD::HandleSlotUsed`,
`AReal33DPlayerController::InteractUnderCursor`,
`FReal33DWorker::DrainUses`,
`p772::BuildLookAtPointCommand`. Do not certify from compilation or from
an input attempt with no observed request.

Next separate 3D milestone, selected by the operator:
`UNREAL-LIVE-VISUAL-EDITOR-001` (`NOT_STARTED`). Scope: select a visible
world instance while navigating; preview and persist reversible local
mesh replacement, orientation and grass presentation; keep tile identity,
stack order, collision and all Fusion32 server state unchanged. Audit
`Real33DWorldActor`, `Real33DStaticSectorActor`,
`Real33DTileActor`, `Real33DAssetRegistry` and the current V08 inspector
before designing overrides. No server/source/runtime or 2D/agent edits.
The current V08 inspector's `Girar 90` button merely appends a `ROTATE_90`
verdict to `wall_inspector_notes.tsv`; it does not change the visual. This is
the natural entry point for an actual local visual editor.

## Closeout - 2026-09-27 18:14 America/Guatemala

At start of closeout, branch and HEAD were
`milestone/unreal-item-use-interaction-001` / `d351e9a9139fde0168d65f68d2731ceb4f05bfb1`,
worktree clean. `origin/main` was still
`bd15cc0a49d8182dc1cc3732b8487859f1662044`; the milestone branch was
not yet on origin. The operator authorized publishing this milestone branch,
and no main merge.

Re-ran `tests/build_clientcore_windows.cmd` under VS 2022 BuildTools x64:
C++17/C++20 archive builds and all eight suites `PASS`. Rebuilt
`REAL33DEditor Win64 Development` after that archive: `Result: Succeeded`
(two incremental actions). Rechecked the three world Look/server InfoMessage
pairs in the ignored Unreal log and the earlier F9 JSON as separate sessions;
the F9 zero protocol counters precede Look, while the live HUD image shows
zero unsupported opcodes and anomalies after Look. Rechecked the previous
certified world/body/container Use and flour-on-bucket Use With reports.
`tests/secret_check.sh` passed all checks in WSL with this Windows worktree's
`GIT_DIR` and `GIT_WORK_TREE` explicitly set. A bare WSL call gave a false
PASS with Git errors and is not accepted as evidence.

Certification remains exactly `PASS` for overall item-use interaction, world
Use, inventory Use, container Use, one valid object-object Use With, and world
Look. Inventory-slot Look and container-slot Look remain
`IMPLEMENTED_UNVERIFIED` for live clicks. Creature/field Use With variants
have no separately retained valid live result. No behavior was promoted by
build success. The next 3D task remains the local visual editor described
above, after this closeout.

Principal checkout's agent/2D changes are pre-existing and remain uncommitted.
Only this milestone branch is pushed. Do not merge main without certification.

## Independent certification attempt - 2026-09-27 22:13 America/Guatemala

Candidate local/remote HEAD verified as
`35284595c44957600615f9cce6b78b9e9b761b86`, clean worktree;
`origin/main` remained `bd15cc0a49d8182dc1cc3732b8487859f1662044`.
Rebuilt native ClientCore (C++17/C++20, eight suites PASS), incremental
Unreal (Result: Succeeded), and ran secret_check successfully with explicit
WSL Git worktree variables. Restarted Fusion32 cleanly after verifying no
active game connections; services identity and ports passed. Fresh account A
REAL33D session entered Game, then closed cleanly without any Use or Look.
Its retained EndPlay export has 170 commands and zero protocol counters,
but zero requested interactions, so it is startup evidence only.

Independent run is `BLOCKED`: computer-use native pipe was unavailable on
initial discovery, retry and reset/retry. The skill requires stopping input
after failed recovery. No live click was executed, no protocol bypass or
local success state was created, and no implementation patch was made.
Milestone remains `PASS`, not `CERTIFIED`; both slot Look routes remain
`IMPLEMENTED_UNVERIFIED`. New failure evidence:
`evidence/clientcore/UNREAL-ITEM-USE-INTERACTION-001-INDEPENDENT-20260927.md`
and `evidence/clientcore/unreal-item-use-independent-20260927/`.
Second REAL33D process PID 10140 was launched for recovery and received no
input; check whether it is still open before any further session.
This blocked-attempt evidence is retained in the worktree without pushing or
merging; it has not yet been committed.
Resume only the certification after restoring the native Windows UI helper;
do not start another milestone or treat prior interaction evidence as the
independent run.

Operator correction after the blocked attempt: launch with the existing
family V08 art, not the plain certification launch. V08 catalog 4,913 and
WideWorld radius 64 were enabled; the session entered Game and loaded 56
sectors. The operator then identified missing inventory icons. The isolated
worktree lacked ignored `Resources/UI/Items`; 5,259 existing PNGs were copied
unchanged from the principal checkout to that ignored local directory, with
matching sample SHA-256 hashes for 2854/2853/3561/3272. No source, V08 art,
REAL33D2D, agent or server gameplay change. Relaunched as account A with
V08/WideWorld and evidence in `build/cert-item-use-20260927/v08-icons`.
Inventory visual confirmation and all required fresh interaction clicks
remain pending; restoring files is not gameplay certification.

## Operator mouse correction - 2026-09-27 22:43 America/Guatemala

This section supersedes the pending visual confirmation and resume-only-UI-helper
directions above. Operator confirmed inventory pictures visible, performed fresh
manual Look/Use clicks, then reported right-click movement through Use and asked
for left-click walking and Look via Shift-left or left+right. Task remains
UNREAL-ITEM-USE-INTERACTION-001, `IN_PROGRESS`, not `CERTIFIED`; no new milestone.
Branch/local/remote candidate remain `milestone/unreal-item-use-interaction-001`
/ `35284595c44957600615f9cce6b78b9e9b761b86`; origin/main remains
`bd15cc0a49d8182dc1cc3732b8487859f1662044`. Worktree has uncommitted corrections
and evidence; no push or merge of the repair.

The V08/icons session PID 4192 ended normally at 04:32:54 UTC. Retained fresh
EndPlay JSON has 967 commands, 49 Uses, 25 Looks, zero residual/unsupported/
anomalies. New world and slot descriptions are real Fusion32 responses.
Slot clicks also leaked right releases into world Use/Look, so this session
does not certify input isolation. The requested isolated F9 test was not found;
do not claim it passed. Exact evidence and pairs are in the independent report.

Narrow permitted repair: reusable mouse gesture ownership, Shift-left/chord
Look, deferred right Use, consumed slot release, Ctrl-left V08 identification,
left-click cardinal path through currently authoritative tiles with one step
outstanding and server-confirmed progress. No protocol/ClientCore, Fusion32
rule/save, 2D/agent, V08 art or principal checkout edits. WORLD Use may cause
Fusion32's legitimate approach walk; it is no longer the client's walk control.
Far static scenery and cross-floor navigation are not click-path sources.

Tests: native mouse gesture C++17 /W4 PASS; movement/player_state/worldview
executables PASS; stable Unreal build PASS (8 actions, 33.90 s); explicit-Git
WSL secret_check PASS. Earlier build raced a header edit and failed; superseded
by the stable successful rebuild. Changes are IMPLEMENTED_UNVERIFIED live.

Fresh corrected REAL33D PID 18128 opened with V08/64 radius/icons/account A,
evidence `build/cert-item-use-20260927/corrected-mouse`. Operator was asked to
try left walk, Shift-left world Look, chord backpack Look, right backpack Use,
then F9 or normal close. Native automation pipe still unavailable; do not inject
input through alternate shell mechanisms. Inspect fresh log/request/description
pairs and final JSON. Preserve each session before relaunch. Finish core live
world/inventory/container Use and a valid Use With, then decide certification.
No artificial fixtures. Do not promote unit tests or server refusals to live
PASS. Commit/push certification only after success; leave main unchanged.

### Follow-up: invisible Look and broken chat - 22:49 America/Guatemala

Operator confirmed walking/Use work, but Look descriptions were invisible and
Local Chat could not send. PID 18128 closed normally; retained mouse-only
EndPlay has 512 commands, 37 accepted/1 refused walk, 6 Uses, 15 Looks,
zero protocol errors, two real open containers. World Use 55/object 435
caused Fusion32 floor 7 -> 8; Use 56/object 1948 returned to floor 7.
Exact timestamps and movement records are in the independent report.
Descriptions arrived but their invisible presentation FAILED this session;
do not certify Look based on the JSON transcript.

Found a real HUD wiring regression: MakeBottomPanel constructed ChatPanel
without Bridge or OnTypingChanged. Forwarded these existing arguments in
Real33DHUD.cpp/.h. No new chat protocol or popup feature. This also restores
movement gating while typing. HUD rebuild PASS, 5 actions, 29.94 seconds.
Fresh corrected-chat PID 2356 launched with unchanged V08/icons; operator
asked for Shift-left Look and normal Say, then F9/normal close. Inspect
`build/cert-item-use-20260927/corrected-chat` and the current Unreal log.
Remain IN_PROGRESS / not CERTIFIED until visible Look and core independent
Use With are reproduced. All repairs/evidence remain uncommitted; remote
candidate/main unchanged. Never patch server saves or invent a Use With result.

### Follow-up: visible Look and original creature assets - 23:35

PID 2356 closed normally, operator confirmed "funciona bien". Three distinct
world Looks and normal Say echo are freshly authoritative/visible; 87 commands,
6 Looks, 2 Uses, 1 Say, zero protocol errors. Exact pairs/exports in independent
report. Slot Look after HUD repair and valid independent Use With remain open.
Milestone IN_PROGRESS, not CERTIFIED; repairs/evidence uncommitted, no push.

Explicit operator request: existing Use With cursor and brother's monsters/NPC
art. Original targetcursor.png copied unchanged, hotspot 9,9. Cursor build PASS,
live cursor check pending. Located actual 16 GLBs in 3DTIBIA ARTE-CRIATURAS
revision 37a758df0fab94080ea7b8128db5321f5225618c; each delivered file matches
remote Git blob. Source/runtime manifests: visual/qa/brother_creatures.
Importer visual/tools/import_brother_creatures_unreal.py verifies source SHA,
all multipart meshes and four clips sharing one skeleton. Initial one-mesh
check failed; corrected import PASS 16/16, 0 errors/warnings. Adapter build PASS,
22 actions/196.81 s. Imported content ignored/separate from V08. Existing
decoded server outfit IDs drive presentation, no invented identities/state.

Operator expected 140+ monsters. Located 156 outfit sprite references and 155
reference folders in Desktop/monsters3DISH, 3 .blend files, no .glb. These are
not 156 completed 3D monsters. Delivered models cover outfits
5,15,16,21,25,26,27,28,30,31,33,36,45,50,56,128 only. No separate finished NPC
catalog found. Do not replace unmatched outfits with arbitrary models.

Next: fresh V08/icons session with full path -real33d-creature-catalog to
visual/qa/brother_creatures/runtime.json; inspect model/cursor startup logs and
operator visibility. Finish legitimate Use With and slot Looks. Preserve all
session evidence, final secret check/main verification, decide certification.
Native computer-use pipe remains unavailable: normal operator clicks only.

Fresh art/cursor PID 18128 (PID reuse, not the earlier mouse-only session)
connected 05:25 UTC. Logs: local Test Player A and Seymour actual outfit 128,
eight imported parts each/height 104 uu; Dixi outfit 136 explicitly placeholder.
16 mappings and original target cursor loaded. Visual/operator live actions
requested, pending. Earlier launcher passed an evidence path as a map due to
argument spacing; FAILED startup log retained, excluded from certification.
Operator closed it and corrected command relaunched. Final secret_check PASS
six checks. Remote milestone/main still match published expected hashes.

### Follow-up: slots/Use With reproduced; normal Use targeting requested

Fresh art/cursor session closed 05:31:41 UTC. Operator confirmed all interactions
and cursor/effect; original art "esta bien por ahora" despite defects. Preserve
original assets. EndPlay: 614 frames/798 commands/24 Uses/6 Looks, zero protocol
errors, viewport synchronized, zero duplicate/orphan events. Seven unanswered
and six refused walks are separately retained, not successful walks. Live slot
Look descriptions now confirmed in restored chat. Valid Use With rapier on
table/drawers; request 110 at 05:29:23.580 targets drawers 2434, then incoming
render loads 3136 wooden trash, matching public DestroyTarget and UseWeapon.
Exact evidence and source correlation in report; refusals on 2520 not PASS.

Operator correction: normal Use should start Use With for eligible items,
including runes. Implemented Unreal-only exact MultiUse metadata read from the
same validated runtime objects.srv; no protocol/ClientCore/server rule changes.
HUD selects the existing target UI from normal slot/world Use. Full pending
map/body/container endpoint preserved. Target selection uses normal existing
builders. Metadata tests PASS (204 types; rapier yes, backpack/bag no). Live
normal Use/rune effects IMPLEMENTED_UNVERIFIED. Unreal build in progress;
first attempt missing LogReal33D declaration, must add owning REAL33D.h after
build ends, then rebuild. Do not edit while build is active. No client running.
No certification commit/push/merge; remote hashes unchanged.

Missing log header corrected after build ended. Final normal-Use build PASS,
4 actions/15.81 s. Runtime metadata tests additionally prove spell runes
3148/3149 targetable and blank rune 3147 not MultiUse. Actual rune effects are
not live-tested. Fresh client PID 8288 launched; evidence normal-use directory.
Need right-click rapier without Shift -> cursor -> valid target, regular
backpack Use/Look sanity, then F9/normal close and final evidence/protocol check.
