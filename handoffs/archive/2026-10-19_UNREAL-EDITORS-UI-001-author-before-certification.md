# HANDOFF - UNREAL-EDITORS-UI-001

Date/time: 2026-10-19, America/Guatemala
Task ID: `UNREAL-EDITORS-UI-001`
Agent / role: Claude; implementation and operator-assisted live QA
Starting commit: `18b587df92c51be60e734b4f2ece2f39f2065d29` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-editors-ui-001`
Worktree: `C:/Users/dell/Desktop/fusion32`

EDITORS_UI_001_STATUS = PASS LOCAL for text; EDIT_LIST UNVERIFIED
MAIN_MERGE = NONE. Branched from origin/main; main untouched.
PUSH = NONE. No remote operation was performed or authorized.

## Objective and allowed scope

A real 7.72 UI for books, signs and house lists: open and display
`SV_CMD_EDIT_TEXT`, show the text and its editor, submit an edit through the
existing ClientCore builder, let Fusion32 rule on it, and the same for
`SV_CMD_EDIT_LIST`. No protocol or server change. No invented writability.
No fake success. Minimal HUD edits.

## Inspection

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
`PARITY_MATRIX.md` and the prior handoff (the chat-channels independent
certification, archived intact at
`handoffs/archive/2026-10-19_UNREAL-CHAT-CHANNELS-001-INDEPENDENT-before-editors.md`).
Branched from a clean worktree at origin/main.

## Discoveries worth keeping

- **ClientCore already had the whole protocol layer.** `editors.cpp` carries
  both builders and the two-command decoder with its own suite, and
  `ApplyServerUpdate` stores nothing for `ServerUpdateKind::Editor`. No
  ClientCore file changed; the suite count stays 13.
- **THE TEXT FLAG IS NOT ENOUGH TO OPEN A WINDOW.** `operate.cc::Use` is an
  if-else chain; an object reaches `UseTextObject` only when it is WRITE or
  WRITEONCE, or INFORMATION, or **TEXT with `FONTSIZE == 1`**, and only if no
  earlier branch claimed it. Of 135 TEXT types in the runtime `objects.srv`,
  **57 can open a window**. The Rookgaard signs (`FontSize=5`) and tutorial
  blackboards (`FontSize=4`) cannot: they answer a Look and refuse a Use with
  NOTUSABLE. This cost the first twenty minutes of the live run.
- **Read-only is not detectable from the wire.** `SendEditText` reports
  `MaxLength = strlen(Text)` for a read-only object and the type's limit
  minus one for a writable one, so the two are indistinguishable. The window
  therefore always offers Submit and Fusion32 rules.
- **Success is silent.** `operate.cc::EditText` writes the text, sets EDITOR
  to the writer's name and answers nothing. Only a refusal comes back, as an
  ordinary `SV_CMD_MESSAGE` through `SendResult`. The only proof an edit was
  kept is reading the object again.
- **A writable object is reachable by ordinary play.** Dixi at
  `[32105,32207,6]` sells a scroll (type 2815, WRITE, MaxLength 512,
  FontSize 1) for 5 gold. No server edit was needed for the whole run.
- **This client cannot send a text beyond about 2041 characters.**
  `kGameClientFrameLimits` caps a sent payload at 2048 while the server reads
  into `char Text[4096]`.

## Changes

New: `Public/Real33DEditorWindow.h` + `Private/Real33DEditorWindow.cpp`.

Changed: `Real33DBridge.h/.cpp` (two editor event kinds, the open-window
record and revision, `RequestEditText`/`RequestEditList`, four counters, the
list-type `static_assert`s), `Real33DPanelRegistry.h/.cpp` (the `TextEditor`
id), `Real33DHUD.h/.cpp` (**17 lines**: include, overlay slot, Register,
member, and the one `Refresh` call), `Real33DWorldActor.cpp` (four counters
in the snapshot), `Real33DPresentationTests.cpp` (a new `EditorWindow` suite
and the registry suite extended), `PARITY_MATRIX.md`, `PROJECT_STATUS.md`.

### Architecture

An isolated centred modal, **registered in `FReal33DPanelRegistry`** under
`Real33D::PanelIds::TextEditor`. A book is transient and there is exactly
one, so it is not a side-column panel; but the registry manages visibility
and nothing else, so routing it through there keeps the HUD with one way to
show and hide a surface.

## Tests and results

| Check | Result |
| --- | --- |
| `clientcore-tests` | PASS 13/13 |
| `real33d-native-tests` | PASS 5/5 |
| Unreal automation | PASS **7/7**, errors 0, warnings 0 |
| `build-sanity-msvc` | PASS |
| `unreal-module-build` | PASS |
| harness selftest | PASS 10/10 |
| harness overall | PASS, 9 checks, 0 FAIL, 0 SKIP |
| `secret-check` | PASS 6/6 |

Live: two operator-driven sessions, 0 undecoded commands and 0 decode
failures throughout. Full account in
`evidence/clientcore/UNREAL-EDITORS-UI-001.md`.

## Two defects found live, both mine, both fixed

1. **The window never appeared.** Session 1: fifteen `SV_CMD_EDIT_TEXT`
   decoded, zero windows shown. Slate does not tick a widget whose
   visibility is `Collapsed`, and the window drove itself from its own
   `Tick` — so it could never un-collapse itself. It is now driven by
   `SReal33DHUD::Refresh`, which is also this HUD's own stated rule. Session
   2 on the fixed build: **9 opened, 9 shown.**
2. **An oversized submission was dropped in silence** while the window had
   already said "Handed to Fusion32" — a fake-success path. `DrainEdits` now
   publishes a ClientNotice naming the length. The defect was observed live
   (four submissions of 4515-7515 characters vanished); **the fix is
   UNVERIFIED live**, having been written after the operator ended the run.

A third issue was caught by inspection before it could ship: the modal fills
its overlay slot so its box can be centred, and a plain `Visible` would have
swallowed every click on the panels behind it. It uses
`SelfHitTestInvisible`; the session-2 capture shows the Channels panel
tooltip rendering with the window open.

## Remaining UNVERIFIED

- **`EDIT_LIST` entirely, live.** Three independent gates: `owners.dat` is
  zero bytes so nobody owns a house; the three spells require
  `GetHouseID(...) != 0`; and both test characters are on Rookgaard, which
  has no houses. The operator was asked and chose not to author server test
  data. Decoder, builders, window rules and the `enums.hh` values are all
  unit-covered.
- The new "not sent" notice; the spellbook path; a WRITEONCE object already
  written (the other NOTACCESSIBLE route); texts between ~2041 and 4095
  characters; and window art.

## Exact next task

Either certify this milestone independently from a fresh worktree, or make
`EDIT_LIST` reachable. For the latter the operator must first authorize test
data: assign a house in `game/reference/dat/owners.dat` and move a character
inside it. The files to read first are `reference/game/src/houses.cc`
(`ShowGuestList`, `ChangeGuests`), `reference/game/src/magic.cc`
(`EditGuests`, spells 71/72/74) and
`clientcore/include/fusion32/protocol772/editors.h`.

## Environment notes

Unchanged from the previous handoff and still true: build with **VS2022
BuildTools MSVC 14.44.35207**; the harness selftest needs `cl.exe` on PATH to
report 10/10; `--objects-srv` must be a local path, not a `\\wsl.localhost`
UNC path. The server was restarted cleanly at the start of this task; no
stale pid and no runtime edits this time.
