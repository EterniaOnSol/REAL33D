# UNREAL-WORLD-PRESENTATION-POLISH-001 final assisted QA

Final local result, 2026-10-15 14:52 America/Guatemala: PASS LOCAL for the
milestone's named runtime/gameplay criteria. This is implementation QA, not
CERTIFIED. No source, asset, protocol or server-rule changes in this continuation.
New grass/roof alignment reports remain open and unclassified; the local result
does not claim that V08 art is complete or those reports are fixed. See limitations.
Final secret_check/publication verification is recorded at closeout below.

2026-10-15 14:15 America/Guatemala. Codex, operator-assisted implementation QA.
Starting local/remote3ace436dd3affbcc6a9621fa9a58963a693befd4, clean worktree;
branch milestone/unreal-world-presentation-polish-001. Main/base remains
fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. No independent certification/merge.

User authorizes QA only: repeat real Use With/targeting feedback, known minimap
click arrival, one legitimate movable creature attempt, final compact regression.
No features/refactor/server-rule changes. Prior final-build regression evidence
in INVENTORY-DROP-20261015 remains retained; completing its missing checks on
the same unchanged code avoids redundant full milestone recertification.

At startup querymanager/login alive but game stopped. Log tail shows all objects
and player data saved, reboot script absent and connections closed. Existing
stop_wsl.sh cleanly stopped remaining services; start_wsl.sh restarted the SAME
runtime, no preparation/reset/save edits. All process identities/ports verified:
querymanager601/game622/login931,7173/7172/7171. REAL33D launch in progress.

Source read: receiving.cc::CUseTwoObjects (selected game386fa9b) validates MULTIUSE,
coordinates/visibility then queues ToDoUse; operate.cc::CheckMoveObject validates
accessibility and race push restrictions. Neither has TIBIA772 branch. HUD
BeginUseTarget/CompleteUseOnField present feedback/send existing command and clear
targeting; PlayerController::RequestKnownWalk reports authoritative arrival.

Current overall verdict IMPLEMENTED_UNVERIFIED. New evidence goes in
unreal-world-presentation-polish-final-qa-20261015/. Operator performs one requested
action at a time; actual logs/captures, not intention alone, establish PASS.
Pending: Use With, minimap arrival, push attempt and final protocol summary.

## Live progress

Client20084, live-20261015T141652. Baseline01 at20:20:21.040Z proves normal login,
player32095,32214,7 and retained equipment;51commands/protocol counters0.
MINIMAP_CLICK PASS: actual minimap destination logged20:22:58.962Z at
32095,32205,7; nine accepted steps and exact authoritative arrival20:23:03.185Z.
Operator confirms arrival. Subsequent F9 is after additional operator walking,
not an arrival screenshot; log/movement journal preserve exact arrival.

Operator reported grass-edge and house-roof offsets. Capture02 at20:25:47.158Z
retains the scene. Visible grass border4532 at32099,32208..10 has37.5x100uu bounds
centered like the ground; corner4536 is25x28.12uu. Catalog marks these FAMILY_RECIPE,
PENDING; current adapter restores only previously reviewed dirt4785..4792 offsets.
Source/mesh-placement cause for newly reported grass/roof issue is not fully
classified; do not call it fixed or silently classify all of it as production art.
No art or runtime edits authorized/performed during this QA-only continuation.
Operator also requests right-click cancellation of Use With. Current supported
cancel remains Escape; requested interaction change noted, not implemented.

Operator independently navigated underground, used ladder, attacked rats and
returned. Do not misattribute all those actions to prompts. Real rope request68
20:30:09.946Z uses3003 on435 at32097,32205,7; server appearance of rat1073743165
at32097,32206,7 at20:30:10.148Z proves Retrieve result, not player rope movement
or creature push. Capture03 at20:30:38.410Z separately shows rope targeting
banner and Use With cue; latest begun3003 at20:30:33.699Z. The earlier shovel
on103 and later rope on dead-rat3994 correctly receive gameplay rejection.
Targeting clear after valid result still needs confirmation/capture. Later
combat killed the retrieved rat; cannot use that sequence as PUSH_CREATURE PASS.

## Completed missing checks

USE_WITH = PASS. The real rope Retrieve result above follows request68 on the
post-drag-fix build. Operator explicitly confirmed the targeting cursor/banner
appeared and later confirmed: "Si, desaparecieron" after selecting the successful
rope destination. Capture03 proves the same rope targeting UI in this connection;
captures04-06 show it cleared. These screenshots are separate checkpoints, not
a fabricated before/after pair for request68. The authoritative result is the
server-supplied rat appearance; client presentation did not create the rat.
No NPC response, object movement or Use With result was fabricated for QA.

MINIMAP_CLICK = PASS, request destination and exact authoritative arrival above.
The retained movement journal contains the nine accepted cardinal steps. No
minimap recertification was performed; current marker agrees with received player
position in all retained snapshots.

PUSH_CREATURE = PASS. Normal left-button drag request168 at20:42:29.947Z selected
live rat1073743164, type99, from32103,32204,8 stack1 to32102,32205,8, count1.
Operator reports "Se movio". Capture06 at20:42:59.865Z has that same creature
at exactly32102,32205,8 in received WorldState and visible left of the player.
The requested source was taken from live received state. There is no optimistic
creature relocation in this input route. No failure message follows request168.
Later request170 at20:44:46.008Z selects the same rat at that destination and
requests the reverse move, further corroborating received destination state.
No server rules, creature flags, positions or health were edited for this test.

Preserve rejected attempts separately:150/153/165 received "Destination is out
of range";162 received "There is not enough room". Attempt158 selected terrain4796,
not the rat, and was rejected. These are authoritative gameplay/selection results,
not unsupported opcodes or successful pushes. Selected receiving.cc::CMoveObject
and operate.cc::CheckMoveObject/CheckMapDestination enforce accessibility, current
object position, adjacent creature destination and MovePossible. The rat is mobile
during the server's delayed move execution; the exact intermediate server position
for each rejected attempt is not retained. Do not over-attribute each rejection
to an unobserved tile or treat them as client-routing failures.

## Underground view and remaining visual reports

Capture04 at20:33:43.464Z records the reported black view: player32097,32205,8,
manual distance2200, obstructed=true, cutaways0. Ground, water, creatures and the
player are lit; surrounding unrendered space is black. Camera distance is not
automatically changed. Read-only source diagnosis: Real33DStaticSectorActor.cpp
DesiredSectors iterates Z=0..7, while ApplyView hides upper static floors under
cover. Underground floor8 therefore has the received live area, without a static
underground extension. This is an existing WideWorld coverage limitation, not a
new lighting blackout, a protocol error or evidence of hidden live-state exposure.

Operator zoomed closer and also walked/orbited; capture05 at20:38:37.438Z is at
32099,32205,8, manual distance1000, obstructed=false, cutaways0. Player and walkable
area are clear. Operator explicitly confirms "Si, se ve y se puede jugar".
This is a bounded playability check, not a pixel-identical zoom comparison or
a claim that unlimited underground scenery exists. Normal walking across the
bridge and the rat push followed. No automatic zoom or proximity wall removal
was restored. Existing indoor/doorway/upper-lower-floor obstruction evidence
remains retained and the same camera/roof code is unchanged.

Known V08 art issues remain: cave continuity, unfinished building separation
frames, wall finishes373/377, missing outfit coverage and mountain defects.
The new grass-edge and roof offsets reported with capture02 remain UNVERIFIED
as to runtime-versus-art cause. They were not silently waived as V08_ART_ISSUE
and no production asset was modified. No named gameplay/readability test failed
because of these offsets in this run; local PASS covers the documented criteria,
not full art acceptance. Right-click Use With cancellation is a requested input
change still unimplemented; supported cancellation remains Escape. No feature
work or new milestone was started during this QA-only continuation.

## Final compact regression, unchanged implementation

The final build used in INVENTORY-DROP-20261015 and this connection is the same
implementation committed at3ace436. This continuation modifies documentation and
evidence only. Retain the previous compact checks; do not claim they were all
re-executed in this connection (its Say and Look request counters are zero).

| Required check | Live result and evidence |
| --- | --- |
| Login/manual movement | PASS; baseline01, current journal, EndPlay07 |
| Look | PASS; prior final-build request21/capture05, sword description |
| Use/container | PASS; prior backpack request11/capture04; current open backpack |
| Use With/target feedback | PASS; current request68, capture03, operator confirmation |
| World-to-world drag | PASS; prior sword request22/capture06 |
| Inventory-to-ground | PASS; prior direct slot5 request20/capture05 |
| Ground-to-inventory/container | PASS; prior requests12-19/24, captures04/07 |
| Follow/Stop | PASS; prior requests27/34, captures07/08 and received relocations |
| Minimap click | PASS; current20:22:58 destination,20:23:03 exact arrival |
| WideWorld | PASS; prior surface movement, current69loads/13unloads/56sectors |
| Player/NPC speech, chat and expiry | PASS; prior hi/heal and captures09-11 |
| Light toggle | PASS; prior request40/capture12 on-to-off, earlier full cycle retained |
| Camera/wall occlusion | PASS; prior final-build temple indoor/outdoor, retained doorway/orbit/floor cases; current underground playable05 |
| Creature push | PASS; current request168/capture06/operator witness |

GAMEPLAY_REGRESSION = PASS for this compact combined final-build session evidence.
No full earlier milestone certification was repeated. All originally retained
camera/floor/roof/label/hover/Look/combat/follow/chat/WideWorld results remain
bounded to their documented scenes. WORLD_SPEECH and NPC_SPEECH both have real
server-backed evidence; no unsupported-case exception is needed.

## Final protocol state and checks

REAL33D20084 closed normally at20:48:17.097Z (EndPlay07),1755session seconds.
7267commands/5480frames, residual_bytes0, unsupported_opcodes0,
protocol_anomalies0, last_diagnostic none, viewport_synchronised true,
duplicate_spawn_attempts0 and orphan_events0. PROTOCOL_ERRORS=0.
Raw walk counters:139requested,133accepted,7rejected,0unanswered,18external
relocations,151local moves. Preserve counters verbatim rather than normalize
them or mistake normal gameplay rejection for a protocol anomaly.
Final player32097,32206,7, no attack/follow target; equipment remains backpack3,
jacket4, sword5 and backpack coins4/shovel/rope. Operator returned to surface
before closing. Normal rat attacks occurred; no healing/state edit for QA.
Fusion32 remains running with the same saved runtime; do not prepare/reset it.

Nine native ClientCore suites freshly PASS, exact output clientcore-tests.txt.
Three Unreal suites freshly Success,0errors/0failed/0skipped/0unfinished:
CameraAndFloors, AuthoritativeWindow, ExperimentalV08. The catalog suite has
one expected warning that TEST_IMPORTED assets are not APPROVED/READY/INTEGRATED;
it is not an art approval. See unreal-automation.json.
REAL33DEditor Win64 Development build PASS, up to date,0actions,2.33seconds.
Existing compiled implementation unchanged; no refactor/rebuild-based gameplay
claim. Compact final logs/journal and six screenshot checkpoints are retained.

READY_FOR_INDEPENDENT_CERTIFICATION = YES for the documented local criteria.
Independent certification was NOT started and main was NOT merged or changed.

## Publication preflight

secret_check PASS: tracked extensions/scratch exclusion, reachable-history key
scan, generated credential scan and reference immutability all pass. Staged
git diff --check PASS;13 retained JSON files parse successfully. Staged changes
are documentation/evidence only, no unreal/clientcore/reference/scripts/tests
source changes. Six PNGs are retained through existing Git LFS attributes.
Remote milestone remains3ace436dd3affbcc6a9621fa9a58963a693befd4 at preflight;
origin/main remainsfa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. No divergence.
Publish this evidence with a normal commit/push on the milestone branch, then
require local HEAD=remote milestone HEAD and clean worktree. The ending evidence
commit is the commit containing this report; no source revision changed.
