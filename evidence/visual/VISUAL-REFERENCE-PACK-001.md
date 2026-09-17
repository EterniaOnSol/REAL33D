# VISUAL-REFERENCE-PACK-001

Status: `PASS`

The 7.72 visual source was located in the authorised local workspace, its
format was established and validated rather than assumed, and the master
inventory now has a navigable artist catalogue with original references.

## 1. Visual source

Found in the project's own authorised workspace, already pinned by
`CLASSIC-CLIENT-772-001`:

```text
build/classic-client-772/app/Tibia.dat   175,805 bytes
build/classic-client-772/app/Tibia.spr   15,877,515 bytes
```

| File | SHA-256 |
| --- | --- |
| `Tibia.dat` | `3C5E857FF72FD1E52879EB8845ADC72C0991786AD0EAF85F6847595C9677AEDD` |
| `Tibia.spr` | `86ABBF5FADCF84313A03615A55A8BD626BAAB98671F0A78C9B36CB61CCE0C64A` |

Internal signatures: dat `0x439D5A33`, spr `0x439852BE`.

Nothing was downloaded and no substitute was used. The files remain local
inputs; neither they nor anything derived from them is committed.

### What does and does not demonstrate the version

Worth stating plainly, because one obvious argument does not hold.

**The live login does not prove it.** `reference/login/src/connections.cc`
lines 598-600 read `DATSIGNATURE`, `SPRSIGNATURE` and `PICSIGNATURE` and discard
them. A successful session proves nothing about which appearance data the client
was using.

What does support it:

- These are the exact files `verify_classic_client_772.py` pins, alongside a
  `Tibia.exe` whose version marker at the Fusion32-documented address
  `0x55C65D` reads `Version 7.72 ... CipSoft GmbH`.
- The client's declared counts line up with the server's own content, three
  ways at once, listed under validation below.

Provenance of the copy itself remains `UNKNOWN`, exactly as
`docs/CLASSIC_CLIENT_772.md` records. That is unchanged by this task.

## 2. Format established, not assumed

No specification for `Tibia.dat` exists in this project's source truth, so
`visual/tools/tibia772.py` treats the layout as a hypothesis and proves it.

| Check | Result |
| --- | --- |
| Records parse and consume the file to exactly EOF | 5,284 records, final byte reached |
| Header counts match what was parsed | item 5089, outfit 254, effect 25, missile 15 |
| Every sprite id is inside the sprite file | highest referenced 10,961 of 10,962 |
| Client options agree with server flags | 9 of 15 at 100%, the rest 97.5-99.0% |

The fourth check is the independent one. The reader never opens
`dat/objects.srv`, yet its option bytes line up with the server's object flags
across thousands of items:

| dat option | server flag | both | only dat | only server | agreement |
| --- | --- | --- | --- | --- | --- |
| `0x01` | `Clip` | 311 | 0 | 0 | 100% |
| `0x03` | `Top` | 82 | 0 | 0 | 100% |
| `0x05` | `Cumulative` | 81 | 0 | 0 | 100% |
| `0x0A` | `LiquidContainer` | 23 | 0 | 0 | 100% |
| `0x0B` | `LiquidPool` | 12 | 0 | 0 | 100% |
| `0x10` | `Take` | 1,063 | 0 | 0 | 100% |
| `0x12` | `HookSouth` | 13 | 0 | 0 | 100% |
| `0x13` | `HookEast` | 13 | 0 | 0 | 100% |
| `0x14` | `Rotate` | 95 | 0 | 0 | 100% |
| `0x11` | `Hang` | 96 | 0 | 1 | 99.0% |
| `0x02` | `Bottom` | 1,245 | 0 | 19 | 98.5% |
| `0x0C` | `Unpass` | 1,947 | 0 | 38 | 98.1% |
| `0x0D` | `Unmove` | 3,572 | 0 | 74 | 98.0% |
| `0x00` | `Bank` | 1,179 | 0 | 28 | 97.7% |
| `0x04` | `Container` | 267 | 1 | 6 | 97.5% |

Disagreement is almost entirely one-directional: the server carries a flag the
client does not, which is what a server tracking behaviour the client need not
draw looks like. A misread layout would give scattered two-way noise.

Two further alignments, independent of the option table:

- `effect` count is 25, and `reference/game/src/enums.hh::EffectType` runs
  `EFFECT_BLOOD_HIT = 1` through `EFFECT_MELODY_WHITE = 25`. Exact.
- `missile` count is 15, and the highest missile value in any `objects.srv`
  `ThrowMissile`, `WandMissile` or `AmmoMissile` attribute is 15. Consistent.

### `patternZ` is present

The first ground reads `01 01 01 04 04 01 01` followed by sprite ids 136..151:
width 1, height 1, one layer, a 4x4 pattern, one z, one frame, so sixteen
sprites, and exactly sixteen valid ids follow. Omitting the z field would shift
every later read and produce ids far outside the sprite file.

## 3. Validation counts

| Metric | Count |
| --- | --- |
| **Appearances decoded** | **5,284** |
| items | 4,990 |
| outfits | 254 |
| effects | 25 |
| missiles | 15 |
| **Sprites decoded** | **10,926** of 10,962, zero failures |
| empty sprite slots | 36 |
| Appearances named from the master inventory | 5,284 of 5,284 |
| Monster races resolved | 159 of 159, each to an outfit id |
| NPCs resolved | 337 of 337 |
| Outfits with a referencing monster, npc or player range | 154 of 254 |
| Objects resolved | 4,990 of 4,991 declared |
| Effects resolved | 25 of 25 |
| Missiles resolved | 15 of 15 |
| **Unresolved** | **101** |
| Distinct sprite sets across all appearances | 4,587 |
| Sets used by more than one appearance | 310, covering 835 ids |
| **P0 Rookgaard appearances** | **456** |
| Previews generated | 5,284, zero failures |

Every sprite decoded without a single failure, and every preview rendered.

## 4. Unresolved, and why

**100 outfits.** Shipped by the client, referenced by no monster file, no npc
file and no player-selectable range. `reference/game/src/sending.cc::SendOutfit`
fixes the player range at 128..131 male and 136..139 female, each extended by
three with premium, and those are resolved. The remaining 100 are real
appearances with real sprites, catalogued and previewed, with nothing in the
server data pointing at them.

**One object type.** Id 5090, `a treasure map`, is declared in `objects.srv`
but the client's item ids stop at 5089. It stays in the master inventory with no
preview. The ranges are otherwise contiguous and gapless, 100..5089 on both
sides, so this is one known id rather than general ambiguity.

**36 empty sprite slots** decode to nothing because there is nothing in them.

Nothing here is an ambiguous association. Every one of the 5,284 appearances
maps to exactly one inventory identity.

## 5. Visual families revalidated

The master inventory's 1,351 visual groups came from name and category, which
was inference. Against real sprite evidence:

- 4,587 distinct sprite sets exist across 5,284 appearances.
- 310 sets are used by more than one appearance, covering 835 ids. Those are
  **demonstrated** identical pictures.
- Sets that overlap without matching share artwork, which is what modular
  pieces and recolour families look like.

Neither figure is a model count, and the docs say so. 4,587 distinct sprite sets
bounds unique artwork, not meshes: a wall drawn from four directions is four
sprite sets and one mesh, while a 4x4 ground pattern is one sprite set that may
want one mesh plus a varying material. `Representation` in the tracker stays
editable because that call belongs to the artist.

Every Fusion32 id keeps its mapping: `appearances.csv` carries the tracker id,
client id, inventory name, category, priority and visual group on every row.

## 6. Artist workspace

`visual/reference_pack/catalogue.html` and `p0_rookgaard.html` are filterable
by name, id, category and group. Each card shows the human name, tracker id,
category and subcategory, priority, visual group, full geometry, distinct
sprite count and provenance, above a sheet with one column per direction,
pattern and layer and one row per animation frame.

Previews are nearest neighbour at 3x with no filtering, smoothing or colour
correction. The reference stays pixel exact because deciding how far to depart
from it is the artist's job and cannot be done against an altered image.

The flow `REFERENCE -> MOCKUP -> REVIEW -> APPROVED/REJECTED -> PRODUCTION ->
READY -> INTEGRATED` is unchanged from `visual/docs/PIPELINE.md`. There is no
fidelity percentage; the artist proposes and the director decides.

## 7. Tests

`visual/tools/test_tibia772.py`: **24 tests, all passing**, with every fixture
built byte by byte inside the test so the suite runs without the client data and
nothing proprietary is committed.

Coverage: minimal file, per-kind id bases, the `patternZ` sprite count, the
exact-size rule for multi-field things, option payload consumption, header
counts, sprite range validation, the option/flag correlation in both the
agreeing and disagreeing direction, RLE decoding with transparent prefixes and
multiple runs, PNG output and nearest-neighbour scaling. Failure cases: trailing
bytes, **every** truncation of a file, unknown option bytes, RLE overruns, a
truncated offset table and a short header.

## 8. Repository and binaries

`Tibia.dat` and `Tibia.spr` are **not committed**. The reference pack, 99 MB of
derived previews, is **not committed**: `/visual/reference_pack/` was added to
`.gitignore`. Previews are derivative works of an artifact with `UNKNOWN`
provenance, so they are rebuilt locally rather than published.

The existing Git LFS rules from `VISUAL-ASSET-MASTER-INVENTORY-001` are
unchanged and still cover future original 3D assets. No binary was committed by
this task either.

`visual/tools/setup_reference_pack.sh` is the single setup command. It verifies
both client files against the recorded SHA-256 and refuses to continue on a
mismatch, so a different client version cannot silently produce a catalogue
describing a different game.

## Checks

- Parser tests: 24/24 `PASS`
- `tests/secret_check.sh`: `PASS`
- `reference/` untouched
- `clientcore/` untouched; no protocol or gameplay change
- No 3D art produced, no Unreal work started
