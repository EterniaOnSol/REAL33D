# UNREAL-TRADE-UI-001 — player-to-player trade

Task ID: `UNREAL-TRADE-UI-001`.
Branch: `milestone/unreal-trade-ui-001`, from `origin/main` at
`76a261992c6740ecfc612b5421dd91391a06c4d5`. No main merge.

## Scope

The real 7.72 player trade: start one from a legitimate interaction, show
both offers, inspect an offered object, accept, reject, and close when
Fusion32 says so. The server stays authoritative and nothing reports a local
success. **NPC shops are out of scope** — they reach the same three server
commands but are driven entirely by TALK and already work; nothing here is
shop-shaped.

## Authority trace

Selected Fusion32 game source, protocol 772.

**Starting a trade** — `cract.cc::TCreature::Trade`, reached from
`receiving.cc::CTradeObject` (CL_CMD_TRADE_OBJECT, 125). Every rule is the
server's: the object must be within reach, the partner must be an online
player, neither side may already be trading with someone else, the offer may
not exceed 100 objects, the partner must be **within 2 fields** and in line
of throw, and neither offer may contain the other.

**The asymmetry that shapes the whole UI.** When the partner has not yet
offered, Fusion32 sends them only
`SendMessage(TALK_INFO_MESSAGE, "%s wants to trade with you.")` — **prose, no
trade command at all** — and sends the offerer its own offer. Both sides see
both offers only once both have put an object up, at which point the second
offerer's `Trade` fires all three `SendTradeOffer` calls together.

**The offer list is flat.** `SendTradeObjects` walks the offered object and
recurses through any container, emitting one item per object in pre-order;
nothing marks where a bag ends. This client lists exactly what arrived.

**Accepting is not completing** — `crplayer.cc::TPlayer::AcceptTrade` sets
`TradeAccepted` and **returns** while the other side has not accepted.

**Success and failure are indistinguishable from the close.** When both have
accepted, `AcceptTrade` validates reach, UNMOVE, TAKE, capacity and container
room; on any failure it sends the relevant `SendResult` and
**`SendCloseTrade` to both sides**, and on success it also sends
`SendCloseTrade` to both sides and then moves the objects. `SV_CMD_CLOSE_TRADE`
carries nothing. **So no client may read a close as success.**

**Rejecting is one-sided.** `TPlayer::RejectTrade` clears both sides, sends
the **partner** a close and "Trade cancelled.", and sends the rejecting
player nothing at all.

## What was implemented

No ClientCore change — `trade.cpp` already carried the builders and decoder,
and `ApplyServerUpdate` stores nothing for `ServerUpdateKind::Trade`.

- `Real33DTradePanel.h/.cpp` — the trade surface, registered under
  `Real33D::PanelIds::Trade`. Shows both offers with the server's names, a
  Look button per object, Accept and Reject, and a status line that reports
  only what was sent and what arrived.
- `Real33DBridge.h/.cpp` — three trade event kinds, the single open trade,
  the four commands as intents, seven counters.
- `Real33DPanelChrome.h/.cpp` — `Ctrl+right-click` on a slot raises
  `EReal33DSlotAction::ContextMenu`, taken **before** the existing
  Look/Use/UseWith gesture machine so that logic and its native suite are
  untouched.
- `Real33DHUD.h/.cpp` — the context menu (Look, Trade with…), the trade half
  of the existing pick-then-aim target, one `AddPanel` call, one member.
- `Real33DWorldActor.cpp` — the seven counters in the evidence snapshot.
- `Real33DPresentationTests.cpp` — a `TradePanel` suite and the registry
  suite extended.

### A design the live run corrected

The first build put the Offer control **inside** the trade window and made
that window appear only once a trade existed. The live run killed it in one
step: A offered, Fusion32 told B *"Test Player A wants to trade with you."*
and nothing else, so B had no window and therefore **no way to respond at
all**.

Reading that English sentence to infer a pending trade was never an option —
it is the same invention this project refused for the text editors. The
operator pointed at the authentic gesture instead: in Tibia you
**ctrl+right-click the item** and choose *Trade with…*. That is now the entry
point, the invented Offer button is gone, and the panel is docked so it can
show the idle state rather than hiding the only way in.

## Offline results

| Check | Result |
| --- | --- |
| `clientcore-tests` | PASS 13/13 |
| `real33d-native-tests` | PASS 5/5 |
| Unreal automation | PASS 8/8, errors 0, warnings 0 |
| `build-sanity-msvc` | PASS |
| `unreal-module-build` | PASS |
| harness overall | PASS, 9 checks, 0 FAIL, 0 SKIP |
| `secret-check` | PASS 6/6 |

## Live QA

Fusion32 `baseline-772-0`. Two real accounts: **Test Player A** on the
original 7.72 `Tibia.exe` through the IP Changer, **Test Player B** on the
REAL33D client. The operator drove both; nothing was clicked by software.
Log: `live_session.log`, timeline: `trade_timeline.txt`.

| Counter | Live |
| --- | --- |
| context menus opened | 9 |
| offers dispatched -> own offers returned | **7 -> 7** |
| partner offers received | 7 |
| inspects sent | 2 |
| accepts sent | 4 |
| rejects sent | 2 |
| closes from Fusion32 | 5 |
| **trade refusals from Fusion32** | **0** |
| undecoded commands / decode failures | **0 / 0** |

**TRADE_START.** `context menu opened for object 2920` ->
`trade offer begun with object 2920; waiting for a player` ->
`trade offer 1: object 2920 to creature 1001`. Ctrl+right-click on the item,
*Trade with...*, then the partner.

**OWN_OFFER and PARTNER_OFFER.** `trade own offer from "Test Player B"` and
`trade partner offer from "Test Player A"`, each carrying the server's own
name and object count.

**Both halves of the server's asymmetry were observed.** When B offered
first: `own=1 partner=0` for seven seconds until A answered, then
`own=1 partner=1`. When A had already offered: both arrived in the same
frame, which is the second-offer branch firing all three `SendTradeOffer`
calls at once. And before any of that, A's first offer reached B only as
`server message [InfoMessage]: Test Player A wants to trade with you.`

**INSPECT, and it proves the side byte is right.** `trade inspect 34: own
offer index 0` -> *"You see a jacket (Arm:1)."*; `trade inspect 35: partner
offer index 0` -> *"You see cheese."* Two sides, two different objects,
each description matching the side asked for -- so the inverted
`TradeOfferSide` encoding (Own = 0) is correct on the wire.

**ACCEPT.** Four accepts. The status line read "Accept sent..." and the
window stayed open; completion only ever arrived as a close from Fusion32.

**REJECT, from this client.** `trade reject 38 sent; Fusion32 answers the
partner, not us`, and **no** `trade closed by Fusion32` followed it --
confirming `TPlayer::RejectTrade` sends the rejecter nothing, which is why
the window is cleared locally. Reproduced twice.

**CLOSE, from the other side.** When A cancelled, B received
`trade closed by Fusion32` **plus** `server message [FailureMessage]:
Trade cancelled.` -- the partner's half of the same function. Twice.

### AUTHORITATIVE_TRANSFER

Two independent proofs, neither of them the close:

1. **Fusion32's own saved character file changed.** A's inventory went from
   `{3: backpack{2853}, 4: 3561, 6: 3272}` to
   `{3: backpack{2853}, 4: 3379, 5: 3561, 6: 3272}` -- A gained an object of
   type **3379**, a type it held nowhere at baseline. Files in
   `inventories_before.txt` and `inventories_after.txt`; the server logged
   `Alle Spielerdaten gespeichert` on the shutdown that wrote them.
2. **B demonstrably held A's object.** At baseline object **3561** was in
   **A's** slot 4 and B had none. Mid-session B offered 3561 and Fusion32,
   asked to describe B's **own** offer, answered *"You see a jacket
   (Arm:1)."* B could only offer it because a completed trade had moved it.

**Stated honestly:** B's saved inventory ended with the same type
composition it started with. That is consistent with objects cycling back
across three reciprocal completed trades -- B gave 2920, 3607 and 3561, and
A offered items back each time -- but the per-trade ledger was **not**
reconstructed and no claim is made about it. What is proven is that
ownership moved authoritatively, not the exact path of every object.

## Remaining UNVERIFIED

- **Which close meant success.** By design this client cannot tell, and
  neither can any client: `SV_CMD_CLOSE_TRADE` is identical for a completed
  trade and a failed one. Only the inventory distinguishes them.
- **The failure closes.** No trade was refused at accept time, so TOOHEAVY,
  NOROOM, NOTMOVABLE, NOTTAKABLE and CONTAINERFULL were never seen live.
- **Multi-object offers.** Every offer in this run carried one object, so the
  flat container walk -- a bag arriving as several consecutive items -- was
  never exercised against a real bag.
- **The refusals at offer time**: ALREADYTRADING, PARTNERTRADING,
  TOOMANYOBJECTS, OUTOFRANGE and CANNOTTHROW are all the server's and none
  occurred.
- **NPC shops** are explicitly out of scope and were not touched.
- Panel art is plain chrome; no classic visual parity is claimed.
