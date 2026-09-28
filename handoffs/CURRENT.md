# CURRENT - independent 3D item interaction certification

Date/time: 2026-09-27 23:49 America/Guatemala / 2026-09-28 05:49 UTC.
Agent: Codex primary; certification reviewer and operator-directed repair.
Task: UNREAL-ITEM-USE-INTERACTION-001. State: CERTIFIED, corrected revision.
Branch: milestone/unreal-item-use-interaction-001.
Starting commit: 35284595c44957600615f9cce6b78b9e9b761b86.
Ending commit: SELF, the certification commit containing this handoff; resolve
with git rev-parse HEAD. Original candidate is not retroactively certified.
Worktree: build/unreal-item-use-interaction-001, isolated from main checkout.
Expected origin/main: bd15cc0a49d8182dc1cc3732b8487859f1662044. No merge.
Prior substantive handoff archived in archive/2026-09-28_UNREAL-ITEM-USE-INTERACTION-001-independent-pre-closeout.md.

## Objective, inspection and authority

Independently reproduce the published interactions through the normal client,
772 protocol and authoritative Fusion32 runtime. Startup docs/Git inspected;
candidate initially clean/local==remote, main unchanged. Inspected ClientCore
movement/player_state/worldview, receiving.cc CUseObject/CUseTwoObjects/CLookAtPoint,
operate.cc Use, moveuse.cc UseWeapon, Unreal controller/HUD/slots/bridge/world,
existing V08/icons and pinned original creature delivery. Reference data read
only. No main checkout, REAL33D2D, agent/gateway/LLM, Fusion32 rules/saves,
protocol definitions or V08 source asset edits.

## Discoveries and scoped corrections

Fresh live clicks exposed slot-release world actions, wrong requested mouse
bindings, and ChatPanel construction missing Bridge/OnTypingChanged. Operator
explicitly requested corrections: left click walks through current authoritative
tiles, right Use, Shift-left or left+right Look; owned mouse releases isolate
slots/world. Walking uses normal existing commands and server-confirmed results.
HUD wiring restores visible descriptions and normal chat, with typing gating.

Operator requested original Use With targetcursor (copied unchanged/hotspot 9,9)
and brother's models. Imported 16 exact pinned GLBs separately; server outfit
IDs drive all parts/materials/idle/walk. Outfit 128 shown for player/NPCs, 21
shown for rats; 136 Dixi unmatched. Art has defects accepted for now; no full
art certification. 156 outfit sprite references are not 156 finished models.
Source/runtime manifests and reproducible importer live under visual/qa/brother_creatures
and visual/tools/import_brother_creatures_unreal.py. Original assets untouched.

Final operator correction: normal Use starts target selection for MultiUse
items, including runes. Unreal-only UI traits read exact flags from the same
validated objects.srv. Full map/body/container pending endpoint preserved;
existing builders send only after target selection. No hardcoded rune/weapon
list, new protocol or invented effects. Non-MultiUse containers keep normal Use.

## Executed tests and live PASS

ClientCore initial clean candidate: all 8 suites PASS, C++17/C++20 archives built.
Final relevant movement/player_state/worldview suites PASS, retained evidence.
Native mouse ownership/gesture and item-use-policy tests PASS. Public runtime
metadata: 204 MultiUse types, rapier/spell runes yes; backpack/bag/blank rune no.
Final Unreal incremental build PASS, 4 actions, 15.81 s. Initial missing log
header build failed, fixed after it ended and superseded by successful rebuild.
Secret check with explicit WSL Git worktree PASS, all 6 checks.

Fresh operator clicks independently matched to authoritative messages/state:
- World Look: sewer grate, pavement, street lamp, visible descriptions.
- World Use: floor 7->8->7 through legitimate objects 435/1948.
- Inventory Use: backpack opens server container 0.
- Container Use: nested bag opens server container 1/parent.
- Use With: rapier -> drawers 2434/3136; final normal Use -> trough 2524/3135.
- Slot Look: backpack/jacket and bag descriptions visibly confirmed.
- Normal Say echo from Fusion32; original target cursor visibly confirmed.

Final normal-use session PID 8288 closed normally, 05:43:36 UTC; 110 frames,
302 commands, one Use/Look, residual/unsupported/anomalies all zero, viewport
synchronized, diagnostic none. Prior independent exports likewise zero protocol
errors. Earlier 6 refused/7 unanswered walks remain recorded, not promoted to
PASS. No local success state or fabricated inventory. Native UI helper was
unavailable: actions were manual operator clicks, not automated agent clicks.

## Evidence, files and publication

Report: evidence/clientcore/UNREAL-ITEM-USE-INTERACTION-001-INDEPENDENT-20260927.md.
Compact exports/actions/tests: evidence/clientcore/unreal-item-use-independent-20260927/.
Raw logs/builds: ignored build/cert-item-use-20260927/. Asset manifest README
explains source pin, source hashes, import checks, missing coverage and replay.
Changes limited to Unreal controller/HUD/slots/world/bridge/creature registry,
portable UI policy headers/tests, original cursor, art importer/manifests,
status/parity/evidence/handoff and separate imported-content ignore rule.

Certification succeeded. User authorized commit and push to this milestone.
Closeout must verify local HEAD==remote milestone HEAD, clean isolated worktree,
origin/main unchanged, principal branch/work preserved and no merge. No force.

## Remaining unverified, risks and exact next step

Actual rune spell effects and automatic world-MultiUse source entry not live
certified; no rune available was manufactured. Imported but unencountered outfits,
unknown NPC outfits, art polish and attack/death animation triggers not certified.
No new gameplay or inferred identity. Native UI helper remains unavailable.

First finish publication verification. Then recommended next 3D milestone:
UNREAL-LIVE-VISUAL-EDITOR-001, local visual mesh/rotation/grass overrides only,
NOT_STARTED; consult Real33DAssetRegistry/WorldActor and prior V08 inspector.
Do not start minimap, REAL33D2D or agent work under this handoff. No new feature
milestone has started; preserve the original authoritative runtime and art.
