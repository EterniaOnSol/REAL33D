# UNREAL-SKILLS-PANEL-001 independent certification

Status: CERTIFIED. Reviewer: Codex, 2026-10-17 America/Guatemala.

## Provenance and scope

Base main: `370b5676e2a186d44e11aeffe1fb4766593f8d9e`.
Candidate: `63a62ddf9477cb8ccdc91525d7f505fee192f2f4`, direct child of base.
Isolated branch: `certification/unreal-skills-panel-001-20261017`.
Checkout: `C:/Users/dell/Desktop/fusion32/build/cert-skills-panel-20261017`.
Fetched origin/main matched base. Root's four pre-existing dirty unreal-slice
files remain untouched. No main merge or implementation-branch mutation.

Verify all seven skills; authoritative values only; unsent dashes versus sent
zero; correct received percent; legitimate live progress; registry and HUD
compatibility; ClientCore13, native4, Unreal tests/build, corrected harness,
Git Bash secret6 and zero protocol counters. Author evidence is context only,
not independent live proof. No production defect established or fix made.

## Source review

The candidate adds tests and extracts RowText/BarFill from existing SetValues;
the panel was already on main. Only its header/implementation and presentation
tests change in production Source. No ClientCore, network, server, HUD registry,
other panel, art or gameplay change exists in the candidate.

Authority trace, selected Fusion32 game386fa9b, protocol772:
- `reference/game/src/sending.cc::SendPlayerData` sends stats and percent fields;
  `SendPlayerSkills` sends seven level/percent pairs in fist, club, sword, axe,
  distance, shielding, fishing order. Neither routine has a local version guard;
  runtime selection is the existing TIBIA772 service documented in SOURCE_TRUTH.
- `clientcore/src/player_state.cpp::DecodePlayerData/DecodePlayerSkills` set
  known only after the complete fields decode; ApplyServerUpdate stores them.
- `Real33DBridge.cpp` publishes field-by-field copies from authoritative state;
  `Real33DWorldActor.cpp` retains those copies and clears them at session reset.
- `SReal33DHUD::Refresh` passes world vitals/skills to SetValues every frame.
  SetValues uses separate known flags, fourteen actual text rows and nine bars;
  it has no local progression calculation. The early return compares both known
  flags and all displayed fields, including skill percentages.

## Independent diagnostic

Temporary `Real33DSkillsCertification.cpp` exercises the actual Slate panel:
seven labels/order, fourteen name/value pairs, initial unknown text, nonzero
unsent values, independent known flags, received zero and >32-bit experience,
nine actual painted bar widths, percentage-only update, known-to-unknown reset,
and all six actual HUD registry windows' close/open/toggle behavior.

Paint checks allocate100px and inspect SProgressBar::OnPaint clipping width;
they do not call the production BarFill helper to calculate observed output.
The separate opt-in `-real33d-cert-skills-observer` records real WorldActor
event copies, actual displayed text and actual painted bar widths after frames.
It never injects values or sends gameplay commands. Fixture values are only for
the disconnected automation test, never for live gameplay. The diagnostic will
be removed from production Source and preserved as evidence before closeout.

## Initial execution checkpoint (superseded by closeout below)

- Fresh Windows MSVC C++17 suites/C++20 archive: PASS.
- Corrected harness selftest: PASS10/10.
- Fresh GCC ClientCore: PASS13/13; native tests: PASS4/4. First native command lacked the public header include path; setup failure retained, corrected command passed.
- Fresh Unreal build with independent diagnostic: PASS (287.42s). Six Unreal tests Success, errors0, failed/notRun/inProcess0; expected V08 catalog warning1. Actual text and painted-bar diagnostic passed.
- UI/live progress, exact candidate final build, harness and final secret check:
  IMPLEMENTED_UNVERIFIED.

Shared experimental art is mounted read-only in purpose through directory
junctions; no art is edited. UI LFS resources were materialized from Git.
Existing Fusion32 game service was found closed after normal save/shutdown;
its prior log was preserved, then only that service restarted. No server rules,
configuration or character data were edited. Login/querymanager retained.

## Independent closeout review

The existing isolated certification worktree and substantive checkpoint were
resumed. The original root checkout and other worktrees were not used for edits.
The reviewer inspected the retained independent live session, rather than
reclassifying the candidate author's earlier session as independent proof.
No production defect was found; implementation remains byte-identical to the
candidate. The diagnostic source hash matches its retained evidence copy and
it has been removed from production Source.

Six Unreal automation tests were repeated at23:53:30Z, all Success/errors0,
failed/notRun/inProcess0; one expected V08 catalog notice. The actual widget
test verifies seven labels, fourteen values, nine painted bars, separate known
flags, zero, percent-only refresh, reset, six real registry panels and chat.

Live evidence: session05:19, before F9 at11:20:13Z, combat F9 at12:07:16Z and
normal EndPlay12:11:39Z. All seven skills remain visible. Received Magic0
renders0; unknown rows render--. Sword11 advances67->83% during three normal
world right-click attacks; actual painted widths follow all17 received skill
vectors. Experience471->486. Skill-level advancement is not claimed.
Inventory, containers, stack counts, minimap, health/mana, Battle List and
chat remain visible and respond to ordinary gameplay. Live button-toggle
coverage is not claimed; actual panel API toggles passed automation.

Replay `python evidence/clientcore/unreal-skills-panel-independent-20261017/verify_live.py`:
50 observer records,46 matching full records and4 expected pre-refresh event
transitions; every received skills vector has a matching rendered observation.
Eight protocol snapshots: residual0/unsupported0/anomalies0. Final session:
3128.8seconds,3073frames,4295commands. Normal application exit is retained.

Closeout setup failures are not hidden: PowerShell initially blocked its test
script under the default execution policy; the process-scoped authorized rerun
passed. A selftest run without the MSVC environment produced6PASS/4FAIL; the
same unmodified selftest under vcvars64 passed10/10. The replay initially
selected geometry JSON as protocol snapshots; its final file filter explicitly
excludes those sidecars. These are review setup issues, not implementation fixes.

Artifacts: `unreal-skills-panel-independent-20261017/` contains source probe,
two automation JSON/log sets, live screenshots/log/snapshots, replay script and
summary, original build/test logs and final gate evidence. Full geometry sidecars
remain in the ignored original live directory; they are not needed for skills
certification. Known world-art limitations and wider gameplay parity are outside
this bounded UI certification. No server/source/data, protocol or ClientCore
implementation changes; no main merge, remote mutation or push.

The live log contains one handled engine FAppTime/IsInGameThread ensure during
initial rendering (Core/Renderer/Engine/RenderCore stack,11:19:34Z), not a
protocol error. The same ensure predates this candidate in main's retained
`evidence/clientcore/unreal-world-presentation-polish-resume-20261014/roof-session-events.txt`
at22:14:11Z on2026-10-14. This pre-existing engine issue is not fixed or claimed
absent. The session continued normally; UI and protocol gates above passed.
Raw logs preserve their original trailing whitespace; authored markdown/source
passes the scoped diff whitespace check.

## Final gates

| Gate | Independently verified result |
| --- | --- |
| Seven visible skills | PASS; screenshots and actual fourteen-row widget traversal |
| Authoritative values/received percentages | PASS; source trace, live event/widget comparison, nine painted bars |
| Unsent values / real zero | PASS; independent known flags, initial/reset dashes; received Magic0 |
| Legitimate live update | PASS; Sword67->83%, three normal attacks, XP471->486 |
| PanelRegistry / HUD | PASS; all six real panels close/open/toggle; retained live HUD controls/content |
| ClientCore | PASS13/13 in fresh corrected harness run |
| Native | PASS4/4 in fresh corrected harness run |
| MSVC | PASS C++17 suites and C++20 linked archive |
| Unreal automation | PASS6/6 with probe; PASS5/5 exact candidate, no diagnostic test remaining |
| Exact-candidate Unreal build | PASS; real link actions, Result:Succeeded,9.06s |
| Corrected harness | PASS9/0FAIL/0SKIP; selftest10/10 under developer environment |
| Secret check | PASS6/6; direct staged-tree check retained as secret-final.txt |
| Protocol | PASS; eight snapshots0/0/0; harness scans42files without errors |

Exact-source automation JSON is `automation-exact-index.json`; no failed,
unfinished or skipped tests. Source tree, ClientCore, tests and scripts equal
candidate63a62dd; final module hash is in final-build-sha256.txt. Certification
commit contains only documentation/evidence and resolves through branch
`certification/unreal-skills-panel-001-20261017`. Main remains370b567 and the
implementation branch remains63a62dd. Ready for a separately authorized merge;
none was performed. WORKTREE_CLEAN refers only to this isolated checkout.
