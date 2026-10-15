# Current handoff

2026-10-15 preservation update: operator explicitly requested committing/pushing
the saved QA evidence before any code change. This commit is that honest
IMPLEMENTED_UNVERIFIED checkpoint, starting at6f864e3; no implementation change.
Prior uncommitted statements below describe the previous closeout. Next scope is
now authorized: trace/classify inventory-to-ground routing, minimal bug fix if
appropriate, normal live creature push, and compact post-fix regression. No
unrelated features, rule changes, independent certification or main merge.
Ending preservation commit is the commit containing this entry; verify normal
push and local/remote equality before editing code. Retain all documented gaps.

2026-10-15 03:38 America/Guatemala. Codex, operator-assisted local QA.
Task UNREAL-WORLD-PRESENTATION-POLISH-001 remains IMPLEMENTED_UNVERIFIED.
No independent certification, new milestone, feature/refactor or main merge.

Branch milestone/unreal-world-presentation-polish-001.
Base/main fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Starting/ending code HEAD 6f864e3f9e9fbb82fccdf4c2ae1e4287c4360b52.
Remote milestone equals HEAD; main unchanged, verified03:38 local.
Worktree C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
The root fusion32 worktree is a DIFFERENT branch; do not edit it for this task.
QA evidence/docs are uncommitted. No push because local acceptance remains unmet.
Prior substantive handoff archived at
archive/UNREAL-WORLD-PRESENTATION-POLISH-001-PRE-CLOSEOUT-20261015T0338.md.

Operator controlled every click/key. Agent inspected F9 PNG/JSON, logs, source and
server identity. Desktop control previously failed; continue operator-assisted.
Current camera keeps manual zoom, does not retract or hide walls by proximity.
Accepted floor/roof restoration and water/geometry/label improvements preserved.

PASS in this assisted run: CAMERA_COLLISION, WALL_OCCLUSION, LOOK_SMOKE,
USE_WITH_SMOKE, USE_WITH_FEEDBACK, DRAG_OBJECT (world-to-world trough2524),
LIGHT_TOGGLE (2108->2109->2108), MINIMAP_CLICK_SMOKE, MINIMAP_COMPATIBILITY,
FOLLOW_SMOKE, STOP_FOLLOW_SMOKE, CONTAINER_SMOKE, WIDEWORLD_SMOKE.
Prior Use/build/ClientCore and presentation passes retained; no code change here.
Legitimate bucket2873 fill from well1930 and spill prove Use With. Operator saw
cursor/notice appear and clear. Shovel593->594 received; rope on435 retrieved a
rat to the surface (not a left-drag push). Local Say and natural Norma response
visible in-world and chat; later image proves speech expired and chat retained.

Remaining: inventory->world drop3031 FAILED (slot drag emits no move; no world
receiver), PUSH_CREATURE IMPLEMENTED_UNVERIFIED (safe NPC attempts rejected;
operator reports monsters movable but no attributable retained success), and
GAMEPLAY_REGRESSION IMPLEMENTED_UNVERIFIED. Do not equate zero protocol counters
or individually passing actions with complete regression acceptance. The compact
final connection has no Look or UseWith request; those passed preceding sessions.
Ledger14 unanswered final-session walks remain for source/event reconciliation;
refused49 includes SNAPBACK without pending walk during blocked NPC Follow.
No protocol anomaly inferred from that counter, and no counter silently zeroed.

Evidence report:
evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-ASSISTED-20261014.md
Artifact folder:
evidence/clientcore/unreal-world-presentation-polish-assisted-20261014/
Contains four EndPlay records, camera/occlusion/floor/Look/lamp/trough/bucket,
container/speech/Norma/minimap/Follow/Stop PNG/JSON, selected logs and movement
journal. Original continuation/resume evidence remains untouched.
Session4 EndPlay09:37:46.799Z:1284commands,residual0,unsupported0,anomalies0,
viewport synchronized; player32097,32216,7, no target/follow, four coins retained.
REAL33D20736 closed normally by operator. Fusion32 services last verified
querymanager584/game599/login867, ports7173/7172/7171. Reverify before reuse.
Current session folder build/unreal-world-presentation-polish-001/live-20261015T031623.

User explicitly authorized earlier offline weapon/backpack/shovel/rope prep.
Already applied once with all services stopped, backup1002.usr.bak-before-operator-
qa-equipment-20261015 in /var/lib/fusion32-server-baseline-772-0/game/state/usr/02.
Never repeat that script. Backpack2854 slot3 contains3457/3003; sword3294 slot5;
jacket3379 and coins3031 preserved. All other save bytes preserved at prep.
Reference source/data and production art unchanged. Inventory acquisition was
synthetic setup, not gameplay acquisition evidence. SHA256s in report.

V08 art issues remain: unfinished separation frames, cave continuity,373/377 wall
finishes, missing outfits, mountains/other asset defects. No production art edits.
Lamp geometry alias/light runtime work already existed at6f864e3 and passed live.
Startup handled FAppTime thread-context ensure is a separate engine diagnostic;
do not claim an entirely clean engine log from protocol counters.

Checks: prior6f864e3 Unreal build/automation/native suites retained; no rebuild
needed for docs/evidence only. git diff --cached --check passes. Final secret_check
on staged docs/evidence and reachable history PASS; secret-check-final.txt retained.
38 source JSON artifacts parse; four EndPlay counters summarized; PNG hashes saved.
Changes staged for review, not committed/pushed. WORKTREE_CLEAN=NO.

Next permitted QA: inspect report and resume only missing live evidence. Do not
start independent certification, another milestone or main merge. A code change
for inventory->world dragging needs scope beyond current no-feature QA request.
Relevant input source: Real33DPanelChrome.cpp::SReal33DSlot::OnDrop,
Real33DHUD.cpp::HandleItemDropped and PlayerController world drag receiver.
For walk attribution: clientcore/src/movement_ledger.cpp::NoteLocalMove and
session4-movement-journal.jsonl; compare authoritative floor changes and NPC Follow.
Server authority: selected game receiving.cc::CUseTwoObjects/CAttack,
operate.cc::Use/CheckMoveObject, moveuse.cc::UseLiquidContainer, crcombat.cc.
QA launch from this worktree:
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/client/run_unreal_presentation_qa.ps1 -Account B
Check existing server with scripts/server/status_wsl.sh via Ubuntu-26.04 root;
start existing runtime only if needed, never reset/prepare it again.
Final readiness for independent certification NO.
