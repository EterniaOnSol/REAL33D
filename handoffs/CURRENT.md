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
