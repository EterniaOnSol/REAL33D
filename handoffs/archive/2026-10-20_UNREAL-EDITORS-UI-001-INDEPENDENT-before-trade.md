# HANDOFF - UNREAL-EDITORS-UI-001 independent certification

Date/time: 2026-10-20, America/Guatemala
Reviewer: Claude; independent certification of the completed scope
Candidate: `a4f2cb5a300c4bd3440d4c824200d7134618d24d`
Base / current main: `18b587df92c51be60e734b4f2ece2f39f2065d29`, unchanged
Branch: `certification/unreal-editors-ui-001-20261019`
Worktree: `C:/Users/dell/Desktop/fusion32/build/cert-editors-ui-20261019`
Ending commit: the certification commit containing this handoff

EDIT_TEXT_CERTIFICATION = CERTIFIED
EDIT_LIST_DISPLAY = IMPLEMENTED_UNVERIFIED
EDIT_LIST_SUBMIT = IMPLEMENTED_UNVERIFIED
OVERALL_EDITORS_STATUS = PARTIALLY CERTIFIED
MAIN_MERGE = NONE
CERT_PUSH = NONE. Only the implementation branch was pushed.

## Preservation, done first

`secret_check` passed, `a4f2cb5` was confirmed a direct descendant of
`origin/main` (so the remote had not diverged), and the implementation was
pushed normally to `origin/milestone/unreal-editors-ui-001`. Remote main was
re-checked afterwards and is still `18b587df`. Only then was the isolated
certification worktree created, from exactly the candidate. No force, no
rewrite, no branch deletion. The root worktree and the unrelated preservation
stash were left untouched.

## What was certified

The text editor, end to end, against the production Slate widgets. 52
assertions, 0 failures, `EDITORS_CERT_RESULT=PASS`. Opening, the server's own
id/type/limit and text shown on screen, attribution in both states, a
legitimate submit, authoritative persistence proven by re-reading the object,
an oversized submit reaching Fusion32 and being refused with the object
keeping its previous text, the modal not blocking the HUD (proven
functionally, by clicking the Channels button through it), eight registry
surfaces, and container/Channels/Local Chat regressions. Protocol counters
zero.

Detail, reproduction and the retained driver:
`evidence/clientcore/UNREAL-EDITORS-UI-001-INDEPENDENT-20261019.md` and
`evidence/clientcore/unreal-editors-ui-independent-20261019/`.

## What was NOT certified, and why

`EDIT_LIST`, both display and submit. The live blocker was re-verified
independently rather than accepted from the author: `owners.dat` is 0 bytes
so no character owns a house; `magic.cc`'s three house spells each require
`GetHouseID(...) != 0`; and every house in `houses.dat` is mainland while
both test characters are on Rookgaard. The live run bears this out with
`EditListsOpened = 0` and `EditListsSubmitted = 0`.

**No house ownership or test data was fabricated**, per the explicit
instruction. Decoder, builders, window rules and the three `enums.hh` values
remain covered offline only.

## Reviewer-side faults, recorded

Three, none a candidate defect, all retained in the evidence:

1. A malformed `\\wsl.localhost` path in the launcher — the client never read
   the secrets and the driver correctly hit its deadline.
2. The driver assumed the scroll was in the backpack; it is held in inventory
   slot 6.
3. The driver asserted seven registry surfaces, carried over from the
   channels certification. The true number is eight.

And two harness failures from invocation and curation, both corrected and
documented in the report: a `C:/` `--objects-srv` path WSL cannot translate,
and a mis-copied 2026-09-22 evidence file already committed in the repository
whose `SV_CMD_LOGIN_ERROR` diagnostic predates this milestone entirely.

## Gates

ClientCore 13/13, native 5/5, Unreal automation 7/7 with zero errors and
warnings, MSVC archive, `REAL33DEditor` built from the exact candidate with
the driver removed, harness selftest 10/10, harness 9 PASS / 0 FAIL / 0 SKIP
over 24 logs, Git Bash `secret_check` 6/6. All run from this worktree.

## Exact next task

`READY_TO_MERGE_MAIN = NO` for the milestone as a whole while `EDIT_LIST` is
unverified. Two ways forward, the operator's choice:

1. Merge only on the understanding that `EDIT_LIST` ships
   `IMPLEMENTED_UNVERIFIED` and is recorded as such in `PARITY_MATRIX.md`.
2. Close `EDIT_LIST` first. That needs operator authorization to author test
   data: assign a house in `game/reference/dat/owners.dat` and move a
   character inside it. Read `reference/game/src/houses.cc`
   (`ShowGuestList`, `ChangeGuests`), `reference/game/src/magic.cc`
   (`EditGuests`, spells 71/72/74) and
   `clientcore/include/fusion32/protocol772/editors.h` first.

Do not merge main into either branch on this task.
