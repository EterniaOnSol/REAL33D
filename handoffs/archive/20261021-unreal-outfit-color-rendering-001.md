# HANDOFF - UNREAL-OUTFIT-COLOR-RENDERING-001

Date/time: 2026-10-21, America/Guatemala
Agent / role: Claude; audit only
Starting commit: `25c4448b7bd36a5a75ae4e2efd679f0ea500e9d1` (origin/main)
Ending commit: the commit carrying this handoff on
`milestone/unreal-outfit-color-rendering-001`
Worktree: `C:/Users/dell/Desktop/fusion32`
Prior handoff: `handoffs/archive/20261020-outfit-dialog-independent-certification.md`

COLOR_RENDERING_001_STATUS = **BLOCKED_VISUAL_ASSET**
MAIN_MERGE = NONE. PUSH = NONE.
PRODUCTION SOURCE CHANGED = **NONE**.

## Objective and outcome

Make the certified outfit colour bytes visibly affect the 3D outfits, after an
audit of the meshes/materials, of whether the four regions can be tinted
independently, and of the exact colour-index to RGB mapping.

The audit's third item **succeeded completely** and is delivered. Its second
item **failed**, and the task's own rule then applied: *"If assets cannot
represent the 4 regions faithfully: STOP implementation and return
BLOCKED_VISUAL_ASSET with exact missing capability."* So no rendering was
written and no production file was touched.

## The palette is derived and verified

Fusion32 has no palette: it validates `Colors[n] > 132` and stores the byte.
The authority is the classic 7.72 client, read statically — opened, never
executed.

**It holds no colour table. It computes every swatch.**
`tests/verify_outfit_palette.py` reproduces that computation and asserts the
structure producing it, 51 checks against the hashed image:

```
0x0047D380  133 swatches, i = 0..132, control id 2000 + i,
            each colour from a call to 0x00509280
0x00509280  row = index / 19 ;  col = index % 19
            col == 0  ->  h = 0, s = 0, v = 1 - row/7
            else      ->  h = col/18, (s,v) = per-row pair
0x00509080  HSV -> RGB, six 1/6 sectors, low channel (1-s)*v,
            round(x * 255) per channel
```

Per-row `(s, v)`, each read from its own case at `0x0050930F..7B`:
`(0.25,1.0) (0.25,0.75) (0.5,0.75) (0.667,0.75) (1.0,1.0) (1.0,0.75) (1.0,0.5)`.

**All six sectors were decoded instruction by instruction, not inferred from
the constants.** They are textbook HSV, but that was the conclusion: the
sectors differ in which value stays on the FPU stack, and two of them enter the
shared tail one instruction later to skip an `fstp`.

`19 * 7 = 133` agrees independently with `receiving.cc`'s `0..132`. And
`CheckOutfit`'s defaults decode to `78 -> #FFAA00` orange hair,
`69 -> #4040BF` blue shirt, `58 -> #BF6A40` brown trousers,
`76 -> #6D6D6D` grey boots — a sensible rookie, which a wrong palette would not
produce. Index 0 is pure white, column 0 descends as greyscale, and row 4
contains all six primaries and secondaries exactly.

## Why rendering stopped — the exact missing capability

`tests/audit_outfit_asset_regions.py` audits any delivered creature `.glb`.
Across **all 16** in the brother delivery:

1. **No feet geometry anywhere. 0 of 16.** No node, mesh or material for feet,
   boots or ankles. The `Feet` byte — stored, validated and broadcast by
   Fusion32, and already round-tripping correctly in this client — has nothing
   to tint. This alone is decisive.
2. **No region mask anywhere. 0 of 16.** Every asset carries `POSITION`,
   `NORMAL`, `TEXCOORD_0`, `JOINTS_0`, `WEIGHTS_0` and nothing else: no
   `COLOR_0`, no `TEXCOORD_1`, zero textures, zero images, no extensions. So a
   region can only be a whole node, never an area inside one — which is how the
   classic client does it, with a template sprite marking the four areas.
3. **Material slots are not regions.** Outfit 128's five materials are named by
   tonal value and are shared across body parts: `128_dark` spans `leg.L`,
   `leg.R` *and* `pelvis`; `128_shadow` spans both arms.
4. **`head` is one sphere including the face**, while 7.72's Head byte tints
   hair only. No hair node, no mask to separate one.
5. **Body is ambiguous** between `tunic` (garment) and `torso`/`arm.L`/`arm.R`
   (skin), and the asset does not record which way. Deciding would be a guess.
6. **Only outfit 128 of either player band has art.** 129, 130, 131 and the
   whole 136-139 series resolve to an empty visual and keep a **single
   cylinder** — one component, one `Color` parameter — which is precisely the
   "global whole-character tint pretending to be correct" the task forbids. The
   QA account's own band is 136-139, the one with no art.
7. The catalog is opt-in (`-real33d-creature-catalog=`) and was not loaded in
   the certified runs; `visual/qa/brother_creatures` marks the delivery
   `APPROVED_PILOT_REFERENCE`, which is not artistic approval.

### To unblock

In the delivered art, not in this client: a `feet`/`boots` node per outfit,
`hair` split from `head`, and an explicit garment/skin split — **or** a
documented four-value region mask (`COLOR_0`, a second UV set, or a mask
texture). Either way, plus mesh coverage for 129, 130, 131, 136, 137, 138, 139,
since an outfit with no mesh cannot show a colour at all.

## Live QA

**Not attempted, and not claimed.** Every step — four visibly distinct colours,
all four regions independently, a second client observing them — presupposes
rendering that does not exist. Running it could only have photographed an
untinted placeholder, and reporting that as a result would be worse than
reporting nothing.

## Changes

New: `tests/verify_outfit_palette.py`, `tests/audit_outfit_asset_regions.py`,
`evidence/clientcore/UNREAL-OUTFIT-COLOR-RENDERING-001.md` and its
`unreal-outfit-color-rendering-001/` directory (palette derivation with all 133
colours, the outfit 128 audit, the region coverage table for all 16).
Changed: `tests/regression_harness.sh` (an `outfit-palette` check, so the
harness runs 11), `tests/README.md`, `PROJECT_STATUS.md`, `PARITY_MATRIX.md`.

**No file under `unreal/` or `clientcore/` was modified.**

## Tests

ClientCore 14/14, native 6/6, Unreal automation 8/8 with zero errors and
warnings, MSVC archive, REAL33DEditor build, colour-mapping 43/43, palette
51/51, asset audit exit 3 (INSUFFICIENT, which is the expected result), harness
11 PASS / 0 FAIL / 0 SKIP, secret_check 6/6.

These confirm the tree is unbroken. They do not show that anything new renders,
because nothing new renders.

## Remaining UNVERIFIED

- The palette is verified against the client's **own computation**, not against
  pixels it actually drew: that needs the binary executed and `AGENTS.md`
  forbids it. The corroborations above are strong but are not a screenshot
  diff.
- How the 2D client combines a palette colour with its template's four marked
  areas was not investigated, because nothing here renders.
- Whether a 7.72-faithful four-region split is achievable in this art style at
  all is an art question and is not answered here.
- Everything the dialog milestone listed stays as it was; nothing here touches
  the dialog or the transport.

## Exact next task

Not an engineering task — an **art** one. `VISUAL-OUTFIT-REGIONS-001`: commission
or author, in the brother delivery, either the four-region node split (with
feet and hair) or a documented region mask, for outfits 128-131 and 136-139.
`tests/audit_outfit_asset_regions.py` is the acceptance gate: it must exit 0.

Only once it does should `UNREAL-OUTFIT-COLOR-RENDERING-002` apply the palette,
and then: per-component dynamic material instances (never a shared material —
`AReal33DCreature::ApplyOutfit` builds one component per mesh, so isolation is
natural there), the colour applied in `UReal33DAssetRegistry` or the actor and
never in Protocol772Core, updated on the authoritative SV_CMD_CREATURE_OUTFIT
that `FReal33DEvent::Outfit` already carries whole. Read
`evidence/clientcore/UNREAL-OUTFIT-COLOR-RENDERING-001.md` first; the palette
is already derived and need not be re-derived, only ported.

## Environment

Unchanged. Fusion32 was left running on 7171/7172/7173 from the previous
milestone and was not needed: this milestone ran no live session. The stale
`game.pid` note in the prior handoff still applies.
