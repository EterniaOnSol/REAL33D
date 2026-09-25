# HANDOFF

Date/time: 2026-09-25 01:50 -06:00, America/Guatemala
Task: `REAL33D-AGENT-BRIDGE-001`
Agent / role: Claude, implementation and live QA
Branch: `milestone/real33d-agent-bridge-001` (not merged to `main`)
Starting commit: `bd15cc0a49d8182dc1cc3732b8487859f1662044`
Ending commit: the commit containing this handoff
Worktrees: `C:\Users\dell\Desktop\fusion32` and separate
`C:\Users\dell\Desktop\REAL33D2D` (at
`aeef6aa939a764de3728c51a3022953dff97f375`)

## Objective and result

`REAL33D-AGENT-BRIDGE-001 = PASS`. The opt-in 2D agent gateway now has explicit
`AgentObservation` and `AgentIntent` schemas, strict schema-driven validation
before dispatch, correlation identifiers on every cycle and a JSONL
observation/intent/validation/dispatch/result trace. A deterministic mock brain
completed the bounded scenario live against authoritative Fusion32 under
ordinary player rights. No ClientCore, WorldState, Fusion32 gameplay, protocol
extension or Unreal work was involved.

## Startup and prior work

The predecessor `REAL33D-AGENT-MVP-001` existed only as uncommitted working-tree
files. It was audited first and reused, not restarted: `agent/real33d2d/`,
`tests/real33d_agent_test.lua` and `evidence/agent/` were treated as the
baseline. The milestone branch was cut from `bd15cc0` carrying that dirty state,
and the MVP handoff was archived as
`handoffs/archive/2026-09-24_REAL33D-AGENT-MVP-001.md`.

## Changes and files

- `agent/real33d2d/modules/real33d_agent/agent_schema.lua` (new): both contracts,
  strict checks, deterministic sorted-key JSON, published projection grouped
  into `self`/`visible`/`owned`/`social`.
- `agent/real33d2d/modules/real33d_agent/agent_bridge.lua` (new): session,
  correlation, observation and action ids; JSONL events; result correlation
  against a pre-dispatch snapshot.
- `agent_runtime.lua`: `R33D_AGENT_MODE=1` canonical opt-in with `R33D_AGENT=1`
  as compatibility alias; three-stage gate; trace sink; bridge mode forces the
  mock brain; the covered-tile fix.
- `agent_core.lua`: `crossSessionMemory`, `followFirst`, `proveCancel` options;
  two `pendingLoot` fixes; container acquisition when nothing carried holds
  items; loot ranked by value; Ollama adapter preserved but unreachable in
  bridge mode.
- `agent.local.env.example` (new) plus rewritten `run_agent.ps1`/`run_agent.sh`:
  no hardcoded deployment path, character or credential anywhere tracked.
- `tests/real33d_agent_test.lua`: MVP section preserved; bridge section added for
  schema acceptance, malformed observation/intent rejection, invisible and stale
  target rejection, invalid item/container references, budget and per-action
  cooldowns, ids/correlation, JSONL serialization, opt-in-off-by-default,
  cross-session memory, and one regression test per defect below.
- `evidence/agent/REAL33D-AGENT-BRIDGE-001.md` and
  `evidence/agent/bridge/REAL33D-AGENT-BRIDGE-001-certification.jsonl`.
- `.gitignore`, `PROJECT_STATUS.md`, `PARITY_MATRIX.md`, this handoff and its
  archive. Unrelated pre-existing changes were left untouched.

## Defects found in the inherited MVP

Each has a regression test that fails against the old code.

1. `tile:isCovered(0)` asks whether anything at all sits above a tile, which is
   true everywhere indoors and underground. The agent observed zero tiles and
   froze under any roof. Fixed to the player's own floor, which is both the
   faithful question and a no-op filter for a same-floor scan.
2. `pendingLoot` was cleared on every loot failure path but not on the success
   path, so after the first loot that worked, target selection was dead.
3. `pendingLoot` never expired while no bag was open, because the loot machinery
   is gated on a carried container. This was the live symptom the operator saw:
   the character stopped attacking and wandered for the rest of the session.

Smaller: the one container-open attempt was spent on an observation taken before
the inventory populated; loot took the first non-container item, so a live
`dead rat` yielded a worm instead of the gold coins beside it.

## Tests and evidence

- `luajit tests/real33d_agent_test.lua`: both sections PASS.
- `tests/secret_check.sh`: PASS.
- Live session `20260925T013642Z`, character `Test Player B`, rights verified
  empty read-only beforehand: 136 JSONL lines, 32 observations, 17 intents, 51
  validations with 0 rejections, 17 dispatches, 17 results, every one
  `authoritative_change=true`. 0 incoming protocol errors.
- `scripts/server/start_wsl.sh` and `stop_wsl.sh` bracketed the runs; services
  were stopped cleanly afterwards.

## Local QA fixture operations

With the server stopped, at the operator's explicit request, each writing a
timestamped backup: a backpack was restored to slot 3 of both characters after
they dropped theirs on death; `Test Player B` was placed at a known hunting
ground for the certified run and restored to full health and its start position
afterwards. No Fusion32 source, rule or protocol was changed and none of this is
readable by a Brain.

## Limits and next work

`cancel_follow` live and sustained low-HP survival remain
`IMPLEMENTED_UNVERIFIED`. Ollama, any other LLM, persistent memory, multiple
agents and autonomous levelling were explicitly out of scope.

The mock brain is a deterministic finite-state policy, not a competent player.
It has no map, no route memory and no supply management, so it explores by local
random walk and loses to sustained damage. The operator asked about teaching an
agent to actually play: the Brain contract is provider-independent, so a richer
policy or an LLM substitutes without touching the observation boundary, the
validator or the trace. The blocking decision for that work is whether an agent
may keep memory of what it has itself observed — its own map, where it died,
where it found monsters — which this milestone forbids and which is the
difference between experience and the omniscience fair play rules out.

Do not merge to `main` until the operator certifies.
