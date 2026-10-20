# UNREAL-TRADE-UI-001 independent certification

State: CERTIFIED for the requested bounded trade/UI scope.
Date: 2026-10-20.

Candidate: `1cf1ec4682eb17df6df8e576da1c822a915e63f4`.
Base main: `76a261992c6740ecfc612b5421dd91391a06c4d5`.
Isolated worktree: `build/cert-trade-ui-20261020`.
Branch: `certification/unreal-trade-ui-001-20261020`.

The candidate was preserved by a normal push to the previously absent remote
`milestone/unreal-trade-ui-001`; its fetched main parent matched the requested
base. No merge is authorized. No production changes are planned.

Scope: independently exercise the production Slate trade controls with two
real Fusion32 accounts; use received inventory/container updates to establish
transfer. Run fresh ClientCore, native, Unreal automation/build, corrected
regression harness and Git Bash secret checks. Retain reproducible diagnostic
source and compact logs/screenshots here.

Authority inspected: selected Fusion32 game revision
`386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, `receiving.cc` CTradeObject,
CInspectTrade, CAcceptTrade, CRejectTrade; `cract.cc` TCreature::Trade;
`crplayer.cc` TPlayer::InspectTrade, AcceptTrade, RejectTrade;
`sending.cc` SendTradeOffer/SendTradeObjects/SendCloseTrade.
No local version guard surrounds those functions; protocol 772 selection is
documented in SOURCE_TRUTH and communication.cc. ClientCore trade codecs/state,
bridge NoteTrade and four request methods, HUD targeting/context menu, Slot
gesture routing, TradePanel and PanelRegistry were inspected independently.

The first offer sends only prose to the other player. One acceptance waits for
the other. CLOSE_TRADE has the same shape on success and failure. These are
separate assertions, never inferred from a client-side success label.

## Final gates

| Gate | Result / evidence |
| --- | --- |
| ClientCore | PASS 13/13, fresh WSL CTest and independent MSVC build |
| Native | PASS 5/5, including runtime MultiUse metadata (204 types) |
| Unreal original-source build | PASS UE 5.8, Win64 Development, MSVC 14.44; no diagnostic source in final module |
| Unreal automation | PASS 8/8, zero errors/warnings: CameraAndFloors, ChannelsPanel, ChatTabs, EditorWindow, PanelRegistry, SkillsPanel, TradePanel, AuthoritativeWindow |
| Corrected strict harness | PASS 9/9, 0 FAIL, 0 SKIP, report `checks/REGRESSION-HARNESS-20261020T090211Z.md` |
| Harness selftest | PASS 10/10 |
| Git Bash secret_check | PASS 6/6; retained under `checks/harness-logs/secret-check.txt` |
| Protocol | 0 residual/unsupported/anomalies for both independent sockets and every retained live JSON; harness scan 71 logs, 0 anomalous |
| Independent evidence audit | PASS 23 named assertions, 28 snapshots; `verify_evidence.py`, `checks/evidence-audit.json` |

No implementation defect found in the exercised scope. All production sources
remain byte-identical in Git to the candidate. Main and milestone branches are
preserved; no merge, rebase or cherry-pick. The certification commit adds only
documentation/evidence and handoff files.

Initial MSVC ClientCore run: PASS, 13/13. Initial exact Unreal build failed from
disk exhaustion while writing its PCH, before compiling project units. That log
is retained; retry uses installed MSVC 14.44 and removes only this run's incomplete
ignored PCH. This infrastructure failure is not evidence of an implementation
defect.

## Independent method and provenance

`Real33DTradeCertification.cpp` is a temporary opt-in diagnostic, now retired
into this evidence directory. It added no replacement implementation. Account
B ran the actual rendered HUD; account A used a second initialized production
bridge/socket and an actual TradePanel widget for its Accept/Reject callbacks.
Both authenticated against the existing sanitized Fusion32 runtime. No legacy
executable, server edit, fabricated packet, injected server event, private
widget access, or replacement trade state was used.

The slot gesture uses production `OnMouseButtonDown/Up` with Ctrl/right,
Alt/right, plain right, or Shift/right pointer events; menu/button selection uses
production Slate `SimulateClick`. This certifies those real callbacks and their
live consequences, not OS-level cursor hit-testing. Account A's panel is an
in-process widget fixture, not a second rendered desktop. The received A
inventory/container semantic events are retained in the log's `CERT_ITEM`
ledger. Its `amount=0` for noncumulative objects is a raw optional field default,
not a claim that the object has zero quantity; the JSON snapshots correctly
represent absent quantities as null.

All visual observations are from newly generated PNGs, not the author's
screenshots. No experimental mesh catalog was mounted; the candidate's base
presentation, native UI resources and four tracked mailbox assets were
materialized through LFS. Existing game/querymanager/login services were reused.

## Live evidence

`live/trade/REAL33D.log` is the independent run starting 02:53:58 local
(08:53:58 UTC). It proves:

| Requirement | Independent observation |
| --- | --- |
| Trade start | Step 3 actual Ctrl/right on inventory object 3294 opens the menu, sends neither Use nor Trade; actual `Trade with...` arms targeting. Step 5 actual Test Player A battle button dispatches the offer. Local state remains absent until the server event. |
| Own offer | Step 6 B gets own name/type 3294, partner absent. A gets only prose and no invented offer. `CertOwn0` PNG/JSON. |
| Partner offer | Step 7 A offers its observed type 3561; both peers receive reciprocal offers. Actual B text contains server names/types. `CertBoth0`. |
| Own inspect | Steps 7–8 actual first Look button, new received description matches prior independent item Look: short sword, Atk:11 Def:11. |
| Partner inspect | Steps 8–9 actual second Look button, new received description matches A's prior Look: jacket, Arm:1. `CertInspect0`. |
| Accept | Steps 10–11 B Accept leaves both models open and ownership unchanged for >4 seconds. Only subsequent A Accept permits completion. `CertWaiting0`. |
| Authoritative transfer | Step 12 at 08:55:08 UTC: inventory and open-backpack deltas on BOTH sockets show B loses 3294 and receives 3561; A loses 3561 and receives 3294. `CertTransfer0` plus both peer ledgers. |
| Reject | Round 1 B actual Reject clears its local trade; only A receives CLOSE_TRADE and `Trade cancelled.`. Both owners unchanged. `CertCancel1`. |
| Partner close/cancel | Round 2 A actual Reject causes B's received CLOSE_TRADE and `Trade cancelled.`; both owners unchanged. `CertCancel2`. |
| PanelRegistry | Nine named surfaces close/open/toggle and restore initial visibility. During both-offer state, hiding/reopening Trade preserves server state. |

The completed trade's observed item locations, not the close opcode:

| Snapshot | A type 3561 | A type 3294 | B type 3294 | B type 3561 |
| --- | --- | --- | --- | --- |
| Baseline | inventory 5 | absent | inventory 5 | absent |
| After first completed trade | absent | inventory 10 | absent | backpack 0, object 0 |
| After B rejection | absent | inventory 10 | absent | backpack 0, object 0 |
| After A rejection | absent | inventory 10 | absent | backpack 0, object 0 |
| After reciprocal return | inventory 5 | absent | inventory 5 | absent |

Counts are scoped to received body inventory and the one open backpack on each
account; nested bags are not opened or used as an offer. Both offered items are
single, distinct, noncumulative types. No container-offer claim is made.

`live/resume/REAL33D.log` starts 02:59:59 local. Fresh server snapshots first
reconfirm the prior transfer, then a normal reciprocal trade returns ownership
and original slots. It ends `TRADE_CERT_RESULT=PASS step=21 round=3`, with:

- actual Shift/right UseWith → real player, one Use request and no Trade request;
  this proves routing, not successful gameplay use of a bucket on a player;
- actual plain-right backpack Use and received contents;
- actual Channel List, offered channel 4 Join with no optimistic tab,
  OPEN_CHANNEL, Send, received echo isolated from Local Chat, and Leave;
- actual Local Chat Send, received echo isolated from channel transcript;
- actual right-click scroll 2815, server EDIT_TEXT, exact received body in the
  rendered editor and Close. Submit/persistence was not rerun in this smoke;
  no text was changed;
- final original ownership, no open trade, zero residual/unsupported/anomaly
  counters on each real connection (B 49 frames/71 commands; A 27/49).

## Interrupted attempts and infrastructure

Certification is based on the named completed assertions across the trade and
resume runs, not a claim that every run passed end to end.

1. First two Unreal compilations exhausted disk in the generated PCH. Logs
   retained. Only explicitly checked ignored PCH caches were removed: old chat,
   skills, editors certification caches and the incomplete current PCH. Source,
   binaries, branches and prior evidence were preserved.
2. First startup had four unmaterialized LFS mailbox packages; no live assertion
   ran. `attempts/missing-lfs.log` retains this preparation failure.
3. `attempts/unscheduled-input.log`: a second menu for type 3607 and an offer
   arrived outside the diagnostic sequence. No transfer occurred. User was
   asked to leave the window idle.
4. `attempts/menu-focus.log`: the context menu lost focus before the delayed
   selector ran. The diagnostic was changed to find the actual visible popup
   and click its entry immediately. No implementation changed.
5. The trade run completed all trade requirements, then another unscheduled
   menu/offer appeared between round 2 cancellation and round 3. It honestly
   ends FAILED at the idle-state guard. User reconfirmed continuation; the resume
   run checked actual ownership before performing the return trade and remaining
   regressions. The complete original log and intermediate probe revisions
   remain available.

## Reproduction

Use an isolated worktree at the exact candidate; materialize
`unreal/REAL33D/Resources` and `unreal/REAL33D/Content`. Build ClientCore C++20
archive with `tests/build_clientcore_windows.cmd`, then REAL33DEditor Win64
Development under UE 5.8/MSVC 14.44.35207. Copy the evidence diagnostic into
`unreal/REAL33D/Source/REAL33D/Private/`, compile, and launch the existing sanitized
runtime's account B with `-game -windowed -ResX=1800 -ResY=1050 -nosound
-real33d-cert-trade -real33d-minimap-qa -real33d-disable-inspector
-ExecCmds="t.MaxFPS 30"` and a fresh `-real33d-evidence`/`-abslog` directory.
Use the local runtime path in `-real33d-runtime`; no credentials belong in args.
Keep the window free of user input.

Normal mode discovers the initial distinct items in both accounts' slot 5 and
opens their received backpacks. Existing test accounts also need scroll 2815 and
bucket 3003 on B for the bounded regression. Accounts must be visible, same
floor, within two fields or reachable by normal Follow. Do not edit server saves
to satisfy preconditions. `-real33d-cert-trade-resume` is only for the recorded
interrupted swap: it requires server-observed ownership of A=3294, B=3561
before returning them. Retire the diagnostic and rebuild before final automation.

## Remaining UNVERIFIED

Accept-time failures; multi-object/container offers; offer-time refusals;
distinguishing successful from failed CLOSE_TRADE purely by opcode (the selected
source sends identical closure on both paths). Existing EDIT_LIST house mode
remains explicitly unverified. This bounded regression does not recertify every
prior channel mode, editor-submit mode, resolution, art catalog, or input device.
