# Current handoff

2026-10-15 04:36 America/Guatemala. Codex, implementation/operator-assisted QA.
Task UNREAL-WORLD-PRESENTATION-POLISH-001 = IMPLEMENTED_UNVERIFIED.
User requested immediate commit/push due remaining tokens. Preserve this state;
no independent certification, new milestone, unrelated features or main merge.

Branch milestone/unreal-world-presentation-polish-001.
Base/main fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Original code6f864e3; saved prior evidence committed/pushed BEFORE code changes
as1e7a2d5097b81e79c7441d819cbd7d585da36ef5, verified clean/local=remote.
Starting implementation HEAD1e7a2d5; ending implementation commit is the commit
containing this handoff. Verify normal push, local=remote and worktree clean.
Remote precloseout unchanged1e7a2d5, main unchangedfa3adcf.
Worktree C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.
Root fusion32 worktree is a DIFFERENT branch; never edit it for this task.
Prior handoff archive/UNREAL-WORLD-PRESENTATION-POLISH-001-PRE-DRAG-FIX-20261015T0402.md.

Authorized scope: trace/classify inventory-ground drag, minimal routing fix,
normal creature push and compact post-fix regression. Operator then reported
pickup failure; fixed that same routing boundary. Operator supplied every click
and key. Desktop automation previously failed. Continue ONE manual action at a
 time via async question; inspect actual timestamped logs/F9 rather than assume
operator performed only the requested action.

Root cause B: pre-existing missing inventory-to-world Slate drop receiver,
not a camera/wall regression. Slot drag existed; only other slots accepted it.
Existing Bridge/ClientCore/Fusion32 move supports all endpoints. Minimal changes:
Public/Private Real33DHUD and Real33DPlayerController; Public Real33DPanelChrome.
HUD forwards drops within world rectangle; controller revalidates source slot,
converts desktop coordinates to viewport pixels and requests existing move to
received visible same-floor tile. No optimistic inventory/world mutation.
Reverse pickup resolves physical Slate item-slot hit path. Panel/chat drops
cannot accidentally target world geometry behind UI. Creature-to-slot refused.
Map drag retains existing count1 semantics; four ground coins were picked up
individually. No quantity picker/refactor/new protocol/server-rule edits.
Source: receiving.cc::CMoveObject; operate.cc::Move/CheckMoveObject/
CheckMapDestination at selected game386fa9b, no TIBIA772 branch in these symbols.

INVENTORY_GROUND_DRAG PASS: direct sword inventory5 removal and ground appearance
on final build, request20/capture05. Container-to-ground four coins also proven.
Ground-to-container and direct ground-to-inventory PASS; all dropped equipment
recovered authoritatively. Final sword5/jacket4/backpack3, backpackcoins4/shovel/
rope. Native protocol/world state remains server-authoritative.
Final-build compact PASS: manual movement, Look, Use/container, world drag,
Follow/Stop, world/NPC speech/chat/expiry, light on->off, camera-wall indoor/outdoor
smoke and WideWorld movement. Prior camera obstruction/full lamp cycle retained.
Remaining PUSH_CREATURE IMPLEMENTED_UNVERIFIED (no attributable other-creature
push here), and GAMEPLAY_REGRESSION IMPLEMENTED_UNVERIFIED: UseWith and minimap
click arrival still need final-build smoke. Prior6f864e3 passes retained but
not substituted for post-fix checks. No local PASS/certification readiness.

Report evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-INVENTORY-DROP-20261015.md.
Artifacts evidence/clientcore/unreal-world-presentation-polish-inventory-drop-20261015/.
Captures01-12, received JSON/geometry, selected logs, movement journals and checks.
Previous ASSISTED-20261014/CONTINUATION-20261009 evidence preserved untouched.
Session1 live-20261015T040507 client9972 closed10:12:23.347Z:
289commands,residual/unsupported/anomalies0,18accepted/0unanswered walks.
Session2 live-20261015T041555 client18884 closed10:35:41.478Z:
640commands,residual/unsupported/anomalies0,14accepted/0rejected/0unanswered;
4external relocations from Follow/server approach for Use. Player32095,32214,7,
HP65 after legitimate Cipfried heal. No combat/follow target. Lamp32094,32214,7
is now2108/off. All gear and four coins recovered; no items left by these tests.
Existing server runtime /var/lib/fusion32-server-baseline-772-0, Ubuntu-26.04,
querymanager584/game599/login867 last verified. Services left running, client
closed. Do not reset runtime or repeat prior equipment preparation script.

Checks: final Unreal build PASS11.92s; CameraAndFloors/AuthoritativeWindow/
ExperimentalV08 suites PASS; nine native ClientCore suites PASS. One intermediate
pickup compile failed and was corrected before session2. Final staged diff/JSON
validation and secret_check required before push; evidence saved during closeout.
FAppTime handled engine ensure remains distinct from zero protocol errors.
Historical14unanswered walks in prior6f864e3 session not silently cleared.
V08 art issues: unfinished frames, cave continuity,373/377 wall finishes,
missing outfits/mountains. No production art changes. Preserve manual zoom,
no proximity wall hiding/retraction, accepted roof/water/proportion behavior.

Next task: resume missing QA only. Launch from this worktree:
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/client/run_unreal_presentation_qa.ps1 -Account B
Recheck existing server with scripts/server/status_wsl.sh, never prepare/reset.
Use normal UseWith (rope3003/shovel3457/bucket if available), known minimap click
arrival and safe other-creature push. Obi unpushable; earlier Cipfried attempts
legitimately rejected. Rat race21 source has no Unpushable flag. Earlier rope
on435 at32097,32205,7 retrieved rat to32097,32206,7; retrieval is NOT push proof.
No Fusion32 rules/data changes for QA. Prefer captured before/request/after and
operator witness for pushing a movable creature. Stop if conditions unsafe.
READY_FOR_INDEPENDENT_CERTIFICATION=NO. No merge to main.
