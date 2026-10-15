# Current handoff

2026-10-15 14:52 America/Guatemala. Codex, operator-assisted implementation QA.
UNREAL-WORLD-PRESENTATION-POLISH-001 = PASS LOCAL (repository state PASS).
Not CERTIFIED. Independent certification NOT started; no merge to main.

Branch milestone/unreal-world-presentation-polish-001.
Base/main fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf, remote main rechecked unchanged.
Starting local=remote3ace436dd3affbcc6a9621fa9a58963a693befd4, worktree clean.
Ending commit is the documentation/evidence commit containing this handoff;
read git HEAD after normal push verification. No force push or code changes.
Task worktree C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Root fusion32 is a DIFFERENT worktree/branch; do not edit it for this task.
Previous substantive handoff archived as
archive/UNREAL-WORLD-PRESENTATION-POLISH-001-PRE-FINAL-QA-20261015T1413.md.

User scope: only final post-drag-fix Use With, minimap click, movable creature push
and compact regression. No features/refactor/independent certification. Every
click/key was operator supplied; use ONE async manual action at a time. Desktop
automation previously failed. Inspect actual logs/F9, because operator sometimes
performs extra actions. Initial services had saved and stopped game normally;
stop/start scripts restored same runtime, no reset/save edits or equipment prep.

Closed this run:
- USE_WITH PASS: request68 rope3003 on435 at32097,32205,7 retrieves live rat
  from server. Targeting capture03, explicit operator confirmation of cursor/
  banner appearance and clearing after choosing target. Not a creature push.
- MINIMAP_CLICK PASS:20:22:58.962Z destination32095,32205,7; authoritative exact
  arrival20:23:03.185Z after nine accepted steps. Later F9 is not arrival image.
- PUSH_CREATURE PASS: request168 at20:42:29.947Z drags rat1073743164 type99
  from32103,32204,8 to32102,32205,8. Operator confirms movement; received F9
  capture06 matches exact creature/destination. Reverse request170 corroborates
  received destination. Earlier range/no-room rejections retained separately.
- GAMEPLAY_REGRESSION PASS: combine these missing checks with retained final-build
  INVENTORY-DROP-20261015 movement/Look/Use/container/world-world/inventory-ground/
  ground-inventory/Follow+Stop/speech/light/camera-wall/WideWorld tests. Same3ace436
  implementation throughout; do not claim every check repeated in new connection.

Read authoritative selected receiving.cc CUseTwoObjects/CMoveObject and operate.cc
CheckMoveObject/CheckMapDestination, revision386fa9b, no TIBIA772 branch in those
symbols. Existing protocol only; no optimistic item/creature relocation or fake
local speech. Rat flags/server movement constraints were not changed for QA.
Previous inventory drop/pickup routing fix and all historical evidence preserved.

New visual reports and limitations, NOT fixed:
- Grass-edge and roof offsets captured02. Grass4532/4536 centered narrow geometry;
  runtime only restores reviewed dirt4785..4792. Runtime-versus-art cause remains
  UNVERIFIED. Do not silently classify as production art or claim fixed.
- Underground at zoom2200 shows black surrounding space but lit received terrain.
  StaticSectorActor::DesiredSectors loads only Z0..7; covered floor8 hides upper
  static floors. Existing surface-only WideWorld coverage, no underground static
  extension. At zoom1000 capture05/player clearly visible, operator explicitly
  confirms playable; bridge movement/push follow. No automatic zoom or proximity
  hiding restored. Bounded camera/wall criteria pass, no unlimited-view promise.
- Operator requested right-click Use With cancellation. Not implemented during
  QA-only work; supported cancel remains Escape. Preserve request for next scope.
- Known V08 art: unfinished frames/cave continuity/walls373/377/outfits/mountains.
  No asset redesign. PASS LOCAL covers named gameplay/readability criteria,
  not complete art acceptance or resolution of new alignment reports.

Live directory build/unreal-world-presentation-polish-001/live-20261015T141652.
REAL33D20084 closed normally20:48:17.097Z,1755seconds. EndPlay7267commands,
5480frames, residual0/unsupported0/protocol_anomalies0, last_diagnostic none,
viewport synchronized,0orphans/duplicates. PROTOCOL_ERRORS=0.
Raw walks139requested/133accepted/7rejected/0unanswered;18external relocations,
151local moves. Do not normalize raw counts or hide legitimate gameplay refusals.
Final player32097,32206,7, no attack/follow target; backpack3/jacket4/sword5,
backpack coins4/shovel/rope. Normal rat attacks occurred; operator returned to
surface before closure. No character data edit. Fusion32 services remain running:
querymanager601/game622/login931 in Ubuntu-26.04, runtime
/var/lib/fusion32-server-baseline-772-0. Never prepare/reset/re-add equipment.

Tests: fresh nine native ClientCore suites PASS. Three fresh Unreal automation
suites Success/0errors: CameraAndFloors, AuthoritativeWindow, ExperimentalV08.
One expected catalog warning: TEST_IMPORTED is not APPROVED/READY/INTEGRATED.
REAL33DEditor Win64 Development up-to-date build PASS,0actions,2.33seconds.
Final secret_check, staged whitespace/JSON checks and push verification recorded
in report closeout. No source changes to rebuild or gameplay rules changed.

Evidence: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-FINAL-QA-20261015.md
and evidence/clientcore/unreal-world-presentation-polish-final-qa-20261015/.
Six PNG checkpoints, received state/geometry, selected log, movement journal,
EndPlay and test results. Earlier CONTINUATION-20261009/ASSISTED-20261014/
INVENTORY-DROP-20261015 preserved unchanged. No historical verdict rewritten.
Files changed: this handoff, PROJECT_STATUS, PARITY_MATRIX, new report/artifacts,
prior handoff archive only. Ready for independent certification YES, but do NOT
start it without a new user instruction and do NOT merge main.

Next work only as explicitly requested: independent reviewer can reproduce
named cases from report; new grass/roof alignment and requested cancel behavior
need separately scoped diagnosis/implementation. Exact files for offset diagnosis:
Private/Real33DAssetRegistry.cpp and the V08 catalog (parse JSON, do not dump its
giant single line); roof types still need identification in received geometry.
Static background scope is Private/Real33DStaticSectorActor.cpp::DesiredSectors/
ApplyView. Do not modify production art under the polish milestone.
Normal launch if requested:
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/client/run_unreal_presentation_qa.ps1 -Account B
Check existing runtime via scripts/server/status_wsl.sh; no provisioning/reset.
