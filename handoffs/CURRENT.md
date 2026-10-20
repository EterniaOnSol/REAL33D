# HANDOFF - UNREAL-TRADE-UI-001

Date/time: 2026-10-20, America/Guatemala
Agent / role: Claude; implementation and operator-assisted two-account live QA
Starting commit: `76a261992c6740ecfc612b5421dd91391a06c4d5` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-trade-ui-001`
Worktree: `C:/Users/dell/Desktop/fusion32`

TRADE_UI_001_STATUS = PASS LOCAL
MAIN_MERGE = NONE. PUSH = NONE.

## Objective

Player-to-player trade: start from a legitimate interaction, show both
offers, inspect, accept, reject, close on SV_CMD_CLOSE_TRADE, server
authoritative, no fake local success, isolated panel, minimal HUD edits.
NPC shops explicitly out of scope.

## Discoveries worth keeping

- **ClientCore already had it all.** `trade.cpp` carries the builders and
  decoder; `ApplyServerUpdate` stores nothing for `ServerUpdateKind::Trade`.
  No ClientCore file changed; the suite count stays 13.
- **The first offer is prose, not a command.** `TCreature::Trade` tells a
  partner who has not yet offered only
  `"%s wants to trade with you."` through SV_CMD_MESSAGE. Both sides see
  offers only once both have put an object up.
- **Success and failure close identically.** `AcceptTrade` sends
  SV_CMD_CLOSE_TRADE to both sides after moving the objects **and** when a
  reach/capacity check fails. No client can tell them apart from the close;
  only the inventory can.
- **Accepting is not completing.** `AcceptTrade` sets the flag and returns
  while the other side has not accepted.
- **Rejecting is one-sided.** `RejectTrade` sends the **partner** a close and
  "Trade cancelled." and sends the rejecter nothing, so the rejecting client
  must clear its own window.
- **The offer list is flat** and is shown exactly as sent; nothing on the
  wire marks where a bag ends.

## Changes

New: `Real33DTradePanel.h/.cpp`.
Changed: `Real33DBridge.h/.cpp` (three event kinds, the single open trade,
four commands as intents, seven counters), `Real33DPanelChrome.h/.cpp`
(ctrl+right-click raises `EReal33DSlotAction::ContextMenu`, taken before the
existing gesture machine so its native suite is untouched),
`Real33DHUD.h/.cpp` (the context menu, the trade half of the existing
pick-then-aim target, one AddPanel, one member),
`Real33DPanelRegistry.h/.cpp`, `Real33DWorldActor.cpp`,
`Real33DPresentationTests.cpp`, `PARITY_MATRIX.md`, `PROJECT_STATUS.md`.

## The defect the live run found, and the operator's correction

The first build put the Offer control inside the trade window and showed that
window only once a trade existed. Live, A offered, Fusion32 told B
*"Test Player A wants to trade with you."* and nothing else — so B had no
window and **no way to respond at all**.

Parsing that English sentence to infer a pending trade was refused: it is the
same invention this project rejected for the text editors. **The operator
named the authentic gesture** — in Tibia you ctrl+right-click the item and
choose *Trade with...* — and that is now the entry point. The invented Offer
button is gone and the panel is docked so it shows the idle state instead of
hiding the only way in.

## Tests

ClientCore 13/13, native 5/5, Unreal automation 8/8 with zero errors and
warnings, MSVC archive, REAL33DEditor build, harness 9 PASS / 0 FAIL / 0 SKIP,
Git Bash secret_check 6/6.

Live, two real accounts: 9 context menus, 7 offers dispatched -> 7 own offers
returned, 7 partner offers, 2 inspects, 4 accepts, 2 rejects, 5 server
closes, **0 trade refusals, 0 undecoded commands, 0 decode failures**.

## Remaining UNVERIFIED

- **Which close meant success** — unverifiable by design, for any client.
- The accept-time failure closes (TOOHEAVY, NOROOM, NOTMOVABLE, NOTTAKABLE,
  CONTAINERFULL) — none occurred.
- Multi-object offers: every offer carried one object, so the flat container
  walk was never exercised against a real bag.
- Offer-time refusals (ALREADYTRADING, PARTNERTRADING, TOOMANYOBJECTS,
  OUTOFRANGE, CANNOTTHROW) — none occurred.
- B's saved inventory returned to its baseline type composition; the
  per-trade ledger was not reconstructed and is not claimed. A's file shows
  the transfer.
- NPC shops: out of scope, untouched.

## Operator request parked for the next milestone

The operator asked for the same ctrl+right-click menu **on their own
character**, to change outfit and colours. That is the outfit dialog and it
is NOT in this milestone: `SV_CMD_OUTFIT` (200) is decoded in ClientCore
(`DecodeOutfitDialog`), but there are **no builders** for
`CL_CMD_GET_OUTFIT` / `CL_CMD_SET_OUTFIT`, so it needs ClientCore work first
plus its own tests. Suggested next task: `CLIENTCORE-OUTFIT-772-001` for the
two builders, then `UNREAL-OUTFIT-UI-001` for the dialog, reusing the context
menu this milestone added. Read `receiving.cc::CGetOutfit`/`CSetOutfit` and
`sending.cc::SendOutfit` first.

## Environment

Unchanged: build with VS2022 BuildTools MSVC 14.44.35207; the harness
selftest needs `cl.exe` on PATH; `--objects-srv` must be a local path, not a
UNC path. The server was stopped once, with operator authorization, purely to
force `Alle Spielerdaten gespeichert` for the inventory diff, and restarted
immediately. No server data was edited.
