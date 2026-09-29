# UNREAL-WORLD-PRESENTATION-POLISH-001 preservation checkpoint

State: IMPLEMENTED_UNVERIFIED. Operator requested a commit before continuing.
This is not PASS LOCAL or certification. Independent certification is not requested yet.

Main integration: PASS. Existing clean main worktree build/integration-item-use
was fast-forwarded from a97cf25e7449a8a3ef35ef2553181c5032c7a247 to certified
fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf, normally pushed and verified clean.
Main secret_check passed. All minimap certification evidence remains present.

Polish branch: milestone/unreal-world-presentation-polish-001.
Base: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Isolated worktree: build/unreal-world-presentation-polish-001.
Frozen agent checkout/branch unchanged. No polish merge to main.

Implemented: presentation-only camera clearance/cutaway from rendered bounds,
smoothed zoom, body focus and floor-change refocus; shared live/static upper-floor
hiding; projected live labels/health/Attack/Follow/speech; normal-hit hover outlines;
Look/Use/Use With request notices; diagnostics opt-in; F9 camera/WideWorld fields;
reproducible normal-session and automation launchers. No ClientCore/protocol/server,
map, gameplay, REAL33D2D or V08 source asset edits.

Executed: ClientCore nine native suites PASS; final Unreal build PASS (26.19s).
Initial build failed on an unavailable UTextRenderComponent getter; fixed to read
its public Text member, then rebuilt successfully. Unreal automation
REAL33D.Presentation.CameraAndFloors and REAL33D.WideWorld.AuthoritativeWindow
both Success, 0 failed, 0 warnings. These tests do not prove live visual acceptance.

Live reference: unchanged certified binary, 20260928T232809. Operator moved normally;
automatic initial/settled captures retained. No manual F9 file was retained.
New binary: 20260928T234644, normal Account B session, closed normally at
2026-09-29T05:47:48.344Z; last authoritative position 32096,32202,7.
22 accepted and 3 rejected steps; zoom/orbit changed to pitch -36.80, yaw229.96,
distance2253.50. One Say requested and one speech shown on creature. WideWorld
76 loads/20 unloads; minimap state agrees with server position. Unsupported opcodes0,
protocol anomalies0, residual bytes0. No full gameplay regression PASS is claimed.

## Exact live failure / remaining work

Operator corrected the initial positive response with NO SE VE BIEN, then reported
misoriented walls. Preserve that correction as the actual verdict.
The after screenshot independently shows the player name around x960/y550 while
the body is around x640/y360: projected overlay alignment is FAILED in this run.
Likely cause to verify: OnPaint Geometry uses window paint space while viewport
GetCachedGeometry uses desktop space. Convert between matching tick-space
geometries (including window position/DPI) and validate screenshots; do not mark
this fixed based only on inspection. Hover/speech/target outlines share that path.

Wall orientation: operator reports some walls require rotation. Exact TypeIds and
runtime-versus-art attribution remain UNVERIFIED. Identify visible normal-client
objects and catalog transforms; do not blindly rotate all walls or redesign V08.
Catalog street lamps2108/2109 carry ASSEMBLY_UNRESOLVED/degenerate UV warnings;
2109 is provisional sprite relief. Placeholder coverage remains for player136 and
several NPC/creature outfits. Runtime success and art completeness remain separate.

Camera/floor/roof/collision live acceptance, fixed label alignment, target/follow,
interaction feedback, world/NPC speech readability, indoor/underground transitions,
WideWorld/minimap coexistence and full gameplay regression remain UNVERIFIED.
The minimal observed movements and zero protocol errors do not close those items.

Raw reproducible evidence stays under build/unreal-world-presentation-polish-001/;
compact selected artifacts are in unreal-world-presentation-polish-001/ beside this
report. Before/after positions and zoom differ: these are defect evidence, not a
pixel-identical visual acceptance comparison. No bulk assets or secrets included.

Final staged secret_check: PASS. Publishing this preservation checkpoint does not
change the IMPLEMENTED_UNVERIFIED verdict.
