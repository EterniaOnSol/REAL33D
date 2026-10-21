# UNREAL-OUTFIT-COLOR-RENDERING-001 — audit, and why rendering stopped

Task ID: `UNREAL-OUTFIT-COLOR-RENDERING-001`.
Branch: `milestone/unreal-outfit-color-rendering-001`, from `origin/main` at
`25c4448b7bd36a5a75ae4e2efd679f0ea500e9d1`. No main merge. No push.

**VERDICT = `BLOCKED_VISUAL_ASSET`.** No rendering was implemented. The task's
own rule was followed: *"If assets cannot represent the 4 regions faithfully:
STOP implementation."* They cannot, and the proof is below and reproducible.

The palette half of the audit succeeded completely and is delivered, because
it is what the next attempt needs and it does not depend on the assets.

## What the audit was asked to establish, and what it found

| Audit item | Result |
| --- | --- |
| current outfit meshes / materials / material slots | 16 delivered outfits, each 8-10 separate nodes of Blender primitives; 5 materials per outfit named by **tonal value**, not by region; untextured flat greyscale base colours |
| can Head/Body/Legs/Feet be tinted independently | **No.** Head, Body and Legs are addressable by node. **Feet has no geometry in any of the 16.** No region mask exists anywhere |
| exact 7.72 colour-index -> RGB mapping | **Derived and verified.** 133 colours, 7 rows x 19 columns, computed not tabulated. 51 assertions against the hashed client image |

## PALETTE — verified, and it is an algorithm, not a table

Fusion32 has no palette at all: it validates `Colors[n] > 132` and stores the
byte. So the authority is the classic 7.72 client, read statically — the binary
is opened, never executed, as `AGENTS.md` requires.

It does not contain a colour table. It *computes* every swatch from the index,
and `tests/verify_outfit_palette.py` reproduces that computation and asserts
the structure that produces it:

```
Tibia.exe 0x0047D380   the palette loop: exactly 133 swatches, i = 0..132,
                       control id 2000 + i, each colour fetched by calling
                       0x00509280 with the index
Tibia.exe 0x00509280   index -> (hue, saturation, value):
                         row = index / 19 ;  col = index % 19
                         col == 0 ->  h = 0, s = 0, v = 1 - row/7
                         else     ->  h = col/18, (s,v) = ROW_SV[row]
Tibia.exe 0x00509080   HSV -> RGB, six 1/6-wide sectors, low channel
                       (1-s)*v, each channel round(x * 255)
```

`19 * 7 = 133`, which is exactly the range `receiving.cc` lines 1440-1443
accept: `0..132`. The two numbers were derived independently and agree.

The per-row saturation/value table, each pair read from the row's own case:

| row | s | v | at |
| --- | --- | --- | --- |
| 0 | 0.25 | 1.0 | `0x0050930F` |
| 1 | 0.25 | 0.75 | `0x00509321` |
| 2 | 0.5 | 0.75 | `0x00509333` |
| 3 | 0.667 | 0.75 | `0x00509345` |
| 4 | 1.0 | 1.0 | `0x00509357` |
| 5 | 1.0 | 0.75 | `0x00509369` |
| 6 | 1.0 | 0.5 | `0x0050937B` |

**All six HSV sectors were decoded from the disassembly instruction by
instruction**, not inferred from the shape of the constants. They turn out to
be textbook six-sector HSV, but that was the conclusion rather than the
assumption: sectors differ in which of the three values stays on the FPU stack
and which two go to locals, and two of them enter the shared tail one
instruction later to skip a `fstp`. The decode is recorded in
`unreal-outfit-color-rendering-001/palette_derivation.txt`.

Independent corroboration nobody arranged: the four colours
`crplayer.cc::CheckOutfit` gives a fresh character come out of this palette as

| byte | index | colour | reads as |
| --- | --- | --- | --- |
| Head | 78 | `#FFAA00` | orange hair |
| Body | 69 | `#4040BF` | blue shirt |
| Legs | 58 | `#BF6A40` | brown trousers |
| Feet | 76 | `#6D6D6D` | grey boots |

A server default that decodes to a sensible rookie is not something a wrong
palette would produce. Together with a pure-white index 0, a descending
greyscale first column, and row 4 containing all six primaries and secondaries
exactly, the derivation is `PALETTE_VERIFIED`.

## ASSET_COLOR_CAPABILITY — insufficient, and exactly why

Reproduce with:

```
python tests/audit_outfit_asset_regions.py <any delivered creature .glb>
```

Outfit 128 (`128_rookie`) is the only outfit in **either** player band with
delivered art. Its entire geometry:

| node | mesh | material | base colour |
| --- | --- | --- | --- |
| `head` | Icosphere.001 | `128_light` | greyscale |
| `torso` | Icosphere | `128_mid` | greyscale |
| `tunic` | Cube | `128_accent` | greyscale |
| `arm.L` / `arm.R` | Cylinder / Cylinder.002 | `128_shadow` | greyscale |
| `pelvis` | Icosphere.002 | `128_dark` | greyscale |
| `leg.L` / `leg.R` | Cylinder.001 / Cylinder.003 | `128_dark` | greyscale |

### The exact missing capability

**1. No feet geometry, in any outfit.** This is the hard blocker. Across all
16 delivered outfits, **0 have any node, mesh or material representing feet,
boots or ankles.** The `Feet` colour byte — a byte Fusion32 stores, validates
and broadcasts, and which this client already round-trips correctly — has
nothing to tint. See `region_coverage_all_outfits.txt`.

**2. No region mask, in any outfit.** A region could otherwise be defined
*inside* a mesh by a mask texture, a second UV set, or vertex colours. All 16
assets carry `POSITION`, `NORMAL`, `TEXCOORD_0`, `JOINTS_0`, `WEIGHTS_0` and
nothing else: **no `COLOR_0`, no `TEXCOORD_1`, zero textures, zero images, no
glTF extensions.** So a region can only ever be a whole node, never an area
within one.

**3. Material slots cannot be regions either.** The five materials are named by
tonal value (`light`, `mid`, `dark`, `shadow`, `accent`) and are **shared
across body parts**: `128_dark` spans `leg.L`, `leg.R` *and* `pelvis`, and
`128_shadow` spans both arms. Tinting by material slot would tint by shading
tone, cutting across the body in a way that corresponds to nothing in 7.72.

**4. Head is one undifferentiated sphere.** In 7.72 the Head colour tints
**hair**; the face is skin and is never tinted. `head` here is a single
Icosphere covering the whole head, so applying the Head byte to it would
colour the face. There is no hair node and no mask to separate one.

**5. Body is ambiguous, and the asset does not record which way.** 7.72's Body
colour tints the torso garment and its sleeves. Here the candidates are
`tunic` (a Cube, plainly a garment), `torso` (an Icosphere, plainly skin) and
`arm.L`/`arm.R` (cylinders, bare arms on a rookie). Choosing among them is a
judgement the asset does not encode, and encoding it here would be the guess
this task forbids.

**6. Three of the four player-band outfits have no art at all.** Only 128 is
delivered. 129, 130, 131 and the entire 136-139 series resolve through
`UReal33DAssetRegistry::ResolveOutfit` to an empty visual, and
`AReal33DCreature::ApplyOutfit` then keeps a **single cylinder** — one
component, one material, one `Color` parameter. Both live runs of the previous
milestone show it: `creature 1002 outfit 136: delivered visual unavailable;
placeholder`. Tinting that cylinder is precisely the *"global whole-character
tint pretending to be correct"* the task rules out, and the QA account's own
band is the one with no art.

**7. The catalog is opt-in and was not loaded in the certified runs.**
`LoadBrotherCreatureCatalog` returns immediately unless
`-real33d-creature-catalog=` is on the command line, and
`visual/qa/brother_creatures/README.md` marks this delivery
`APPROVED_PILOT_REFERENCE`, which is **not** artistic approval.

### What would unblock it

Any one of these, in the delivered art and not in this client:

* a `feet` / `boots` node per outfit, plus a `hair` node separated from `head`
  and an explicit garment/skin split for the torso and arms; **or**
* a region mask — a `COLOR_0` channel, a second UV set, or a mask texture —
  whose four values are documented as Head/Body/Legs/Feet, which is how the
  classic client does it (a template sprite marking the four areas); **and**
* coverage for the rest of both player bands: 129, 130, 131, 136, 137, 138,
  139, since an outfit with no mesh cannot show a colour at all.

Until then the honest state is: the four bytes are certified end to end on the
wire and in the dialog, and nothing in 3D can express them.

## What was NOT done, deliberately

No rendering, no tinting, no material work, no asset edits, no change to the
certified dialog or transport. `Real33DBridge`, `Real33DOutfitPanel`,
`Real33DOutfitSelection`, `Real33DCreatureActor` and `Real33DAssetRegistry` are
**untouched**. Object-disguise rendering was out of scope and remains so.

Live QA was not attempted. Every one of its steps — four visibly distinct
colours, all four regions independently, a second client observing them —
presupposes rendering that does not exist; running it could only have produced
a screenshot of an untinted placeholder, and reporting that as a result would
be worse than reporting nothing.

## Delivered

* `tests/verify_outfit_palette.py` — derives and verifies the palette, 51
  assertions, failing closed if the image moves. Now a harness check.
* `tests/audit_outfit_asset_regions.py` — reproducible region audit of any
  delivered creature asset; exit 3 means insufficient.
* `evidence/clientcore/unreal-outfit-color-rendering-001/` — the palette
  derivation with all 133 colours, the outfit 128 audit, and the region
  coverage table for all 16 outfits.

## Tests

| Suite | Result |
| --- | --- |
| ClientCore CTest | 14 / 14 |
| `real33d_*` native | 6 / 6 |
| Unreal automation `REAL33D.Presentation` | 8 / 8, 0 errors, 0 warnings |
| REAL33DEditor Win64 Development | built |
| `verify_outfit_color_mapping.py` | 43 / 43 |
| `verify_outfit_palette.py` | 51 / 51 |
| `audit_outfit_asset_regions.py` | exit 3, INSUFFICIENT, as expected |
| regression harness | 11 PASS / 0 FAIL / 0 SKIP |
| `secret_check.sh` | PASS 6 / 6 |

No production source changed, so these confirm the tree is unbroken rather
than that anything new works.

## UNVERIFIED

* Everything the previous milestone listed stays as it was; nothing here
  changes the dialog or the transport.
* Whether a 7.72-faithful four-region split is achievable in this art style at
  all is an art question, not answered here.
* The palette is verified against the classic client's own computation. It has
  **not** been compared against pixels the classic client actually drew,
  because that needs the client executed and `AGENTS.md` forbids it. The
  corroborations above are strong but are not a screenshot diff.
* The 2D client's rendering of a colour onto a sprite — how the template's four
  marked areas combine with the palette colour — was not investigated, because
  nothing here renders.
