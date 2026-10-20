# UNREAL-EDITORS-UI-001 — independent certification of the completed scope

Reviewer: Claude, 2026-10-19/20 America/Guatemala.

**Verdict: the TEXT EDITOR scope is CERTIFIED. The milestone as a whole is
NOT certified, because `EDIT_LIST` remains `IMPLEMENTED_UNVERIFIED`.**

## Provenance

| | |
| --- | --- |
| Candidate | `a4f2cb5a300c4bd3440d4c824200d7134618d24d` |
| Base main | `18b587df92c51be60e734b4f2ece2f39f2065d29`, unchanged throughout |
| Certification branch | `certification/unreal-editors-ui-001-20261019` |
| Worktree | `build/cert-editors-ui-20261019`, created from exactly the candidate |
| Push | Candidate pushed to `origin/milestone/unreal-editors-ui-001` **before** the worktree was made, after `secret_check` passed. `a4f2cb5` is a direct descendant of `origin/main`, so the remote had not diverged and the push was an ordinary fast-forward of a new branch. No main merge, no force, no rewrite |

The candidate changes nothing under `clientcore/`, `reference/`, `scripts/` or
`tests/`. `editors.cpp` and `editors.h` are still at commit `20c22cd`, their
own ClientCore milestone — so the claim that the existing builders and decoder
are used as they stand is verified by history, not by assertion.

## Source review, independently confirmed

Three claims matter most, and each was checked against the candidate's own
source rather than its documentation:

1. **No invented writability.** The only occurrences of "write" anywhere in
   `Real33DEditorWindow.h/.cpp` are in comments. `CanSubmit` gates on
   `bOpen` plus a non-zero object id (text) or a named list type — never on
   any guess about the object being writable.
2. **No fake success.** The submit status reads "Handed to Fusion32. It does
   not acknowledge an accepted edit; a refusal appears in Local Chat, and
   reading the object again is what shows whether it was kept." There is no
   "saved" path.
3. **List types pinned.** Three `static_assert`s tie `EReal33DEditListType`
   to `p772::EditListType`, so a guest list cannot be submitted under the
   subowner byte.

## Independent live verification

A temporary opt-in driver (`-real33d-cert-editors`) exercised the **production
Slate widgets**: `SimulateClick` invokes the real button callbacks, `SetText`
drives the real input box, and every displayed value asserted here was read
back off the painted `STextBlock`s and the real `SMultiLineEditableTextBox`,
not from the code's intent. No private access, no injected events, no
fabricated packets, no server edit, no implementation change. The only
non-click entry point used is `RequestUseObject` — the exact call
`SReal33DHUD::HandleSlotUsed` makes — and only to *open* a window, never to
assert anything.

**52 assertions, 0 failures, `EDITORS_CERT_RESULT=PASS`.** Full list in
`unreal-editors-ui-independent-20261019/live/cert_assertions.txt`.

### What was proved, and how

**EDIT_TEXT opens.** The window is registered and **collapsed** before
Fusion32 sends anything, which is what makes it a server-opened surface.
Using the scroll produced `SV_CMD_EDIT_TEXT` and the production window
appeared, with the registry agreeing it was open.

**The server's values are what is shown.** Read off the painted widgets:

```
Text | object 19866777, type 2815 | Written by Test Player B.
     | Fusion32 reported a limit of 511 character(s) for this object. | Submit | Close
```

The object id, the type id and the limit all match the decoded command, and
the body box's text equals the server's text **exactly**. `max 511` is
`MAXLENGTH 512 − 1`, which `SendEditText` computes — asserted as such, not as
a magic number.

**Editor attribution.** The scroll arrived already attributed, and the painted
line matched the server's editor string. After the certification's own edit,
the attribution changed to this character — verified again from the painted
text, not from the struct.

**A legitimate edit, and authoritative persistence.** Clicking the production
Submit put exactly one `CL_CMD_EDIT_TEXT` on the wire. The window was then
closed locally — the server is told nothing — and the object was used again.
Fusion32 returned the new text **and** had set EDITOR to this character.
That re-read is the only possible proof, since `operate.cc::EditText` answers
a successful edit with silence.

**An oversized submit reaches Fusion32 and is refused.** A text over the
server's 511 limit and inside the client's frame limit was submitted; the
counter confirms it reached the wire rather than being dropped locally, and
Fusion32 answered "There is not enough room." A further re-read showed the
object had **kept the accepted text** — the refused one was not persisted.

**The modal does not block the rest of the HUD.** Two ways: the open window's
visibility is `SelfHitTestInvisible` rather than `Visible`, and —
functionally — the Channels panel's button was clicked **while the modal was
open** and produced a real server round trip returning four genuine rows
(Game-Chat, Trade, RL-Chat, Help). The modal stayed open throughout.

**PanelRegistry.** The registry holds **eight** surfaces; all eight ids were
asserted present by name, so a wrong count says which one is missing. The
modal closes, collapses, toggles back open and refuses a duplicate
registration, and the other panels are unaffected.

**Regressions.** Containers: the backpack opened with real contents. Channels:
the list round trip above. Local Chat: a Say was echoed back by Fusion32 and
landed in the transcript.

**Protocol.** `residual_bytes 0, unsupported_opcodes 0, protocol_anomalies 0`
across the run (final: 24 frames, 50 commands), and `EditListsOpened` and
`EditListsSubmitted` are both 0, as this server requires.

### Two aborted attempts, recorded rather than hidden

Both were faults in the reviewer's own driver or invocation, not in the
candidate, and their artifacts are retained under `live/` with names saying so:

1. A malformed `\\wsl.localhost` path in the launcher meant the client never
   read the runtime secrets; the driver correctly failed on its deadline.
2. The driver assumed the scroll was in the backpack. It is held in inventory
   slot 6. The driver now searches body slots and containers.

A third correction: the driver asserted seven registry surfaces, carried over
from the channels certification. The true number is eight.

## Offline gates, all from the certification worktree

| Check | Result |
| --- | --- |
| `clientcore-tests` | PASS 13/13 |
| `real33d-native-tests` | PASS 5/5 |
| Unreal automation | PASS 7/7, errors 0, warnings 0 |
| `build-sanity-msvc` | PASS |
| `unreal-module-build` | PASS, MSVC 14.50.35717, 29 objects compiled fresh |
| harness selftest | PASS 10/10 |
| harness overall | PASS, 9 checks, **0 FAIL**, 0 SKIP, 24 logs |
| `secret-check` | PASS 6/6 |

The final build and the final automation ran with the driver **removed** from
production Source; UBT logged `Invalidating makefile (source file removed)`
and `git diff` against the candidate over `unreal/REAL33D/Source`, `scripts`,
`clientcore` and `tests` is empty. The driver is retained only as evidence.

### A harness FAIL that was the reviewer's fault, and is recorded

The first harness run reported two failures, both corrected by invocation and
curation, neither a candidate defect:

- `real33d-native-tests FAIL 4/1` — `--objects-srv` was given a `C:/...`
  path, which `to_wsl_path` cannot translate; the item-use test then could
  not open the file. Re-run with a `/c/...` path: 5/5.
- `protocol-log-check FAIL 1 of 26` — the flagged file was
  `unreal_slice_evidence_Disconnected.json`, **dated 2026-09-22**, an
  artifact already committed to the repository that a copy glob had swept
  into the new evidence folder. Its `SV_CMD_LOGIN_ERROR is not decoded`
  diagnostic predates this milestone and is unrelated to editors. Removing
  the mis-copied duplicate: 24 logs scanned, clean.

## EDIT_LIST — IMPLEMENTED_UNVERIFIED, and why

`EDIT_LIST_DISPLAY = IMPLEMENTED_UNVERIFIED`
`EDIT_LIST_SUBMIT  = IMPLEMENTED_UNVERIFIED`

**No house ownership or test data was fabricated.** The live blocker was
re-verified independently against this server rather than taken from the
author's report. Three gates, each sufficient alone:

1. `game/reference/dat/owners.dat` is **0 bytes** — no character owns a
   house. `houses.cc::ShowGuestList` throws NOTACCESSIBLE unless
   `IsOwner(HouseID, Player)` or the player is a subowner with a premium
   account.
2. `magic.cc::EditGuests`, `EditSubowners` and `EditNameDoor` each begin with
   `GetHouseID(Actor->posx, Actor->posy, Actor->posz)` and throw
   NOTACCESSIBLE when it is 0 — the player must stand inside a house.
3. Every house in `houses.dat` is on the mainland around x 32258-32272,
   y 32308-32320. Test Player B is on Rookgaard, which has none.

The live run confirms the consequence: `EditListsOpened = 0` and
`EditListsSubmitted = 0`.

What **is** verified for lists, offline: the ClientCore decoder and both
builders (`CLIENTCORE-EDITORS-772-001`, inside the 13/13); the window's
naming, empty-attribution and submit rules, including that a list whose type
the server never named cannot be submitted
(`REAL33D.Presentation.EditorWindow`); and the three `enums.hh` values pinned
by `static_assert`.

## Remaining UNVERIFIED beyond EDIT_LIST

Carried forward from the candidate and not closed by this certification:

- The "not sent" notice for a submission beyond the ~2041-character frame
  limit. The defect it replaces was observed live by the author; the fix is
  covered only by reasoning and the surrounding suites.
- The spellbook path (`UseAnnouncer`'s `INFORMATION_SPELLBOOK` branch).
- A WRITEONCE object already written — the other NOTACCESSIBLE route through
  `CEditText`. The live refusal certified here is NOROOM.
- Texts between ~2041 and 4095 characters: the server reads `char Text[4096]`
  but `kGameClientFrameLimits` caps a sent payload at 2048.
- Window art. Plain chrome; no classic visual parity is claimed.
- The author's finding that only 57 of 135 TEXT types can open a window was
  re-derived from `operate.cc::Use` and `objects.srv` during review and holds,
  but no exhaustive per-type live sweep was attempted.

## Conclusion

`EDIT_TEXT_CERTIFICATION = CERTIFIED` for the scope listed above.
`OVERALL_EDITORS_STATUS = PARTIALLY CERTIFIED`, because `EDIT_LIST` has never
run against a live server and this server cannot make it do so without
authored test data, which was explicitly declined.

No implementation defect was found and no implementation line was changed.
Main remains `18b587df`. No merge was performed.
