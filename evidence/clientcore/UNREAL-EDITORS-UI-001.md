# UNREAL-EDITORS-UI-001 — books, signs and house lists

Task ID: `UNREAL-EDITORS-UI-001`.
Branch: `milestone/unreal-editors-ui-001`, created from `origin/main` at
`18b587df92c51be60e734b4f2ece2f39f2065d29`. No main merge.
Worktree: `C:/Users/dell/Desktop/fusion32`.

## Scope

A real 7.72 UI for the two editor commands: open and display
`SV_CMD_EDIT_TEXT`, show the text and who wrote it, allow an edit to be
submitted through the existing ClientCore builder, let Fusion32 rule on it,
and do the same for `SV_CMD_EDIT_LIST`'s guest, subowner and door lists. No
protocol or server change. No invented writability. No fake success.

## Authority trace

Selected Fusion32 game source, protocol 772.

**What opens a text window.** `moveuse.cc::UseTextObject` requires the TEXT
flag and calls `SendEditText`; the `INFORMATION_SPELLBOOK` branch of
`UseAnnouncer` is the only other caller. Both are reached by *using* an
object, which this client already does. There is no command that asks for a
text window, and none that closes one.

**Which objects actually reach it, which is narrower than the TEXT flag.**
`operate.cc::Use` is an if-else chain and the TEXT flag alone is near the
*end* of it. An object reaches `UseTextObject` only when it is WRITE or
WRITEONCE, or INFORMATION (the spellbook), or **TEXT with
`FONTSIZE == 1`** — and only if no earlier branch claimed it first
(CONTAINER, CHEST, LIQUIDCONTAINER, FOOD, RUNE, KEY, NAMEDOOR, LEVELDOOR,
QUESTDOOR, CHANGEUSE, USEEVENT). Anything else is `throw NOTUSABLE`.

This was established the hard way during the live run and is the single most
useful fact in this report. Of the 135 TEXT-flagged types in the runtime
`objects.srv`, **only 57 can open a window at all**; the Rookgaard signs
(2015/2016, `FontSize=5`) and the tutorial blackboards (2602/2597,
`FontSize=4`) are **not** among them. They are Look-only: "You see a
blackboard." answers a look, and a use of the very same object is refused
with "You cannot use this object." The first live attempts targeted those
objects and were correctly refused by the server; the client had sent exactly
the right command, which the log proves
(`use 134 from world: object 2602 at 32094,32196,7 stack 2`).

**What `SendEditText` carries** (`sending.cc`): `quad ObjectID`,
`word TypeID`, `word MaxLength`, `string Text`, `string Editor`. `MaxLength`
is the type's MAXLENGTH or MAXLENGTHONCE **minus one** for a WRITE or
WRITEONCE object, and `strlen(Text)` for everything else, including a
spellbook. **No field marks an object read-only**, and no name for the object
is on the wire at all.

**Who rules on an edit** (`receiving.cc::CEditText`): the object must exist,
and it must carry WRITE, or WRITEONCE with `TEXTSTRING == 0` — otherwise
NOTACCESSIBLE. A text at or beyond the type's own limit is NOROOM.
`operate.cc::EditText` additionally requires `ObjectAccessible(..., 1)`, so
the object must be in reach, and it **returns silently when the text has not
changed**.

**Success is silent.** `operate.cc::EditText` sets TEXTSTRING and sets EDITOR
to the writer's name, and sends nothing. `SendResult` is the only answer, and
it arrives as an ordinary `SV_CMD_MESSAGE` with `TALK_FAILURE_MESSAGE`:
"Sorry, not possible." for NOTACCESSIBLE, "There is not enough room." for
NOROOM. **So no client may report that an edit was saved.** The only proof of
acceptance is reading the object again.

**What opens a list window.** `magic.cc::EditGuests`, `EditSubowners` and
`EditNameDoor` — spells 71, 72 and 74, cast by speaking. Each requires
`GetHouseID(posx, posy, posz) != 0`, so the player must be standing inside a
house, and `houses.cc::ShowGuestList` throws NOTACCESSIBLE unless the player
`IsOwner` or is a subowner with a premium account.

**Who rules on a list** (`receiving.cc::CEditList`): GUESTLIST and
SUBOWNERLIST narrow the id to `uint16` and treat it as a house id; DOORLIST
keeps the full quad as an object id and additionally requires NAMEDOOR and
TEXT. An unknown type byte is logged as "Unbekannter Typ" and dropped.

## What was implemented

No ClientCore change. `editors.cpp` already carried both builders and the
two-command decoder with its own CTest suite, and `ApplyServerUpdate` already
stores nothing for `ServerUpdateKind::Editor` — Fusion32 owns the text and
resends the whole body every time the object is used. The open window is
therefore presentation state on the game thread, as the channel tabs are.

New:

- `Real33DEditorWindow.h/.cpp` — one window for both commands, because the
  server treats them as one: each new editor replaces whatever was open, and
  neither has a close or a cancel on the wire.

Changed:

- `Real33DBridge.h/.cpp` — two editor event kinds, the open-window record and
  its revision, `RequestEditText` / `RequestEditList` as intents, and four
  counters. The worker decodes and names; the game thread holds the window.
- `Real33DPanelRegistry.h/.cpp` — the `TextEditor` id.
- `Real33DHUD.h/.cpp` — **16 lines**: one include, one overlay slot, one
  `Register` call and one member.
- `Real33DWorldActor.cpp` — the four counters in the evidence snapshot.
- `Real33DPresentationTests.cpp` — a new `EditorWindow` suite and the registry
  suite extended over the new id.

### Architecture: a modal, still owned by the registry

The task allowed either. A book is not a side-column panel: it is transient,
centred, and there is exactly one. So the widget is an isolated modal in a
HUD overlay slot — but it is **registered in `FReal33DPanelRegistry`** under
`Real33D::PanelIds::TextEditor`, because the registry manages visibility and
nothing else, and routing it through there keeps the HUD with one way to show
and hide a surface rather than two.

### The rule this milestone turns on

`SendEditText` cannot tell a read-only object from a writable one — for a
read-only object it reports `MaxLength = strlen(Text)`, which is a real
number that is not a limit. Inferring read-only from it is exactly the
invention the ClientCore header warns against. So **the body is always
editable and Submit is always offered**, refused only when there is nothing
to submit *against*: no object id, or a list whose type the server never
named. Fusion32 decides, and the window says so.

Submit therefore reports only that the text was handed over, never that it
was kept, and the window stays open afterwards — closing it would read as the
confirmation Fusion32 never sends.

## Offline results

| Check | Result |
| --- | --- |
| `clientcore-tests` | PASS 13/13, GCC via WSL `Ubuntu-26.04` |
| `real33d-native-tests` | PASS 5/5 |
| Unreal automation | PASS **7/7** Success, errors 0, warnings 0 |
| `build-sanity-msvc` | PASS |
| `unreal-module-build` | PASS, built first attempt |
| harness selftest | PASS 10/10 |
| harness overall | PASS, 9 checks, **0 FAIL**, 0 SKIP |
| `secret-check` | PASS 6/6 |

New suite `REAL33D.Presentation.EditorWindow` covers: a closed window says
nothing; the three attribution states, including the empty editor the server
really sends; the length note phrased as the server's report; **a read-only
shaped object and a writable-shaped one treated identically**; an unnamed
list refused; and the three `enums.hh` list values.

## Live QA

Fusion32 `baseline-772-0` in WSL `Ubuntu-26.04`, all three services verified
ALIVE. Test Player B on the REAL33D client. The operator drove the GUI
throughout; nothing was clicked or typed by software. No server data was
edited to make any of this reachable — the writable object was **bought in
game for 5 gold**.

### Session 1 — the window never appeared, and why

Two numbers from `session1_REAL33D.log`:

```
SV_CMD_EDIT_TEXT decoded : 15
windows actually shown   :  0
```

The bridge decoded every command correctly — `edit text: object 18974332
type 2815, 0 char(s), max 511, editor ""` — and `editor window shown` never
once appeared, because `Rebuild()` never ran.

**Cause, entirely in this milestone's own code:** Slate does not tick a
widget whose visibility is `Collapsed`. The window was built collapsed and
drove itself from its own `Tick`, so it could never notice that Fusion32 had
opened one and could never un-collapse itself. A deadlock.

It was also a violation of this HUD's own rule — *"Refresh is called once a
frame from the player controller… No panel reaches for state on its own"* —
so the fix restores the architecture and removes the deadlock together:
`Refresh()` is now called by `SReal33DHUD::Refresh`, like every other panel.

A second defect was found by inspection while fixing the first and is
recorded because it would have been a regression in the rest of the UI: the
window fills its overlay slot so its box can be centred, and a plain
`Visible` would have made that whole rectangle hit-testable, swallowing every
click on the panels behind it for as long as a book was open. It now uses
`SelfHitTestInvisible`, which leaves its own children clickable and lets
everything outside the box through. `Collapsed` still means closed, which is
what the registry reads.

Session 1 is retained in full rather than discarded: it is the evidence that
the protocol layer and the bridge were right and the presentation was wrong.

### What session 1 did establish

- The bridge decodes `SV_CMD_EDIT_TEXT` exactly as the source predicts. The
  scroll reported `max 511`, which is `MAXLENGTH 512 − 1` precisely as
  `SendEditText` computes it, with empty text and an empty editor for a
  newly bought scroll.
- The dispatcher finding above, which cost the first twenty minutes of the
  run: signs and blackboards cannot be opened at all. The client had sent
  exactly the right command each time
  (`use 134 from world: object 2602 at 32094,32196,7 stack 2`) and Fusion32
  refused it with NOTUSABLE, which is correct behaviour by both sides.
- Ordinary gameplay reached a writable object with no server edit: Dixi at
  `[32105,32207,6]` sells a scroll (type 2815, `Write`, `MaxLength=512`,
  `FontSize=1`) for 5 gold, which the character paid out of its own 25.

### Session 2 — on the fixed build

Log: `session2_REAL33D.log`. Timeline: `session2_timeline.txt`.
Capture: `session2_window_open.png`.

The window appears. `editor window shown: Text` eight times against eight
`edit text:` commands.

**The accepted path, proven by re-reading the object three times.** The only
proof available, because `operate.cc::EditText` answers a successful edit
with silence:

| | object 19036773, type 2815 (a scroll) |
| --- | --- |
| bought from Dixi | `0 char(s), max 511, editor ""` |
| submitted 34 | re-read: `34 char(s), editor "Test Player B"` |
| submitted 111 | re-read: `111 char(s), editor "Test Player B"` |
| submitted 315 | re-read: `315 char(s), editor "Test Player B"` |

Fusion32 kept each text **and** set EDITOR to the writer's name, exactly as
`EditText` does with `Change(Obj, TEXTSTRING, ...)` and
`Change(Obj, EDITOR, AddDynamicString(Creature->Name))`. `max 511` throughout
is `MAXLENGTH 512 − 1`, as predicted from `SendEditText` before the run.

The capture shows the window carrying all of it: the heading `Text`, the
detail `object 19036773, type 2815`, the body, **"Written by Test Player B."**
and **"Fusion32 reported a limit of 511 character(s) for this object."**

Repeated submissions of an unchanged text are no-ops on the server, which is
`EditText`'s documented "Text hat sich nicht geändert" early return; several
appear in the timeline and none changed anything.

**Both attribution states observed live**, which is what the rule is about:
the empty editor the server really sends for a new scroll, rendered as
"Nobody has written on this.", and the named one after the first accepted
edit.

**The rest of the HUD stays usable while the window is open.** The capture
shows the Channels panel's hover tooltip rendered with the modal on screen,
which is the `SelfHitTestInvisible` fix working; a plain `Visible` would have
swallowed that hover.

**The server's refusal, observed live.** A 600-character text into the same
scroll, which is over the server's limit and well inside the client's:

```
editor submit 32: 600 char(s)
edit text for object 19036773 submitted to Fusion32 as 32, 600 char(s);
  acceptance is silent and only a refusal answers
server message [FailureMessage]: There is not enough room.
```

51 ms. This one is unambiguously the **server's** ruling and not a local
drop, because the `submitted to Fusion32` line only prints after
`SendCommand` succeeded. It is `CEditText`'s NOROOM for
`TextLength >= MaxLength` (600 >= 512), and the object kept its 315
characters. Reproduced twice, submissions 32 and 33.

### A second defect, found live and fixed: a silently dropped submission

Four submissions of 4515 to 7515 characters produced an `editor submit` line
and **no** `submitted to Fusion32` line — `kGameClientFrameLimits` caps a
sent payload at 2048 bytes, so `SendCommand` failed and `DrainEdits` dropped
them with `continue`. Meanwhile the window had already said "Handed to
Fusion32". That is a fake-success path and this milestone's rules forbid it.

`DrainEdits` now publishes a ClientNotice naming the length, so the player is
told the text never left the client, and logs a warning. The fix is covered
by the frame-limit reasoning above and is **UNVERIFIED live**: it was written
after the live session the operator chose to end, and only the unit suites
cover the surrounding code.

This is also a real client limitation worth recording: the server reads into
`char Text[4096]`, but this client cannot transmit a text beyond about 2041
characters at all. No object seen in this run comes close — the scroll's
limit is 511 and the largest WRITE limit in `objects.srv` is 2000 — but a
`MAXLENGTH 1024` book is comfortably inside it and a 2000-character letter
(type 3505) is near the edge.

## Remaining UNVERIFIED

**`SV_CMD_EDIT_LIST` and `CL_CMD_EDIT_LIST` were never exercised live, and
the reason is a property of this server, not of this client.** Three
independent gates each block it on their own:

1. **Nobody owns a house.** The runtime's `game/reference/dat/owners.dat` is
   **zero bytes**. `houses.cc::ShowGuestList` throws NOTACCESSIBLE unless
   `IsOwner(HouseID, Player)` or the player is a subowner with a premium
   account, so no character on this server can open a guest or subowner list.
2. **The player must stand inside a house.** `magic.cc::EditGuests`,
   `EditSubowners` and `EditNameDoor` all begin with
   `GetHouseID(Actor->posx, Actor->posy, Actor->posz)` and throw
   NOTACCESSIBLE when it is 0.
3. **The test characters are on Rookgaard, which has no houses.** Every
   entry in `houses.dat` lies on the mainland around x 32258-32272,
   y 32308-32320; Test Player B stands at roughly 32095, 32200.

Making it reachable would mean assigning a house in `owners.dat` and moving
the character — a server-data edit. The operator was asked and chose to leave
it UNVERIFIED rather than author test data, so no server file was touched.

What *is* covered for lists: the ClientCore decoder and both builders by
`CLIENTCORE-EDITORS-772-001`; the window's naming, attribution and submit
rules by `REAL33D.Presentation.EditorWindow`, including that a list the
server did not name cannot be submitted; and the three `enums.hh` values
pinned by `static_assert` in the bridge against
`p772::EditListType`, so a guest list can never be submitted under the
subowner byte.

Also UNVERIFIED:

- **The new "not sent" notice** for an oversized submission. Written after
  the operator ended the live session; the defect it replaces *was* observed
  live, the fix is not.
- **The spellbook path.** `UseAnnouncer`'s `INFORMATION_SPELLBOOK` branch
  sends a text window with `MaxLength = strlen(Text)`; no spellbook was used.
- **A WRITEONCE object already written**, which is the other NOTACCESSIBLE
  route through `CEditText`. The live rejection was NOROOM.
- **Texts between about 2041 and 4095 characters.** The server reads into
  `char Text[4096]` but `kGameClientFrameLimits` caps a sent payload at 2048,
  so this client cannot reach the server's own ceiling. The largest WRITE
  limit in `objects.srv` is 2000 (type 3505, a letter), which sits just
  inside it and was not tested.
- **Window art.** The window is plain chrome, not the 2D's book background,
  and nothing here claims classic visual parity.
