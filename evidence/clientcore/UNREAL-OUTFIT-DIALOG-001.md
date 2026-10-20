# UNREAL-OUTFIT-DIALOG-001 — the 7.72 outfit chooser in Unreal

Task ID: `UNREAL-OUTFIT-DIALOG-001`.
Branch: `milestone/unreal-outfit-dialog-001`, from `origin/main` at
`b8e7433baeddc2875943f519d33a5cb9380f3eaa`. No main merge. No push.

## Scope

The real 7.72 outfit chooser on top of the ClientCore outfit protocol that
`CLIENTCORE-OUTFIT-772-001` built: ask for the chooser, open it only when
Fusion32 answers, obey the band the server granted, preview and select an
outfit, edit the four colour bytes, submit, and close without an
acknowledgement that does not exist. Plus the two bridge gaps that were
dropping outfit data at the Unreal boundary.

No ClientCore change, no protocol change, no server change. The suite count
stays 14.

## The colour slots are now named, and the names are proven

This is the one claim in this client that Fusion32 cannot settle.
`reference/game/src/cr.hh` line 21 declares `uint8 Colors[4]`, and every use in
the server — `CheckOutfit`, `CSetOutfit`'s bound test, `SendOutfit`'s
`SendBytes` — addresses them by index. The server never names a body part, and
the 7.72 protocol never states the mapping.

So the task's instruction was to prove it against the classic client or keep
indexed labels. **It is proven**, by a read-only static walk of the local
`Tibia.exe` recorded in `docs/CLASSIC_CLIENT_772.md`
(SHA-256 `1A82FE97…B76F8`). The binary is opened, never executed.

`tests/verify_outfit_color_mapping.py` asserts 43 byte patterns at fixed
offsets and fails closed if any one of them moves. Six links, each an
instruction in that image:

| # | What | Where |
| --- | --- | --- |
| 1 | the chooser builds four colour rows in one loop over `i = 0..3`, and a **four-entry jump table** picks each row's caption from `i` | table at `0x0047D648`; cases at `0x0047D28C/95/9E/A7` |
| 2 | row `i` takes control id `1000 + i`, is stored at `this+0x30 + i*4`, and a click stores `id - 1000` as the selected slot | `0x0047D2C3`, `0x0047D302`, `0x0047128E` |
| 3 | a palette click writes `colours[slot] = swatchId - 2000` | `0x004712E2` |
| 4 | SV_CMD_OUTFIT's four colours load into `colours[0..3]` in wire order | `0x0047D0EF`…`0x0047D113` |
| 5 | OK passes `&colours[0]` and the outfit id to the sender | `0x0047137B`, `call 0x00409830` |
| 6 | the sender writes opcode **211**, the id word, then `colours[0..3]` in ascending order | `0x0040993E`, `0x0040995B/66/72/7E` |

The jump table's four entries are literally `"Head"`, `"Body"`, `"Legs"`,
`"Feet"` in index order. Therefore:

```
Colors[0]  wire byte 0  ->  Head
Colors[1]  wire byte 1  ->  Body
Colors[2]  wire byte 2  ->  Legs
Colors[3]  wire byte 3  ->  Feet
```

Proof output: `unreal-outfit-dialog-001/color_mapping_proof.txt`. The check also
runs in the harness as `outfit-color-mapping`, which `SKIP`s rather than passes
when the client artifact is absent — it is ignored by Git and is not on a clean
checkout.

**Noted, not claimed.** The client's swatch guard spans control ids 2000..2215,
which is 216 ids, while the server refuses any colour above 132. That guard is
a control-id range test, not a count of populated swatches, so it does not
establish how large the palette actually is. 132 is the bound, from
`receiving.cc` lines 1440-1443; the palette's true size is `UNVERIFIED`.

## Authority trace

**Asking.** `receiving.cc::CGetOutfit` (line 1390) reads nothing, so
CL_CMD_GET_OUTFIT (210) is a bare opcode.

**Two correct answers to one request.** Normally `sending.cc::SendOutfit`
replies with SV_CMD_OUTFIT (200). But a character holding `GAMEMASTER_OUTFIT`
is refused: `CGetOutfit` sends a TALK_FAILURE_MESSAGE reading *"You may not
change your outfit."* and returns **without sending 200 at all**. A surface
that waited for the chooser before showing anything would wait for ever, which
is why the request opens nothing.

**A band, not a list.** `SendOutfit` lines 1671-1675 put only `FirstOutfit` and
`LastOutfit` on the wire; everything between is implied. The band is 128-131 for
`Sex == 1` and **anything else at all** takes 136-139, each three wider under
`CheckRight(PREMIUM_ACCOUNT)`. Later protocol versions replaced this with a
counted list; nothing here is shaped for that.

**The client cannot compute the band.** The 7.72 *game* protocol never carries
this player's sex — that byte lives in the querymanager's character record
(`querymanager/src/query.cc` line 720) and never reaches this connection. So
the only honest source for the range is the two words that arrived.

**Answering.** `CSetOutfit` (line 1410) reads a word and then four bytes
**unconditionally** — no `OutfitID == 0` branch — so CL_CMD_SET_OUTFIT is always
seven bytes and **can never express an object disguise**.

**Nothing acknowledges it.** `CSetOutfit` returns void: no confirmation, no
second SV_CMD_OUTFIT, and no error when it drops the selection. Its one effect
is `AnnounceChangedCreature(CREATURE_OUTFIT_CHANGED)` (`operate.cc` line 81),
reaching every connection that can see the creature as SV_CMD_CREATURE_OUTFIT
(142) — and that broadcast is **conditional** on
`if(Player->Outfit == Player->OrgOutfit)`, i.e. only when the player is wearing
their own outfit. A player who is invisible or under an illusion keeps their
appearance and the selection surfaces later, when `crmain.cc` line 606 restores
it. **So the chooser closes on its own and never waits for a 142.**

## The two bridge gaps, and why they were gaps

`Real33DBridge.cpp` had two places that copied an `OutfitDescriptor` across the
engine boundary, and **both copied two of its four fields**:

```cpp
Appearance.OutfitId          = Source.outfit.outfit_id;
Appearance.bDisguisedAsObject = Source.outfit.disguised_as_object;
```

The four colour bytes and a disguise's `object_type` were dropped — on every
SV_CMD_CREATURE_OUTFIT and on every creature in a FULLSCREEN. ClientCore had
them all along (`movement.cpp` line 1148 stores the whole descriptor into the
creature record); the loss was at this boundary and nowhere else.

Fixed structurally rather than by patching both sites: `FReal33DOutfit` now
carries the whole descriptor, `FReal33DEvent::Outfit` replaces the two loose
fields, and **one** `ToUnrealOutfit` converts. A field added to
`OutfitDescriptor` is now either carried or caught by the compiler.

The two branches stay exclusive, because `TOutfit` is a union and `SendOutfit`
writes the id and then *either* an object type *or* the colours: each is copied
only under its own flag and the other is left at zero rather than filled with
something plausible. `AReal33DCreature::ApplyOutfit` takes the whole descriptor
and logs a disguise as an object type instead of drawing it as outfit zero,
which is what dropping the type used to make it look like.

Three `static_assert`s in the bridge pin the restated constants to ClientCore's:
the colour count, the slot count and the 132 bound.

## What was implemented

- `Real33DOutfitSelection.h` — new. The chooser's rules with no Slate, no
  bridge and no protocol: the four named slots, the 132 bound, and `OutfitDraft`,
  which cannot leave the band it was opened with. Tested natively.
- `Real33DOutfitPanel.h/.cpp` — new. The chooser, registered under
  `Real33D::PanelIds::Outfit`, a centred modal like the editor and trade
  windows. Opens only from `OutfitDialogOpened`.
- `Real33DBridge.h/.cpp` — `FReal33DOutfit`, `FReal33DOutfitDialog`, the
  `OutfitDialogOpened` event, the SV_CMD_OUTFIT decode, the two commands as
  intents with local validation, `NoteOutfit`, five counters, three
  `static_assert`s, and the two gap fixes.
- `Real33DCreatureActor.h/.cpp`, `Real33DWorldActor.cpp` — the whole descriptor
  through to the actor, and the five counters in the evidence snapshot.
- `Real33DHUD.h/.cpp` — **+80 lines, no deletions**: one include, one overlay
  slot plus its `Panels.Register`, one `Refresh` line, one member, and
  `ShowSelfContextMenu`. Most of that is the menu entry and its comments.
- `Real33DPlayerController.cpp` — a right-click on your own character, which
  used to be a no-op, opens that menu.
- `Real33DPanelRegistry.h/.cpp` — the `Outfit` id.
- `tests/real33d_outfit_selection_tests.cpp` — new, 20 cases.
- `tests/verify_outfit_color_mapping.py` — new, 43 assertions.
- `Real33DPresentationTests.cpp` — an `OutfitPanel` suite.
- `tests/regression_harness.sh`, `tests/README.md` — the corrections below.

### The entry point is the classic client's own documented gesture

Its help text, at `0x00565734` in that image, reads: *"To change your outfit,
right-click on your character and select 'Set outfit' from the context menu."*
So the gesture is not a guess and the entry's wording is the client's own.
Right-clicking your own character did nothing before this —
`InteractUnderCursor` returns early for the local player, because attacking
yourself is not something Fusion32 offers.

### Harness corrections

`real33d-native-tests` names its suites explicitly rather than globbing. That
is deliberate — a deleted or renamed suite should fail the check, not vanish
from it — but it means a new suite has to be added, and
`real33d_outfit_selection_tests` was the first one to prove the list can be
forgotten. Added.

`outfit-color-mapping` is a new check, so the harness now runs 10.

And a real defect the new check exposed: it selected its interpreter with
`command -v python3`, which on Windows resolves to an **App Execution Alias**
that exists on PATH, prints "Python was not found" and exits 49. The check
reported `FAIL` on a host with a working `python`. `working_python()` now probes
by running the interpreter, which is the same class of silent-false-result
hazard the three Windows helpers above it were written for.

## Tests

| Suite | Result |
| --- | --- |
| ClientCore CTest | **14 / 14** |
| ClientCore MSVC native (C++17 suites + C++20 archive) | **PASS** |
| `real33d_*` native | **6 / 6** (was 5; `outfit_selection` is new, 20 cases) |
| Unreal automation `REAL33D.Presentation` | **8 / 8**, 0 errors, 0 warnings (was 7) |
| REAL33DEditor Win64 Development | **built**, 0 warnings |
| `verify_outfit_color_mapping.py` | **43 / 43** |
| regression harness | **10 PASS / 0 FAIL / 0 SKIP** |
| harness selftest | **10 / 10** |
| `secret_check.sh` | **PASS 6 / 6** |

## Live QA

Fusion32 `baseline-772-0` on 7171/7172/7173, one real account (**B**,
creature 1002) on the REAL33D client. **The operator drove; nothing was clicked
by software.** Two runs, because the first one found a defect.

Timelines: `unreal-outfit-dialog-001/run1_outfit_timeline.txt` and
`run2_outfit_timeline.txt`. Counters: `run2_evidence.json`.

### Run 1 found a defect in this milestone's own UI

Run 1 opened the chooser twice, obeyed the band, selected two outfits (138, then
137) and got both authoritative updates back — but **both submits carried
`78,69,58,76`, the server's own defaults from `CheckOutfit`**. No colour was
ever changed.

It was not a protocol or model bug: the outfit id *did* change across both
submits, so the draft and the stepper bindings worked, and no rebuild happened
in between. The cause was the UI. Each colour row's **label was a button** whose
only effect was to highlight the row — copied from the 2D window, where the four
rows select which one a *shared palette* edits. This window gives every row its
own arrows, so the label was a control that looked like it did something and did
not, and the arrows went untouched.

Worse, **nothing logged it**, so "the operator did not press the arrows" and
"the arrows are broken" looked identical in the evidence. Both were fixed: the
label is plain text, the arrows are wider, and every preview step now logs its
before/after value and the whole four-tuple.

### Run 2, the complete chain

| Requirement | Observed |
| --- | --- |
| chooser opens only from the server | `self context menu opened` → `outfit chooser 1 requested` → **then** `outfit chooser offered` → `shown`. Three times, never out of order |
| server outfit range | `band=136..139 (4 outfit(s))` on all three — the non-premium non-male band, `136` with no `+3` |
| ≥ 2 legitimate outfits | previewed `137 → 136 → 139`, **wrapping at `First` and staying in band**; submitted `139` |
| each colour slot changed | **Head, Body, Legs and Feet all stepped** — 47 preview steps over three sessions, 12 / 12 / 11 / 12 per slot, each line moving exactly one position of the tuple |
| submit | 3 × CL_CMD_SET_OUTFIT: `77,67,56,73`, then `76,66,55,72`, then `68,59,49,66` |
| authoritative update | `creature 1002 outfit 139` after each submit — SV_CMD_CREATURE_OUTFIT for the local player, 3 of 3 |
| **reopen shows server state** | submit `77,67,56,73` → reopen `current=139 colours=77,67,56,73`. submit `76,66,55,72` → reopen `current=139 colours=76,66,55,72`. **Exact, twice** |
| protocol errors / anomalies / unsupported | **0 / 0 / 0** (`residual_bytes=0`, `unsupported_opcodes=0`, `protocol_anomalies=0`) |

The reopen lines are the strongest evidence in this milestone and they prove
four things at once: SET_OUTFIT encoded the colours correctly, Fusion32 stored
all four, SV_CMD_OUTFIT's decode carries them back, and **the colour bridge fix
works** — before it, those bytes would have returned as `0,0,0,0`.

| Counter | Run 2 |
| --- | --- |
| `outfit_dialogs_requested` | 3 |
| `outfit_dialogs_received` | 3 |
| `outfits_submitted` | 3 |
| `outfit_selections_refused` | **0** |
| `outfit_appearance_updates` | 3 |

The only `LogReal33D` warning in either run is the pre-existing V08 QA catalog
notice, which is unrelated and predates this milestone. The two log lines
matching "unsupported" are D3D12 reporting `atomic64 unsupported`.

## Remaining UNVERIFIED

- **The outfit failure path did not occur.** Nothing was refused, locally or by
  the server: every outfit was in band and every colour far below 132, because
  the controls cannot produce anything else. The three refusal reasons are
  covered by ClientCore's suite and by 20 native cases, and the local-refusal
  path is wired to a `ClientNotice` and a counter — but **no live refusal was
  observed**, and `outfit_selections_refused` is 0 for that reason and not
  because a refusal was handled.
- **The gamemaster-outfit refusal** — `CGetOutfit` answering with *"You may not
  change your outfit."* and no SV_CMD_OUTFIT. Account B does not hold
  `GAMEMASTER_OUTFIT`, so this was not reachable without editing server data,
  which was out of scope. The code path is the ordinary `ServerMessage` one and
  is shared with every other server message, but it is not claimed.
- **The colours change nothing on screen.** They are decoded, carried, stored on
  the actor, submitted and proven to round-trip, but `ResolveOutfit` still
  selects a mesh from the outfit id alone. Tinting needs the palette's RGB
  values, which live in the classic client's `.dat`/`.spr` and are **not
  derived here** — the mapping from a colour byte to a colour is `UNVERIFIED`.
  The chooser therefore shows each slot's numeric index and no swatch, rather
  than a fabricated colour.
- **The palette's true size.** 132 is the server's bound; whether the 2D client
  offers 133 swatches or some other number is not established, see above.
- **An object disguise was never seen live.** The disguise note and the
  `object_type` preservation are covered by tests and by the `OutfitPanel`
  suite; no creature in either session was disguised.
- **Only one character and one band.** The 128-series male band and the premium
  `+3` widening were not seen live; both are covered by native cases.

## Environment

Unchanged: VS2022 BuildTools MSVC 14.44.35207, UE 5.8. The harness selftest
needs `cl.exe` on PATH; `--objects-srv` must be a Git-Bash-style path
(`/c/...`), not `C:\...` — a Windows-style path reaches the WSL compiler
unconverted and the suite that reads it fails on `file.good()`.

**One server-side recovery, recorded in full.** Fusion32 would not start:
`Initialisierungsfehler: Game-Server is already running, PID file exists`. The
lock at `game/state/save/game.pid` named PID 562, which was **not alive**, with
nothing listening on 7171-7173 and no game process at all. It was **moved
aside, not deleted** — kept at
`build/unreal-outfit-dialog-001/stale-game.pid.bak` — and the server then
started cleanly. No server data was edited and no gameplay state was touched.
