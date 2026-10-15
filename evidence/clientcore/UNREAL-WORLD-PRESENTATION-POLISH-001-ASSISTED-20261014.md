# UNREAL-WORLD-PRESENTATION-POLISH-001 operator-assisted live QA

Preservation update2026-10-15: operator explicitly authorized this evidence-only
checkpoint commit/push before further code work. The uncommitted descriptions
below are historical closeout state. Overall remains IMPLEMENTED_UNVERIFIED;
none of the failed/unverified criteria is promoted by committing these artifacts.

Closeout 2026-10-15 03:38 America/Guatemala: IMPLEMENTED_UNVERIFIED.
Code/local/remote HEAD6f864e3f9e9fbb82fccdf4c2ae1e4287c4360b52 unchanged;
main/basefa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf unchanged (remote verified).
No code/refactor/art edits, certification or main merge. Evidence/docs preserved
locally, uncommitted; no conditional PASS-local closeout/push performed.

| Check | Final local result |
| --- | --- |
| CAMERA_COLLISION | PASS |
| WALL_OCCLUSION | PASS |
| LOOK_SMOKE | PASS, session1 |
| USE_WITH_FEEDBACK / USE_WITH_SMOKE | PASS, session3 + operator observation |
| DRAG_OBJECT | PASS, world-to-world only |
| INVENTORY_TO_WORLD_DROP | FAILED, coins3031 |
| PUSH_CREATURE | IMPLEMENTED_UNVERIFIED |
| LIGHT_TOGGLE | PASS |
| FOLLOW_SMOKE / STOP_FOLLOW_SMOKE | PASS, session4 |
| CONTAINER_SMOKE / USE_SMOKE | PASS |
| MINIMAP_CLICK_SMOKE / MINIMAP_COMPATIBILITY | PASS, session4 |
| WIDEWORLD_SMOKE / WIDEWORLD_COMPATIBILITY | PASS |
| WORLD_SPEECH / NPC_SPEECH / CHAT_COMPATIBILITY | PASS, retained + fresh session4 |
| GAMEPLAY_REGRESSION | IMPLEMENTED_UNVERIFIED |
| UNSUPPORTED_OPCODES / PROTOCOL_ANOMALIES / PROTOCOL_ERRORS | 0 in retained snapshots/EndPlay |
| CLIENTCORE_TESTS / UNREAL_BUILD | PASS retained at unchanged6f864e3; no code change |
| SECRET_CHECK | PASS, final staged tree/history check |
| READY_FOR_INDEPENDENT_CERTIFICATION | NO |

The last compact connection covered login, manual movement, floor changes,
Use/container, speech, minimap, Follow/Stop and WideWorld. Look and Use With were
proved in preceding connections; no all-actions single-session claim is made.
Inventory-to-world remains a concrete failed case, and unmatched walk-ledger
entries are retained for reconciliation. Do not promote the milestone on these
partial/composite results. Creature push was optional if safely available:
latest Cipfried126/127 attempts returned no room/not possible; no safe eligible
successful sample was captured. Operator's monster-push report is preserved but
does not replace attributable authoritative evidence. No further combat sought.

V08_ART_ISSUES: unfinished building separation frames, cave continuity, wall
finishes373/377, missing outfits, mountains and other asset defects remain art
work. No production mesh redesign. Runtime lamp alias/light already at6f864e3
passed OFF/ON/OFF. Retained roof/water/label/selection changes remain intact.

Current checkpoint 2026-10-15 03:18 America/Guatemala: overall
IMPLEMENTED_UNVERIFIED. USE_WITH_SMOKE and USE_WITH_FEEDBACK now PASS in the
bounded bucket/well test below. Camera, walls, Look, map-to-map drag and lamp
toggle retain PASS. Inventory-to-world coin drop remains FAILED. Creature push
attribution and the complete gameplay regression remain IMPLEMENTED_UNVERIFIED.
No implementation, production art, reference source, or protocol changes.

Session3, live-20261014T233114, ran unchanged 6f864e3. Equipment receipt is
confirmed by F9 05:36:19.487Z: backpack2854 slot3, sword3294 slot5, retained
jacket3379 and four3031 coins. Normal Use74 at05:35:12.674Z opened container0,
capacity20, with shovel3457 and rope3003. equipment-container.png shows both tools.
Acquisition remains the explicitly authorized synthetic preparation described below.

USE_WITH_SMOKE=PASS. Initial server Look at05:33:50.936Z reports bucket2873
empty. After normal world dragging, source was32093,32201,7 and player
32092,32202,7. Targeting began05:34:44.361Z; UseWith58 at05:34:45.098Z selected
well1930 at32093,32202,7 stack2. Look59 received "You see a bucket of water."
at05:34:46.584Z, repeated by Look61 at05:34:50.517Z. UseWith63 at05:34:54.960Z
spilled onto ground870 at32092,32201,7. F9 05:35:06.638Z shows blue pool, bucket
and server chat; filtered geometry preserves pool2886 and bucket2873. Authority:
selected game386fa9b receiving.cc::CUseTwoObjects, operate.cc::Use and
moveuse.cc::UseLiquidContainer (1692ff): empty container fills from LIQUIDSOURCE;
nonempty container spills onto eligible floor and clears liquid. No TIBIA772
conditional override in these paths. This proves a real authoritative change.

USE_WITH_FEEDBACK=PASS, operator-assisted visual evidence. Operator explicitly
confirmed selection cursor and Use With notice appeared and disappeared after
choosing target ("Si, aparecieron y se quitaron"). Logs retain source selection,
target dispatch and legitimate result; post-action F9 shows ordinary interaction
hints and correct chat. No screenshot during transient targeting was captured;
that part rests on the named operator observation. Inspected runtime symbols:
Real33DHUD.cpp::BeginUseTarget, CompleteUseOnField, CancelTargeting and
UpdateTargetingBanner; live observation, rather than code alone, supports PASS.

Operator also reports rope and shovel work properly. Shovel UseWith226 at
05:37:39.644Z targets593 at32077,32151,7; end geometry receives594 at same tile
(shovel-result-geometry.json). Rope requests114/121 target435 at32097,32205,7;
earlier player floor changes were ladder Use113/120, so those relocations are NOT
attributed to rope. Subsequent source/event review corroborates rope retrieval:
runtime moveuse.dat line582 maps rope3003 on435 to Retrieve with below-floor
source and south destination. After request121 at05:36:16.633Z, rat1073743165
appears at32097,32206,7 at05:36:16.903Z; equipment-container.png shows it outside.
This matches the rope rule and operator report; it is not left-drag PUSH_CREATURE
proof. Retained rope-retrieve-events.txt. The complete bucket test independently
closes required Use With smoke and feedback.

Session3 closed normally05:38:20.139Z. EndPlay1478 decoded commands,
residual_bytes=0, unsupported_opcodes=0, protocol_anomalies=0, viewport synced.
Ledger155 accepted,23 rejected,9 unanswered,22 external relocations retained;
not all unmatched requests are explained. No Say, Follow or cancel occurred.
Last32079,32155,7; operator combat included snake poison. Startup contains a
handled FAppTime thread-context ensure, separate from protocol health; no clean
engine-log claim. Artifacts: session3-endplay.json, session3-use-with-events.txt,
use-with-bucket-result PNG/JSON, bucket-result-geometry.json,
equipment-container PNG/JSON, shovel-result-geometry.json.

03:16 local2026-10-15: services querymanager584/game599/login867 identity/port
verified. Client had closed; launched unchanged client20736 with normal QA script,
evidence live-20261015T031623. Continue one manual action at a time for compact
regression, no code/refactor/art changes or independent certification.

Session4 navigation: minimap destination32088,32172,7 logged09:21:57.207Z,
authoritative arrival09:22:14.146Z. F9 09:22:57.559Z matches both player and
minimap marker to that exact destination. Operator confirms arrival. Counts413
commands,26 accepted walks,1 rejected,0 unanswered; residual/unsupported/anomalies0.
MINIMAP_CLICK_SMOKE and MINIMAP_COMPATIBILITY PASS (bounded presentation regression,
not minimap recertification). WideWorld loads68->78, unloads0->8 and readable
expanded scenery while live viewport remains18x14; WIDEWORLD_SMOKE PASS.
Same snapshot shows backpack open with both tools after a normal Use; container
smoke remains PASS. Evidence minimap-arrival PNG/JSON and navigation events.

Session4 local Say09:23:55.731Z automatic capture receives one legitimate talk,
resolved to local creature1002; screenshot visibly shows "qa polish 15" over the
player. JSON contains same chat line and451 decoded commands, all protocol error
counters0. The automatic screenshot was taken before the chat panel painted the
line; manual post-timeout capture is still pending. Prior WORLD_SPEECH/CHAT PASS
retained; do not use this one image alone to claim visible chat compatibility.
Artifacts regression-speech PNG/JSON. Operator's next F9 did not register;
suspected chat focus but cause was not proven. Asked for Escape, then capture
again. No implementation edit.

Manual09:27:19.369Z now visibly shows "Test Player B: qa polish 15" in chat
while the text above the creature has expired. This closes the fresh visible
speech/chat/timeout checkpoint; exact six-second expiry was not timed. Evidence
regression-speech-timeout PNG/JSON and regression-speech-events.txt. Next guided
action: approach a reachable same-floor NPC for Follow/Stop, avoiding combat.

03:23 local preflight tests/secret_check.sh via Git-for-Windows bash PASS on the
current tracked tree/history. New evidence remains uncommitted; before any push,
stage the exact evidence/docs and re-run the required secret check.

Operator independently approached Norma and said "hi"; legitimate NPC response
09:29:47.467Z is visible above her in norma-speech.png and preserved in JSON.
Her response limits service to premium accounts; no account/service bypass made.
Follow screenshot09:34:03.208Z later shows the same Norma response in chat. Thus
the naturally encountered NPC speech path is freshly supported without hardcoding.

Follow attempt on Norma could not reach her through the building access from
32095,32181,7; server reported no way/refusals. Operator then walked to Cipfried
and activated Follow121 at09:32:54.664Z, with authoritative player relocations
32097,32213,7 ->32214 ->32215 ->32216. Toggle122 clears state, then Follow123
at09:32:56.677Z reactivates it; server follows to32097,32217,7 at09:33:05.343Z.
F9 09:34:03.208Z shows blue [Follow] Cipfried label/selection and battle-list [F],
target1073741889,following=true, same-floor adjacent coordinates. FOLLOW_SMOKE=PASS.
Authority receiving.cc::CAttack(Follow), crcombat.cc::SetAttackDest/CheckAttack:
following forces CLOSE approach, independent of current attack chase mode, no
TIBIA772 conditional here. No speculative client movement used. Snapshot1104
commands,residual/unsupported/anomalies0,ledger14unanswered retained. Rejection
count48 includes server refusals with request0 during blocked Follow, so do not
interpret it as48 distinct requested walks.

STOP_FOLLOW_SMOKE=PASS. General cancel145 at09:34:55.120Z clears combat target
at09:34:55.270Z; operator confirms blue mark removed. Operator then independently
repeated Follow146 at09:36:00.726Z and same-target toggle147 at09:36:03.788Z,
clearing target again09:36:03.951Z. Manual09:36:05.819Z shows normal Cipfried label,
no blue selection, target0/following=false. Evidence stop-follow-cipfried PNG/JSON
and regression-follow-events.txt. The final screenshot belongs to the button
toggle, not the earlier Escape action; both paths are distinguished in the log.

Session4 closed normally by operator09:37:46.799Z,1173.5seconds. EndPlay1284
commands,residual0,unsupported0,anomalies0,viewport synchronized. Walk ledger93
accepted,49 refused,14 unanswered,44 external relocations. Player32097,32216,7,
no combat target/follow. Four coins remain; no inventory-to-world repair. Saved
session4-endplay.json and full movement journal, plus final event excerpts.
Across four preserved EndPlay records no protocol error is reported. This does
not erase the gameplay/evidence gaps above. No new builds/tests needed for code
because source/binary unchanged; diff check and artifact integrity checked below.

Final verification: git diff --cached --check PASS after fixing one report EOF
blank line. All38 pre-summary JSON artifacts parsed successfully; generated
protocol-health-summary.json preserves all four EndPlay counters. SHA256SUMS.txt
records PNG hashes only, avoiding Git text newline-conversion ambiguity.
Git-for-Windows tests/secret_check.sh PASS after staging the exact docs/evidence;
output secret-check-final.txt retained. No source/build or historical continuation
file changed. Changes are staged for review, not committed/pushed; worktree is
not clean. Local/remote code HEAD stays6f864e3, no certification/main merge.

Started 2026-10-14 22:07 America/Guatemala, Codex observing; operator provides all
mouse/keyboard input. Branch milestone/unreal-world-presentation-polish-001,
HEAD and remote6f864e3f9e9fbb82fccdf4c2ae1e4287c4360b52. Main remains
fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. Starting worktree clean.
Scope: evidence only, no features/refactor/art edits. Remaining camera/walls,
Look, Use With, object drag, safe creature push, lamp toggle, compact regression.
Overall IMPLEMENTED_UNVERIFIED. Individual tests IN_PROGRESS until evidence.

Setup: verified existing querymanager519/game540/login717 exe/cwd/start and ports.
REAL33D23020 responding, existing live-20261014T214746 session. No restart/seeding.
Operator action1: F9. Baseline04:08:44.777Z at32104,32205,7 inside house.
1039 decoded commands; residual0, unsupported0, anomalies0; walks9 accepted9,
rejected0/unanswered0. No Look/drag requested. Two prior Uses not attributed to
this guided QA. Camera813.5 requested=actual, obstructed=false, cutaways0;
upper floors hidden. Screenshot proves readable indoor starting pose only, not
camera obstruction acceptance. Evidence assisted-20261014/baseline PNG/JSON.

Server idle warning04:08:38; connection closed04:09:40 after prior inactivity.
Client auto-reconnected04:09:47.045. This is a connection boundary: do not infer
new connection protocol health from the pre-disconnect baseline.
Pending operator action2: Shift+left click nearby visible table for world Look.
Next capture F9 after actual response. No desktop input injected by agent.

LOOK_SMOKE=PASS: operator Shift+left click; request12 at04:10:18.641Z targets32108,32203,7. Fusion32 InfoMessage at04:10:18.683Z: You see a table. F9 at04:10:59.254Z visibly preserves chat response; looks_requested1;107 commands, residual/unsupported/anomalies0. Operator also moved:12 accepted/1refused/0unanswered. Refusal04:10:45.540Z is blocked north walk request6, not Look failure. Evidence look.png/json.


Door obstruction checkpoint04:13:40.217Z,32105,32206,7: camera diagnostic obstructed=true,813.5 requested=actual,cutaways0. Screenshot shows door crossing part of the blue cylinder but label/body remain readable; no disappearance. Arrival14 accepted/1refused/0unanswered,268commands/residual0/unsupported0/anomalies0. This bounded case is evidence, not yet full CAMERA_COLLISION/WALL_OCCLUSION PASS. Next operator action: zoom inward three notches without walking.


Operator clarification: the geometry crossing the character is the unfinished separation/door frame of the building. Classify its malformed appearance as V08_ART_ISSUE, not runtime occlusion. Operator reports character still visible after closer zoom. No art edit and no readability waiver; F9 at closer zoom pending.


Close-zoom04:16:25.949Z: operator also moved three accepted steps into narrow side room32106,32208,7, so this is not a fixed-position zoom comparison. Camera400 requested=actual; obstructed=true; cutaways0. Cylinder/label clearly visible between tall wall and frame. Operator confirms visible and controllable.435commands,unsupported/anomalies/residual0;17accepted/1refused/0unanswered. Retained tight-room-close-zoom PNG/JSON. Next camera orbit from opposite wall side.


Opposite-angle04:18:33.293Z at32105,32206,7: operator also changed zoom760 and walked5 accepted steps; yaw299.60,pitch-52.45,obstructed=true,cutaways0. Frame masks top portion, body+label remain readable, operator says playable.571commands with residual/unsupported/anomalies0;22accepted/1refused/0unanswered. Source/target light metadata inspected read-only: bottle2875 MultiUse/LiquidContainer/Take; water cask2520 LiquidSource water1. UseLiquidContainer in selected moveuse.cc1692ff supports empty bottle fill; operate.cc::Use checks accessibility and invokes it, no version-specific branch in this path. Pending floor change via visible stairs1947 behind cask.


Upper floor04:19:57.122Z:32110,32206,6, camera760 requested=actual,no obstruction,upperfloors hidden,player visible.674commands, residual/unsupported/anomalies0. One unanswered walk is explained by retained movement journal: request31 north from32110,32208,7 followed by authoritative relocation32110,32206,6; MovementLedger::NoteLocalMove only consumes exact cardinal same-floor StepPosition, leaving stair-trigger request to expire. Do not silently zero this counter. Clicking stair hole to descend reports no reachable observed route/frontier; no movement command sent for those clicks. Operator asked to try single Down key. Capture upper-floor PNG/JSON retained.


Floor return04:24:25.553Z at32110,32208,7: authoritative south-trigger relocation downstairs confirmed; second unanswered matches floor-down mismatch in same ledger. Requested/actual880,obstructed=false,cutaways0,roofhidden; player readable beside stairs/tall walls. Operator resized window, snapshot960x1120; screenshots cannot be pixel-compared to1280x720.949commands, residual/unsupported/anomalies0;37accepted/1refused/2unanswered/2relocations. Camera and wall bounded cases now include tall walls, doorway, narrow room, orbit, zoom400,up/down floors; exterior/foreground checkpoint pending before final PASS. No implementation changes.


DRAG_OBJECT=FAILED for operator inventory->world case: user cannot drop3 gold coins3031 fromslot10. Snapshot04:31:06.388Z retainsamount3; moves_requested19 unchanged from04:28:28.006Z; no inventory move request. Source: Real33DPanelChrome.cpp::SReal33DSlot::OnDrop consumes slot targets; Real33DHUD.cpp::HandleItemDropped maps only inventory/container. No world drop receiver exists in this implementation. No code fix per QA-only scope. World drag attempts selected table2319(stack2), watercask2520,stairs1947,fixture2014, not bottle2875. Multiple requests received FailureMessage; no authoritative object displacement demonstrated. Bottle model appears beneath tabletop (art/stack placement issue); do not claim tabletop selection selects bottle. Earlier proposed bottle/cask is a valid type pair, but the current source selection path has not been exercised successfully. Snapshot1444commands,residual/unsupported/anomalies0. GAMEPLAY_REGRESSION remains IMPLEMENTED_UNVERIFIED; inventory drop failure explicitly retained.


CAMERA_COLLISION=PASS and WALL_OCCLUSION=PASS (local live acceptance, documented camera angles/scenes). Preconditions:6f864e3 current manual-distance/no-proximity-hiding policy; received local scene in Obi building and exterior. Evidence baseline,door-obstruction,tight-room-close-zoom,frame-opposite-angle,upper-floor,lower-floor-return,exterior-lamp-off. Obstructed diagnostic true in door/passage cases, visible cylinder+label and operator explicitly confirms control; zoom400 did not trap camera inside opaque geometry; orbit760 and floor transitions readable. Outside04:32:50.477Z restores roof/upper geometry(hide_upper_floors=false) at32100,32205,7,requested=actual880; no cutaway, no hidden-state exposure mechanism added. Existing WorldActor floor logic uses received cover; static scenery stays distinct from live gameplay. Upper/lower wall combinations retained and gameplay authoritative; no render-driven state change observed.1558commands,residual/unsupported/anomalies0. Imperfect separation frame is operator-identified unfinished V08 art, not a runtime patch target. This is local bounded acceptance, not independent certification or guarantee for every orbit/asset.


Lamp ON live: source32100,32206,7 OFF2108 captured04:32:50.477Z. Request116 Use2108 at04:33:51.921Z; F9 at04:33:53.824Z receives2109, upright mesh02108,bounds36x36x170, same screen position672.23,332.28; screenshot shows warm lit lantern compared to dark OFF lantern. Operator confirms switched on.1623commands, residual/unsupported/anomalies0; uses increased4->5. Awaiting OFF return capture.


LIGHT_TOGGLE=PASS. Return OFF F9 at04:35:18.589Z: same tile32100,32206,7 now2108,uses6,1705commands/residual0/unsupported0/anomalies0. Lamp dark in captured image; same mesh02108,bounds36x36x170 and screen center as ON. Operator confirms both transitions. Use requested visible. Three snapshots plus filtered geometry and lamp-use-events.txt prove authoritative OFF->ON->OFF and fixed upright geometry; no local synthetic state. USE_FEEDBACK retained with fresh successful normal Use checkpoint. Next safe NPC push attempt adjacent Obi using left drag only.


Session1 EndPlay04:57:07.142Z preserved:4788commands/residual0/unsupported0/anomalies0,connected at close. Player last32098,32205,8 after additional operator movement; no speech/follow/cancel in this connection;25move,7use,4look requests. All3coins remain. User says Obi not pushable, but captured later drag requests target Cipfried1073741889 in temple. Several rejected; autonomous NPC movement makes a before/after position without per-request move event insufficient to attribute a successful push. PUSH_CREATURE remains IMPLEMENTED_UNVERIFIED. Source CheckMoveObject restricts race-unpushable under server world/peaceful rules; no invented NPC behavior. Client closed normally22:57 local. At23:02 user resumed; server519/540/717 still verified; relaunch same built6f864e3 for continued manual QA. No implementation/refactor/art edits.


DRAG_OBJECT=PASS specifically map->map (supersedes broad FAILED label above; inventory->world remains FAILED). Objects2524 are troughs per objects.srv, not crates. Before05:10:16.834Z at32080,32186,7 and32080,32187,7; commands113/114 move them to32081,32185,7 and32081,32187,7. F9 05:10:22.125Z confirms both destination tiles and Move requested visibly; request115 returns first trough, F9 05:10:32.347Z confirms32080,32186,7. Actor state changes only through Bridge server event consumption, no speculative movement. moves3->5->6,unsupported/anomalies0. User reports monsters can move too; current session only logged self1002 drag, so separate successful nonlocal-creature push attribution still unverified. Inventory-drop remains failed; character now has4coins and jacket3379 obtained during operator gameplay, not QA seeding.
New explicit operator instruction: give test character weapon,backpack,shovel,rope for UseWith. This authorizes bounded synthetic inventory preparation; no feature/refactor. Planned short sword3294 slot5,backpack2854 slot3 containing3457 shovel and3003rope. Reference runtime types checked read-only; source crplayer.cc Load/SaveInventory and map.cc LoadObjects/SaveObjects delimit nested content. Must stop client/server cleanly, back up1002.usr, preserve all other state and reopen. This preparation is NOT proof of acquisition/parity; subsequent actions still require live authoritative results.


Operator-authorized equipment preparation APPLIED after normal client EndPlay05:24:56.874Z and all three services stopped cleanly. Single save1002.usr backed up as1002.usr.bak-before-operator-qa-equipment-20261015 in same runtime usr directory. Before SHA256d4884a62d62331d481cb2541a591a2a70d02ec61d61208f1a7dbad51d6aa0c18; afterebf7930987ed2ad0e326e74ef7999ee810532312a580bb916c8309759ac3ef31. Added slot3 backpack2854 containing shovel3457 and rope3003; slot5 short sword3294. Preserved jacket3379 slot4,coins3031 Amount4 slot10 and byte-identical remainder(position/stats/quests). One-time guarded script only under ignored build/, verifies stopped ports/processes, exact old inventory, backup and atomic save. No reference/source/production art changed. WSL auto-stopped after service shutdown; restarted and bounded keeper18924 holds distro2h. Server restart in progress; actual received inventory confirmation pending. Session2 EndPlay1620commands,residual/unsupported/anomalies0. Prior player exploration/combat is operator activity, not guided acquisition evidence.
