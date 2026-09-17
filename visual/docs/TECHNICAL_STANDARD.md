# Technical Standard

Provisional. Nothing here has been validated against a running Unreal project,
because no Unreal project exists yet. Revisit every figure when one does.

## Scale

```text
1 SQM = 100 Unreal Units
```

One Fusion32 map field is one square metre of world at 100 uu, matching
Unreal's default centimetre convention. The 7.72 viewport is 18x14 fields, so a
full screen spans 1800 x 1400 uu at the player's floor.

Floor height is **UNRESOLVED**. The protocol addresses floors 0..15 and
`reference/game/src/sending.cc::SendFullScreen` shifts x and y by one field per
floor of depth, which fixes the horizontal offset per floor but not a vertical
distance. Choose one when the first multi-floor scene is built, record it here,
and state that it is a presentation choice rather than something the data
declares.

## Orientation and grid

- +X east, +Y south, +Z up. The first two follow the server: a step east is
  `x += 1` and a step south is `y += 1`, per
  `reference/game/src/receiving.cc::ReceiveData` dispatching `CGoDirection`.
- A field's origin is its north-west corner. World position of field
  `(x, y)` is `(x * 100, y * 100)`.
- Creature facing uses `enum Direction`: 0 north, 1 east, 2 south, 3 west.

## Pivots

- **Ground and border**: pivot at the field's north-west corner, on the floor
  plane. Geometry occupies the +X/+Y quadrant.
- **Props, furniture, vegetation**: pivot at the centre of the field footprint,
  on the floor plane, so a prop can be dropped on a field without offset maths.
- **Walls**: pivot at the field's north-west corner, so wall segments tile.
- **Creatures**: pivot between the feet, at the field centre.
- **Hangable objects** (`Hang`, `HookSouth`, `HookEast`): pivot at the wall
  contact point, on the hooking side named by the flag.

## Naming

```text
<category>_<subcategory>_<tracker-id>_<slug>
```

The tracker `ID` is the stable key, so a rename never breaks the link back to
the Fusion32 identity:

```text
terrain_ground_obj103_grass
structure_door_obj1629_wooden_door
creature_monster_mon12_valkyrie
effect_fx11_energy
```

Reused assets are named for their group representative, not for each member.

## Files

| Kind | Location | Format |
| --- | --- | --- |
| Authoring source | `visual/production/<id>/src/` | `.blend` |
| Engine export | `visual/production/<id>/export/` | `.glb` preferred, `.fbx` where a skeleton demands it |
| Textures | `visual/production/<id>/tex/` | `.png` authoring, compressed at import |
| Mockups | `visual/mockups/<id>/v<N>/` | images plus a short note |
| Approved | `visual/approved/<id>/v<N>/` | the accepted mockup, copied not moved |
| Rejected | `visual/rejected/<id>/v<N>/` | kept, never deleted |

## Materials

Prefer one material per visual group with parameters, over one material per id.
Recolours and variants should be material instances so that a group of ids
sharing a mesh also shares its material, differing only by parameter.

## Skeletal assets and animation

Creature and outfit assets are skeletal. Share one skeleton across humanoid
outfits so animations are reusable; creature races that are not humanoid get
their own.

Animation naming:

```text
anim_<skeleton>_<action>[_<direction>]
anim_humanoid_idle
anim_humanoid_walk_north
anim_valkyrie_attack
```

The action vocabulary is driven by what the protocol can actually express
today: idle, walk per direction, and turn. Attack and spell animations are not
yet driven by anything `clientcore/` decodes, so they are out of scope until
combat is.

## Git and binaries

See `visual/docs/PIPELINE.md` for the Git LFS policy. No binary asset is
committed by this task.
