# Pipeline

## States

```text
TODO -> MOCKUP -> REVIEW -> APPROVED -> MODELING -> TEXTURING
                         -> REJECTED     -> RIGGING -> ANIMATION
                                                    -> READY -> INTEGRATED
```

Not every asset walks every stage. A ground tile goes
`TODO -> MOCKUP -> REVIEW -> APPROVED -> MODELING -> TEXTURING -> READY ->
INTEGRATED` and never sees `RIGGING` or `ANIMATION`. A graphical effect may go
straight from `APPROVED` to `READY`. Skipped stages are left blank rather than
marked done.

| State | Meaning |
| --- | --- |
| `TODO` | in the inventory, no work started |
| `MOCKUP` | the artist is preparing a proposal |
| `REVIEW` | a mockup version is waiting on the project director |
| `APPROVED` | the director accepted a version; it is recorded in `Approved Version` |
| `REJECTED` | the director declined; the version stays in `visual/rejected/` |
| `MODELING` | the approved direction is being built |
| `TEXTURING` | materials and textures |
| `RIGGING` | skeleton and weights, skeletal assets only |
| `ANIMATION` | animation clips, skeletal assets only |
| `READY` | exported and validated against the technical standard |
| `INTEGRATED` | consumed by the client and seen in a running scene |

`INTEGRATED` cannot be reached until an Unreal client exists. No asset should
claim it before then.

## Tracker columns

```text
ID | Name | Category | Subcategory | Source | Priority | Status |
Mockup Version | Approved Version | Production Version |
Representation | Rig | Animation | VFX | Notes
```

Derived, overwritten on every sync: `ID`, `Name`, `Category`, `Subcategory`,
`Source`, `Priority`, `Representation`.

Human-owned, never overwritten: `Status`, `Mockup Version`,
`Approved Version`, `Production Version`, `Rig`, `Animation`, `VFX`, `Notes`.

`sync_tracker.py` merges on `ID`. New entities arrive as `TODO`. Entities that
vanish from a regenerated manifest are kept and annotated `ORPHANED` rather than
deleted, because they may carry approved work.

## ID namespaces

| Prefix | Entity | Key |
| --- | --- | --- |
| `obj:` | object type | `objects.srv` TypeID |
| `mon:` | monster race | `.mon` RaceNumber |
| `npc:` | npc | `.npc` Name |
| `outfit:` | outfit identity | outfit id referenced by `mon/` or `npc/` |
| `fx:` | graphical effect | `EffectType` value |
| `msl:` | projectile | missile attribute and value |

## Representation

| Value | Meaning |
| --- | --- |
| `MESH` | needs its own static mesh; this id represents its visual group |
| `SKELETAL_MESH` | needs a skeleton, for creatures, npcs and outfits |
| `VFX` | a particle or material effect, no mesh of its own |
| `ALIAS_OF:<id>` | the data demonstrates it is drawn as another type; produces nothing |
| `SHARED_CANDIDATE:<id>` | inferred to share an asset; the artist confirms or splits |

## Git LFS policy

Nothing binary is committed by the inventory task. The policy below is set up
in advance so the first asset does not land in Git as a raw blob.

**Tracked by LFS**, configured in `.gitattributes`:

```text
*.blend *.fbx *.glb *.gltf *.png *.jpg *.jpeg *.tga *.psd *.exr *.wav *.ogg
```

**Never committed at all**, configured in `.gitignore`: Blender autosaves and
backups (`*.blend1`, `*.blend2`), and any local render or bake cache.

Rationale. These files are large, opaque to diff and rewritten wholesale on
every save, which is exactly the shape Git handles worst and LFS handles well.
Text manifests, the tracker and the extractors stay in plain Git so they remain
diffable and reviewable.

Before the first binary asset is committed, run `git lfs install` once per
clone. The `.gitattributes` entries are inert until then, so adding them now
costs nothing and prevents a painful history rewrite later.

Mockup images are usually small, but they are the files most likely to arrive in
bulk and be revised repeatedly, so they follow the same rule as everything else
binary.
