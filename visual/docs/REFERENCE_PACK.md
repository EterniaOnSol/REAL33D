# Reference Pack

The artist-facing catalogue: every visual identity in the 7.72 world with its
original sprite, its name, its category, its priority and where it came from.

## Rebuild it after cloning

The pack is **not in the repository**. It is derived from the client's
`Tibia.dat` and `Tibia.spr`, whose provenance is recorded as `UNKNOWN` in
`docs/CLASSIC_CLIENT_772.md`, so its previews are derivative works of an
artifact this project does not redistribute.

Put the authorised client files here:

```text
build/classic-client-772/app/Tibia.dat
build/classic-client-772/app/Tibia.spr
```

Then run one command:

```powershell
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/visual/tools/setup_reference_pack.sh
```

It verifies both files against the SHA-256 recorded in
`evidence/client/CLASSIC-CLIENT-772-001.md`, refuses to continue if they differ,
regenerates the master inventory, and builds the catalogue. About four and a
half minutes end to end.

Open `visual/reference_pack/catalogue.html` for everything, or
`visual/reference_pack/p0_rookgaard.html` to start with Rookgaard.

Do not substitute another client version or a download from elsewhere. The
catalogue would silently describe a different game, and the hash check exists to
stop that.

## What you get

```text
visual/reference_pack/
  catalogue.html        every appearance, filterable by name, id, category, group
  p0_rookgaard.html     the 456 P0 object types, same view
  previews/<kind>/<id>.png
  manifests/appearances.csv     geometry and sprite ids per appearance
  manifests/visual_families.csv families that share artwork
  summary.json          the validation report
```

Each card shows the human name, the tracker id, category and subcategory,
priority, visual group, the full geometry, how many distinct sprites it uses,
and its provenance. The image is a sheet: one column per direction, pattern and
layer, one row per animation frame.

Previews are nearest neighbour at 3x. No filtering, no smoothing, no colour
correction. The reference has to stay pixel exact, because deciding how far to
depart from it is the artist's job and cannot be done against an already
altered image.

## How the format was established

There is no published specification for `Tibia.dat` in this project's source
truth, so the reader treats its layout as a hypothesis and proves it four ways.

1. **It consumes the file exactly.** 5,284 records parse and land on the final
   byte. A wrong option payload size derails the stream within a few records.
2. **Every sprite id is in range.** The highest referenced is 10,961 against a
   sprite file declaring 10,962.
3. **The header counts match** what was parsed, per kind.
4. **The client agrees with the server.** This is the independent check: the
   reader never looks at `dat/objects.srv`, yet its option bytes line up with
   the server's own object flags across thousands of items.

| dat option | server flag | agreement |
| --- | --- | --- |
| `0x01` | `Clip` | 100% |
| `0x03` | `Top` | 100% |
| `0x05` | `Cumulative` | 100% |
| `0x0A` | `LiquidContainer` | 100% |
| `0x0B` | `LiquidPool` | 100% |
| `0x10` | `Take` | 100% |
| `0x12` | `HookSouth` | 100% |
| `0x13` | `HookEast` | 100% |
| `0x14` | `Rotate` | 100% |
| `0x11` | `Hang` | 99.0% |
| `0x02` | `Bottom` | 98.5% |
| `0x0C` | `Unpass` | 98.1% |
| `0x0D` | `Unmove` | 98.0% |
| `0x00` | `Bank` | 97.7% |
| `0x04` | `Container` | 97.5% |

The residual disagreements are almost entirely one-directional: the server
carries a flag the client does not, which is what you would expect when the
server tracks behaviour the client need not draw. Scattered two-way noise would
have meant a misread layout.

### `patternZ` exists in this file

The first ground reads `01 01 01 04 04 01 01` followed by sprite ids 136..151.
That is width 1, height 1, one layer, a 4x4 pattern, **one z**, one frame, so
sixteen sprites, and exactly sixteen valid ids follow. Dropping the z field
would shift every subsequent read and produce ids far outside the sprite file.

## Tests

`visual/tools/test_tibia772.py` builds every fixture byte by byte inside the
test, so the suite runs without the client data and nothing proprietary is
committed. 24 tests covering geometry, the `patternZ` shape, the exact-size
rule, option payloads, RLE decoding, PNG output, and the failure cases:
trailing bytes, every truncation, unknown option bytes and RLE overruns.

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/test_tibia772.py
```

## Visual families

The master inventory grouped 5,003 object ids into 1,351 visual groups by name
and category, which was inference. The reference pack revalidates that against
what the client actually draws.

4,587 distinct sprite sets exist across all 5,284 appearances. 310 of those sets
are used by more than one appearance, covering 835 ids: those are
**demonstrated** identical pictures, not guesses. Families that overlap without
matching share artwork, which is the shape of modular pieces and recolours.

Neither number is a model count. 4,587 distinct sprite sets is an upper bound on
unique artwork, not on meshes: a wall drawn from four directions is four sprite
sets and one mesh, while a 4x4 ground pattern is one sprite set that may want
one mesh and a material that varies. Deciding that is the artist's call, which
is why `Representation` in the tracker stays editable.

## What is still unresolved

**100 outfits** are shipped by the client but referenced by no monster file, no
npc file, and no player-selectable range. They are real appearances with real
sprites, listed and previewed, simply with nothing in the server data pointing
at them.

**One object type**, id 5090, `a treasure map`, is declared in
`dat/objects.srv` but has no client counterpart: the client's item ids stop at
5089. It appears in the master inventory and has no preview.

**36 sprite slots** out of 10,962 are empty in the sprite file. They decode to
nothing because there is nothing there.
