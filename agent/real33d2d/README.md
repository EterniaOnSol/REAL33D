# REAL33D 2D Agent Bridge

An opt-in Lua module over REAL33D2D's own OTClient parser, map panel,
`LocalPlayer`, containers and `g_game` functions. It introduces no ClientCore,
no WorldState, no protocol extension and no hand-crafted packet. Fusion32 stays
the sole gameplay authority.

`REAL33D-AGENT-BRIDGE-001` formalises the `REAL33D-AGENT-MVP-001` gateway into
explicit schemas, three-stage validation, correlation identifiers and a JSONL
trace. Install `modules/real33d_agent/` into the REAL33D2D checkout.

## Opt-in

| Variable | Meaning |
| --- | --- |
| `R33D_AGENT_MODE=1` | canonical switch; enables the bridge, mock brain only |
| `R33D_AGENT=1` | compatibility alias for the MVP launchers (legacy mode) |
| `R33D_AGENT_TRACE` | JSONL trace path; defaults to `real33d_agent_trace.jsonl` |
| `R33D_AGENT_BRAIN` | legacy mode only: `mock` or `ollama` |

With neither switch set the module autoloads and returns immediately: no timer,
no `g_game` subscription, no opcode hook, no trace file. A human session is the
ordinary client. The test suite asserts this rather than assuming it.

Bridge mode is mock-only by construction. A requested LLM provider is refused
out loud and forced back to `mock`, and the Ollama adapter is never constructed
on the bridge path.

## Files

| File | Role |
| --- | --- |
| `agent_schema.lua` | `AgentObservation` / `AgentIntent` schemas, strict checks, deterministic JSON |
| `agent_core.lua` | state-legality validator, action budget, deterministic mock brain, preserved Ollama adapter |
| `agent_bridge.lua` | session/correlation/observation/action ids, JSONL events, result correlation |
| `agent_runtime.lua` | opt-in wiring, bounded observation, dispatch through `g_game.*` |

## Contracts

`AgentIntent` is a closed set of 11 actions: `attack`, `cancel_attack`,
`cancel_follow`, `combat_mode`, `follow`, `move`, `move_item`, `open_container`,
`say`, `use`, `use_with`. Unknown actions, unknown fields on a known action,
wrong types, out-of-range values and control characters are rejected.

`AgentObservation` is a closed whitelist of 14 fields; anything else is a
`schema_forbidden_field` reject. That is the fair-play boundary as code: a later
change that starts copying the full map, the minimap cache, spawn data or
another player's state into the observation fails the schema instead of
shipping. The published projection groups it into `self`, `visible`, `owned`
and `social`.

Every dispatch passes `schema`, then `state` against freshly re-read client
state, then `budget`. The state gate re-reads after the Brain answers, so a
target that left the viewport in the meantime is refused rather than acted on.

Limits: 24 actions per rolling minute, 600 ms global gap, 750 ms between steps,
15 s between chat lines, 1 s otherwise, one decision per 2.2 s.

## Trace

One cycle emits `observation`, `intent`, `validation` (three), `dispatch` and,
on the next observation, `result`. Every line carries `session_id`,
`correlation_id` and `observation_id`; everything from the intent onwards also
carries `action_id`. The result reports `observed_in`, `elapsed_ms`,
`authoritative_change` and the named fields Fusion32 changed, so a reader can
join a decision to its server-side consequence. Keys are sorted, so equal state
encodes byte-identically and two runs diff cleanly.

Credentials never reach the trace: the runtime reads the account and password
only inside the login closure and copies them into no observation, intent or
event.

## Observation boundary

Current 11x11 subset of the map panel on the player's own floor, sight
spectators inside that subset, own stats/skills/position, received chat,
equipment, open containers, and client attack/follow/combat mode. Never
`g_map.getTiles()`, the minimap cache, the full map, off-screen creatures, spawn
data, other players' inventories, Fusion32 files or database state.

No cross-session memory participates in a bridge decision. The within-session
`visits` counter comes only from positions the brain itself observed.

## Mock brain

Deterministic finite-state policy; no learning, no LLM. It greets, opens and
uses a carried container, acquires one from the ground if it carries none,
engages a visible rat, rabbit, bug, deer or snake, enables the client's ordinary
chase mode, loots the most valuable item from a corpse, eats what it looted,
retreats at low health and otherwise explores adjacent visible walkable tiles.

Two scenario options exist so a bounded certification run exercises dispatchers
the world would not otherwise trigger. Both default off and fire once per
session: `followFirst` routes the first engagement through Follow, `proveCancel`
releases a target once after chase is confirmed.

Loot is ranked by value, not slot order. The table is ordinary 7.72 item ids —
prior knowledge of the game, the same thing a human player brings. It says
nothing about what any particular corpse contains.

## Running

Copy `agent.local.env.example` to `agent.local.env` (gitignored) and fill in the
local client path, WSL distribution, QA credentials path and character. No
deployment path, character or credential is hardcoded in any tracked file. The
launchers read the account and password from the prepared QA runtime's own
secrets file into the child process only; they are never echoed, passed as
arguments or written anywhere.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File agent/real33d2d/run_agent.ps1 -Mode bridge
```

`run_agent.sh` is the Git Bash equivalent; set `R33D_AGENT_RUN_MODE=legacy` for
the MVP path. Unit tests run from the repository root with the client's LuaJIT:

```powershell
& '<REAL33D2D>\build\vcpkg_installed\x64-windows\tools\luajit\luajit.exe' tests/real33d_agent_test.lua
```

## Aldric LLM mode

`-Mode aldric -Brain ollama` is a separate opt-in path. It requires persistent
memory and refuses a mock Brain. It uses the same current client observation,
schema/state/budget gates and `g_game` dispatcher as the certified bridge.
One provider failure yields no action, a logged error and a bounded retry.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File agent/real33d2d/run_agent.ps1 -Mode aldric -Brain ollama -Model qwen3:4b -Character C
```

The ordinary-rights QA character must already exist in the local runtime.
The model must be available on the local Ollama API. `R33D_AGENT_TRACE` and
`R33D_AGENT_MEMORY_DIR` can be set through the ignored local configuration or
the launcher's `-Trace` and `-MemoryDir` arguments. The versioned, read-only
concept catalogue is in `modules/real33d_agent/knowledge/`. Personal memory
remains a separate per-character file. The Brain receives a compact current
observation and at most seven relevant knowledge records per model decision.

The system identity explicitly says this is Tibia 7.72 on Fusion32 through
REAL33D2D. The durable knowledge pack supplies player-level concepts; exact
server prices, NPC offerings, spawns and geography are unknown until observed
or separately verified for this version.

The model sets a goal and proposed intent. A short movement horizon can repeat
only that selected direction on freshly visible walkable tiles, subject to the
same gates and action budget. Material changes or blocked movement end the
horizon. NPC purchase/sale is not exposed by the 11-action bridge; economic
reasoning can occur, but executing a purchase is `NOT_IMPLEMENTED`.

Run `tests/real33d_aldric_test.lua` with the same LuaJIT before any live run.

## Aldric milestone limits

Only the Ollama provider is implemented for Aldric. There is no OpenAI,
Claude, Gemini, Musebook, Web3, multi-agent, guild, or 3D path. The client
bridge has no NPC buy/sell action and no protocol extension. Static veteran
knowledge is read-only; personal experience is stored in the certified
per-character memory. Human play remains opt-in-off by default.
