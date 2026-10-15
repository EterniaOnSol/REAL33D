# CLIENTCORE-EDITORS-772-001

Status: `PASS` for the deterministic protocol layer. **Not** `CERTIFIED`: no
live session has opened a book, a sign or a house list, and nothing here
renders one.

ClientCore only. No Unreal, no HUD, no input, no gameplay rule, no Fusion32
server or protocol change, no 2D, no agent, no V08 asset, and no document
another milestone owns. Base `origin/main` at
`fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, on
`milestone/clientcore-editors-772-001` in its own worktree, concurrent with
and disjoint from `milestone/unreal-world-presentation-polish-001`. No main
merge.

These are the last two server commands `PLAYERSTATE-772-001` left
"recognized by name but unparsed" apart from the request queue.

## Layouts, traced

| Command | Source | Layout |
|---|---|---|
| `CL_CMD_EDIT_TEXT` (137) | `receiving.cc::CEditText` | `quad ObjectID`, `string Text` |
| `CL_CMD_EDIT_LIST` (138) | `receiving.cc::CEditList` | `byte Type`, `quad ID`, `string Text` |
| `SV_CMD_EDIT_TEXT` (150) | `sending.cc::SendEditText` | `quad ObjectID`, `word TypeID`, `word MaxLength`, `string Text`, `string Editor` |
| `SV_CMD_EDIT_LIST` (151) | `sending.cc::SendEditList` | `byte Type`, `quad ID`, `string Text` |

List types from `reference/game/src/enums.hh:280-282`: `GUESTLIST = 1`,
`SUBOWNERLIST = 2`, `DOORLIST = 3`.

Four details the source settles, each pinned by a test:

1. **`SV_CMD_EDIT_TEXT` always carries two strings.** Every branch of
   `SendEditText` writes a body and an editor name, and the fallback writes
   two empty ones. A decoder that stopped after the body would silently lose
   the second string and leave the stream misaligned, so its absence is
   asserted as its own truncation case rather than only as one prefix among
   many.

2. **`max_length` is not uniformly a capacity.** For a `WRITE` or
   `WRITEONCE` object `SendEditText` reports the type's `MAXLENGTH` (or
   `MAXLENGTHONCE`) attribute **minus one** — the longest text the server
   will take back. For a spellbook, and for anything else read-only, it
   reports `strlen(Text)` instead. So `max_length == text.size()` does
   **not** prove the object is read-only, and nothing in this command marks
   one definitively. This layer reports both numbers and infers nothing;
   `CEditText` re-checks the flags and answers `NOTACCESSIBLE`, so Fusion32
   refuses the edit rather than the client guessing.

3. **The `id` beside an edit-list type means two different things.**
   `CEditList` narrows it to `uint16` and treats it as a house id for
   `GUESTLIST` and `SUBOWNERLIST`, and keeps the full quad as an object id
   for `DOORLIST`:
   ```
   if(Type == GUESTLIST)         ChangeGuests((uint16)ID, Player, Text);
   else if(Type == SUBOWNERLIST) ChangeSubowners((uint16)ID, Player, Text);
   else if(Type == DOORLIST)     { Object Door = Object(ID); ... }
   ```
   This layer carries the quad through unchanged. A test sends a door list
   with id `0xDEADBEEF` and asserts the high bytes survive, because
   narrowing here would corrupt an object id.

4. **Lengths and truncation are the server's business.** Both handlers read
   into `char Text[4096]` and `TReadStream::readString` keeps
   `MaxLength - 1` bytes and skips the rest, so a long text is truncated to
   4095 rather than refused. `CEditText` separately answers `NOROOM` when
   `strlen(Text) >= MaxLength` using the type's own attribute — and since
   `SendEditText` reports that attribute minus one, a text no longer than
   the `max_length` the client was given is exactly what the server accepts.
   The builders enforce none of this. As in channels, the only length they
   can never emit is `0xFFFF`, the escape `TReadStream::readString` takes as
   "a quad length follows"; text is clamped to `0xFFFE` bytes so the frame
   can never be read as one.

An unknown list-type byte is reported with `known_type == false` rather than
rejected: the message is still fully framed, the server's own writer is its
only producer, and dropping it would lose a list the client could show.

## Architecture

Ordinary ClientCore builders and `MapScanner` parsing; no raw packets.
`editors.h` sits at the `player_state.h` layer and does not include
`movement.h`, which includes it. Both builders take scalars and strings, so
neither had to be declared elsewhere.

**Editors store nothing in WorldState.** Fusion32 resends the whole body
every time the window is opened and re-reads the object before accepting
anything back. A test asserts WorldState is untouched.

## Tests

`clientcore/tests/editors_tests.cpp`, registered as
`protocol772_editors_tests` in `clientcore/CMakeLists.txt` and in
`tests/build_clientcore_windows.cmd`. The emitter in the suite is a literal
port of `SendEditText` and `SendEditList`, asserted against hand-computed
golden hex before it is used for anything else.

Golden byte vectors:

- `892e16000002006869` — edit text, object 5678, body "hi"
- `89010000000000` — edit text, empty body
- `8a01f40100000300426f62` — edit list, GuestList, house 500, "Bob"
- `8a03efbeadde010078` — edit list, DoorList, object `0xDEADBEEF`, "x"
- `962e1600009e07ff03020068690300416c`-shaped: the writable book is
  `962e1600009e07ff03020068690300426f62` — object 5678, type 1950,
  max length 1023, body "hi", editor "Bob"
- `96010000000000000000000000` — the fallback branch, both strings empty
- `9702f40100000200416c` — edit list, SubownerList, house 500, "Al"

Decoding runs through the real `DecodeServerUpdate` dispatcher, so opcode
routing is proven and not just the body readers.

Negative and malformed cases:

- **46 truncated prefixes**: every proper prefix of four well-formed
  messages fails as `MapDecodeError::Truncated` with `bytes_consumed == 0`
- the editor string missing entirely, asserted separately
- a body length that outruns the payload
- an editor length that outruns the payload
- a list text length that outruns the payload
- unknown list types 0, 4 and 255: decoded, flagged `known_type == false`
- five wrong opcodes handed to `DecodeEditor` (110, 149, 152, 170, and the
  *client* opcode 137), each `UnexpectedCommand`
- neighbouring opcodes 125, 171 and 174 still decode as `Unsupported`
- output reuse in both directions: a list after a text leaves no text, and a
  text after a list leaves no list

Edge cases: empty body, empty editor, the `0xFFFE` builder clamp on both
commands, and a 4096-character text sent whole for the server to truncate.

## One existing suite changed

`player_state_tests.cpp` listed 150 among the opcodes asserted unsupported,
with a comment saying the list exists "so that removing an opcode from this
list cannot pass unnoticed". It did not. The list becomes `{125, 126, 171,
174}` — trade and the quest/channel commands.

## Results

- `ctest`: **10/10 suites PASS** via WSL `Ubuntu-26.04`, g++ 15.2.0,
  `-Wall -Wextra -Wpedantic -Werror`
- ASan + UBSan rebuild of all ten: **10/10 PASS**
- `tests/regression_harness.sh --repo-root <this worktree>`:
  `PASS_WITH_SKIPS`, 6 PASS / 0 FAIL / 3 SKIP
- `tests/build_clientcore_windows.cmd` from a VS 2022 Build Tools x64
  developer environment: **WINDOWS CLIENTCORE: PASS**, all nine suites plus
  `editors_tests` at `/W4 /WX /permissive-`, C++17 for the suites and C++20
  for the archive the Unreal module links.

### The MSVC pass found a real defect

The first MSVC run failed to compile this suite, and the failure was genuine
rather than environmental:

```
editors_tests.cpp(160): error C2220: warning treated as error
editors_tests.cpp(160): warning C4127: conditional expression is constant
editors_tests.cpp(161): warning C4127: ...
editors_tests.cpp(162): warning C4127: ...
```

Three assertions about the `EditListType` enum values were written as runtime
`CHECK(...)` calls even though they are compile-time facts. GCC accepts a
constant condition silently; MSVC at `/W4 /WX` does not. Fixed by stating
them as `static_assert`, which is both what MSVC wants and a better
expression of the intent — the enum values are fixed by
`reference/game/src/enums.hh:280-282` and cannot vary at run time. The
neighbouring `IsKnownEditListType` checks stay runtime `CHECK`s because they
exercise a function over several inputs.

This is the second time the Windows build has caught something GCC waved
through; `UNREAL-SLICE-001` recorded four such defects on its first native
run. Re-verified after the fix: MSVC `PASS`, and GCC `ctest` still 10/10.

## Audit: do books and signs need more protocol than these four commands?

**No. The protocol side is complete.** Every step of opening, reading,
writing and failing already has a command that is built or decoded, and all
but these four were already certified.

| Step | Command | State before this milestone |
|---|---|---|
| Open a book, scroll, sign or tombstone | `CL_CMD_USE_OBJECT` (130) | built, certified by `UNREAL-ITEM-USE-INTERACTION-001` |
| Open a spellbook | the same `CL_CMD_USE_OBJECT` | same |
| Open a house guest / subowner / door list | `CL_CMD_TALK` (150) — they are spells | built, certified by `CHAT-772-001` |
| Server shows the text | `SV_CMD_EDIT_TEXT` (150) | **added here** |
| Server shows the list | `SV_CMD_EDIT_LIST` (151) | **added here** |
| Client submits the text | `CL_CMD_EDIT_TEXT` (137) | **added here** |
| Client submits the list | `CL_CMD_EDIT_LIST` (138) | **added here** |
| Refusal (`NOTACCESSIBLE`, `NOROOM`, …) | `SV_CMD_MESSAGE` (180) | decoded, live-proven |

Source for the open paths:

- `moveuse.cc:1850-1868` — `UseTextObject(CreatureID, Obj)` ends in
  `SendEditText(Player->Connection, Obj)`. It is reached by the ordinary use
  dispatcher, which is `CL_CMD_USE_OBJECT`.
- `moveuse.cc:1948` — the `INFORMATION` type 4 branch (spellbook) of the
  same dispatcher, also `SendEditText`.
- `magic.cc:2926`, `:2949`, `:2972` — `EditGuests`, `EditSubowners` and
  `EditNameDoor`, each ending in `SendEditList` with `GUESTLIST`,
  `SUBOWNERLIST` and `DOORLIST` respectively. They are dispatched from the
  spell table at `magic.cc:3348-3351` (cases 71, 72, 74), so a house list is
  opened by **saying a spell**, not by a dedicated command.
- `sending.cc::SendResult` ends in
  `SendMessage(Connection, TALK_FAILURE_MESSAGE, "%s", Message)`, so every
  editor refusal arrives on the already-decoded message path.

**What books and signs still need is entirely client-side**: a text window
that can display a body with its editor attribution and, when the object is
writable, submit it back. That is Unreal UI work on `Real33DHUD.*`, blocked
by another milestone, and it needs no further protocol from ClientCore.

One caveat worth recording: a client cannot tell from `SV_CMD_EDIT_TEXT`
alone whether the object is writable (see detail 2 above). The practical
consequence for UI is that the window should offer editing and let Fusion32
refuse with `NOTACCESSIBLE`, rather than deciding locally — which is the
same posture the use and trade builders already take.

## Limits

- No live session has opened a book, sign, tombstone, spellbook or house
  list. Nothing here is live proof.
- The request-queue family (174–177) remains the only undecoded server
  command group.
- No UI exists.
