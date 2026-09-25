# REAL33D-AGENT-BRIDGE-001

State: `PASS`. Date: 2026-09-24/25, America/Guatemala.
Branch: `milestone/real33d-agent-bridge-001`, not merged to `main`.

Formalises the `REAL33D-AGENT-MVP-001` gateway into a schema-driven bridge with
explicit `AgentObservation` and `AgentIntent` contracts, three-stage validation,
correlation identifiers and a JSONL observation/intent/validation/dispatch/result
trace. The MVP implementation was reused, not restarted.

## Scope and authority

- REAL33D2D `aeef6aa939a764de3728c51a3022953dff97f375` supplied the OTClient
  parser, `LocalPlayer`, UIMap, tiles, creatures, containers and `g_game`
  bindings. No ClientCore or WorldState was introduced.
- Fusion32 remained the sole gameplay authority. No Fusion32 source, gameplay
  rule, protocol or opcode was changed, and no packet was hand-crafted. Every
  action leaves through an existing `g_game.*` function.
- REAL33D 3D / Unreal was out of scope and untouched.

## Opt-in

`R33D_AGENT_MODE=1` is the canonical switch. `R33D_AGENT=1` is retained as a
compatibility alias for the MVP launchers. With neither set, `R.init()` returns
before scheduling a timer, connecting a `g_game` event, installing the opcode
hook or opening a trace file: the ordinary human client is unaffected. This is
asserted by `REAL33D_AGENT_BRIDGE_OPT_IN` below, not merely by inspection.

Bridge mode is mock-only by construction. `init()` refuses a requested LLM
provider out loud and forces `mock`, and `onStart()` never constructs the Ollama
adapter on the bridge path, so certification cannot contact an LLM.

## Schemas and validation

`agent_schema.lua` owns both contracts and a deterministic JSON encoder with
sorted keys, so equal state encodes byte-identically and a trace diffs cleanly.

`AgentIntent` is a closed table of 11 actions (`attack`, `cancel_attack`,
`cancel_follow`, `combat_mode`, `follow`, `move`, `move_item`, `open_container`,
`say`, `use`, `use_with`). An unlisted action, an unlisted field on a listed
action, a wrong type, an out-of-range value or a control character is rejected.

`AgentObservation` is a closed whitelist of 14 fields. Anything outside it is a
hard `schema_forbidden_field` reject, which is the fair-play boundary expressed
as code: a future change that starts copying a full map, minimap cache, spawn
data or another player's state into the observation fails the schema instead of
shipping. The published projection groups the observation into `self`,
`visible`, `owned` and `social`.

Every dispatch passes three gates in order:

| Stage | Question | On failure |
| --- | --- | --- |
| `schema` | is this a well-formed AgentIntent | `validation accepted=false` |
| `state` | is it legal against freshly re-read client state | `validation accepted=false` |
| `budget` | is the agent allowed to act right now | `validation accepted=false` |

The `state` gate re-reads the client after the Brain answers, so a target that
left the viewport between decision and dispatch is rejected rather than acted on
from a stale view.

Rate limits are unchanged from the MVP, the audit having found no defect in
them: at most 24 actions per rolling minute, a 600 ms global gap, 750 ms between
steps, 15 s between chat lines, 1 s otherwise, and one decision per 2.2 s.

## Identifiers and trace

Each cycle carries `session_id`, `correlation_id`, `observation_id` and, once a
Brain answers, `action_id`. The result event is emitted on the *following*
observation, because the authoritative answer to an action is what Fusion32
sends back afterwards; it carries the originating ids plus `observed_in`.

One certified cycle, verbatim from the trace with `schema` and `session_id`
elided for width:

```
intent      a-…-000014  c-…-000014  o-…-000014  {"action":"attack","creatureId":1073743329}
validation  stage=schema  accepted=true
validation  stage=state   accepted=true
validation  stage=budget  accepted=true
dispatch    accepted=true  path=g_game.attack
result      authoritative_change=true  elapsed_ms=3003  observed_in=o-…-000015
            changed=[{hp 139→131},{attackId 0→1073743329}]
```

## Named checks

| Check | Command / precondition | Observed | State |
| --- | --- | --- | --- |
| `REAL33D_AGENT_BRIDGE_SCHEMA` | `luajit tests/real33d_agent_test.lua` | All 11 actions accepted with valid fields; unknown action/field, wrong type, out-of-range, control character, bad `use_with` target count rejected; observation rejects non-table, missing `online`, missing required tables, bad health/identity/combat, and each of six forbidden fields | PASS |
| `REAL33D_AGENT_BRIDGE_STALE` | Same | Intent formed against one observation is refused against a later one where the creature left the viewport (`creature_not_visible`) or the tile is gone (`tile_not_visible_walkable`) | PASS |
| `REAL33D_AGENT_BRIDGE_REFS` | Same | Invisible item, non-container open, unknown destination, count above stack, invisible `use_with` source and target all rejected | PASS |
| `REAL33D_AGENT_BRIDGE_BUDGET` | Same | 750 ms move gap, 600 ms global gap, 15 s chat gap, 24-per-minute ceiling and window roll-off | PASS |
| `REAL33D_AGENT_BRIDGE_IDS` | Same | Deterministic id sequences; result event carries the originating `action_id`, `correlation_id` and `observation_id` plus `observed_in`; no result invented without a pending action; an unacknowledged dispatch reports `authoritative_change=false` | PASS |
| `REAL33D_AGENT_BRIDGE_JSONL` | Same | Sorted-key determinism, escaping, nan/inf to null, array vs object, one object per line with no embedded newline | PASS |
| `REAL33D_AGENT_BRIDGE_OPT_IN` | Same, with neither variable exported | `init()` schedules no timer, connects no event and leaves `ProtocolGame.onOpcode` untouched | PASS |
| `REAL33D_AGENT_BRIDGE_MEMORY` | Same | With `crossSessionMemory=false` the brain produces no intent from an item id remembered across logins; MVP default behaviour preserved | PASS |
| `REAL33D_AGENT_BRIDGE_LIVE` | Prepared local Fusion32, `run_agent.ps1 -Mode bridge -Character B` | Session `20260925T013642Z`: 136 JSONL lines, 32 observations, 17 intents, 51 validations, 17 dispatches, 17 results | PASS |

## Live certification

Session `20260925T013642Z`, character `Test Player B`, ordinary player rights
(`CharacterRights` empty, verified read-only before the run). Trace retained at
`evidence/agent/bridge/REAL33D-AGENT-BRIDGE-001-certification.jsonl`.

Every one of the 17 dispatches produced `authoritative_change=true`:

| Required proof | Observed authoritative change |
| --- | --- |
| login | `session_start`, then observations with `online=true` |
| observation | 32 observations, each schema-checked before publication |
| chat | `say` → `chat "0:" → "1:Test Player B\|Hello, I am exploring."` |
| movement | 7 `move` dispatches, each followed by a server-confirmed `x`/`y` change |
| visible creature targeting | `follow` on a visible `Rat` → `followId 0 → 1073743329` |
| attack | `attack` → `attackId 0 → 1073743329` |
| cancel | `cancel_attack` → `attackId 1073743329 → 0` |
| inventory/container observation | `owned.equipment` populated; `owned.containers` → `0:3[3492x3,3031x1,2853x1]` |
| valid use/open | `open_container` → containers populated; `use` → containers emptied |

`combat_mode` is not in this trace because the client's chase mode was already
enabled when the session began, so the brain had nothing to change. It is
covered by the deterministic tests and appeared in earlier runs of the same
build. `move_item` is likewise not in this trace; it is covered by tests and was
observed live in an earlier run of the same build, described under loot below.

Incoming protocol errors: **0**. The client log recorded no `[error]` or
`[fatal]` line for the session. Validation rejections: **0** of 51.

## Defects found and fixed

The audit of the inherited MVP found three real defects, each now carrying a
regression test that fails against the old code:

1. **Blind indoors and underground.** `tile:isCovered(0)` asks "is anything at
   all above this tile", which is true for every tile under a roof or below
   ground. The agent observed zero tiles and froze. `Tile::isCovered` returns
   false when the tile sits on the floor it is asked about, and the scan only
   ever visits the player's own floor, so `p.z` is both the faithful question
   and a no-op filter. This narrows nothing: the bound is still same-floor plus
   the panel's own viewport test.
2. **`pendingLoot` leaked on success.** Every failure path cleared it; the path
   where the loot actually arrived did not. After the first successful loot,
   target selection (`not self.pendingLoot`) was dead for the session.
3. **`pendingLoot` never expired without an open bag.** The loot machinery is
   gated on a carried container, so with the bag closed none of its timeouts
   ran and a flag set by a target walking out of view disabled target selection
   permanently. This was the observed live symptom: the character stopped
   attacking and wandered. The expiry now runs on its own clock before target
   selection.

Two smaller scenario fixes: the single container-open attempt is no longer spent
on an observation taken before the inventory populated, and loot is ranked by
value rather than by slot order. A live `dead rat` held `worm x2` in slot 0 and
`gold coin x2` in slot 1, and the old code took the worm; the ranking is prior
knowledge of ordinary 7.72 item ids, the same thing a human player brings, and
tells the agent nothing about what a corpse contains.

Two scenario options exist only to make the bounded run exercise dispatchers
that the world would not otherwise trigger: `followFirst` routes the first
engagement through Follow, and `proveCancel` releases a target once after chase
is confirmed. Both are off by default, fire at most once per session, and are
plain validated actions.

## Fair-play boundaries

Observation is the current 11x11 subset of the map panel on the player's own
floor, sight spectators inside that subset, own stats/skills/position, received
chat, equipment, open containers and client attack/follow/combat mode. It does
not read `g_map.getTiles()`, the minimap cache, the full map, off-screen
creatures, spawn data, other players' inventories, Fusion32 files or database
state. The observation whitelist is enforced by the schema, so this is a test,
not a promise.

No cross-session memory participates in a bridge decision. The within-session
`visits` counter is derived from positions the brain itself observed and resets
on every session.

## Local QA fixture operations

Performed by the operator with the server stopped, at the operator's explicit
request. They changed no Fusion32 source, gameplay rule or protocol, and none of
it is readable by a Brain. Each wrote a timestamped backup beside the file.

- Both QA characters had dropped their backpack on death during agent testing,
  which left the scenario with no container to open, use or loot into. A
  backpack was restored to inventory slot 3 of `1001.usr` and `1002.usr`.
- `Test Player B` was placed at a known hunting ground before the certified run
  and restored to full health and its start position afterwards. This is
  operator-side test placement, equivalent to choosing where a human tester logs
  in; the agent still observes only its own viewport.
- `Test Player B` died during an uncertified exploratory run at 8/160 HP in a
  rat cave. The low-HP retreat policy behaved as designed but lost. Sustained
  low-HP survival remains `IMPLEMENTED_UNVERIFIED`, unchanged from the MVP.

## Explicit limits

- `FOLLOW` is `PASS`: dispatched live and server-confirmed via `followId`.
  `cancel_follow` has a validated dispatcher and deterministic tests but was not
  separately dispatched live, because the brain converts a confirmed follow into
  an attack; it is `IMPLEMENTED_UNVERIFIED`.
- Ollama and any other LLM are out of scope for this milestone and unreachable
  in bridge mode. The adapter's own status is unchanged from the MVP:
  `IMPLEMENTED_UNVERIFIED`.
- Long-duration autonomous survival is out of scope and unproven.
- The mock brain is a deterministic finite-state policy, not a competent Tibia
  player. It has no map, no route memory and no supply management, so it
  explores by local random walk and dies to sustained damage.
