# CURRENT - 3D polish preserved, live defects pending

Date: 2026-09-28 America/Guatemala. Agent: Codex, implementation/QA.
Task: UNREAL-WORLD-PRESENTATION-POLISH-001.
State: IMPLEMENTED_UNVERIFIED. Operator requested immediate preservation commit.
Branch: milestone/unreal-world-presentation-polish-001.
Starting/base commit: fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Ending commit: this preservation commit (git log -1).
Worktree: C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001.

Main integration completed by fast-forward from a97cf25 to fa3adcf, pushed and
verified; main worktree build/integration-item-use clean. Prior certified minimap
reports remain intact. Prior handoff archived byte-for-byte at
archive/2026-09-28_UNREAL-MINIMAP-NAVIGATION-001-certified-before-main.md.
Frozen primary checkout and agent branch b9ed335 remain untouched. Never switch
that checkout or resume agent work. No polish merge to main.

Inspection: 15-category audit in docs/UNREAL_WORLD_PRESENTATION_POLISH.md.
Changes limited to camera/render visibility and read-only world feedback in
WorldActor, TileActor, StaticSectorActor, CreatureActor, HUD and new WorldOverlay,
PresentationPolicy/Tests. Added QA/automation scripts. Protocol, WorldState,
movement authority, map data and V08 source assets unchanged.

Tests: nine ClientCore native suites PASS; Unreal build PASS after correcting
TextRender getter compilation error; CameraAndFloors and AuthoritativeWindow
Unreal automation Success. No live presentation PASS. Final secret check and
publishing outcome are reported with the preservation commit.

Evidence: evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001.md and its
companion directory. Raw build/session artifacts under this worktree's
build/unreal-world-presentation-polish-001/. Current-binary session closed normally;
unsupported0/anomalies0/residual0, 22 accepted/3 refused steps. Actual last position
32096,32202,7. Say reached client but readable speech screenshot is unverified.

Exact failure: player label drawn displaced down/right from body. Operator said
NO SE VE BIEN, superseding earlier affirmative answer; also reports walls needing
rotation. Keep verdict IMPLEMENTED_UNVERIFIED. No weakening acceptance.
Likely overlay issue: mixing desktop cached viewport geometry with window paint
geometry in SReal33DWorldOverlay::OnPaint Project and cursor hit test. Verify using
matching GetTickSpaceGeometry conversions and screenshots/window movement/DPI.
Identify misoriented walls by normal known objects/TypeId and existing registry
rotation before deciding runtime correction versus V08 art defect. Do not rotate
assets wholesale. Lamp2108/2109 catalog has unresolved assembly warnings;
player/NPC outfit coverage remains partial.

Exact next task: fix/prove overlay projection first, identify the reported walls,
then continue requested camera/indoor/roof/wall/floor/underground/entity/speech/
interaction/Follow/Stop/minimap/WideWorld live matrix. Native computer-use runtime
failed with kernel assets OS error3; operator normal inputs + F9/engine evidence
are the available route. Latest session has only automatic initial/settled PNGs,
not manual captures; do not claim otherwise. Build with UE5.8 Build.bat for this
uproject; scripts/client/test_unreal_presentation.ps1 runs two automation tests;
scripts/client/run_unreal_presentation_qa.ps1 -Account B opens normal gameplay.
No other milestone or independent certification until live defects are resolved.
