# UNREAL-WORLD-PRESENTATION-POLISH-001

State: IMPLEMENTED_UNVERIFIED. Audit before implementation, 2026-09-28 America/Guatemala.

Base: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf. Main was integrated by
fast-forward from a97cf25e7449a8a3ef35ef2553181c5032c7a247, pushed and
verified against the remote. Main secret_check passed. Certified minimap
evidence and handoff are preserved. Work proceeds on
milestone/unreal-world-presentation-polish-001 in the isolated worktree
build/unreal-world-presentation-polish-001. The frozen agent checkout and
branch are not implementation targets.

## Before-change audit

Paths below are under unreal/REAL33D/Source/REAL33D unless otherwise stated.
This is source inspection, not a substitute for live acceptance.

| Area | Observed implementation | Classification / intended response |
|---|---|---|
| Camera implementation | WorldActor owns a camera directly attached to its root, without a collision arm. | BUG: prevent the view from entering rendered geometry using presentation-only bounds. |
| Angle, distance, zoom, follow | Yaw 45, pitch -42, distance 1173.5 uu; pitch -85 to -5, distance 400 to 3000. Immediate zoom; VInterpTo speed 8 follows the drawn body, including through floor changes. | POLISH: comfortable pitch, smooth zoom, body focus and immediate refocus on authoritative floor transitions. |
| Floor rules | UpdateFloorVisibility hides upper live tiles/creatures only when a covering tile exists directly above. | BUG: use consistent upper-floor visibility underground and under cover. Do not change logical tile membership. |
| Roof/wall occlusion | No camera-to-player wall cutaway. Static context does not share live roof hiding. | BUG for inconsistent roof visibility; POLISH for near-wall cutaway. |
| Sorting/overlap | Tile actors preserve server stack order. Static HISM placement adds 8 uu per nonground stack, whereas live blocking geometry is not raised. | Suspected BUG requiring visual confirmation; preserve stack identity. No speculative map edits. |
| Names | CreatureActor uses yaw-only, world-size 18 UTextRender names colored from received health. | POLISH: screen-readable, outlined labels using only known creature state. |
| World speech | PresentSpeech resolves semantic speaker IDs; CreatureActor shows timed text. Unresolved speakers remain in chat. | POLISH: readable wrapped presentation; preserve transcript and speaker resolution. |
| Target/follow | Actual combat state drives small ATTACK/FOLLOW world tags and Battle List feedback. | POLISH: improve world feedback without creating local combat state. |
| Hover/selection | No continuous world hover feedback; inspector is separate and disabled for normal QA. | POLISH: highlight only normal live interaction hits. |
| Tile/object interactions | Existing controller/HUD route Look, Use and Use With through the bridge. Pending Use With already has a cursor/banner. | POLISH: show actual hover/targeting/request state, never fabricated success. |
| Lighting/postprocess | GameMode directional intensity 3.4, skylight 1.6; fixed exposure, bloom/motion blur/AO off; World camera saturation 1.15 and tone curve 0. | POLISH: retain baseline unless comparative evidence identifies a rendering defect. |
| V08 presentation | Registry contains reviewed aliases/rotations/scales; full catalog has 4913 mappings. Individual mesh gaps, texture quality and proportions need scene-specific evidence. | ART ASSET DEFECT when intrinsic to source mesh; document IDs and captures instead of redesigning V08. |
| Creature/NPC coverage | Catalog supplies 16 exact outfits. Prior certified session loaded deer 31; player B 136, Cipfried 57 and rabbit 74 used placeholders. | ART ASSET DEFECT / missing coverage. Runtime readability can pass separately; no invented replacement identity. |
| WideWorld | StaticSector ApplyView suppresses the authoritative 18x14 window and radius, but not covered upper floors. DesiredSectors includes floors 0..7. | BUG: roof visibility mismatch. OUT OF SCOPE: new underground static data or streamer architecture. Static context remains non-authoritative. |
| Minimap integration | Independently certified at this base. Map panning intentionally retains its center. No contradictory presentation regression found in inspection. | Preserve certified behavior; new changes require live coexistence smoke. |

Key symbols: WorldActor UpdateCamera/ApplyCameraTransform/UpdateFloorVisibility/
PresentSpeech/WriteEvidence; CreatureActor Tick/CommitPosition/SetCombatFeedback;
TileActor ApplyStack/SetFloorVisible; StaticSectorActor Build/ApplyView;
PlayerController Tick/InteractUnderCursor; HUD MakeViewportFrame/Refresh;
AssetRegistry ResolveThing/ResolveGround; GameMode lighting setup.

## Implementation and verification boundary

Only rendering, camera and UI presentation may change. Fusion32 owns positions,
floor transitions, combat, interactions and contents. Bounds used for camera
clearance are visual geometry, not walkability. Neither static art nor minimap
data may grant interaction or movement authority. No protocol, server state,
V08 production source, REAL33D2D or agent work changes are authorized here.

Planned evidence: camera/floor geometry tests; existing ClientCore regression
suites; Unreal build; secret_check; fresh normal Fusion32 gameplay with before/
after F9 screenshots and semantic journals under
build/unreal-world-presentation-polish-001/. Prior exact-base screenshots in
evidence/clientcore/unreal-minimap-navigation-independent-delta-20260928 provide
additional provenance, not new polish PASS claims. Native computer-use is
currently unavailable (kernel assets, OS error 3); normal operator inputs can
be independently corroborated by engine screenshots and received state.

All polish acceptance items remain UNVERIFIED until their named checks execute.

## Preservation checkpoint

Operator requested an immediate commit. Build/native/automation checks PASS, but
new live overlay alignment FAILED and wall rotations require identification.
See evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001.md. Do not claim PASS
or start another milestone before fixing and validating these presentation gaps.
