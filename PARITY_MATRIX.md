## 2026-10-21 live connection resume

REAL33D independently reconnected to the normal Fusion32 runtime at implementation 1abb013. EndPlay: 46.6 seconds, 57 server commands, residual/unsupported/anomalies=0; moves/uses/looks=0. Computer Use native pipe still unavailable after retry/reset, so all requested gesture rows remain UNVERIFIED. No implementation changes, deterministic reruns, main merge or fabricated data. Evidence: evidence/clientcore/unreal-stack-move-controls-independent-20261021/README.md (resume section). Certification remains BLOCKED; READY_TO_MERGE_MAIN=NO.

## 2026-10-21 independent UNREAL-STACK-MOVE-CONTROLS-001 review

Independent live certification is BLOCKED: the Windows Computer Use native pipe is unavailable after retry and session reset. No new live gesture was exercised; all requested live rows remain UNVERIFIED. Candidate 1abb013bf711978b9439b6abe836672b5f4148ac was normally pushed and reviewed in a fresh isolated worktree; main was not merged. No implementation change. Independent ClientCore 14/14, native 7/7 and Git Bash secret_check 6/6 PASS. Build/automation details and explicit limits: evidence/clientcore/unreal-stack-move-controls-independent-20261021/README.md. READY_TO_MERGE_MAIN=NO.

UPDATE 2026-10-21 UNREAL-STACK-MOVE-CONTROLS-001 PASS (branch milestone/unreal-stack-move-controls-001 from fbc0de99; no merge, no push, no protocol/server change). Classic stack-movement UX on all four drag routes: plain drag = whole stack, Shift+drag = one, Ctrl+drag = ask (clamped 1..stack count), Cancel sends nothing. One decision point, Real33DStackMove.h, proven by a 7th native suite and by REAL33D.Presentation.StackMoveControls. Before this a slot drag always moved the whole pile with no way to ask for fewer, and a GROUND drag always sent count 1. CONTAINERS/ITEMS rows gain: partial-amount moves are live and authoritative. Two defects fixed en route -- a modifier HELD BEFORE THE PRESS never reached the drag (Shift's Look branch and Ctrl's inspector branch both returned before arming it, so Shift+drag silently moved the whole stack), and STACK SPRITE TIERS had never loaded because the delivered art is <id>_p<n>.png while ItemBrush looked for <id>_<n>.png, so every cumulative object drew its single-item picture at every amount -- the exact defect Real33DStackSprite.h exists to fix, still live until now. Live: 9 moves, counts 12/1/2/1/1/1/1/1, clamp held at both ends, cancel sent nothing (moves_requested unchanged), 688 commands decoded, residual/unsupported/anomalies all 0. ITEM_STACK_SPRITE_2D now PASS for amount tiers; ITEM_STACK_MESH_3D stays NOT_IMPLEMENTED -- the asset registry resolves a mesh by type id with no amount tier, so twenty coins on a tile still look like one, which remains the accepted world-mesh amount-invariance boundary recorded in the 2026-10-15 stackable certification. Container-to-container was proven between two slots of one open bag (both endpoints genuine container coordinates) and not between two distinct container numbers; no second bag existed on the account. See evidence/clientcore/UNREAL-STACK-MOVE-CONTROLS-001.md.

UPDATE 2026-10-21 UNREAL-OUTFIT-COLOR-RENDERING-001 = BLOCKED_VISUAL_ASSET on branch milestone/unreal-outfit-color-rendering-001 from origin/main 25c4448b; no main merge, no push. NO RENDERING WAS IMPLEMENTED and that was the task's own instruction: stop if the assets cannot represent the four 7.72 colour regions faithfully. They cannot. NO PRODUCTION SOURCE CHANGED -- Real33DBridge, Real33DOutfitPanel, Real33DOutfitSelection, Real33DCreatureActor and Real33DAssetRegistry are untouched, so the certified dialog and transport are preserved exactly. THE PALETTE HALF SUCCEEDED AND IS DELIVERED. Fusion32 has no palette at all -- it validates Colors[n] > 132 and stores the byte -- so the authority is the classic 7.72 client read statically, never executed. It holds no colour table: it COMPUTES every swatch. tests/verify_outfit_palette.py reproduces that computation and asserts its structure in 51 checks against the hashed image. The palette loop at 0x0047D380 builds exactly 133 swatches with control id 2000+i, each colour from 0x00509280, which splits the index as row = index/19 and col = index%19, sends col == 0 to greyscale with v = 1 - row/7, and otherwise takes h = col/18 with a per-row (s,v) pair read from the seven cases at 0x0050930F..7B: (0.25,1.0) (0.25,0.75) (0.5,0.75) (0.667,0.75) (1.0,1.0) (1.0,0.75) (1.0,0.5). 0x00509080 then converts HSV to RGB over six 1/6-wide sectors with the low channel (1-s)*v, rounding each channel after multiplying by 255. ALL SIX SECTORS WERE DECODED INSTRUCTION BY INSTRUCTION, not inferred from the constants: they turn out to be textbook HSV, but that was the conclusion, and two of them enter the shared tail one instruction later to skip an fstp. 19*7 = 133 agrees independently with receiving.cc's 0..132. Corroboration nobody arranged: CheckOutfit's defaults 78/69/58/76 decode to #FFAA00 orange hair, #4040BF blue shirt, #BF6A40 brown trousers and #6D6D6D grey boots; index 0 is pure white, the first column descends as greyscale, and row 4 contains all six primaries and secondaries exactly. PALETTE_VERIFIED. THE ASSETS ARE THE BLOCKER, systemically. tests/audit_outfit_asset_regions.py audits any delivered creature glb; over all 16 in the brother delivery: ZERO have any feet, boots or ankle geometry, and ZERO carry a region mask -- no COLOR_0, no TEXCOORD_1, zero textures, zero images, no glTF extensions. So the Feet byte has nothing to tint and a region can only ever be a whole node, never an area inside one. Outfit 128 is the ONLY outfit in either player band with art; its eight nodes are Blender primitives (head Icosphere, torso Icosphere, tunic Cube, two arm Cylinders, pelvis Icosphere, two leg Cylinders) and its five materials are named by TONAL VALUE and shared across body parts -- 128_dark spans leg.L, leg.R and pelvis -- so a material slot is not a region either. head is one undifferentiated sphere including the face, while 7.72's Head byte tints hair only; and Body is ambiguous between tunic (garment) and torso/arms (skin), which the asset does not record. 129, 130, 131 and the whole 136-139 series have no art at all and fall back to a SINGLE CYLINDER, which is exactly the global whole-character tint the task forbids -- and 136-139 is the QA account's own band. The catalog is also opt-in via -real33d-creature-catalog= and was not loaded in the certified runs, and visual/qa/brother_creatures marks the delivery APPROVED_PILOT_REFERENCE, which is not artistic approval. TO UNBLOCK: delivered art needs a feet/boots node, hair separated from head, an explicit garment/skin split, OR a documented four-value region mask the way the classic client does it with a template sprite; plus mesh coverage for 129/130/131/136/137/138/139. LIVE QA WAS NOT ATTEMPTED and is not claimed: every step presupposes rendering that does not exist, so it could only have photographed an untinted placeholder. ClientCore 14/14, native 6/6, Unreal automation 8/8 errors0 warnings0, MSVC, REAL33DEditor build, colour-mapping 43/43, palette 51/51, asset audit exit 3 INSUFFICIENT as expected, harness 11PASS/0FAIL/0SKIP, secret_check 6/6 -- these confirm the tree is unbroken, not that anything new renders. UNVERIFIED: the palette is verified against the client's own computation but NOT against pixels it actually drew, because that needs the binary executed and AGENTS.md forbids it; and how the 2D client combines a palette colour with its template's marked areas was not investigated. COLOR_VISUAL_RENDERING stays NOT_IMPLEMENTED. See evidence/clientcore/UNREAL-OUTFIT-COLOR-RENDERING-001.md and CURRENT.

## 2026-10-20 independent UNREAL-OUTFIT-DIALOG-001 certification

CERTIFIED for dialog/byte transport at corrected revision
`61ceaa955933672c526864b5ed24dca0e4c8de84`, isolated branch
`certification/unreal-outfit-dialog-001-20261020`. Requested `56bf5085` was
pushed normally and independently tested. It passed the functional flow but
had ten blank arrow buttons: UE 5.8 padding consumed their entire width.
The nine-line padding correction is local on the certification branch;
the published milestone is preserved at `56bf5085`. No main merge;
main/origin/main remain `b8e7433baeddc2875943f519d33a5cb9380f3eaa`.

Corrected live run: 90 executed assertions PASS, four GET/200 exchanges,
two SET/142 updates, exact 136..139 band, all four colour bytes changed and
restored, server reopening reproduces them, Cancel sends nothing, ten registry
surfaces, channel hit testing and backpack/Look regression PASS. Original
server outfit restored. All protocol counters zero. Closed colour verifier
43/43 proves Head/Body/Legs/Feet. ClientCore14/14, native6/6, production Unreal
automation9/9, separate converter fixture PASS, MSVC/build PASS, final harness
9PASS/0FAIL/1 optional-build SKIP (separate Unreal build PASS), secret_check6/6.

COLOR_VISUAL_RENDERING = NOT_IMPLEMENTED. Object disguise bridge fixture PASS;
live disguise unverified and object disguise rendering NOT_IMPLEMENTED.
Other live bands, refusal/GM paths and OS input injection remain unverified.
The centred dialog preserves background hit testing; no globally blocking
modal claim. Full outfit visual parity is not certified. The original
ClientCore integration markers remain CLIENTCORE_OUTFIT_772 = PASS_LAYER and
LIVE_CERTIFICATION = NOT_DONE.

Evidence: `evidence/clientcore/unreal-outfit-dialog-independent-20261020/README.md`.

---

UPDATE 2026-10-20 UNREAL-OUTFIT-DIALOG-001 PASS LOCAL on branch milestone/unreal-outfit-dialog-001 from origin/main b8e7433b; no main merge, no push. The real 7.72 outfit chooser on the ClientCore outfit protocol: request, server-opened dialog, server band obeyed, outfit preview, four colour bytes, submit, and close without the acknowledgement CSetOutfit never sends. No ClientCore, protocol or server change; suite count stays 14. THE FOUR COLOUR SLOTS ARE NOW NAMED AND THE NAMES ARE PROVEN, not assumed: Fusion32 declares uint8 Colors[4] and names nothing, so the mapping was taken from the classic 7.72 client statically, never executing it. tests/verify_outfit_color_mapping.py asserts 43 byte patterns in the hashed Tibia.exe across six links -- the four-entry jump table at 0x0047D648 whose cases are literally Head/Body/Legs/Feet in index order, row i taking control id 1000+i and storing i as the selected slot, the swatch store colours[slot]=id-2000, SV_CMD_OUTFIT loading colours[0..3] in wire order, OK passing &colours[0], and the sender writing opcode 211 then colours[0..3] ascending. So Colors[0..3] = Head, Body, Legs, Feet. It is also a harness check that SKIPs rather than passes when the ignored client artifact is absent. TWO BRIDGE GAPS FIXED STRUCTURALLY: both places that copied an OutfitDescriptor across the engine boundary copied two of its four fields and silently dropped the four colour bytes and a disguise's object_type on every SV_CMD_CREATURE_OUTFIT and every FULLSCREEN creature. ClientCore had them all along. FReal33DOutfit now carries the whole descriptor, one ToUnrealOutfit converts, and three static_asserts pin the colour count, slot count and the 132 bound. Entry point is the classic client's own documented gesture, from its help text at 0x00565734: right-click your character and select 'Set outfit'. That right-click was a no-op before. Panel registered as PanelIds::Outfit, a centred modal; HUD edits are +80 lines across two files with no deletions. LIVE, one real account on REAL33D, operator-driven, nothing clicked by software. TWO RUNS, because the first found a defect in this milestone's own UI: it obeyed the band and got both authoritative updates, but both submits carried 78,69,58,76, the server's own CheckOutfit defaults, because each colour row's LABEL was a button that only highlighted the row -- copied from the 2D window where the rows select which one a shared palette edits, while this window gives every row its own arrows. Nothing logged a colour step either, so 'not pressed' and 'broken' were indistinguishable in evidence. Label is now plain text, arrows are wider, and every preview step logs its before/after value and the whole tuple. RUN 2 COMPLETE: 3 requests -> 3 server-opened choosers, band=136..139 (4 outfits) every time, outfit previewed 137->136->139 wrapping at First and staying in band, 47 colour steps across all four slots (12/12/11/12), 3 submits, 3 authoritative SV_CMD_CREATURE_OUTFIT updates for creature 1002, 0 selections refused, and residual_bytes/unsupported_opcodes/protocol_anomalies all 0. THE REOPEN LINES ARE THE STRONGEST EVIDENCE: submit 77,67,56,73 reopened as current=139 colours=77,67,56,73 and submit 76,66,55,72 reopened as 76,66,55,72 -- exact, twice -- which proves in one line that 211 encoded the colours, Fusion32 stored all four, 200's decode carries them back, and the colour bridge fix works, since before it those bytes would have returned 0,0,0,0. ClientCore 14/14, native 6/6, Unreal automation 8/8 errors0 warnings0, MSVC, REAL33DEditor build 0 warnings, harness 10PASS/0FAIL/0SKIP, harness selftest 10/10, secret_check 6/6. Harness corrected twice: the explicitly-named native suite list had to gain the new suite, and the new python check selected its interpreter with command -v python3, which on Windows resolves to an App Execution Alias that exists, prints 'Python was not found' and exits 49 -- it now probes by running the interpreter. UNVERIFIED: no outfit refusal occurred live, locally or server-side, because the controls cannot produce one, so the 0 refusals is not a handled refusal; the GAMEMASTER_OUTFIT path that answers with a message and no chooser was unreachable without editing server data; the colours change nothing on screen because the palette's byte-to-RGB mapping lives in the client .dat/.spr and is NOT derived here, so the chooser shows numeric indices and no fabricated swatch; the palette's true size is unestablished (132 is the server bound, the client's swatch guard spans 216 control ids and is not a count); no object disguise was seen live; only the 136 band on one character, so the 128 series and the premium +3 were not seen live. One server recovery recorded: a stale game.pid naming dead PID 562 blocked startup and was MOVED ASIDE, not deleted, with no server data edited. See evidence/clientcore/UNREAL-OUTFIT-DIALOG-001.md and CURRENT.

## 2026-10-20 independent UNREAL-TRADE-UI-001 certification

CERTIFIED for the requested bounded scope at implementation
`1cf1ec4682eb17df6df8e576da1c822a915e63f4`, isolated branch
`certification/unreal-trade-ui-001-20261020`. Implementation preserved by normal
push; main remains `76a261992c6740ecfc612b5421dd91391a06c4d5`, no merge.
No production changes. Two real Fusion32 connections independently prove actual
Ctrl/right -> Trade with -> player, own/partner offers, both inspections,
accept waiting/completion, own Reject, partner cancel and authoritative ownership
transfer from BOTH received inventories/containers. Reciprocal return restores
original ownership. Nine registry surfaces and Look/Use/UseWith, Channels,
Local Chat, containers and EDIT_TEXT display/Close regression PASS. The trade run
was interrupted after these assertions; retained resume run revalidated ownership,
returned objects and finished PASS. No uninterrupted-run claim.

ClientCore13/13, native5/5, original-source Unreal build PASS, Unreal automation8/8
errors0 warnings0, corrected harness9PASS/0FAIL/0SKIP, selftest10/10,
Git Bash secret_check6/6; both connections protocol counters0. UNVERIFIED:
accept-time failures, multi-object/container offers, offer-time refusals,
success/failure from CLOSE_TRADE alone. Existing EDIT_LIST remains UNVERIFIED;
text submit/persistence not rerun in this bounded smoke. Detailed independent
report: `evidence/clientcore/unreal-trade-ui-independent-20261020/README.md`.

---

UPDATE 2026-10-20 UNREAL-TRADE-UI-001 PASS LOCAL on branch milestone/unreal-trade-ui-001 from origin/main 76a26199; no main merge, no push. Real 7.72 player trade: start, both offers, inspect, accept, reject, and close on SV_CMD_CLOSE_TRADE. No ClientCore or server change; trade.cpp builders and DecodeTrade used as they stand. Entry point is the authentic gesture the operator named: ctrl+right-click an item -> Trade with... -> click the player. Panel registered as PanelIds::Trade; HUD edits confined to two files. Live with TWO REAL ACCOUNTS, Test Player A on the original Tibia.exe and Test Player B on REAL33D: 9 context menus, 7 offers dispatched and 7 own offers returned 1:1, 7 partner offers, 2 inspects, 4 accepts, 2 rejects, 5 server closes, 0 trade refusals, 0 undecoded commands and 0 decode failures. Both halves of the server asymmetry observed: A's first offer reached B only as the prose 'Test Player A wants to trade with you', and the second offerer's Trade fired all three SendTradeOffer calls at once. Inspect proved the inverted TradeOfferSide byte: own index 0 answered 'a jacket (Arm:1)' and partner index 0 answered 'cheese'. Reject from this client produced NO server close, confirming RejectTrade answers only the partner; A's reject produced a real close plus 'Trade cancelled.' on B. AUTHORITATIVE_TRANSFER proven two ways, neither of them the close: Fusion32's saved character file shows A gained an object of type 3379 it held nowhere at baseline, and B was seen offering object 3561 which was A's at baseline with the server itself describing it as a jacket on B's own side. B's saved inventory returned to its baseline type composition, consistent with items cycling back across three reciprocal trades; the per-trade ledger was NOT reconstructed and is not claimed. One design defect found live and fixed: the Offer control sat inside a window that only appeared once a trade existed, so the player A offered to had no way to respond at all; reading the server's English sentence to infer a pending trade was refused as invention, and the item context menu replaced it. ClientCore 13/13, native 5/5, Unreal automation 8/8 errors0 warnings0, MSVC, REAL33DEditor build, harness 9PASS/0FAIL/0SKIP, Git Bash secret_check 6/6. Which close meant success is UNVERIFIABLE by design; the accept-time failure closes, multi-object offers and the offer-time refusals were not seen live. NPC shops out of scope. See evidence/clientcore/UNREAL-TRADE-UI-001.md and CURRENT.

## UNREAL-EDITORS-UI-001 — integrated status (authoritative)

Integrated into main on 2026-10-20 with partial certification. These are the
statuses that travel with the code; prose elsewhere elaborates but does not
override them.

    EDIT_TEXT                = CERTIFIED
    EDIT_LIST_DISPLAY        = IMPLEMENTED_UNVERIFIED
    EDIT_LIST_SUBMIT         = IMPLEMENTED_UNVERIFIED
    OVERALL_EDITORS_STATUS   = PARTIALLY CERTIFIED

`PARTIALLY CERTIFIED` is a composite label for the milestone and is not one of
the seven states in `AGENTS.md`; the two components above are, and they are
what each half of the feature actually holds.

**EDIT_LIST has never run against a live server.** Three gates on this server,
each sufficient on its own, re-verified during independent certification:

1. `game/reference/dat/owners.dat` is **0 bytes** — no character owns a house,
   and `houses.cc::ShowGuestList` throws NOTACCESSIBLE unless `IsOwner` or a
   premium subowner.
2. `magic.cc::EditGuests`, `EditSubowners` and `EditNameDoor` each require
   `GetHouseID(posx, posy, posz) != 0`, so the player must stand in a house.
3. Every house in `houses.dat` is mainland (x 32258-32272, y 32308-32320);
   both test characters are on Rookgaard, which has none.

No house ownership, character or server test data was fabricated to close
this, and none may be without explicit operator authorization. What is covered
offline: the ClientCore decoder and both builders, the window's naming,
empty-attribution and submit rules including the refusal of a list whose type
the server never named, and the three `enums.hh` values pinned by
`static_assert`.

UPDATE 2026-10-20 UNREAL-EDITORS-UI-001 independent certification: TEXT EDITOR scope CERTIFIED, milestone overall PARTIALLY CERTIFIED because EDIT_LIST stays IMPLEMENTED_UNVERIFIED. Candidate a4f2cb5a300c4bd3440d4c824200d7134618d24d preserved by a normal push to origin/milestone/unreal-editors-ui-001 after Git Bash secret_check, remote had not diverged; isolated worktree build/cert-editors-ui-20261019 created from exactly the candidate on certification/unreal-editors-ui-001-20261019. Main remains 18b587df, no merge. Candidate touches no clientcore, reference, scripts or tests; editors.cpp/.h are still at their own milestone commit 20c22cd. Source review confirmed the three load-bearing claims first-hand: no writability inference anywhere in the window, Submit never claims an edit was kept, and three static_asserts pin the list bytes to enums.hh. Independent live run with a temporary opt-in driver against the PRODUCTION Slate widgets (SimulateClick on real buttons, values read off painted STextBlocks and the real text box): 52 assertions, 0 failures, EDITORS_CERT_RESULT=PASS. Verified the window is registered and collapsed until Fusion32 opens it; painted object id 19866777, type 2815 and the 511 limit all match the decoded command and 511 is MAXLENGTH 512-1; attribution correct before and after; one CL_CMD_EDIT_TEXT on the wire per Submit; persistence proven by re-reading the object and finding the new text with EDITOR set to this character, which is the only possible proof since EditText answers success with silence; an oversized text reached the wire and was answered 'There is not enough room.' with the object keeping the accepted text; the modal is SelfHitTestInvisible and the Channels button was clicked THROUGH it for a real four-row server round trip; eight registry surfaces all asserted by name. Container, Channels and Local Chat regressions PASS. Protocol residual/unsupported/anomalies 0. ClientCore 13/13, native 5/5, Unreal automation 7/7 errors0 warnings0, MSVC, exact-candidate build with the driver removed (UBT logged the removal; diff against the candidate over Source/scripts/clientcore/tests is empty), selftest 10/10, harness 9PASS/0FAIL/0SKIP, secret_check 6/6. Two reviewer-side harness failures are recorded, not hidden: a C:/ --objects-srv path WSL could not translate, and a mis-copied 2026-09-22 evidence file already in the repo whose SV_CMD_LOGIN_ERROR diagnostic predates this milestone. EDIT_LIST live blocker re-verified independently and NOT worked around: owners.dat is 0 bytes, the three house spells need GetHouseID()!=0, and all houses are mainland while the test characters are on Rookgaard; EditListsOpened and EditListsSubmitted were 0 in the live run. No house ownership or test data was fabricated. No implementation defect found and no implementation line changed. See evidence/clientcore/UNREAL-EDITORS-UI-001-INDEPENDENT-20261019.md and CURRENT.

UPDATE 2026-10-19 UNREAL-CHAT-CHANNELS-001 independent certification CERTIFIED for the requested bounded scope. Exact candidate 239f89f07430ecd4d4348d69f183a9935e4bd992 preserved by normal milestone push before creating isolated certification/unreal-chat-channels-001-20261018. Fresh live Fusion32 session: 58/58 assertions; four server channel rows; Join only after OPEN_CHANNEL; Leave button and tab close each verified by subsequent messages no longer reaching the departed peer; separate Game/RL/Local transcripts; private delivery A->B and B->A; seven real PanelRegistry panels. Nine snapshots protocol0, both peers residual/unsupported/anomalies0. ClientCore13/13, native5/5, MSVC, exact-source Unreal build and automation6/6 PASS, selftest10/10, corrected strict harness9PASS/0FAIL/0SKIP, Git Bash secret6/6. No implementation defect or change; temporary UI driver retained only as evidence. Own-channel/guild/GM/request-queue remain explicitly UNVERIFIED; additional bounded limitations documented. Main remains ca3121c, remote milestone239f89f, no main merge. See evidence/clientcore/UNREAL-CHAT-CHANNELS-001-INDEPENDENT-20261018.md and CURRENT.

UPDATE 2026-10-17 HUD registry independent certification CERTIFIED: current-main611939af plus older implementation64aa86a merged without overlap/conflicts in isolated certification/unreal-hud-panel-registry-001-20261016. Actual six-panel registry automation and existing polish suites5/5 PASS; fresh live HUD/backpack gold5 and worms6 compatibility PASS. ClientCore13/13, native4/4, selftest10/10, MSVC, real Unreal build, Git Bash secret6/6 PASS. Corrected strict harness9PASS/0FAIL/0SKIP;1137live commands and all5snapshots protocol0. Production implementation unchanged; main/implementation branches intact. Certification branch only; no main integration. See UNREAL-HUD-PANEL-REGISTRY-001-INDEPENDENT-20261016 and CURRENT.

UPDATE 2026-10-15 stackable independent certification CERTIFIED: candidate 68f2ffe, isolated certification/unreal-stackable-sprite-patterns-001-20261015. Fresh extraction verified 432 cumulative frames/81 types and 4832 single-image non-stackables; gold x1/frame0 and x12/frame5 live in inventory/container, legitimate Obi worms x6/frame4, authoritative counts unchanged, actual Slate missing/malformed fallback PASS. ClientCore 13/13, native 4/4, MSVC, Unreal, six secret checks PASS. Final harness 9 PASS/0 FAIL/0 SKIP; all 51 live snapshots protocol0. Implementation unchanged; world mesh amount invariance is the accepted boundary. No push/main merge. See UNREAL-STACKABLE-SPRITE-PATTERNS-001-INDEPENDENT-20261015.

UPDATE 2026-10-15 16:10: polish milestone CERTIFIED independently at bc9ce8fd. Seventeen named runtime criteria all reproduced live by the reviewer in one new connection, not carried over from prior sessions; each row in the report cites its own request id, server answer and received-state snapshot. PROTOCOL_ERRORS/UNSUPPORTED_OPCODES/PROTOCOL_ANOMALIES=0 over30 snapshots and293 scanned logs. Push creature confirmed only on request447 where the snapshot1.64s later holds creature1073743167 on the exact requested tile; earlier push attempts were authoritative refusals and are retained as such. Creature readability is bounded: outfits136/57 stayed documented placeholders, outfits21/128 loaded delivered models. Camera obstruction is detect-only by design, cutaways always0. No art approval, no main merge. See INDEPENDENT-20261015 report.
UPDATE 2026-10-15 14:52: polish local runtime acceptance PASS (PASS LOCAL, not CERTIFIED). USE_WITH feedback/result, MINIMAP_CLICK and PUSH_CREATURE PASS from normal Fusion32 actions on3ace436; all compact GAMEPLAY_REGRESSION checks now PASS with retained same-build evidence. EndPlay protocol/unsupported/anomalies0. Build/nine ClientCore/three Unreal suites PASS. Known V08 issues retained; newly reported grass/roof alignment cause UNVERIFIED, no art approval implied. Surface-only static WideWorld underground limitation documented, manual closer view confirmed playable. No independent certification/main merge; see FINAL-QA-20261015 and CURRENT.
UPDATE 2026-10-15 04:36 preservation: INVENTORY_GROUND_DRAG PASS with Fusion32 removal/ground appearance; reverse pickup to inventory/container also PASS. Root cause B: missing pre-existing UI routing, minimally completed. Final live session640 commands, unsupported/anomalies/residual0,14accepted/0unanswered walks. Look/Use/container/world drag/Follow+Stop/speech/lamp/camera-wall/WideWorld smoke PASS. PUSH_CREATURE and complete GAMEPLAY_REGRESSION remain IMPLEMENTED_UNVERIFIED; final-build UseWith and minimap click pending. User requested immediate commit/push. Overall IMPLEMENTED_UNVERIFIED, no certification/main merge. See INVENTORY-DROP-20261015 report.
UPDATE 2026-10-15 inventory drop continuation: saved QA committed/pushed at1e7a2d5 before code. Root cause B, pre-existing missing inventory-to-world UI receiver; existing ClientCore/server move path already supports it. Minimal HUD/controller routing implemented, Unreal build and nine ClientCore suites PASS; live inventory removal/ground appearance, creature push and compact regression pending. Overall IMPLEMENTED_UNVERIFIED. See INVENTORY-DROP-20261015 report.
UPDATE 2026-10-15 03:38 assisted QA: UseWith feedback/smoke, minimap arrival, Follow/Stop, local/NPC speech now live evidenced; camera/walls/Look/lamp/world-drag PASS retained. Final session1284 commands, unsupported/anomalies/residual0. Inventory->world drop FAILED; successful creature push and full regression remain IMPLEMENTED_UNVERIFIED. Client closed normally; no code change, certification, merge or local PASS. See ASSISTED-20261014 report and CURRENT.
UPDATE 2026-10-15 03:18 operator QA: UseWith smoke/feedback PASS from authoritative bucket fill/spill, captured result and explicit operator cursor/banner confirmation. Equipment received and backpack opened. Session3 EndPlay1478 commands with residual/unsupported/anomalies0. Inventory->world coins FAILED; push attribution/full regression pending. Overall IMPLEMENTED_UNVERIFIED. Same6f864e3 client relaunched for manual QA; see ASSISTED-20261014 report.
UPDATE 2026-10-14 operator QA: camera/walls/Look/lamp toggle/world-world drag PASS with captures. Inventory->world drop FAILED; UseWith/push attribution/full regression pending. Operator explicitly authorized synthetic equipment setup; saved backup then added sword/backpack/shovel/rope, preserving gear/coins and other state. Server restart/received inventory check pending. No implementation changes. See ASSISTED-20261014 report.
UPDATE 2026-10-14 21:53 checkpoint: build/three suites/secret_check PASS; fresh login screenshot proves lit2109 uses upright2108 mesh. Live toggle/drag pending: window activation timed out twice, input stopped. Full milestone IMPLEMENTED_UNVERIFIED; no certification/main merge. See RESUME-20261014 report.
UPDATE 2026-10-14 21:48: lamp/world-drag build PASS, three Unreal automation suites PASS, secret_check PASS. Live lamp/drag verification pending; overall IMPLEMENTED_UNVERIFIED. Accepted house restoration retained.
UPDATE 2026-10-14 restoration: HISM cluster refresh fixes incomplete exterior/distant scenery after visibility changes; build and three suites PASS,3652 current trees in live F9, operator accepted. New authorized lamp2109->2108 geometry/light and world drag fixes implemented, build/live verification pending. Overall IMPLEMENTED_UNVERIFIED;22 unanswered walks in operator session prevent full gameplay PASS.
UPDATE 2026-10-14 live temple roof regression PASS: same original player tile and zoom now readable with current-floor walls intact; outdoor roof restoration and new authoritative minimap arrival verified. Cutaways0; latest724 commands have residual/unsupported/anomalies0. Full camera/wall/Use With/gameplay acceptance remains IMPLEMENTED_UNVERIFIED. See RESUME-20261014 report.
UPDATE 2026-10-14 afternoon: rejected no-cutaway version was NOT committed/pushed. F9 reveals temple roof two floors above player bypassed immediate-floor cover detection. UpdateFloorVisibility now checks all received overhead tiles in the same column. Build and three automation suites PASS; live roof check pending. No automatic camera approach/proximity cutaways restored. Overall IMPLEMENTED_UNVERIFIED; no certification/main merge. See RESUME-20261014 report.
UPDATE 2026-10-14 resumed polish: operator authorized removing BOTH automatic camera retraction and proximity cutaways. UpdateCamera keeps manual zoom; live/static MeasureCameraObstruction is diagnostic-only and no longer hides meshes, changes collision or zeroes instance transforms. Floor/roof and authoritative-window rules remain. Camera/wall readability on this changed policy is IMPLEMENTED_UNVERIFIED pending new live QA; earlier cutaway PASS is historical, not acceptance of the new policy. New minimap-click arrival and legitimate Seymour world/chat speech PASS. Overall IMPLEMENTED_UNVERIFIED; no certification/main merge. See evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RESUME-20261014.md.
2026-10-14 polish operator QA: existing certifications retained. Fresh bounded world speech, Use feedback/server-backed bookcase open, Follow/Stop and wall cutaway smoke PASS. Camera collision acceptance FAILED by operator rejection of retraction. Use With, minimap-click smoke, NPC response and full regression remain IMPLEMENTED_UNVERIFIED. Protocol errors0. No global parity/certification promotion. Evidence: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-OPERATOR-QA-20261014.md.

# Parity Matrix

## Skills-panel independent review - 2026-10-17

UNREAL-SKILLS-PANEL-001: `CERTIFIED` for the bounded skills-panel scope.
Candidate63a62dd, base370b567. Seven visible skills, received vitals/skills only,
unknown-- versus actual0, nine received-percent bars, live Sword67->83%, six
real registered HUD panels and retained HUD layout: named widget/replay tests
PASS. Independent live evidence has8snapshots with protocol counters0.
ClientCore13/13, native4/4, independent Unreal6/6, exact-source Unreal5/5,
MSVC C++17/C++20 and Unreal build PASS; corrected strict harness9/0/0,
selftest10/10 and secret_check6/6 PASS.
No skill-level advancement, full gameplay parity or world-art certification is
claimed. No implementation changes or main merge. See
evidence/clientcore/UNREAL-SKILLS-PANEL-001-INDEPENDENT-20261017.md.

2026-10-09 gap QA: authorized safe push completed; remote now 6ac8a8e.
Fresh client connected, but native input helper failed initialization twice.
Operator input pending; no remaining live gap closed. Prior PASS items retained,
no runtime/art changes. Overall IMPLEMENTED_UNVERIFIED; see GAP-QA-20261009 report.

2026-10-09 live review at 55e468b: cave relief proportions and water/floor return
proven in two F9 captures. FLOOR_READABILITY PASS for this scene; full milestone
still IMPLEMENTED_UNVERIFIED. Remaining source wall-family inconsistency is
V08_ART_DEFECT (373/377 and provisional relief mix), recorded separately. Client
closed normally; 22 accepted walks, zero unanswered or protocol errors. See
UNREAL-WORLD-PRESENTATION-POLISH-001-CONTINUATION-20261009.md.

2026-10-09 continuation: water overlap fix proven in normal live QA and accepted
by operator. Raised cave walls rejected:20 provisional4.5uu reliefs were stretched
48.44x. Runtime now preserves their native proportions; volumetric walls retain
room height. Build and three actual-catalog/geometry/authority tests PASS; new
live wall review pending. Full milestone remains IMPLEMENTED_UNVERIFIED. See
evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-CONTINUATION-20261009.md.

2026-10-09 preservation: recovered all existing polish work at local/remote4305d23
and certified mainfa3adcf. Fresh ClientCore, Unreal build and initial three-case
automation PASS. Real operator gameplay has zero protocol errors and no unanswered
walks in the new sessions, but underground wall/water flicker remains FAILED.
A newly identified coplanar lower-floor wall cap now has2uu clearance; final build
PASS, live comparison pending. Added opt-in received-speech capture. Operator
requested immediate commit; milestone remains IMPLEMENTED_UNVERIFIED, no main
merge. Full results: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RECOVERY-20261009.md.

2026-10-07 fresh operator review: underground readability FAILED; inventory
sprites absent in the isolated worktree. Sprite generation/pixel regression,
corrected Unreal build and three automation tests now PASS; terrain border XY
correction implemented after source tracing. Post-fix live validation remains
IMPLEMENTED_UNVERIFIED. Brother-creature catalog enabled with live rat21/snake28
loads; no complete outfit/art parity claim.

2026-10-07 polish recovery: IMPLEMENTED_UNVERIFIED. Existing terrain-layer,
wall-orientation and transient instancing-material fixes preserved. Fresh
ClientCore nine suites, Unreal build and CameraAndFloors/AuthoritativeWindow/
ExperimentalV08 automation PASS. Recovered October 3 screenshot shows textured
WideWorld terrain; it does not close underground, Use With, Stop Follow, speech
or full minimap/gameplay coexistence acceptance. No prior certification changed.
See evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RECOVERY-20261007.md.

2026-09-29 16:58 operator preservation checkpoint: UNREAL-WORLD-PRESENTATION-POLISH-001
remains IMPLEMENTED_UNVERIFIED, not PASS LOCAL. Fresh native nine-suite tests,
Unreal build and two Unreal automation cases PASS. Fresh live camera/zoom/follow,
aligned labels, rat attack/follow feedback, building roof switching and floor7->8->7
were exercised. Underground ground/water overlap FAILED readability; wall aliases
show perpendicular fins. Fixes and full live regression remain pending. Protocol
errors0 in last retained snapshot. Operator requested commit/push now; no main
merge. Details: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-CONTINUATION-20260929.md.

2026-09-29 presentation recovery: IMPLEMENTED_UNVERIFIED. Pending overlay-space
fix preserved with recovered aligned-label screenshot, not full live acceptance.
Fresh ClientCore nine-suite/build checks PASS; Unreal build PASS; secret check
PASS. Fresh Unreal automation interrupted; no new live QA. Camera, floors,
occlusion, entity/interaction feedback, speech and WideWorld/minimap/gameplay
coexistence retain IMPLEMENTED_UNVERIFIED for this polish milestone. Earlier
certifications unchanged. See UNREAL-WORLD-PRESENTATION-POLISH-001-RESUME-20260929
in evidence/clientcore/. Frozen agent checkout and main remain unchanged.

2026-09-28 main integration: certified UNREAL-MINIMAP-NAVIGATION-001 was
fast-forwarded and pushed to main at fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Main was clean and secret_check passed; prior certification evidence is retained.
UNREAL-WORLD-PRESENTATION-POLISH-001 = IMPLEMENTED_UNVERIFIED on its isolated milestone
branch from that exact base. Audit is in docs/UNREAL_WORLD_PRESENTATION_POLISH.md.
Camera/floor visibility and read-only world feedback changes are being validated.
ClientCore nine-suite native regression, Unreal build and two Unreal geometry/
WideWorld automation tests PASS. Live overlay alignment FAILED; operator reports
misoriented walls. Remaining live acceptance is UNVERIFIED. Work preserved by
explicit operator request; see the polish evidence report. Older statements below about unmerged minimap / polish not started
are historical and superseded by this entry. Frozen agent work is untouched.

Independent minimap delta certification = CERTIFIED, starting candidate
`34e9bfd91f8863b642536221427ce297779d441a`, existing clean isolated clone.
New live south, floor round-trip/per-floor restoration/no stale marker,
known minimap arrival, legitimate server refusal, manual path interruption,
authority boundary, Follow/Stop and container-open smoke all PASS; protocol0.
All previously independently proven PASS results retained without repetition.
Both prior reports preserved unchanged. Evidence-only closeout, no source changes,
no main merge, frozen agent checkout untouched. WORLD-PRESENTATION-POLISH not started.
See `evidence/clientcore/UNREAL-MINIMAP-NAVIGATION-001-INDEPENDENT-DELTA-20260928.md`.

2026-09-28 original implementation record, before independent delta:
`UNREAL-MINIMAP-NAVIGATION-001 = PASS` local. REAL33D now
projects only authoritative current-floor observations into separate terrain
history, renders actual coordinates/floor/player, and retains known terrain
without entities or walkability. Local pan/zoom/recenter/floor browsing and
map-click routing are implemented. Classic 7.72 appearance colors
come from a bounded local palette, not static map/DB seeding. Following live
review, known-terrain A* replaces the live-window routing limit. Unknown
same-floor destinations retain their goal while observed exploration segments
continue as normal terrain descriptions arrive; planning reveals no terrain.
Each adjacent step still uses live validation and server acceptance. Uniform classic colors
have no live-tile footprint or brightness split. Native/sanitizer
tests and Unreal build pass. Live terrain expansion, all cardinal directions,
unknown-goal continuation, real blocked-path stop, restored cache and floor
7->6->7 are recorded; Follow/Stop request/state/cancel and operator confirmation
are retained. Pan/zoom/recenter and remaining interactions are operator-observed
with limited individual checkpoints. No full original-client appearance parity
or independent certification claim. See the milestone report for exact limits.

Latest independent check: three distinct world Looks, slot Looks, normal Say,
world/inventory/container Use and valid Use With reproduced with authoritative
results. Final automatic MultiUse entry from normal Use has tests/build PASS
and live rapier-to-trough confirmation. Corrected interaction milestone CERTIFIED.
Rune spell effects remain IMPLEMENTED_UNVERIFIED live. Original cursor confirmed;
16 original creature outfits imported, matching outfit 128 shown and accepted
with defects for now. Complete art coverage/certification is not claimed.

## Item interaction correction during independent certification (2026-09-27)

Operator requested left-click walking, Shift-left-click or left+right-click
Look, and right-click Use. Mouse gesture tests, relevant native ClientCore
tests, Unreal build and secret_check are `PASS`. Corrected live gestures and
click walking are `IMPLEMENTED_UNVERIFIED` pending a new operator session.
The previous fresh session returned authoritative world and slot descriptions,
but slot releases also issued unwanted world actions; it does not certify
input isolation. Independent world Use and a valid Use With effect still
require retained successful results. No protocol, Fusion32 rule, 2D, agent or
V08 art change. Overall milestone is not `CERTIFIED`.

Follow-up live mouse run: operator confirmed walking and Use; EndPlay retained
37 accepted steps and open backpack/bag. World Use caused authoritative floor
changes. Look descriptions reached the transcript but were invisible, and
Local Chat could not send: visible Look FAILED that run. The HUD had omitted
ChatPanel's Bridge and OnTypingChanged arguments. Forwarding them built PASS;
visual Look/chat and a valid fresh Use With still require live validation.

> **2D client note (2026-09-21).** The `2D<->3D parity` column now refers to
> **REAL33D 2D**, the mehah/OTClient-derived production client, not to the
> official `Tibia.exe` 7.72, which is QA/parity/reference only.
> `DUAL_CLIENT_LIVE_CAPTURE = PASS` (`0f9bd505`) established the 2D side of the
> ordinary 7.72 flow on a stock build with configuration only: zero server
> changes, zero parser patches, and zero incoming decode errors, unknown
> opcodes or unsupported opcodes. Rows below marked *2D live* were exercised in
> that session. The 3D columns are unchanged by it.

| Feature | Server understood | Protocol specified | Unreal implemented | Tested | 2D<->3D parity | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Connection | Service topology/runtime PASS; classic live route PASS; client outer framing and crypto source-traced | Transport/framing/RSA/XTEA, Login and Game Login implemented; persistent session smoke PASS | Yes. `UReal33DBridge` runs Protocol772Core on a worker thread and publishes semantic events to the game thread; the module links the prebuilt `protocol772core.lib` and contains no protocol source | 4/4 retained/new suites normal and sanitized; bounded Game Login smoke PASS; native Windows MSVC build PASS | Partial. Unreal and the original `Tibia.exe` held a session against one Fusion32 world at the same time | IN_PROGRESS |
| Login / character list | Source traced; sanitized Login runtime and exact classic client flow PASS | Source-traced request/response and typed character list implemented | Yes, through the bridge. Credentials and modulus are read from the sanitized runtime at launch; no login UI | `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and `CLIENTCORE-LOGIN-772-001` PASS | Partial. Both clients authenticated against the same Login service in one run | IN_PROGRESS |
| Game login | Source traced; sanitized Game runtime and classic `JoinGame` flow PASS | Source-traced request, initial messages and persistent session implemented; fullscreen decoding now handed to the Initial World layer | Yes. World entry through to a drawn scene, plus in-session reconnect after a real server-side drop | `CLASSIC-GAME-ENTRY-772-001`, `CLIENTCORE-GAMELOGIN-772-001` and `UNREAL-SLICE-001` PASS | Partial. Both clients entered the same world; a real drop and reconnect left no residue | IN_PROGRESS |
| Keepalive | Source traced: server-initiated `SendPing` from the connection timer and `EmergencyPing`; `CPing` is a no-op | `SV_CMD_PING` decoded and `CL_CMD_PING` built | Yes. 20-second `CL_CMD_PING` from the worker; a real 90-round drop under machine load was observed and recovered from | `PLAYERSTATE-772-001` fixtures; the drop and recovery are recorded in `UNREAL-SLICE-001` | Not compared | IN_PROGRESS |
| Map | Source traced for `FULLSCREEN` and the incremental deltas: window, scan order, skip markers, tile stacks, viewport anchor and pruning | `FULLSCREEN`, `ROW_*`, `FIELD_DATA` and `ADD`/`CHANGE`/`DELETE_FIELD` decoded and applied to WorldState | Yes. `WorldView` diffs WorldState into tile events; `AReal33DTile` spawns, updates and destroys one actor per visible field | `INITIALWORLD-772-001` and `MOVEMENT-772-001` PASS; live walk matched a fresh server `FULLSCREEN` tile for tile | Partial, by state and not by appearance. Tile actors matched WorldState exactly at every sampled point of the live run; no visual comparison was attempted | IN_PROGRESS |
| Floors | Source traced: the `FULLSCREEN` floor range and the `SendFloors` sets, with the anchor shift NotifyGo applies | Floor set, offsets, `FLOOR_UP`/`FLOOR_DOWN` and the per-floor pruning decoded and applied | Transform only. `Real33D::ToWorld` inverts z and spaces floors by a presentation-chosen 100 uu, which is `UNRESOLVED` in the data | `MOVEMENT-772-001` fixtures cover both directions and the empty case; no live floor change observed, in `UNREAL-SLICE-001` either | No | IN_PROGRESS |
| Movement | Source traced: `Move` ordering, `NotifyGo`, `CGoDirection`, the refusal paths | Cardinal walk/turn/stop commands and `MOVE_CREATURE`/`SNAPBACK`/`MESSAGE` implemented; diagonals and `GO_PATH` deliberately unexposed | Yes, and deliberately one-way. Input becomes an intent for Fusion32; there is no code path by which a key moves an Actor, so a refusal changes nothing on screen. A `MovementLedger` separates steps this client asked for from relocations Fusion32 imposed, such as being pushed | `MOVEMENT-772-001`, `TWO-CLIENT-VERTICAL-SLICE-001` PASS; `UNREAL-SLICE-001` corrective run PASS: 19 operator-driven requests from Unreal resolved as 18 accepted and 1 refused, each joined from key press to authoritative position by an `input_id`, with 6 external relocations counted apart | Partial. Operator-driven Unreal steps were seen on the original client (human observation, machine-corroborated), and the original client's steps moved the Unreal actor | IN_PROGRESS |
| Creatures | Source traced for the three descriptor forms, the known-creature table and its lifetime, creature relocation and the six attribute updates | Descriptors, the known-creature mirror, creature moves and the 140-145 attribute updates decoded and applied; the mirror now retains entries exactly as `TConnection::KnownCreatureTable` does | Yes. One `AReal33DCreature` per visible creature, named, facing per `enums.hh`, with interpolation applied to drawing only and never to the logical position | The four suites above plus `UNREAL-SLICE-001`, where creature actors equalled WorldState's visible creatures at every sampled point, with zero duplicate spawns and zero orphan events | Partial. Both players were represented simultaneously in both clients through appearance, movement, viewport exit and return | IN_PROGRESS |
| Items | Source traced for map items, including the object type flags and stack priority the wire omits | Map item encoding and `PlaceObject` stack insertion decoded via an explicit object type table; inventory and container items not started | Presence and passability only. Stack order is preserved and the `UNPASS` flag from `objects.srv` separates blocking objects from walkable clutter; every mesh is a placeholder | `INITIALWORLD-772-001` and `MOVEMENT-772-001` PASS; extra-byte, priority and `UNPASS` paths verified against the real `objects.srv` | No. Nothing about item appearance is comparable while every mesh is an engine primitive | IN_PROGRESS |
| Inventory | Source traced: `SendSetInventory` / `SendDeleteInventory` and the `INVENTORY_*` body positions in `enums.hh` | `SET_INVENTORY` (120) and `DELETE_INVENTORY` (121) decoded and applied to WorldState, which holds all ten body slots | Yes. The body panel draws the ten slots from WorldState; a changed slot republishes the whole set so the panel can never mix two moments | `UNREAL-INVENTORY-CONTAINERS-001` PASS: body slots read at login matched the server's own save file and were unchanged by every container operation of the run | 2D live; 3D live | IN_PROGRESS |
| Containers | Source traced: `SendContainer`, `SendCloseContainer`, `SendCreateInContainer`, `SendChangeInContainer`, `SendDeleteInContainer` in `sending.cc`, and `UseContainer`'s open/close toggle in `moveuse.cc` | All five decoded and applied; an open replaces the container wholesale, a create prepends, a change replaces in place, a delete erases, a close clears the entry. The special coordinate `y = CONTAINER_FIRST + number` stays inside ClientCore | Yes. One window per open container, capacity-sized grid, drag between any two addressable slots. `CL_CMD_CLOSE_CONTAINER` and `CL_CMD_UP_CONTAINER` are not built: closing goes through the 7.72 use toggle and the parent arrow is drawn inert | `UNREAL-INVENTORY-CONTAINERS-001` PASS: open, contents matching the server save object for object including amount and liquid bytes, a move that both windows followed, a nested bag as its own window with `has_parent`, and a close that removed the entry | 2D live; 3D live | IN_PROGRESS |
| Use / use-with | Source traced: `CUseObject`, `CUseTwoObjects`, `CUseOnCreature` in `receiving.cc`, including the open-container slot byte `CUseObject` refuses out of range | `CL_CMD_USE_OBJECT` (130), `CL_CMD_USE_TWO_OBJECTS` (131) and `CL_CMD_USE_ON_CREATURE` (132) built byte for byte | Yes. Right button uses, shift+right begins a use-with that sends nothing until a second click names a slot, a creature or a field; Escape discards it | Byte-exact vectors for all three commands and for the container number a nested open picks; `UNREAL-INVENTORY-CONTAINERS-001` PASS live for the two-object form against a real `dat/moveuse.dat` rule (flour on a bucket of water became dough and emptied the bucket) | 3D live for use and use-with-object; the creature and field forms are built but not live | IN_PROGRESS |
| Combat / follow | `connections.hh` opcodes 160/161/162/190 and server 163; `receiving.cc::CSetTactics`, `CAttack`, `CCancel`; `crcombat.cc::TCombat::SetAttackDest`, `StopAttack`; `sending.cc::SendClearTarget` | Exact byte builders and tests; one `WorldState::combat` record updated only after send and cleared by server command/session reset; semantic `CombatChanged` events | Yes. Battle rows and world creatures share one target; explicit Follow/Stop control; right-click creature attacks; red attack and blue follow feedback; real stance buttons | Live request/state/rejection/removal evidence plus final snapshot in `UNREAL-COMBAT-FOLLOW-001.md` | 3D live | CERTIFIED |
| Magic / effects | Source traced for the wire form of the graphical, textual and missile effects and creature marking | Decoded and surfaced as typed events; they carry no WorldState semantics and store nothing. Spell casting itself is untouched | No | `PLAYERSTATE-772-001` fixtures; one live graphical effect observed at login | No | IN_PROGRESS |
| Stats / skills | Source traced: `SendPlayerData`, `SendPlayerSkills`, `SendPlayerState` and the `CheckState` flag table | Decoded and applied to WorldState; `PLAYER_STATE` is only emitted when the flags change | Partial. A player-facing HUD shows server-owned HP, mana and level from `PlayerData`; skills have no UI yet | `PLAYERSTATE-772-001` PASS; `UNREAL-PLAYER-VITALS-001` compiled and its live log showed a connected Player B and updated HUD text, with visual inspection pending | No | IN_PROGRESS |
| Chat | Source traced: `SV_CMD_TALK` and its three `SendTalk` overloads, the mode-dependent tail, and the `TALK_MODE` values no overload accepts | Server talk decoded into a typed semantic event with the position, channel and gamemaster-request fields flagged rather than sentinel-valued; it stores nothing in WorldState. Client-side talk is implemented. `UNREAL-CHAT-CHANNELS-001` adds the four channel commands (`CL_CMD_GET_CHANNELS` 151, `JOIN` 152, `LEAVE` 153, `PRIVATE_CHANNEL` 154) and the five server answers (171/172/173/178/179) through the existing ClientCore builders and `DecodeChannel`, plus `TALK_CHANNEL_CALL` and `TALK_PRIVATE_MESSAGE` sending. Channel membership is presentation state on the game thread, because `ApplyServerUpdate` deliberately stores none: Fusion32 owns the lists. Channel 3 is refused by this client, since its answer `SV_CMD_OPEN_REQUEST_QUEUE` (174) is undecoded; creating an own channel (`CL_CMD_OPEN_CHANNEL`) has no builder and is not offered | Visible. Positional speech is drawn above the creature that said it, resolved by matching both the talk position and, where present, the sender name against visible WorldState creatures; anything not uniquely resolvable goes to a small on-screen fallback rather than to a guessed creature. Display lifetime and consecutive-message behaviour are client presentation and `NOT_PROVEN` against 7.72. The console carries the `console.otui` tab strip for real: Local Chat first and never closable, then one tab per channel Fusion32 confirmed and per private conversation, each with its own bounded transcript. A line is addressed by the tab it is typed in. The channel list is a registry panel showing only rows the server sent | `CHAT-772-001` PASS: 7 deterministic cases including golden bytes per form, every accepted mode, all truncation lengths and stream alignment; live session decoded Say and Whisper with zero unsupported opcodes, anomalies and residual bytes | Partial, by association not appearance. Speech reaches the right creature in both clients; the 3D presentation is a plain text tag whose exact colour and lifetime are not classic-parity | IN_PROGRESS |
| Look / inspect | `connections.hh::CL_CMD_LOOK_AT_POINT=140`; `receiving.cc::CLookAtPoint` reads x/y/z, validates special coordinates and visibility, resolves through `info.cc::GetObject`; `operate.cc::Look` sends `TALK_INFO_MESSAGE` | Six-byte builder for map, body or open-container point; no client-computed description | Alt+right-click world/creature/occupied slot posts a bridge intent; existing server-message presentation shows any reply | Exact byte vectors and native ClientCore suites PASS; Unreal build PASS; three live world Look requests received server `InfoMessage` descriptions; slot Look clicks not retained | World Look `PASS`; slot variants `IMPLEMENTED_UNVERIFIED` | PASS |
| Text editors | Source traced: `receiving.cc::CEditText`/`CEditList`, `sending.cc::SendEditText`/`SendEditList`, `moveuse.cc::UseTextObject` and the spellbook branch of `UseAnnouncer`, `magic.cc::EditGuests`/`EditSubowners`/`EditNameDoor` (spells 71/72/74), `houses.cc::ShowGuestList`, `enums.hh` GUESTLIST/SUBOWNERLIST/DOORLIST | `SV_CMD_EDIT_TEXT` (150) and `SV_CMD_EDIT_LIST` (151) decoded into a typed event storing nothing in WorldState, because Fusion32 resends the whole body on every use; `CL_CMD_EDIT_TEXT` (137) and `CL_CMD_EDIT_LIST` (138) built by `editors.cpp`. `UNREAL-EDITORS-UI-001` adds the UI | An isolated centred modal, registered in the panel registry under `TextEditor` so visibility has one owner. Shows the server's text, the three attribution states including the empty editor the server really sends, and the reported length. **The body is always editable and Submit always offered**: `SendEditText` reports `MaxLength = strlen(Text)` for a read-only object, so read-only is not distinguishable on the wire and `CEditText` rules. Submit never claims the edit was kept, because `operate.cc::EditText` answers success with silence | `CLIENTCORE-EDITORS-772-001` suites PASS; `REAL33D.Presentation.EditorWindow` PASS including the read-only/writable indistinguishability case | EDIT_TEXT CERTIFIED by independent live review: open, server id/type/limit and text shown, attribution both states, submit accepted with persistence proven by re-reading the object, oversized submit refused by Fusion32 with the object keeping its text, modal non-blocking. EDIT_LIST_DISPLAY and EDIT_LIST_SUBMIT remain IMPLEMENTED_UNVERIFIED: no live run is possible on this server, see the integrated status block at the top | PARTIALLY CERTIFIED |
| Outfit chooser | Source traced: `receiving.cc::CGetOutfit` (1390) and `CSetOutfit` (1410), `sending.cc::SendOutfit` (1661/162) and `SendCreatureOutfit` (1011), `crplayer.cc::CheckOutfit` (1175), `operate.cc::AnnounceChangedCreature` (81), `cr.hh::TOutfit` (17). Four facts shape everything: the selectable set is a **band** (`First`/`Last` only, 128-131 for `Sex == 1` and 136-139 otherwise, each `+3` under `PREMIUM_ACCOUNT`); `CSetOutfit` reads a word and four bytes **unconditionally**, so a set command is seven bytes and cannot carry an object disguise; it **acknowledges nothing**, returning void; and its `CREATURE_OUTFIT_CHANGED` broadcast is conditional on `Player->Outfit == Player->OrgOutfit`, so a 142 may never arrive | `CL_CMD_GET_OUTFIT` (210) and `CL_CMD_SET_OUTFIT` (211) built by `outfit.cpp` with `ValidateOutfitSelection` mirroring CSetOutfit's two tests in its order; `SV_CMD_OUTFIT` (200) and `SV_CMD_CREATURE_OUTFIT` (142) already decoded. 210/211 collide with `SV_CMD_BUDDY_DATA`/`BUDDY_ONLINE`, pinned by two `static_assert`s: direction disambiguates an opcode here, never the number | `UNREAL-OUTFIT-DIALOG-001`. An isolated centred modal under `PanelIds::Outfit`, opened **only** by `SV_CMD_OUTFIT` — the game protocol never carries this player's sex, so a client cannot compute the band and must not invent one. Entry is the classic client's own documented gesture: right-click your character -> *Set outfit*. Outfit and colour steppers cannot leave the band or exceed 132; submit closes the window, because nothing will confirm it. Also fixed the two bridge gaps that dropped `Colors[4]` and `object_type` at the Unreal boundary. **The four slots are named Head/Body/Legs/Feet, proven from the classic client's own jump table at `0x0047D648` and 43 asserted byte patterns, not assumed** | `CLIENTCORE-OUTFIT-772-001` suites PASS (golden vectors for both client commands and both branches of both server commands, 36 truncated prefixes, every band boundary); `real33d_outfit_selection_tests` 20 cases; `REAL33D.Presentation.OutfitPanel` PASS; `verify_outfit_color_mapping.py` 43/43, also a harness check | Not comparable by appearance. The colours round-trip through Fusion32 and are proven stored, but `ResolveOutfit` still selects a mesh from the outfit id alone and the palette's byte-to-RGB mapping is `UNVERIFIED`, so no colour is drawn in 3D | CERTIFIED (dialog/bytes at 61ceaa9; rendering incomplete) |
| NPC | Located | No | No | No | No | NOT_STARTED |
| Trade | Located | No | No | No | No | NOT_STARTED |
| Death / loot | Located, untraced | No | No | No | No | NOT_STARTED |
| Social systems | Partial locations | No | No | No | No | NOT_STARTED |

`Located` means a likely authoritative implementation symbol was found. It is not protocol understanding and never means `PASS`.

`Partial` in the parity column means the same world state was represented in both clients at the same time and agreed. It never means the two clients look alike: every mesh in Unreal is an engine primitive, so no cell in this matrix claims visual parity and none can until the visual pipeline approves art.

`SERVER-RUNTIME-SMOKE-001` is infrastructure evidence only. The classic-client PASS results validate the 2D baseline, not an Unreal client. They do not change any 2D-to-3D parity cell to PASS.

`CLIENTCORE-TRANSPORT-772-001 = PASS` is limited to the portable TCP and outer-framing component. It does not prove crypto, application login, world entry or 2D-to-3D parity.

`CLIENTCORE-CRYPTO-772-001 = PASS` is limited to deterministic RSA/XTEA, key lifecycle and encrypted inner-packet behavior. It does not prove application Login, Game entry or parity.

`INITIALWORLD-772-001 = PASS` is limited to decoding the one `FULLSCREEN` snapshot the server sends at login and folding it into a minimal WorldState. It proves no rendering and no 2D-to-3D parity.

`MOVEMENT-772-001 = PASS` is limited to cardinal walking and the incremental map updates a step produces. It proves no combat, inventory, container, chat, diagonal or path movement, no rendering and no 2D-to-3D parity. Its live evidence covers surface walking on floor 7 only; floor changes and the field commands are fixture-covered.

`TWO-CLIENT-VERTICAL-SLICE-001 = PASS` demonstrates that the original Tibia 7.72 client and Protocol772Core coexist as two independent clients in one authoritative Fusion32 world: appearance, movement tracking, viewport-edge departure and return, client-initiated movement observed from the other client, movement refused because the other player physically occupied the destination field, and disconnect/reconnect without ghosts. It proves protocol coexistence and world consistency only. Player B has no presentation at all, so no 2D-to-3D parity cell changes on its account, and the `Unreal implemented` column stays `No` everywhere.

`PLAYERSTATE-772-001 = PASS` closes the command set an ordinary session emits: a real login burst and walking session decoded with zero residual bytes and no unsupported opcode. It proves decoding, not behaviour. Inventory, buddy, outfit-chooser and effect commands are decoded for structure and length but deliberately store nothing, and containers, trade, the request queue and the editors remain undecoded and still report by name with zero bytes consumed. Chat was in that list until `CHAT-772-001` decoded `SV_CMD_TALK`. No row in this matrix reaches `PASS` on protocol decoding alone.

`REAL33D-VISUAL-INGEST-AUDIT-001 = PASS` describes REAL33D's internal reference area only; it was the wrong artist source. `REAL33D-3DTIBIA-ART-AUDIT-001 = PASS` for read-only inventory and bounded 7.72 outfit identity cross-checks in the separate 3DTIBIA clone. Four experimental GLBs map exactly to outfit identities, but none is approved or imported. Technical import, in-engine rendering and director visual approval remain `NOT_STARTED`; no parity row changes. See `visual/docs/EXTERNAL_3DTIBIA_INGEST_AUDIT_2026-09-20.md`.

`VISUAL-FULL-CATALOG-INGEST-TEST-001 = STANDBY`: 4,913/4,913 V08 GLBs imported and deterministic TypeId-to-mesh mapping checks passed. The UE 5.8 gallery and live Fusion32 client load the experimental catalog, but full visual certification remains open. The operator confirmed mailbox visuals and wall 01294; all wall orientation, floor visibility, camera feel, warning severity, and representative gallery screenshots require final review. TEST_IMPORTED=YES; APPROVED=NO; READY=NO; PRODUCTION_INTEGRATED=NO.


Operator direction (2026-09-21): V08 visual certification is STANDBY, with no new PASS claim. The inspector notes and latest live-session journal are retained for later review while gameplay client programming resumes.

UNREAL-WIDE-WORLD-001 = CERTIFIED_PASS. Strict generated 32ÃƒÆ’Ã¢â‚¬â€32 sector caches stream outside the complete WorldState-owned 18ÃƒÆ’Ã¢â‚¬â€14 live rectangle; async work reads and parses rows, while the game thread owns actor/component creation and destruction. Static tiles entering live authority are suppressed, distant sectors unload, parsed data is cached, and missing V08 meshes resolve to identity-retaining local classic sprite proxies or a separately counted non-blocking placeholder. This adds static-map reach and makes no dynamic-world or final-art parity claim. See evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

Live round-trip at x=32096: 9 sector loads and 9 unloads in each direction; all crossing/return audits had zero visibility mismatches and zero static/live overlaps. TypeId 469 uses the exact classic sprite fallback.

## REAL33D 2D live coverage - DUAL_CLIENT_LIVE_CAPTURE (`0f9bd505`)

Stock `mehah/otclient`, configuration only, against the unmodified Fusion32.

| Feature | 2D live result | Authoritative evidence |
| --- | --- | --- |
| Connect / login / character list | PASS | server: `loggt ein an Socket 16` |
| Game login | PASS | `op=10 (0x0A)` INIT_GAME, `onGameStart` |
| Initial world | PASS | `op=100 (0x64)` FULLSCREEN; `pos=(32098,32205,7)` |
| Client-initiated movement | PASS | N/S/E/W exact round trip; `op=109` + row updates; `op=181` SNAPBACK decoded when refused |
| Say chat | PASS | `op=170 (0xAA)`, echo in 20 ms, `mode=1` TALK_SAY |
| Items | PASS | tile stack `n=2`, TypeId 870 |
| Inventory | PASS | 4 slots at login, 3 after the move |
| Containers | PASS | `op=110` open, `op=111` close |
| Item move | PASS | `op=121` + `op=112` 68 ms after the request; persisted across logout |
| Logout / reconnect | PASS | two distinct server-side socket records |
| Combat / follow | NOT_EXERCISED | out of the milestone's scope |

Incoming protocol decode errors, unknown incoming opcodes and unsupported
incoming opcodes: **0 each**, over 103 traced packets and 20 distinct opcodes.
The only `[error]` lines were outbound OTClient ecosystem noise
(`ExtendedIds.Locale = 1`, `GAME_SHOP_CODE = 201`), blocked client-side by
`m_enableSendExtendedOpcode = false` and never transmitted to Fusion32.

## Operator-assisted continuation - 2026-10-14 22:10 local
At6f864e3, evidence-only QA with user providing one action at a time. Existing server/client verified. Baseline F9 captured; no new remaining criterion PASS. Idle disconnect auto-reconnected04:09:47Z; pending world Look action. Report: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-ASSISTED-20261014.md. Overall IMPLEMENTED_UNVERIFIED. No code/build/art changes or certification/main merge.
