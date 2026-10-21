## 2026-10-21 independent UNREAL-STACK-MOVE-CONTROLS-001 review

Independent live certification is BLOCKED: the Windows Computer Use native pipe is unavailable after retry and session reset. No new live gesture was exercised; all requested live rows remain UNVERIFIED. Candidate 1abb013bf711978b9439b6abe836672b5f4148ac was normally pushed and reviewed in a fresh isolated worktree; main was not merged. No implementation change. Independent ClientCore 14/14, native 7/7 and Git Bash secret_check 6/6 PASS. Build/automation details and explicit limits: evidence/clientcore/unreal-stack-move-controls-independent-20261021/README.md. READY_TO_MERGE_MAIN=NO.

UPDATE 2026-10-21 UNREAL-STACK-MOVE-CONTROLS-001 = PASS on branch milestone/unreal-stack-move-controls-001 from origin/main fbc0de99; no main merge, no push, NO PROTOCOL AND NO SERVER CHANGE. Classic stack-movement UX on every drag route: plain drag moves the whole stack, Shift+drag moves one, Ctrl+drag asks how many clamped to 1..the stack count, and Cancel sends nothing. Before this a drag of a pile always moved the whole pile with no way to ask for fewer, and a drag off the GROUND always sent count 1, so a pile of fifty coins on a tile moved one coin and left forty-nine. THE RULE LIVES IN ONE PLACE: Real33DStackMove.h holds the decision and the clamp, free of Unreal types so tests/real33d_stack_move_tests.cpp proves it with a plain compiler (16 cases, now the 7th native suite), and all four routes -- slot to slot, slot to ground, ground to slot, ground to ground -- end in SReal33DHUD::RequestStackMove, so the gesture cannot mean one thing in a panel and another on the ground and the clamp cannot be applied in three places and forgotten in a fourth. Both bounds are the protocol's: receiving.cc::CMoveObject reads ONE count byte and refuses a cumulative object whose count is zero, so every answer folds into 1..min(stack,255). FReal33DSlotRef and the world drag now carry bHasAmount from the wire beside the count, because after the count is floored at one a single gold coin and a sword read alike -- that is what keeps the modifiers off a non-stackable structurally rather than by luck. The question is SReal33DAmountPanel, a centred modal registered like the other three; its title is the classic 7.72 client's OWN 'Move Objects' at 0x00563C00, the same window-title table four strings earlier than the 'Select Outfit' at 0x00563C78 the outfit milestone already cited, against the same hashed image. Cancel sends nothing because no move was ever offered; Escape closes it the same way, ahead of the use-with banner. TWO DEFECTS WERE FOUND WHILE PROVING IT AND BOTH ARE FIXED. First, A MODIFIER HELD BEFORE THE PRESS NEVER REACHED THE DRAG: MouseGesture::Press returns Look for Shift and both SReal33DSlot::OnMouseButtonDown and BeginLeftClick returned at that branch before arming the drag, Ctrl over the world hit the inspector branch and returned too, and EndLeftClick completed a world drag only when the gesture had been a walk -- so holding Shift the natural way gave a look and NO DRAG AT ALL and the pile moved whole, which was observed live as a 20-pile Shift+drag sending count 20. The look and the inspection still fire first unchanged; both now also arm the drag, and EndLeftClick completes whenever the pointer really travelled, keeping the gesture test on the walk branch. Slate raises OnDragDetected only once the pointer moves, so a shift+click that stays put is unchanged -- proven live, the probe click still produced look 3 at world point 32097,32209,7. Second, STACK SPRITE TIERS HAD NEVER LOADED: extract_item_sprites.py documents and writes <id>_<n>.png and ItemBrush looked for exactly that, but the delivered art under Resources/UI/Items is named <id>_p<n>.png -- 343 such files and ZERO <id>_<n>.png among them -- so the frame scan always stopped at one, StackSpriteFrame was always handed a count of 1 and always answered frame 0, and every cumulative object drew its single-item picture at every amount. Gold coin 3031 ships all eight pictures and a pile of twenty was drawn as one coin, which is the exact defect Real33DStackSprite.h was written to fix and it was still live. ItemFramePath now accepts both spellings rather than declaring one right. Confirmed on screen: the backpack draws the mound for 20 coins and a bundle for the x12 where both drew a single item before. LIVE QA against Fusion32, account B Test Player B creature 1002, gestures synthesised as real OS-level input with hardware scan codes so every one went through the same Slate path a hand would use, nothing invoked directly in code. Every required row PASS: full stack (move 8 count 12, x12 gone from the bag), Shift held from the start (move 5 count 1, bag slot 0 = 1 coin), Ctrl held from the start (move amount asked 1..19 opening on 19), clamp (>> at 19/19 stayed 19 of 19, << twice from 19 landed 1 of 19 with no wrap, > gave 2 of 19), chosen partial (move amount chosen 2 of 19 -> move 6 count 2, bag holds a separate x2), CANCEL (abandoned, nothing reached Fusion32, moves_requested stayed 3, x2 untouched), container to container (1..1 -> move 7 count 1, both endpoints container coordinates), inventory to world (move 9 count 1, body slot 5 emptied), world to inventory with SHIFT HELD (move 11 count 1, non-stackable ignored it), inventory to container with CTRL HELD (move 12 count 1 and NO DIALOG). Stack counts 1, 2, 12 and 19/20 all exercised. moves_requested rose by exactly one per intended gesture across every snapshot -- 0,2,3,3,4,5,6,7,8,9 -- so no gesture produced a stray or duplicated move and the cancel produced none. EndPlay: 366 frames, 688 server commands decoded, residual_bytes 0, unsupported_opcodes 0, protocol_anomalies 0, walks_unanswered 0, last_diagnostic none; every intermediate snapshot carries the same four zeroes. ClientCore 14/14, native 7/7, Unreal automation 10/10 errors0 warnings0 including the new REAL33D.Presentation.StackMoveControls, REAL33DEditor Win64 Development built, MSVC C++17 suites and C++20 archive built, colour-mapping 43/43, palette 51/51, harness 11PASS/0FAIL/0SKIP over 103 scanned logs, secret_check 6/6. UNVERIFIED: ON THE GROUND A 3D PILE IS ONE MESH WHATEVER THE AMOUNT -- the asset registry resolves a mesh by type id with no amount tier, so twenty coins on a tile look exactly like one; the 2D slot art now tiers correctly and the 3D world does not, which is an asset and registry question with no bearing on what reaches the wire, not attempted and not claimed. kStackSpriteTiers 10/25/50 remain UNVERIFIED against the client binary exactly as that header already says; the fix makes the tiers reachable, it does not newly prove those thresholds. Only one open container existed on this character, so container-to-container was proven between two slots of ONE open bag -- both endpoints genuine container coordinates on the wire, but not two distinct container numbers, and no second bag was invented. A second client observing the same ground pile was not run, so 'authoritative counts both sides' is evidenced as source and destination counts from the server's own F9 dumps, not as two observers. The amount question's bound is captured at the drop and deliberately not re-read when answered; a stack that shrank meanwhile yields a refused move, and that path was not provoked live. One environment note: the Fusion32 game service shut itself down cleanly mid-session (full sector save, LogoutAllPlayers, 'Reboot-Skript existiert nicht') and was restarted; it was not caused by any gesture here. See evidence/clientcore/UNREAL-STACK-MOVE-CONTROLS-001.md and CURRENT.

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

UPDATE 2026-10-19 UNREAL-EDITORS-UI-001 PASS LOCAL for text; EDIT_LIST UNVERIFIED. Branch milestone/unreal-editors-ui-001 from origin/main 18b587df; no main merge, no push. Books, signs and house lists through an isolated centred modal registered in the panel registry as TextEditor; HUD edits 17 lines. No ClientCore or server change; editors.cpp builders and DecodeEditor used as they stand. Live on Fusion32 baseline-772-0 with Test Player B, two operator-driven sessions, 0 undecoded commands and 0 decode failures. A writable object was reached by ordinary play, not by editing server data: Dixi sells a scroll (type 2815, WRITE, MaxLength 512) for 5 gold. EDIT_TEXT_DISPLAY, EDITOR_ATTRIBUTION in both states (empty "Nobody has written on this." then "Written by Test Player B."), EDIT_TEXT_SUBMIT accepted at 34/111/315 chars each proven by re-reading the object because operate.cc::EditText answers success with silence, and SERVER_REJECTION live as NOROOM "There is not enough room." for a 600-char text against the server's 511 limit. Two defects found live and fixed, both mine: the window never appeared at all in session 1 (15 SV_CMD_EDIT_TEXT decoded, 0 shown) because Slate does not tick a Collapsed widget and it drove itself from Tick, now driven by SReal33DHUD::Refresh which is this HUD's own rule, and session 2 showed 9 opened/9 shown; and an oversized submission was dropped in silence while the window claimed it was handed over, now reported as a ClientNotice, defect observed live but fix UNVERIFIED. A third was caught before shipping: the modal would have swallowed every click on the panels behind it, fixed with SelfHitTestInvisible and shown working in the capture. Key authority finding: the TEXT flag alone does not open a window, operate.cc::Use needs WRITE/WRITEONCE, INFORMATION, or TEXT with FONTSIZE==1, so only 57 of 135 TEXT types are openable and the Rookgaard signs and blackboards are Look-only. EDIT_LIST live is UNVERIFIED with three independent reasons: owners.dat is 0 bytes so nobody owns a house, the spells require GetHouseID()!=0, and the test characters are on Rookgaard which has no houses; the operator declined to author server test data. ClientCore 13/13, native 5/5, Unreal automation 7/7 errors0 warnings0, MSVC, REAL33DEditor build, selftest 10/10, harness 9PASS/0FAIL/0SKIP, Git Bash secret_check 6/6. See evidence/clientcore/UNREAL-EDITORS-UI-001.md and CURRENT.

UPDATE 2026-10-19 UNREAL-CHAT-CHANNELS-001 independent certification CERTIFIED for the requested bounded scope. Exact candidate 239f89f07430ecd4d4348d69f183a9935e4bd992 preserved by normal milestone push before creating isolated certification/unreal-chat-channels-001-20261018. Fresh live Fusion32 session: 58/58 assertions; four server channel rows; Join only after OPEN_CHANNEL; Leave button and tab close each verified by subsequent messages no longer reaching the departed peer; separate Game/RL/Local transcripts; private delivery A->B and B->A; seven real PanelRegistry panels. Nine snapshots protocol0, both peers residual/unsupported/anomalies0. ClientCore13/13, native5/5, MSVC, exact-source Unreal build and automation6/6 PASS, selftest10/10, corrected strict harness9PASS/0FAIL/0SKIP, Git Bash secret6/6. No implementation defect or change; temporary UI driver retained only as evidence. Own-channel/guild/GM/request-queue remain explicitly UNVERIFIED; additional bounded limitations documented. Main remains ca3121c, remote milestone239f89f, no main merge. See evidence/clientcore/UNREAL-CHAT-CHANNELS-001-INDEPENDENT-20261018.md and CURRENT.

UPDATE 2026-10-18 UNREAL-CHAT-CHANNELS-001 PASS LOCAL on branch milestone/unreal-chat-channels-001 from origin/main ca3121c4; no main merge, no push. Real 7.72 channel UI through the existing PanelRegistry: list, join, leave, one tab per joined channel or private conversation, channel send/receive and the private-message flow. No ClientCore or server change; the existing channels.cpp builders and DecodeChannel are used as they stand, and channel membership lives on the game thread because ApplyServerUpdate deliberately stores none. HUD edits total 15 lines: one AddPanel call, one include, one member. Live QA in two operator-driven sessions on Fusion32 baseline-772-0, Test Player B (Unreal) and Test Player A (original Tibia.exe): channel list 4 real rows (Game-Chat/Trade/RL-Chat/Help, no own-channel offer since MayOpenChannel is false); 5 joins, 5 server confirmations; 5 channel messages sent and 5 broadcast back; private message proven BOTH directions (A->B talk received, B->A answered "Message sent to Test Player A." which is the confirmation operate.cc sends instead of an echo); 4 leaves, one by the panel button and three by the tab close box. Both EndPlay snapshots residual/unsupported/anomalies 0 over 406 and 296 commands. channel_leaves_requested 3 with channels_closed 0 is the live proof that CLeaveChannel answers nothing. Local Chat preserved: positional Say still resolves its speaker and no channel line entered its transcript. ClientCore 13/13, native 5/5, Unreal automation 6/6 errors0 warnings0, MSVC archive, REAL33DEditor build, selftest 10/10, harness 9PASS/0FAIL/0SKIP over 101 logs, Git Bash secret_check 6/6. One defect found and fixed mid-QA: the leave log line sat at the Channels panel button, so the tab close box reached RequestLeaveChannel invisibly and session 1 read two joins against one logged leave; moved into the bridge and re-proved 3=3 in session 2, with the channel list rows and twelve channel counters added to the log and the evidence snapshot. Own private channels, the 174-177 request queue, a live guild channel and a live SV_CMD_CLOSE_CHANNEL remain UNVERIFIED. See evidence/clientcore/UNREAL-CHAT-CHANNELS-001.md and CURRENT.

UPDATE 2026-10-17 HUD registry independent certification CERTIFIED: current-main611939af plus older implementation64aa86a merged without overlap/conflicts in isolated certification/unreal-hud-panel-registry-001-20261016. Actual six-panel registry automation and existing polish suites5/5 PASS; fresh live HUD/backpack gold5 and worms6 compatibility PASS. ClientCore13/13, native4/4, selftest10/10, MSVC, real Unreal build, Git Bash secret6/6 PASS. Corrected strict harness9PASS/0FAIL/0SKIP;1137live commands and all5snapshots protocol0. Production implementation unchanged; main/implementation branches intact. Certification branch only; no main integration. See UNREAL-HUD-PANEL-REGISTRY-001-INDEPENDENT-20261016 and CURRENT.

UPDATE 2026-10-15 stackable independent certification CERTIFIED: candidate 68f2ffe, isolated certification/unreal-stackable-sprite-patterns-001-20261015. Fresh extraction verified 432 cumulative frames/81 types and 4832 single-image non-stackables; gold x1/frame0 and x12/frame5 live in inventory/container, legitimate Obi worms x6/frame4, authoritative counts unchanged, actual Slate missing/malformed fallback PASS. ClientCore 13/13, native 4/4, MSVC, Unreal, six secret checks PASS. Final harness 9 PASS/0 FAIL/0 SKIP; all 51 live snapshots protocol0. Implementation unchanged; world mesh amount invariance is the accepted boundary. No push/main merge. See UNREAL-STACKABLE-SPRITE-PATTERNS-001-INDEPENDENT-20261015.

UPDATE 2026-10-15 16:10: UNREAL-WORLD-PRESENTATION-POLISH-001 CERTIFIED by independent review of bc9ce8fd in isolated worktree/branch certification/unreal-world-presentation-polish-001-20261015, built from scratch; the implementing worktree was not used and main was NOT merged. All seventeen named criteria reproduced live first-hand in ONE new 2811s connection: camera collision, roof/floor and wall readability, creature/NPC readability, world and NPC speech, Look, Use, Use With, world-world drag, inventory-ground drag, ground-inventory pickup, creature push, Follow/Stop, container, minimap click with exact authoritative arrival, WideWorld 187loads/125unloads/62sectors, lamp 2109->2108 with untouched twin as control. EndPlay residual/unsupported/anomalies0 across all30 snapshots,0unanswered walks. Nine ClientCore suites, three Unreal suites, MSVC archive, REAL33DEditor build and six-subcheck secret_check PASS; harness from its preserved worktree 377b47be OVERALL PASS 9/0/0 with 293 logs scanned. Minimap cache started EMPTY and discovered1819cells. Finding recorded, not patched: secret_check.sh passes vacuously under WSL in a linked worktree because git cannot resolve the Windows gitdir; run it from Git Bash and require six ok lines. Art completeness, grass/roof offsets, underground WideWorld coverage and right-click Use With cancel remain open exactly as the candidate left them. See UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-20261015.
UPDATE 2026-10-15 14:52: UNREAL-WORLD-PRESENTATION-POLISH-001 PASS LOCAL. Missing final-build Use With/feedback, minimap exact arrival and real rat push now live evidenced; combined compact regression on unchanged3ace436 implementation PASS. EndPlay7267commands, residual/unsupported/anomalies0,0unanswered walks. Nine ClientCore suites, three Unreal suites and up-to-date build PASS. No source/art/server changes, independent certification or main merge. New grass/roof offsets remain unclassified/open; underground WideWorld is surface-only static coverage and closer manual zoom is operator-confirmed playable. Right-click Use With cancel remains requested/unimplemented. See FINAL-QA-20261015 report and CURRENT for bounded acceptance, evidence and closeout checks.
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
2026-10-14 operator-assisted polish QA: preserved 54f6abc pushed/verified. No code changes. CAMERA_COLLISION FAILED operator acceptance (requests removal). WORLD_SPEECH, USE_FEEDBACK, CONTAINER_SMOKE, FOLLOW/STOP smoke and bounded WALL_OCCLUSION PASS with live PNG/JSON/log evidence. Use With, minimap-click attribution, NPC response and full regression IMPLEMENTED_UNVERIFIED. Protocol errors0 in both EndPlay snapshots. Overall IMPLEMENTED_UNVERIFIED; no certification/main merge. See evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-OPERATOR-QA-20261014.md.

# Project Status

## UNREAL-SKILLS-PANEL-001 independent review - 2026-10-17

`CERTIFIED`. Candidate63a62ddf9477cb8ccdc91525d7f505fee192f2f4
on base main370b5676e2a186d44e11aeffe1fb4766593f8d9e, isolated certification
worktree. No implementation defect or change. Independent actual-widget tests
and retained live session verify all seven skills, authoritative values,
unsent dashes, received zero and percentage bars. Sword11 progresses67->83%
after three ordinary world attacks; XP471->486. Six Unreal tests PASS; final
ClientCore13/13/native4/4 and harness selftest10/10 PASS. Eight live snapshots
have zero residual bytes/unsupported opcodes/anomalies. Final exact-candidate
Unreal build PASS (9.06s), five exact-source automation tests PASS; corrected
strict harness9PASS/0FAIL/0SKIP,42logs scanned; secret_check6/6 PASS.
Report: evidence/clientcore/UNREAL-SKILLS-PANEL-001-INDEPENDENT-20261017.md.
No merge to main and no push.

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

2026-10-07 live follow-up: operator rejected underground appearance and reported
missing inventory pictures. Captures confirm centered dirt borders forming
disconnected strips. Traced source masked_ground/collapse to lost XY offsets;
runtime correction restores eight reviewed border offsets, with twelve actual
mesh-footprint checks. Inventory sprites regenerated and independently compared
(4990 layouts/8163 sprites, zero failures); launcher now generates them per
worktree. Brother's existing16-entry creature catalog is enabled; live rat21 and
snake28 loads verified. Build and three targeted automation tests PASS; post-fix
live comparison still pending. Milestone remains IMPLEMENTED_UNVERIFIED.

2026-10-07 recovery: UNREAL-WORLD-PRESENTATION-POLISH-001 remains
IMPLEMENTED_UNVERIFIED. Remote/main fa3adcf and ancestry through 393e7e8 to
local/remote polish HEAD 4305d23 verified; eight existing modified source/test
files and render-fix evidence preserved. Recovered successful material-adapter
automation and October 3 textured-WideWorld screenshot/EndPlay. Fresh nine-suite
ClientCore, Unreal build, three targeted automation tests, stale-report refusal,
diff check and tracked secret check PASS. Test runner now validates fresh named
results. Live acceptance continues with normal operator input; no reset, branch
creation, main merge, push or frozen-agent edit. See
evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RECOVERY-20261007.md.

2026-09-29 16:58 operator preservation checkpoint: UNREAL-WORLD-PRESENTATION-POLISH-001
remains IMPLEMENTED_UNVERIFIED, not PASS LOCAL. Fresh native nine-suite tests,
Unreal build and two Unreal automation cases PASS. Fresh live camera/zoom/follow,
aligned labels, rat attack/follow feedback, building roof switching and floor7->8->7
were exercised. Underground ground/water overlap FAILED readability; wall aliases
show perpendicular fins. Fixes and full live regression remain pending. Protocol
errors0 in last retained snapshot. Operator requested commit/push now; no main
merge. Details: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-CONTINUATION-20260929.md.

2026-09-29 recovery/preservation: UNREAL-WORLD-PRESENTATION-POLISH-001 remains
IMPLEMENTED_UNVERIFIED. Existing branch 393e7e8 descends from certified main
fa3adcf. Preserved pending overlay projection fix and live geometry diagnostics;
added all-live-floor diagnostics and non-overwriting automation evidence paths.
Fresh native nine-suite ClientCore and Unreal build PASS; secret check PASS.
Recovered prior screenshot shows aligned labels, but fresh automation was
interrupted and no new gameplay QA occurred before operator requested commit/push.
All live acceptance gaps remain; no main merge or agent changes. See
evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-RESUME-20260929.md.

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

2026-09-28 delta certification: `UNREAL-MINIMAP-NAVIGATION-001 = CERTIFIED`.
Unchanged starting/remote HEAD `34e9bfd91f8863b642536221427ce297779d441a`;
base/main remains `a97cf25e7449a8a3ef35ef2553181c5032c7a247`.
New independent live south marker checkpoint, real floor7->6->7/restoration,
known minimap-click arrival, server-refused route step, manual route interruption,
authoritative boundary and Follow/Stop/container smoke all PASS. Protocol errors0.
Retained independent PASS tests/build/unknown-arrival evidence were not repeated.
Both prior evidence reports and the prior handoff are preserved unchanged.
Certification evidence/status/handoff changes only; frozen agent checkout untouched.
Milestone remains unmerged. WORLD-PRESENTATION-POLISH has not started.
See `evidence/clientcore/UNREAL-MINIMAP-NAVIGATION-001-INDEPENDENT-DELTA-20260928.md`.

2026-09-28 closeout: `UNREAL-MINIMAP-NAVIGATION-001 = PASS` local,
isolated branch `milestone/unreal-minimap-navigation-001`, base
`a97cf25e7449a8a3ef35ef2553181c5032c7a247`. The certified item-use milestone
was integrated by fast-forward and pushed to main at that exact commit.
The earlier visual-editor next-step recommendation below is superseded by the
operator's explicit minimap scope; no visual editor or V08 art changes here.
Observed terrain cache, north-up HUD map, player/floor synchronization and local
controls are implemented. Live exploration and floor round-trip evidence is
retained; operator accepted the original 7.72 colors. The final revision removes
live-tile highlighting and accepts unknown same-floor destinations, advancing
through observed segments as regular map descriptions arrive. Known routes
have no viewport/distance cutoff. Current-step validation/server acceptance are
unchanged. Final live unknown-goal continuation, real refusal stop, all cardinal
directions, cache restoration, 7->6->7 floor round trip, Follow/Stop and zero
protocol errors are retained. Operator confirms the remaining UI interactions;
individual pan/zoom/recenter/container checkpoints are limited. Native nine
suites, pure view/palette, sanitizers, Unreal build and secret check pass.
The earlier independent attempt was IMPLEMENTED_UNVERIFIED; the delta above closes
its gaps and certifies the milestone. The branch remains unmerged.
See `docs/UNREAL_MINIMAP_NAVIGATION.md` and the current handoff.

Latest independent run: world/slot Look and normal chat visibly confirmed after
HUD repair; live world/inventory/container Use and valid Use With reproduced.
Operator requested automatic target selection from normal Use for MultiUse
items, including runes. That correction has tests/build and live rapier Use With
PASS. Original cursor works; 16-outfit creature art loaded
and accepted with defects for now. The 156-outfit sprite reference catalog is
not 156 finished 3D models. Corrected interaction milestone is CERTIFIED;
rune spell effects and complete art coverage remain unverified.

Current phase: `PHASE 2 - GAMEPLAY CLIENT PROGRAMMING` (`IN_PROGRESS`; the first 3D representation is live, a stock 2D client now completes the ordinary 7.72 flow, and V08 visual review is on operator-directed standby)
Current milestone: UNREAL-MINIMAP-NAVIGATION-001 - `CERTIFIED` after independent delta certification. Both prior reports are preserved; see `evidence/clientcore/UNREAL-MINIMAP-NAVIGATION-001-INDEPENDENT-DELTA-20260928.md`.
Next task: await explicit minimap integration instructions; no merge authorized. WORLD-PRESENTATION-POLISH has not started.
Last certified: UNREAL-MINIMAP-NAVIGATION-001 on the unmerged milestone branch. Prior UNREAL-ITEM-USE-INTERACTION-001 remains certified and integrated at `a97cf25e7449a8a3ef35ef2553181c5032c7a247`; original candidate `3528459` is not retroactively certified. Rune spell effects and complete art coverage remain `IMPLEMENTED_UNVERIFIED`. Prior combat/follow, inventory/containers and WideWorld certifications remain preserved.
Branch: `milestone/unreal-minimap-navigation-001` (from integrated `origin/main`, not merged)
Classic baseline review commit: `f65f3a7645ff40b39b7cc8399760fd4f0b69ecee`
Transport implementation commit: `abd2d0a25bd9632f5aa3955e822876268c7ca96c`
Crypto implementation commit: `64e9217ef64181d44cdce815a36b6bb1d2aa9038`
Login implementation commit: `a56add56e7ad5a11fd2e28d642bada4390b16756`
Game Login implementation commit: `3db23f9`
Initial World implementation commit: `fedd536`
Movement implementation commit: `37fa5f6`
Player State implementation commit: `af3e3ac`
Two-client slice commit: `a048150`
Visual inventory commit: `4260d98`
Visual reference pack commit: `3e0bea0`
Unreal slice commit: `da36b86`; corrective certification commit: `cae6450`
Chat commit: `e756bb7`; chat presentation commit: `cf034eb`
Dual-client research commit: `faf8852`; live capture commit: `0f9bd505`
Worktree: V08 presentation corrections and QA evidence recorded on main; final visual certification is `STANDBY` at the operator's direction
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`; V08 presentation corrections were published on `main` without force push. Screenshots are the first binaries committed and go through Git LFS per `.gitattributes`. No history was rewritten and no force push was used. The first attempt returned HTTP 403 because the stored credential belonged to `leodavidsoto`, which holds only `READ` on that repo; the operator re-authenticated `gh` as `EterniaOnSol`, which holds `admin`

Server reference source: immutable candidates selected, independently statically reviewed, materialized and `BUILD PASS`; `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` after independent live repetition
Protocol: `IN_PROGRESS`; Transport, Crypto, Login, Game Login, the `FULLSCREEN` initial world snapshot, cardinal movement with its incremental map updates, and the player/session command set are `PASS` for deterministic fixtures plus bounded local synthetic-account smoke. A whole login burst and ordinary session traffic now decode with zero residual bytes; containers, trade, the request queue and the editors remain recognized by name but unparsed. Chat was in that list until `CHAT-772-001` decoded `SV_CMD_TALK` from the three `SendTalk` overloads, with the mode-dependent tail and the modes no overload accepts both handled explicitly
Client core: Unreal-independent C++17 TCP/framing/crypto/Login/Game Login/Initial World/Movement/Player State components implemented, plus a `WorldView` adapter that diffs successive WorldStates into semantic events so the engine never sees protocol. Retained and new suites pass in normal and ASan/UBSan WSL builds. The native Windows build is no longer unverified: `tests/build_clientcore_windows.cmd` builds every source with MSVC at `/W4 /WX /permissive-`, at C++17 for portability and again at C++20 for the Unreal link, and runs all eight suites against the C++17 build. Its first run found four real portability defects that GCC accepts silently
Client: selected local Tibia 7.72 EXE/DAT/SPR/PIC set hashed and statically validated; Login, character list, Game entry and a session over 30 minutes are `PASS`; historical acquisition provenance is `UNKNOWN`
IP Changer: official Fusion32 revision `8215db18...` independently reviewed, materialized and Windows x86 `BUILD PASS`; exact 7.72 address table and fresh-modulus live patch are `PASS` for the selected client hash
Visual: `visual/` area created; master inventory regenerated entirely by tooling from `objects.srv`, `map.dat`, `origmap`, `monster.db`, `mon/`, `npc/` and the source enums. 5,690 tracker rows covering 5,003 object types, 159 monster races, 337 npcs, 152 outfit identities, 26 effects and 13 projectiles; 1,351 visual groups, so roughly three quarters of the object ids are reuse candidates. `VISUAL-ASSET-MASTER-INVENTORY-001 = PASS`. `VISUAL-REFERENCE-PACK-001 = PASS`: the 7.72 appearance data was located in the authorised local workspace, its format established by four independent validations rather than assumed, and 5,284 appearances decoded with 10,926 sprites and 5,284 previews, zero failures. The artist catalogue is rebuilt locally by one command and gitignored, since previews derive from an artifact with `UNKNOWN` provenance. `VISUAL-FULL-CATALOG-INGEST-TEST-001 = STANDBY`: 4,913 V08 models imported and mapped; UE 5.8 gallery and Fusion32 live loading were observed. Operator visual review found short and misoriented walls, floor occlusion, and camera issues. Unreal-only corrections are under review; artistic approval remains NO.
Unreal: `IN_PROGRESS`. `unreal/REAL33D/` is an Unreal Engine 5.8.2 project with one runtime module that links Protocol772Core as a prebuilt static library and contains no protocol source of its own. A worker thread owns the socket and WorldState; the game thread drains semantic events and is the only thread that touches an Actor. The first 3D representation is live: one actor per visible field, one per visible creature, a documented horizontal transform at `1 SQM = 100 Unreal Units` with 220 Unreal units of presentation floor spacing relative to the session's first anchor, and input that produces an intent for Fusion32 rather than a movement. `UReal33DAssetRegistry` resolves engine placeholders by default and command-line-gated V08 meshes for experimental QA. The project carries no `.umap`, no `.uasset` and no input asset: the scene is built in C++ on the empty engine map. The experimental UI now includes a full V08 catalog gallery for programmatic visual inspection.
Parity: `IN_PROGRESS` for state, `NOT_STARTED` for appearance. `UNREAL-SLICE-001` had the original `Tibia.exe` and the Unreal client in one Fusion32 world simultaneously, agreeing on positions, appearances, movement and viewport membership. No cell claims visual parity while every mesh is a placeholder.

Last certified test: `SERVER-RUNTIME-SMOKE-001`; independent reviewer repeated prepare, start, smoke and stop with all assertions passing
Latest functional tests: classic baseline IDs, `CLIENTCORE-TRANSPORT-772-001`, `CLIENTCORE-CRYPTO-772-001`, `CLIENTCORE-LOGIN-772-001`, `CLIENTCORE-GAMELOGIN-772-001`, `INITIALWORLD-772-001`, `MOVEMENT-772-001`, `PLAYERSTATE-772-001`, `TWO-CLIENT-VERTICAL-SLICE-001`, `VISUAL-ASSET-MASTER-INVENTORY-001`, `VISUAL-REFERENCE-PACK-001` are `PASS`; `VISUAL-FULL-CATALOG-INGEST-TEST-001` is `STANDBY`; `UNREAL-SLICE-001` is `PASS` with criterion 12 qualified, after a corrective run re-established criteria 6 and 7, which the first report had credited to a push rather than to Unreal input
Two-client result: the original `Tibia.exe` and Protocol772Core held two distinct characters in one world for 19 minutes; appearance, 11 tracked steps, viewport-edge departure and return, two accepted client-initiated moves, three refusals caused by the other player physically blocking the field, disconnect and reconnect, with 689 commands, zero residual bytes, zero anomalies and zero unsupported opcodes
Unreal slice result: the original `Tibia.exe` and the Unreal client held two distinct characters in one world simultaneously. Player A appeared in Unreal by name and position, his steps moved his capsule on the correct axis, A left and re-entered B's viewport with no duplicate or orphan, and a real server-side drop tore the scene from 418 actors to zero before an in-session reconnect rebuilt it to 434. Tile and creature actor counts equalled WorldState at every point sampled. Criterion 12 is qualified: `protocol_anomalies` was always zero and two multi-minute windows were entirely clean, but `SV_CMD_TALK` remains undecoded and the operator used the chat
Chat presentation result: speech decoded by `CHAT-772-001` is now drawn above the creature that said it. The speaker is resolved in ClientCore by matching both the talk position and, where present, the sender name against visible WorldState creatures, so Unreal receives a creature id and never a matching rule; anything not uniquely resolvable goes to a visible fallback rather than to a guessed creature. Fusion32 sends no duration and has no command to retract speech, so lifetime is client presentation and its exact 7.72 rule is unprovable from this project material, which holds the classic client only as a binary
Chat result: a live session using chat reached zero unsupported opcodes, zero protocol anomalies and zero residual bytes over 221 frames and 235 commands, with three talk commands decoded. The decoded speech position cross-checks against the same snapshot's independently tracked creature position for Player A. `TALK_YELL` could not be exercised because yelling is level-gated in 7.72 and both test characters are level 1; the channel and plain forms need a channel or a gamemaster, which this sanitized runtime has not, so they are covered deterministically only
Unreal slice correction: the first report credited criteria 6 and 7 to a session in which the operator never controlled B from Unreal. B moved there because A pushed him, which Fusion32 resolved authoritatively; that is incoming-path evidence only. Every retained snapshot from that session shows `steps_requested: 0`, so the numbers the report quoted had no preserved artifact behind them. Both criteria were withdrawn and re-established by a corrective run in which the operator drove B from the Unreal window: 19 requests, 18 accepted, 1 refused, 0 unanswered, each joined from key press to authoritative position by an `input_id`, plus 6 external relocations counted apart, two of them diagonal and therefore impossible to have been requested. A `MovementLedger` in ClientCore now makes the distinction structurally, with six deterministic tests including a replay of the original eight-field push. Details in `evidence/clientcore/UNREAL-SLICE-001-CORRECTION.md`
Current certification blockers: no verifiable original source/chain of custody for the operator-supplied local client copy; no independent repetition of the live client procedure; the Unreal slice is one run by one operator, with no floor transition exercised in 3D and criterion 7 resting on human observation alone
Closing ritual from `PLAYERSTATE-772-001` onwards: tests + sanitizers + evidence + docs + commit + handoff + push, with `tests/secret_check.sh` run before every push
Next gameplay candidate: trade or a live floor transition. Select one bounded scope from authoritative Fusion32 source before implementation. V08 certification remains on STANDBY.

Updated: 2026-09-17

## Visual ingest audit ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â 2026-09-20

`REAL33D-VISUAL-INGEST-AUDIT-001 = PASS` only for the internal `visual/` reference area; its former artist-source conclusion was corrected. `REAL33D-3DTIBIA-ART-AUDIT-001 = PASS` for a read-only inventory of the actual source clone `C:/Users/dell/3DTIBIA_leo/motor3d/assets` at `21fafb57dd86b594223bab7dbe9076d1fa380640`: 96,478 files, including four GLB and four BLEND. Four GLBs map `EXACT` to three 7.72 outfit visual identities by pixel-checked source evidence, but the source project documents all four as failed/rejected experiments. No primary-source 3D asset is ready for a P0 vertical import. `TECHNICAL_IMPORT`, `IN_ENGINE_RENDER` and `VISUAL_APPROVAL` remain `NOT_STARTED`; no Unreal, gameplay, protocol or ClientCore change was made. The chat milestone and prior functional claims above are unchanged. Corrected evidence and next step: `visual/docs/EXTERNAL_3DTIBIA_INGEST_AUDIT_2026-09-20.md`.

## Operator direction - 2026-09-21

V08 visual certification is on standby, not PASS. Preserve the current imports and inspection notes; final gallery review, live identity proof, and artistic approval remain open. Resume gameplay client programming. The next feature has not yet been selected.

## Gameplay client programming - 2026-09-21

UNREAL-PLAYER-VITALS-001 is IMPLEMENTED_UNVERIFIED. The worker publishes semantic PlayerVitals after applying PlayerData to WorldState; the game thread stores it and the HUD shows HP, mana and level, clearing them on disconnect. No protocol, server, or V08 visual asset was changed. The live log showed Player B connected and the HUD changed from unknown to HP 145/145, Mana 0/65531, Level 1. See evidence/clientcore/UNREAL-PLAYER-VITALS-001.md. The V08 catalog remains STANDBY and is not PASS.

## Wide-world streamer - 2026-09-21

UNREAL-WIDE-WORLD-001 = CERTIFIED_PASS. Unreal loads strict, generated 32ÃƒÆ’Ã¢â‚¬â€32 .wws sector caches around the authoritative anchor, keeps the complete 18ÃƒÆ’Ã¢â‚¬â€14 live window owned by WorldState, suppresses static tiles as they enter live authority, retains parsed data for cache hits, and destroys distant sector actors on the game thread. Async work is restricted to file IO and row parsing. Static baseline sectors never invent dynamic state.

Frozen V08 meshes resolve first. Missing physical V08 meshes use local gitignored classic previews as CLASSIC_SPRITE_FALLBACK; MISSING_PHYSICAL_ASSET remains separate and non-blocking. Thais exact-radius V08/fallback/missing occurrence counts are 12,286/0/0 at 32, 38,826/3/0 at 64, 71,845/8/0 at 96, and 110,073/17/0 at 128. TypeId 469 is explicitly CLASSIC_SPRITE_FALLBACK. The catalogue hash is unchanged and no V08 asset, ClientCore, protocol, server, or reference source changed. See evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

Live certification: at (32094,32203,7), three initial stationary visibility audits returned zero mismatches after correcting the classic billboard hidden-in-game default. A real round-trip across x=32096 loaded and unloaded nine sectors in each direction. Six crossing/return audits found zero static/live overlaps, zero visibility mismatches, and a live tile under the player. See evidence/clientcore/UNREAL-WIDE-WORLD-001.md.

## Dual-client architecture closeout - 2026-09-21

`DUAL-CLIENT-ARCHITECTURE-CLOSEOUT-001`. Documentation only; no code, protocol,
server, Unreal or V08 change.

### Selected production architecture

```
                        Fusion32
                           |
             +-------------+-------------+
             |                           |
        REAL33D 2D                    REAL33D 3D
     OTClient-derived                   Unreal
             |                           |
     own parser/model            ClientCore + WorldState
             |                           |
             +---- same protocol spec ---+
                  shared fixtures
```

- `REAL33D_2D` = mehah/OTClient-derived production client.
- `REAL33D_3D` = Unreal + ClientCore production client.
- `Fusion32` = sole authoritative game server.
- Official `Tibia.exe` 7.72 = QA / parity / reference only, **not** a production client.
- IP changer = legacy QA utility only, **not** production infrastructure.

OTClient keeps its own parser and game model; ClientCore remains the 3D
implementation and the protocol reference. We do **not** replace OTClient's model
with ClientCore's WorldState: its UI rests on 1,061 Lua bindings and 207 distinct
`g_game.*`/`g_map.*` call sites over its own object model, so substitution would
be a rewrite rather than an integration. The two clients share the protocol
specification, byte-level fixtures, semantic expectations and Fusion32 authority,
so drift surfaces as a failing fixture.

### DUAL_CLIENT_LIVE_CAPTURE = PASS (`0f9bd505`)

A stock `mehah/otclient` build completed the full ordinary 7.72 flow against the
unmodified server with configuration only. `STOCK_OTCLIENT_CONNECT`, `LOGIN`,
`CHARACTER_LIST`, `GAMELOGIN`, `INITIAL_WORLD`, `CLIENT_INITIATED_MOVEMENT`,
`SAY_CHAT`, `ITEMS`, `INVENTORY`, `CONTAINERS`, `ITEM_MOVE` and
`LOGOUT_RECONNECT` are all PASS. Incoming protocol decode errors, unknown
incoming opcodes and unsupported incoming opcodes are each 0 over 103 traced
packets and 20 distinct opcodes. `SERVER_CHANGES_REQUIRED = 0`,
`STOCK_CLIENT_PARSER_PATCHES_REQUIRED = 0`, `REQUIRED_PRODUCTION_PATCHES = 1`.
`OPTION_C_VIABLE_FOR_PRODUCTION = YES`.

Remaining patch: `TALK_MODE_14 = KNOWN_PRODUCTION_PATCH_REQUIRED`,
`NOT_LIVE_TESTED`, one `protocolcodes.cpp` table entry. It needs a gamemaster
anonymous channel call this QA runtime cannot produce.

DIV-08: `CATALOGUE_STATIC_MATCH = 4990/4990`, `LIVE_CONFIRMED = 5/5` observed
TypeIds; cumulative and liquid classes are `NOT_LIVE_COVERED` because every
TypeId seen live was zero-extra-byte. TypeId 5090 is
`KNOWN_SERVER_ONLY_OR_UNREACHED_EXCEPTION`.

### Commit attribution correction (documentary; no history rewritten)

- `3fd5d1d` = `UNREAL-WIDE-WORLD-001` CERTIFIED_PASS.
- `c7bcfe7` = principally `UNREAL-PLAYER-VITALS-001` and other concurrent work,
  despite its message naming UNREAL-WIDE-WORLD-001.
  `UNREAL-PLAYER-VITALS-001` remains `IMPLEMENTED_UNVERIFIED`.

### Next milestone - REAL33D-2D-BOOTSTRAP-001 (NOT_STARTED)

Turn the successful isolated OTClient probe into a clean, reproducible
REAL33D-owned 2D client: pinned upstream commit, REAL33D branding, Fusion32
host/port, protocol 772 preset, terminal OS/type, Fusion32 public RSA modulus,
`GameEnvironmentEffect` compatibility, the TALK mode 14 entry, justified removal
of irrelevant OTClient ecosystem startup modules, a reproducible Windows build,
no embedded credentials, no private RSA/XTEA material, no accidental
`Tibia.dat`/`Tibia.spr` publication, and a login to gameplay to logout/reconnect
acceptance run.

Out of scope: shops, extended opcode 50, new TerminalTypes, 8.6 protocol, new
server features, Unreal changes, V08 changes, visual/art work.

The probe scratch tree `C:\r33dprobe` stays untouched until the bootstrap
reproduces the live PASS. Nothing in this repository references it.

## Inventory, containers and use - 2026-09-23

`UNREAL-INVENTORY-CONTAINERS-001 = CERTIFIED_PASS`. The round trip `723b557`
left explicitly uncertified was exercised against the running server on one
uninterrupted session, and all eight acceptance criteria hold:

```
RIGHT_CLICK_OPENS_CONTAINER      PASS   DISPLAYED_CONTENTS_MATCH_SERVER  PASS
MOVE_INSIDE_CONTAINER_LIVE       PASS   CLOSE_REMOVES_STATE_AND_PANEL    PASS
NESTED_CONTAINER_OWN_WINDOW      PASS   USE_WITH_ONE_VALID_TARGET        PASS
INVENTORY_EQUIPMENT_CORRECT      PASS   PROTOCOL_ERRORS                  0

residual_bytes 0   unsupported_opcodes 0   protocol_anomalies 0   over 88 commands
```

The contents drawn were compared against the server's own `1002.usr` save file
object for object, including the amount byte of a cumulative object and the
colour byte of a liquid container - the two classes `DUAL_CLIENT_LIVE_CAPTURE`
had to leave `NOT_LIVE_COVERED`. Use-with was exercised on a real
`dat/moveuse.dat` rule: flour on a bucket of water produced a lump of dough and
emptied the bucket, with one of the five flour units converted because flour is
cumulative. Closing went through `UseContainer`'s 7.72 toggle;
`CL_CMD_CLOSE_CONTAINER` and `CL_CMD_UP_CONTAINER` remain unbuilt.

Evidence, with the eight `F9` snapshots and the command log:
`evidence/clientcore/UNREAL-INVENTORY-CONTAINERS-001.md`.

### One live failure, found and fixed

The operator could not tell a bag from a barrel in the container window.
`scripts/client/extract_item_sprites.py` indexed the sprite offset table with
the sprite id, but sprite ids are one-based, so every object was drawn with the
sprite after the one it meant. Invisible when that neighbour was another frame
of the same object, glaring when it belonged to the next object. Fixed, and
`tests/verify_item_sprite_extraction.py` now holds that extractor to
`visual/tools/tibia772.py`, the reader `VISUAL-REFERENCE-PACK-001` validated:
4990 items agree on geometry and first-frame sprite ids and 8163 decoded
sprites agree byte for byte. The same test reports 8163 of 8163 mismatching
against the pre-fix indexing.

### Operator requests

Container windows now draw their whole capacity, as a 7.72 client does, rather
than their contents plus one empty square, and the containers panel grew from
200 to 420 so a backpack and a bag opened inside it both fit. Still open, and
the natural next milestone: the 2D client opens container windows minimised,
resizable, and movable between columns. None of that exists here.

## Combat and follow - 2026-09-23

`UNREAL-COMBAT-FOLLOW-001 = CERTIFIED_PASS`. ClientCore emits Fusion32's exact
7.72 `CL_CMD_ATTACK`, `CL_CMD_FOLLOW`, `CL_CMD_CANCEL` and
`CL_CMD_SET_TACTICS` bodies and owns the logical combat state in `WorldState`.
The server remains authoritative: accepted targets are silent on this protocol,
while rejection, cancellation, death, disappearance and range loss converge on
`SV_CMD_CLEAR_TARGET`. Slate and creature actors only present that shared state.

Live REAL33D acceptance covered attack and follow, target replacement, both
cancel paths, a real `Target lost` rejection, target removal, Battle List/world
agreement, world right-click attack, and right-click use/open on a real dead
rabbit container. The operator confirmed the Battle List follow flow and the
fight-stance controls. The final evidence snapshot retained correct inventory
and an open `dead rabbit` container, with 0 residual bytes, 0 unsupported
opcodes and 0 protocol anomalies over 496 decoded commands. Native ClientCore
tests and the final `REAL33DEditor Win64 Development` build pass. Evidence:
`evidence/clientcore/UNREAL-COMBAT-FOLLOW-001.md`.

All three attack stances and stand/follow are supported because
`receiving.cc::CSetTactics` implements them. Range chase remains absent because
the same handler rejects it. No damage, cooldown, HP, client-side follow
movement, protocol extension or fake Slate target state was added. V08,
WideWorld, REAL33D2D, shops, action bars, automap, reconnect handling and opcode
50 were not changed.

## Item use and Look - 2026-09-27

`UNREAL-ITEM-USE-INTERACTION-001 = PASS`. The audited 3D
client already had ordinary Use from world, body and open containers, Use With
object/creature/field targeting, and server-owned container/message results.
`UNREAL-INVENTORY-CONTAINERS-001` certified body and nested-container opening
and a real flour-on-bucket Use With result; `UNREAL-COMBAT-FOLLOW-001`
recorded world right-click opening a corpse. This milestone adds the missing
`CL_CMD_LOOK_AT_POINT` builder and Alt+right-click from a world point,
creature position, body slot or open-container slot. Fusion32 selects the
object and sends the description as its existing `SV_CMD_MESSAGE`; no local
description is constructed.

Native MSVC C++17/C++20 builds and all eight ClientCore suites passed, as did
`REAL33DEditor Win64 Development`. A live REAL33D session connected account B
and drew the operator's local V08 meshes with WideWorld radius 64 and 69
sector-load records. A retained F9 sample at 23:37:59 UTC had 532 decoded
commands, zero residual bytes, zero unsupported opcodes and zero protocol
anomalies. A later live log retained three world Look request/reply pairs:
mountain, framework wall and grass descriptions arrived as server
`InfoMessage` within 76-131 ms of the respective requests. The live HUD
showed zero unsupported opcodes and anomalies after Look. Body/container
Look UI routes are compiled and byte-tested but do not yet have a retained
live click. The milestone is local `PASS`, not independently `CERTIFIED`.
See `evidence/clientcore/UNREAL-ITEM-USE-INTERACTION-001.md`.

The V08/WideWorld launcher now checks for a sector cache and passes its path,
preview path and radius explicitly. Its local QA meshes and cache remain ignored.
The next 3D task is the operator-requested in-world visual editor. Its approved
scope is local presentation overrides only: select a visible instance, change
mesh, orientation or grass presentation while navigating, and persist the
visual edits without altering Fusion32 world/gameplay state.
The existing V08 inspector's `Girar 90` only stores a `ROTATE_90` note; it
does not yet rotate a wall, so the editor is still `NOT_STARTED`.

## Operator-assisted continuation - 2026-10-14 22:10 local
At6f864e3, evidence-only QA with user providing one action at a time. Existing server/client verified. Baseline F9 captured; no new remaining criterion PASS. Idle disconnect auto-reconnected04:09:47Z; pending world Look action. Report: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-ASSISTED-20261014.md. Overall IMPLEMENTED_UNVERIFIED. No code/build/art changes or certification/main merge.
