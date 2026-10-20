# CLIENTCORE-OUTFIT-772-001

Status: `PASS` for the deterministic protocol layer. **Not** `CERTIFIED`: no
live session has opened the chooser or changed an outfit, and nothing in this
milestone draws one.

ClientCore only. No Unreal, no HUD, no input, no gameplay rule, no Fusion32
server or protocol change, no 2D, no agent, no V08 asset. Base `origin/main`
at `2428a14167ffec32e2fa2c922f60f26b063a0cea`, developed on
`milestone/clientcore-outfit-772-001` in its own worktree. No main merge.

## Scope, after auditing what was already there

The audit came first, and it changed the scope. `receiving.cc` and
`sending.cc` name exactly four outfit paths, and three of them were **already
complete** before this milestone:

| Path | Opcode | State before | State after |
|---|---|---|---|
| creature outfit inside `SendMapObject` | — | decoded (`MapScanner::ReadOutfit`) | unchanged, newly covered on both branches |
| `SV_CMD_CREATURE_OUTFIT` | 142 | decoded (`CreatureAttribute::Outfit`) | unchanged, newly covered on the disguise branch |
| `SV_CMD_OUTFIT` | 200 | decoded (`OutfitDialogUpdate`) | unchanged, newly covered on the disguise branch |
| `CL_CMD_GET_OUTFIT` | 210 | **missing** | built |
| `CL_CMD_SET_OUTFIT` | 211 | **missing** | built |

So the genuinely missing work was the client's half plus the server's
accept/reject rules, and the genuinely missing *coverage* was the
`OutfitID == 0` branch of both server commands, which no existing test
exercised. Three `SendOutfit` call sites exist in `sending.cc` (lines 262,
1024, 1678) and all three are accounted for above; `reference/login` and the
querymanager carry no outfit over the wire at all.

Nothing was invented to fill a gap. In particular there is **no addon byte, no
mount and no second look-type word** in 7.72: `SendOutfit` writes the id and
then *either* an object type *or* four colour bytes, and that is the whole of
an outfit on this wire.

## Layouts, traced

Every field below comes from the server's own reader or writer, not from a
7.72 protocol description.

| Command | Source | Layout |
|---|---|---|
| `CL_CMD_GET_OUTFIT` (210) | `receiving.cc::CGetOutfit` (1390) | no payload — 1 byte |
| `CL_CMD_SET_OUTFIT` (211) | `receiving.cc::CSetOutfit` (1410) | `word OutfitID`, `byte Colors[4]` — **always** 7 bytes |
| `SV_CMD_CREATURE_OUTFIT` (142) | `sending.cc::SendCreatureOutfit` (1011) | `quad CreatureID`, outfit — 11 or 9 bytes |
| `SV_CMD_OUTFIT` (200) | `sending.cc::SendOutfit(TConnection*)` (1661) | outfit, `word FirstOutfit`, `word LastOutfit` — 11 or 9 bytes |

The shared outfit encoding, `sending.cc::SendOutfit(TConnection*, TOutfit)`
(162), over `cr.hh::TOutfit` (17):

```
word OutfitID
  if OutfitID == 0:  word ObjectType      // the union's other member
  else:              byte Colors[4]
```

Six details the source settles, and which the tests pin:

1. **The set command does not branch, and the send command does.**
   `SendOutfit` tests `OutfitID == 0`; `CSetOutfit` does not — it reads
   `readWord()` and then `readBytes(Colors, 4)` unconditionally. A set command
   is therefore always seven bytes and **can never express an object
   disguise**. `ValidateOutfitSelection` reports that as
   `DisguiseNotSelectable` rather than letting a caller mangle it on the wire.

2. **The dialog carries a band, not a list.** `SendOutfit` writes only
   `FirstOutfit` and `LastOutfit`; every id between them is implied. This is
   the 7.72 shape, and it is worth stating because later protocol versions
   replaced it with a counted list of individual outfits.

3. **The band is 128–131 / 136–139, plus three for premium.**
   `(Player->Sex == 1) ? 128 : 136`, last `? 131 : 139`, then
   `if(CheckRight(PREMIUM_ACCOUNT)) LastOutfit += 3`. `sending.cc` (1671-1675)
   and `receiving.cc` (1425-1429) compute it identically — the server states
   the band and then re-derives it to check the answer. Note the shape: the
   test is `Sex == 1`, so male is the 128 series and **anything that is not 1**
   takes the 136 series. Fusion32 never tests for 2 on that branch.

4. **The colour bound is `> 132`,** applied to each of the four slots
   independently (`receiving.cc` 1440-1446). So 132 is valid and 133 is not.

5. **The server acknowledges a set with nothing at all.** `CSetOutfit` returns
   `void`, sends no confirmation, no second `SV_CMD_OUTFIT`, and no error when
   it drops the selection. The one observable effect is indirect:
   `AnnounceChangedCreature(CREATURE_OUTFIT_CHANGED)`, which
   `operate.cc` line 81 turns into `SV_CMD_CREATURE_OUTFIT` for every
   connection that knows the creature.

6. **…and that announcement is conditional.** `CSetOutfit` broadcasts only
   `if(Player->Outfit == Player->OrgOutfit)` — only when the player is wearing
   their own outfit at that moment. A player who is invisible or under an
   illusion keeps the appearance they have; the selection is stored in
   `OrgOutfit` and surfaces later, when `crmain.cc` line 606 restores it. **A
   dialog must therefore close on its own and must not wait for a 142.**

And one more, about the request rather than the answer: `CGetOutfit` refuses a
character holding `GAMEMASTER_OUTFIT`, sending a `TALK_FAILURE_MESSAGE`
(mode 23, "You may not change your outfit.") and returning **without sending
200 at all**. That arrives as `ServerUpdateKind::Message`, so nothing is lost
— but a caller that waits for 200 before showing anything waits forever.

## What the four colour bytes mean, honestly

**Fusion32 never names them.** `TOutfit` declares `uint8 Colors[4]` and every
use in the server — `CheckOutfit`, `CSetOutfit`'s bound test, `SendOutfit`'s
`SendBytes` — addresses them by index only. There is no `lookHead`, no
`lookBody`, nothing of the kind anywhere in `reference/`; a grep for every
spelling of a named colour slot returns nothing.

Which slot tints which part of the sprite is decided entirely by the classic
client's `.spr`/`.dat` template, outside this repository, and the 7.72
protocol never states it. This layer therefore carries four indexed bytes in
wire order and **claims no body part for any of them**. A renderer has to
settle that mapping by observation.

The only thing the source does give is a reference point: `CheckOutfit` forces
a fresh character to `{78, 69, 58, 76}`, exposed here as
`kDefaultOutfitColors`, and a gamemaster to id 75 with all four zero.

## The opcode collision, which is real

210 and 211 are *also* `SV_CMD_BUDDY_DATA` and `SV_CMD_BUDDY_ONLINE`.
`ClientCommand` and `ServerCommand` are separate enums in `connections.hh` and
7.72 reuses values freely between the two directions. Direction is what
disambiguates an opcode on this protocol, never the number alone.

This is a silent failure, not a loud one, and the suite proves it: a
well-formed `CL_CMD_SET_OUTFIT` fed to the server-side dispatcher **decodes
successfully** as a buddy record with a character id assembled from the outfit
id and the first two colours. Two `static_assert`s pin the collision so a
renumbering of either enum stops the build.

## Architecture

Ordinary ClientCore builders. No raw packets: the builders return byte vectors
the existing session sends, and the two server commands keep going through
`MapScanner`'s bounds-checked reads and the shared `ReadScannerOutfit` /
`ReadScannerWord` helpers — unchanged, because they were already correct.

`outfit.h` sits above `player_state.h` and includes it, because
`OutfitDialogUpdate` and the two server opcodes live there and are **not
redeclared**. Nothing in `player_state.h`, `map_scan.h` or `movement.h` was
modified, so no existing decode path changed behaviour.

`OutfitSelectionRange` and `ValidateOutfitSelection` are separate from the
builders on purpose. `BuildSetOutfitCommand` encodes what it is handed without
applying the band or colour tests, for the same reason
`BuildUseObjectCommand` does not test MULTIUSE and `BuildTradeObjectCommand`
does not pre-apply `isMapContainer`: the caller states what it means, Fusion32
rules on it, and a caller that wants to refuse first says so explicitly.

**The dialog stores nothing in WorldState**, which was already the existing
decision and is now justified by a test rather than only by a comment.
`SendOutfit` sends `Player->OrgOutfit`, not `Player->Outfit` — what the player
would look like *without* an active illusion, which is not what is on screen.
Writing it into the creature record would make an invisible player visible at
the moment they opened the chooser. `SV_CMD_CREATURE_OUTFIT` does reach the
creature record, colours and all, and a test proves that too.

## Tests

`clientcore/tests/outfit_tests.cpp`, registered as `protocol772_outfit_tests`
in `clientcore/CMakeLists.txt` and in `tests/build_clientcore_windows.cmd`
(both its source set and its suite set).

Golden vectors are hand-computed from the Fusion32 source first. The shared
emitter in `fixtures/fullscreen_772_vectors.h` is a literal port of
`SendOutfit`, so it is asserted against the hand-computed bytes **before** it
is used to build anything else: a generator that agrees with a decoder written
from the same misreading proves nothing.

Client commands:

- `d2` — get outfit, the bare opcode
- `d380004e453a4c` — set outfit 128 with the default colours `{78,69,58,76}`
- `d3341201020304` — id `0x1234` and four distinct colours, pinning the
  little-endian word and the raw byte order together
- `d3000000000000` / `d3ffff84848484` — both word extremes, and 132 in all
  four slots
- `d38e0000848400` — 142, the last premium female id, with the colour extremes
  interleaved
- `d3c8004e453a4c` / `d3800085000000` — an out-of-band id and a colour of 133,
  encoded verbatim, with `ValidateOutfitSelection` refusing both

Server commands, each decoded through the real `DecodeServerUpdate`
dispatcher rather than only the body reader, so opcode routing is proven too:

- `c880004e453a4c80008300` — `SV_CMD_OUTFIT`, colour branch, male
  non-premium. Identical to the `kGoldenOutfitDialogHex` the `player_state`
  suite already pins, reached from here as well so the two cannot drift
- `c80000d20480008600` — `SV_CMD_OUTFIT`, **disguise branch**, 9 bytes, band
  128–134 (premium male). Previously uncovered
- `c80000000088008b00` — the same with `TOutfit::Invisible()` exactly: id 0,
  object type 0
- `8ee9030000810001020304` — `SV_CMD_CREATURE_OUTFIT`, colour branch
- `8ee90300000000260b` — `SV_CMD_CREATURE_OUTFIT`, **disguise branch**, which
  is the shape every illusion arrives in. Previously uncovered

Negative, truncation and boundary cases:

- **36 truncated prefixes**: every proper prefix of all four well-formed
  server vectors — both branches of both commands — must fail as
  `MapDecodeError::Truncated` with `bytes_consumed == 0`, so nothing is
  partially applied. The count is asserted, not just printed
- an empty buffer
- trailing bytes after a dialog: decode succeeds and `bytes_consumed` stays
  exact, so a caller sees the residue. This layer does not reject it, because
  a framed connection carries several commands back to back
- the id word flipped to zero inside otherwise identical bytes, which changes
  which branch is read and moves the two band words two bytes earlier — the
  one field a reader cannot skip past without understanding it
- a `CL_CMD_SET_OUTFIT` misrouted into the server-side dispatcher, which
  succeeds as a buddy record (see the collision section)
- every band boundary, both sexes, premium and not: 127/128/131/132 and
  134/135, 135/136/139/140 and 142/143
- sex values 0, 2, 3 and 255 all taking the 136 series, proving the shape of
  `Sex == 1` rather than two known values
- the colour bound at 132, 133 and 255, applied to each of the four slots
  independently
- band-before-colour ordering: a selection failing both is reported as the
  band failure, which is the first complaint `CSetOutfit` would make
- id 0 refused as out-of-band and not as a disguise, because `CSetOutfit` has
  no disguise branch to take
- a male id offered to a female band and the reverse
- a premium id refused on a non-premium band and accepted on the premium one
- an inverted band (`140..130`): `count() == 0`, `contains()` false
  everywhere, and the decoder deliberately **accepts** it. Unlike a container
  number or an inventory slot, an inverted band leaves the rest of the command
  perfectly locatable, so there is nothing to resynchronise and nothing to
  guess; the test documents that as a decision rather than an oversight
- a band of one (`first == last`) and a full-word band (`0..65535`)
- outfit 75 outside every band, consistent with `CGetOutfit` refusing the
  chooser to a gamemaster rather than offering a band that could not contain
  what the character wears

## Results

Measured in this session, in this worktree.

- `ctest`: **14/14 suites PASS** (was 13 before this milestone added one), via
  WSL `Ubuntu-26.04`, g++ 15.2.0, cmake 4.2.3,
  `-Wall -Wextra -Wpedantic -Werror`
- **ASan + UBSan** rebuild of all fourteen:
  `-fsanitize=address,undefined -fno-sanitize-recover=all`,
  `ASAN_OPTIONS=detect_leaks=1`, **14/14 PASS**, outfit suite clean.
  The instrumentation was verified rather than assumed: `libasan.so.8` and
  `libubsan.so.1` are linked into the suite binary, `__asan_init`,
  `__asan_report_load1` and `__ubsan_handle_add_overflow` are present and are
  *absent* from the plain Debug binary, and a deliberate heap-buffer-overflow
  canary built with the same flags aborts with the expected diagnostic. A
  first attempt at this check grepped a quiet build log for the flag string
  and reported "NOT FOUND"; that check was vacuous and was replaced, not
  believed
- **MSVC `/W4 /WX /permissive-`**: `tests\build_clientcore_windows.cmd` under
  VS 2022 BuildTools with the UE 5.8 ThirdParty OpenSSL —
  `WINDOWS CLIENTCORE: PASS`, exit 0. That covers the library at **both**
  C++17 and the C++20 the Unreal module links, plus all 14 suites natively.
  Two `C4127` "conditional expression is constant" errors in the first
  attempt were real: three compile-time facts were being asserted at runtime,
  and are now `static_assert`s
- `tests/regression_harness.sh`: **`PASS_WITH_SKIPS`**, 7 PASS / 0 FAIL /
  2 SKIP. `clientcore-tests` PASS with 14 suites, `real33d-native-tests` PASS
  5/5, `secret-check` PASS, `build-sanity-msvc` **PASS** (the harness was run
  from a shell with `cl.exe` on PATH so the check executed rather than
  skipping). The two SKIPs are `unreal-module-build`, which is opt-in, and
  `protocol-log-check`, which has no machine-written log in scope because this
  milestone produced no live capture
- `tests/secret_check.sh`: **PASS**, 6/6 sub-checks

## What REAL33D receives and renders today

Measured against the Unreal module in this worktree, not assumed.

Already working end to end:

- `OutfitDescriptor` reaches `FReal33DEvent` as `OutfitId` and
  `bDisguisedAsObject` (`Real33DBridge.cpp` 762-763 for 142, 1170-1171 for the
  initial world)
- `AReal33DCreature::ApplyOutfit(OutfitId, bDisguisedAsObject, Registry)`
  resolves a mesh through `UReal33DAssetRegistry::ResolveOutfit(uint16)` and
  plays an idle/walk clip
- an outfit change mid-session repaints: `Real33DWorldActor.cpp` 611 and 650
  both call `ApplyOutfit`

Not reaching Unreal at all today:

- **the four colour bytes.** The bridge copies `outfit_id` and the disguise
  flag and drops `colors` entirely, so every creature of a given id renders
  identically. ClientCore has had the colours since `INITIALWORLD-772-001`;
  nothing consumes them
- **`object_type`.** `bDisguisedAsObject` crosses, the type does not, so a
  disguised creature cannot be drawn as the object it is disguised as
- **`SV_CMD_OUTFIT` itself.** ClientCore decodes it into
  `ServerUpdateKind::OutfitDialog`; the bridge has no branch for that kind, so
  it is decoded, counted as handled — and discarded. It is *not* reported as
  an unsupported opcode, so no protocol anomaly is raised and nothing in the
  logs would reveal the gap

## Limits

- Nothing live. No session has opened the chooser, no outfit has been changed,
  and no screenshot exists. `PASS` here is the deterministic protocol layer
  only.
- No UI. The Unreal outfit dialog is a separate milestone; this one
  deliberately adds no Unreal, HUD or input code.
- The four colour slots are carried in wire order with no body-part meaning
  claimed, for the reason given above. Whoever draws the dialog owns that
  mapping and will have to establish it against the classic client.
- `CGetOutfit`'s gamemaster refusal is documented and its reply shape named,
  but no test exercises a `TALK_FAILURE_MESSAGE` arriving in place of a 200:
  that is a live-session behaviour and the message path is covered elsewhere.
