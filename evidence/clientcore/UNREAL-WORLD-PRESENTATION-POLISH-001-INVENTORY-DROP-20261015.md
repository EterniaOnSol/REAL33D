# UNREAL-WORLD-PRESENTATION-POLISH-001 inventory drop continuation

2026-10-15, Codex implementation and operator-assisted live QA. Overall
IMPLEMENTED_UNVERIFIED. No independent certification or main merge.

## Preserved starting state

Branch milestone/unreal-world-presentation-polish-001; base/main
fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. All pre-existing saved QA evidence and
handoffs were committed before code edits as
1e7a2d5097b81e79c7441d819cbd7d585da36ef5 (parent 6f864e3). Normal push succeeded,
local/remote equality and clean worktree verified. Secret check PASS.
Historical ASSISTED-20261014 and CONTINUATION-20261009 evidence remains intact.

## Scope and root cause

User authorized closing inventory-to-ground dragging, attributable creature
pushing and a compact post-fix regression. Production art, server rules,
protocol framing and unrelated features remain outside scope.

Classification B: pre-existing missing UI routing, not a camera/wall regression.
The inventory milestone tested slot/container drops, not inventory-to-map drops.
The original HUD at 9b0cb83 likewise has no world drop receiver.

Exact path before the fix:

- Real33DPanelChrome.cpp::SReal33DSlot::OnDragDetected creates FReal33DItemDrag
  containing inventory/container slot, type and amount. OnDrop accepts another
  slot, so slot-to-slot works.
- Real33DHUD.cpp::HandleItemDropped only converts inventory/container endpoints.
  MakeViewportFrame has no drop receiver. A drag released on the world stops here.
- PlayerController::BeginWorldDrag/CompleteWorldDrag starts from a world actor;
  it cannot receive a Slate inventory drag.
- Bridge::RequestMoveObject/DrainMoves and ClientCore::BuildMoveObjectCommand
  already support map, inventory and container endpoints. No protocol addition
  is needed.
- Selected Fusion32 game revision 386fa9b8078a1b32187dfcbfc2a0ed7543e16346:
  receiving.cc::CMoveObject parses special source/destination coordinates and
  queues ToDoMove; operate.cc::Move/CheckMoveObject/CheckMapDestination enforce
  authoritative movement. These symbols have no TIBIA772-specific branch.

## Minimal implementation

Four Unreal HUD/controller source/header files changed. HUD recognizes item
drags only within the cached world viewport rectangle and forwards the desktop
drop position. Controller revalidates the received source slot and stack amount,
converts Slate desktop coordinates to viewport pixels, traces a received visible
tile/creature tile on the player's floor, and calls existing RequestMoveObject.
It presents the existing Move requested cue. It does not mutate inventory or
world state locally; authoritative received updates must prove success.

## Checks and live acceptance

- UnrealEditor Win64 Development build PASS, 7 actions, 81.54 seconds.
- Nine native ClientCore suites PASS (crypto, gamelogin, initial_world, login,
  minimap, movement, player_state, transport, worldview); ClientCore unchanged.
- git diff --check PASS.
- Three existing Unreal presentation/authority/catalog suites PASS.
- Inventory removal plus ground appearance: IMPLEMENTED_UNVERIFIED pending live.
- Creature push: IMPLEMENTED_UNVERIFIED; prior rejected NPC attempts do not pass.
- Compact post-fix regression: IMPLEMENTED_UNVERIFIED pending live.
- Protocol counters for the new build: not yet sampled.

Build/test outputs currently under ignored
build/unreal-world-presentation-polish-001/drag-fix-20261015/; retain selected
results and new live PNG/JSON/log evidence before closeout. Compilation is not
live acceptance. Operator performs one requested action at a time.

Prior camera/wall/world speech/Use With/lamp/world drag/minimap/Follow evidence
remains valid for its recorded build and bounds. New compact regression must
exercise the requested paths after this input change. The prior 14 unanswered
walk ledger entries were not silently cleared or called protocol anomalies.

V08 art issues remain documented separately: unfinished separation frames,
cave continuity, wall finishes 373/377, missing outfits and mountain defects.

## Live session 1 and operator-reported pickup defect

Client9972, live-20261015T040507, normal Fusion32 service identities verified.
Baseline01 at10:06:12Z: sword3294 slot5, jacket3379 slot4, coins3031x4 slot10.
Operator performed extra actions between prompts; requests are individually
attributed rather than assumed to match the requested action.

- Request17 moved coins from inventory slot10 into open backpack0.
- Request18 at10:07:12.379Z moved all4 coins from container0 slot0 to
 32096,32218,7. Capture02 proves received container removal and visible3031 there.
 This proves container-to-ground, not direct inventory-to-ground.
- Request40 at10:09:46.469Z directly moved sword3294 from inventory slot5 to
 32097,32219,7. Capture03 at10:10:38Z proves received slot5 removal and visible
 sword there. Jacket3379 also dropped at32096,32219,7. Inventory contains only
 backpack3; no local inventory/scene mutation exists in the request path.
 INVENTORY_GROUND_DRAG PASS for this tested build/action; retest after pickup fix.
- Operator reported inability to pick up the coins. Existing world drag always
 traced the world, even when released over UI. Requests19/20/21 targeted blocked
 map32094,32221,7 behind the panel, and Fusion32 legitimately returned NOROOM.
 This is a client destination-routing defect, not an unmovable coin/server rule.
 Operator's follow-up requests correcting collection; minimal reverse route
 added to the same world-drag handler. It resolves the physical Slate slot hit
 path, sends existing map-to-inventory/container command, and rejects panel
 background/chat drops instead of tracing through them. Creature-to-slot is
 rejected. Existing world-drag quantity remains1; no quantity picker added.
- No push-creature PASS: the initial world drag11 moved the local player1002,
 not a different creature. Do not use that as required PUSH_CREATURE evidence.

Operator closed normally before rebuilding reverse-route fix. Session1 EndPlay,
selected client log, movement journal and screenshots/JSON saved under
evidence/clientcore/unreal-world-presentation-polish-inventory-drop-20261015/.
Unreal build for reverse route running; next normal launch must recover dropped
equipment/coins through received authoritative inventory updates, then compact
regression and legitimate creature push. No server data/rule edits.

## Live session 2: reverse route and final build

Reverse route build PASS11.92s; three existing Unreal suites PASS again. The
first compile attempt failed on a protected Slate API and unsupported range
iteration; changed to public LocateWindowUnderMouse and indexed FArrangedChildren.
No live test used that failed intermediate build.

Session1 ended normally10:12:23.347Z with289commands, residual/unsupported/
anomalies0,18walks accepted,0rejected/unanswered. Self-drag relocation is separate.
Session2 client18884 uses live-20261015T041555, same server runtime, no reset.
Received login map retains the dropped objects. Requests12/13 collect jacket
and sword into backpack;14-17 collect four coins individually using existing
count1 map drag semantics. Requests18/19 re-equip sword/jacket. Capture04
10:17:55.132Z confirms authoritative inventory4=3379,5=3294; backpack has
3031x4,3457,3003. Geometry no longer contains dropped3294/3379/3031, PNG shows
clear floor and restored equipment. Operator confirms collection.100commands,
residual/unsupported/anomalies0,10walks accepted,0rejected/unanswered.

Direct inventory-ground retest on final pickup build and remaining compact
regression/creature push still pending. Full milestone IMPLEMENTED_UNVERIFIED.

Session2 final-build retest: request20 at10:20:29.327Z moved3294 from inventory
slot5 to32096,32218,7. Request21 Look at10:21:27.286Z received InfoMessage
at10:21:27.366Z describing short sword Atk11/Def11,35.00oz. Capture05
10:21:55.668Z proves empty slot5, sword present at destination and chat result.
INVENTORY_GROUND_DRAG and LOOK_SMOKE PASS for final build.224commands,
residual/unsupported/anomalies0;10accepted/0unanswered walks. Drop does not
optimistically alter received state. Further smoke remains pending.

Request22 at10:22:28.481Z moves sword3294 from32096,32218,7 to32097,32219,7.
Capture06 at10:22:57.230Z shows it only at the new tile: WORLD_DRAG PASS.
Operator then right-used sword (request23); legitimate server rejection
"You cannot use this object" at10:23:44.760Z is not a protocol anomaly.
Request24 at10:23:46.570Z collects sword directly into inventory slot5;
operator confirms equipped (received snapshot to follow).

Post-fix compact regression tracking, same session2 connection:

| Check | Current evidence/status |
| --- | --- |
| Login/manual movement |10 accepted walks,0unanswered; capture04 |
| Look | PASS, request21/capture05 |
| Use/container | PASS, request11 opens backpack; capture04 contents |
| Use With | IMPLEMENTED_UNVERIFIED in this connection |
| World drag | PASS request22/capture06 |
| Inventory-ground | PASS request20/capture05 |
| Follow/Stop | in progress;27 Follow causes authoritative relocation |
| Minimap click | IMPLEMENTED_UNVERIFIED in this connection |
| WideWorld | received loads/unloads during movement, final capture pending |
| Speech | IMPLEMENTED_UNVERIFIED in this connection |
| Light toggle | IMPLEMENTED_UNVERIFIED in this connection |
| Camera/wall | temple readable, obstruction case pending |
| Creature push | IMPLEMENTED_UNVERIFIED |

Do not infer full GAMEPLAY_REGRESSION PASS from this partial table.

Follow/Stop closed: request27 at10:24:57.322Z follows Cipfried1073741889;
received relocation32097,32218->32217 at10:24:57.460Z with no pending walk.
Capture07 at10:27:04.820Z shows blue Follow feedback/target and sword equipped
in slot5 (also closes direct ground-to-inventory pickup). Operator toggled
Follow multiple times; do not attribute their extra clicks to a single prompt.
Request34 Escape/general cancel10:28:20.512Z clears target at10:28:20.671Z.
Capture08 at10:28:49.400Z proves target0/no blue mark;449commands,protocol0,
11accepted/0unanswered walks. FOLLOW_SMOKE/STOP_FOLLOW_SMOKE PASS.

Real Say hi at10:30:21.070Z, Cipfried reply10:30:22.061Z. Say heal10:30:50.860Z
elicits normal scripted healing (HP41->65) and reply10:30:51.876Z. Captures09/10
show player and NPC speech in world. Existing live script cipfried.npc has
HP<65 healing rule; no script/data edited. Capture11 at10:32:41.718Z shows
speech expired and full transcript retained in chat. WORLD_SPEECH/NPC_SPEECH/
CHAT smoke PASS. Usual first-frame automatic speech screenshots precede chat
widget refresh; the later F9 independently proves chat presentation.

Walk to32097,32214,7 completes10:31:57.994Z. Capture11 shows player readable
outside tall temple walls, upper floor restored, cutaways0, actual/requested
zoom1293.5 equal. Indoor/outdoor camera-wall regression case PASS; original
genuine collision/obstruction evidence remains retained, not independently
recertified. WideWorld movement remains visible with static surrounding city.

## Operator-requested commit/push checkpoint, 04:36 local

User explicitly requested immediate commit/push because remaining tokens were
limited. End further live QA and preserve current honest state; no local PASS.
No independent certification or main merge. Remote before closeout still
1e7a2d5; main still fa3adcf, no divergence.

Request40 at10:33:49.710Z uses lit lamp2109 at32094,32214,7. Fusion32 moves
player near it and changes lamp to2108. Capture12 at10:34:50.043Z proves the
unlit upright lamp, light removed and player readable beside tall wall.
LIGHT_TOGGLE smoke PASS (on->off here; historical full cycle retained).
WideWorld at this capture loads69/unloads7/sectors62; authoritative minimap
marker matches32095,32214,7. WIDEWORLD_SMOKE PASS. This is not minimap-click
arrival: no map click occurred in this final connection.

REAL33D18884 closed normally. Final session2 EndPlay10:35:41.478Z:
640commands, residual0, unsupported0, protocol anomalies0;14walks requested/
accepted,0rejected/0unanswered;4external relocations correspond to Follow and
server approach for lamp use. Inventory backpack3/jacket4/sword5; backpack
coins4/shovel/rope; player32095,32214,7 with65HP. No equipment lost. Existing
Fusion32 services remain running; no server data/rule edits in this continuation.

Final bounded results:

| Result | Verdict |
| --- | --- |
| INVENTORY_GROUND_DRAG | PASS, direct sword and jacket; server removal + world appearance |
| ROOT_CAUSE | B: pre-existing missing inventory/container-to-world UI drop receiver |
| FIX | Minimal HUD/controller routing plus operator-reported reverse pickup routing |
| Ground pickup to container and inventory | PASS, received inventory/ground updates |
| Look, Use/container, world drag, Follow/Stop | PASS in final build |
| World/NPC speech and chat, lamp, camera/wall, WideWorld smoke | PASS in final build |
| PUSH_CREATURE | IMPLEMENTED_UNVERIFIED; no attributable other-creature push tested here |
| GAMEPLAY_REGRESSION | IMPLEMENTED_UNVERIFIED; final-build Use With and minimap-click arrival not exercised |
| UNSUPPORTED_OPCODES / PROTOCOL_ANOMALIES / PROTOCOL_ERRORS | 0 / 0 / 0 |
| UNREAL_WORLD_PRESENTATION_POLISH | IMPLEMENTED_UNVERIFIED |
| READY_FOR_INDEPENDENT_CERTIFICATION | NO |

Earlier Use With/minimap arrival passes remain retained at6f864e3; they do not
replace the requested post-fix compact checks. No acceptance criterion weakened.
The last original session's14unanswered ledger entries stay historical; this
new final session has0, not a reset or retroactive correction of that record.

Checks: nine ClientCore suites PASS; final Unreal build and three existing
automation suites PASS. First failed intermediate pickup compile documented
above; corrected final build used in all session2 tests. Startup handled
FAppTime thread-context ensure still appears, separate from protocol counters;
no claim of an entirely clean engine log. Final staged diff/JSON validation and
secret_check must pass before the requested preservation commit/push. Source
changes are five Unreal files only; reference, ClientCore, production art and
server rules untouched. Commit containing this closeout is the implementation
checkpoint; final git equality/clean checks are returned to the operator.

Final closeout checks executed: staged diff --check PASS; all28 JSON artifacts parse; secret_check PASS (tracked tree plus reachable history and immutable reference check). Saved secret-check-final.txt. No live acceptance promotion.
