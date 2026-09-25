# HANDOFF

Date/time: 2026-09-23, America/Guatemala
Task: `UNREAL-COMBAT-FOLLOW-001`
Agent: Codex
Role: implementation completion and live certification
Branch: `main`
Starting commit: `0cbcaf771091b18cd5c92afbe07333dfda918473`
Ending commit: the commit containing this handoff
Worktree: `C:\Users\dell\Desktop\fusion32`

## Objective and result

Make REAL33D attack and follow live through Fusion32's existing 7.72 protocol,
with ClientCore/WorldState as the single client-side state owner and Unreal as
input/presentation only. Result: `CERTIFIED_PASS`.

Attack, target replacement, cancel, follow, follow replacement/cancel, server
rejection, target removal, session reset, shared Battle/world feedback and all
supported tactics are wired and exercised. Final protocol counters are zero.

## Authority inspected

- `reference/game/src/connections.hh`: client 160/161/162/190, server 163.
- `reference/game/src/receiving.cc`: `CSetTactics`, `CAttack`, `CCancel`.
- `reference/game/src/crcombat.cc`: `TCombat::SetAttackDest`,
  `CanToDoAttack`, `StopAttack`.
- `reference/game/src/sending.cc`: `SendClearTarget`.
- Existing ClientCore movement/update and WorldState/WorldView paths.
- REAL33D bridge, Battle List, creature/tile actors, HUD, inventory stance
  controls and player-controller input.

## Changes

- Added byte-exact builders for attack, follow, cancel and set-tactics, plus
  deterministic tests.
- Added `CombatState` to WorldState, server-clear application, semantic
  `CombatChanged` diff/reset, and bridge requests that record state only after
  a successful send.
- Battle List attack and explicit Follow/Stop use that shared state. Shift-click
  was removed because no authoritative classic-client source established it.
- World right-click attacks creatures and uses world objects/corpses; camera
  orbit retains its drag threshold. Tile hit testing preserves the exact
  WorldState type and stack position.
- Creature actors and Battle rows present attack/follow feedback from the same
  state. Fight stance and stand/follow buttons send real opcode 160 fields.
- Evidence counters/snapshot cover requests and server clears. No damage, HP,
  cooldown, fake follow movement, protocol extension or independent Slate
  target was added.

Files are limited to ClientCore combat state/builders/tests, the relevant
REAL33D bridge/input/UI/actors, evidence and required project documentation.
Excluded systems were not touched.

## Tests and evidence

- Native Windows ClientCore: C++17 and C++20 builds PASS; all eight suites PASS.
- `REAL33DEditor Win64 Development`: `Result: Succeeded`.
- `git diff --check`: PASS.
- `tests/secret_check.sh`: required immediately before push.
- Live: attack/follow states, switching/cancellation, `Target lost`, target
  removal, right-click creature attack, real corpse open, tactics and unaffected
  movement/inventory observed. The operator explicitly accepted Battle/follow,
  fight stances and final right-click interaction.
- Final snapshot: 246 frames, 496 commands, residual 0, unsupported 0,
  anomalies 0; inventory known; open `dead rabbit` container.

Full source trace, repeat evidence and exact log excerpts:
`evidence/clientcore/UNREAL-COMBAT-FOLLOW-001.md`.

## Remaining unverified / risks

- This is one operator on one machine; no independent live repetition.
- Fusion32 intentionally sends no positive target acknowledgement. WorldState
  therefore records the command after it reaches the wire and waits for server
  clear/rejection, matching the authoritative implementation.
- Corpse type/container behavior is correct, but the 3D body remains the
  existing generic placeholder because no approved corpse asset exists. V08
  was explicitly out of scope and untouched.

## Exact next step

Select a new bounded milestone from authoritative Fusion32 source. The current
candidate list is trade, a live floor transition, REAL33D-2D-BOOTSTRAP-001, or
the already documented container mini-window presentation request. Do not infer
authorization to change V08, WideWorld, REAL33D2D, shops, action bars, automap,
reconnect handling, opcode 50 or protocol extensions.
