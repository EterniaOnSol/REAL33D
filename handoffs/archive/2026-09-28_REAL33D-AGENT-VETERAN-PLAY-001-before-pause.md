# HANDOFF

Date/time: 2026-09-25/26, America/Guatemala
Task: `REAL33D-AGENT-VETERAN-PLAY-001`
Agent / role: Codex, implementation, client audit and live QA
Branch: `milestone/real33d-agent-veteran-play-001` (not merged to main)
Starting commit: `49bad6043de685e6b32753c5876ec1cddbe579d2` (ALDRIC certified)
Ending commit: this handoff commit on the branch; resolve with `git rev-parse HEAD`
Worktree: `C:\Users\dell\Desktop\fusion32`
Executable mirror: `C:\Users\dell\Desktop\REAL33D2D`

## Objective and outcome

Add sourced static player-level world knowledge and enable LLM-selected
Tibia 7.72 progression goals using only live client observation, certified
personal memory, ordinary REAL33D2D actions and final validation. The
implementation and deterministic tests pass. Full veteran-play certification
`FAILED`: the long real-model session walked repeatedly around Rookgaard
Academy without choosing combat, looting, conversation or economy. A short
follow-up loaded its saved memory but proposed two blocked moves, correctly
rejected. New knowledge/action support is `IMPLEMENTED_UNVERIFIED` in live QA.
The base ALDRIC bounded-control certification remains `CERTIFIED`.

## Startup, inspection and branch history

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
`PARITY_MATRIX.md` and the prior `handoffs/CURRENT.md`. Inspected status,
branch, HEAD, remotes, recent commits, ALDRIC modules/tests/evidence and the
separate REAL33D2D checkout. The base branch was clean at `49bad60`. Before
new work it was pushed, and remote `milestone/real33d-agent-aldric-001` was
verified at the exact full hash. Then this VETERAN branch was created from it.
No main merge occurred. The prior handoff was archived at
`handoffs/archive/2026-09-25_REAL33D-AGENT-ALDRIC-001-certification.md`.

Audited REAL33D2D's `src/client/game.cpp`, `game.h`,
`luafunctions.cpp`, `protocolgameparse.cpp`, and `modules/game_npctrade` for
NPC trade events, `g_game.buyItem`, `sellItem`, item movement, equip and player
trade. The mirror's pre-existing ALDRIC files matched the base; only reviewed
opt-in module files were copied from this worktree for live QA. No Fusion32
reference/runtime data, protocol source, server gameplay, Unreal, Musebook,
Web3 or multi-agent code was changed.

## Discoveries and changes

- `knowledge/world_v1.lua` and `knowledge/README.md`: sourced public
  Rookgaard/Thais landmarks, coordinate and visible-cue lookup, regional
  leads, approximate bearing/distance and version caveats. Public pages are
  the sole catalogue source. No Fusion32 map/database/spawn data was used.
- `agent_aldric.lua`: high-level progression objective, explicit four-level
  information priority, 7x7 current-viewport map, own recent position/decision
  history to expose backtracking, world/general knowledge references in
  decisions. No waypoint sequence or hunt script was added.
- `agent_runtime.lua`, `agent_schema.lua`, `agent_core.lua`,
  `agent_provider_ollama.lua`, `real33d_agent.otmod`: opt-in NPC trade callbacks,
  current shop observation, schema/state/budget gated buy/sell through normal
  client methods; ten-second per-action trade cooldown. A shop projection
  initially mutated an empty observed table (`[]` versus `{}`), causing false
  freshness rejections. It now copies the table. A non-secret changed-field
  log remains for future diagnosis. No raw packet path.
- `tests/real33d_veteran_knowledge_test.lua` and existing action count test:
  landmark cue/region, fair-play priority, closed shop, current price/money/
  goods checks, and the projection mutation regression.
- `evidence/agent/veteran/session_f_trace.jsonl` and
  `session_g_trace.jsonl`: complete, sanitized traces. Full raw traces and
  personal memory remain ignored local evidence.
- `evidence/agent/REAL33D-AGENT-VETERAN-PLAY-001.md`, project status and
  parity matrix: exact results, source links, limits, hashes and replay steps.

## Tests and live results

Bundled LuaJIT MVP/BRIDGE/MEMORY, ALDRIC and VETERAN deterministic suites:
PASS. Two-session MEMORY replay: PASS. PowerShell launcher regression: PASS.
`bash tests/secret_check.sh`: PASS. Provider `/api/tags` reported `qwen3:4b`;
`/api/chat` returned a completed matching-model reply. Fusion32 services
were restarted cleanly and identities/ports verified before live sessions.

Early A-D local-only sessions diagnosed the projection mutation; E verified
movement after the fix. Complete session F `20260925T201237Z`: mock off,
`qwen3:4b`, personal memory loaded (32) and saved (54), 346 real-model
decisions, 101 distinct goal strings, 41 bounded continuations, 287 accepted
`g_game.walk` calls, 277 later authoritative changes, 38 distinct positions
on floors 6 and 7. First `(32099,32192,6)`, last `(32101,32200,7)`. The
client observed level 1 and 150/150 HP throughout. Every model intent was
movement. The existing bounded ALDRIC live replay on F PASS; this is not a
veteran progression PASS. There were 65 malformed provider decisions with
no dispatch, 50 freshness, 21 blocked-tile and 29 cooldown refusals, and no
`protocol_error` trace event. F ended with `session_end`.

Final short session G `20260926T001257Z`: loaded F's 54 records, two
LLM-selected north moves, both state-gate rejected as
`tile_not_visible_walkable`; zero dispatch. It ended with
`session_end memory_saved=true`. The user requested ending the test. Fusion32
stopped cleanly and the client closed offline. Ollama may remain running;
check it before a future run.

## States, risks and exact next task

`PASS`: sourced static landmark lookup and observation priority in
deterministic tests; real LLM bounded navigation; validator refusals;
memory load/save. `IMPLEMENTED_UNVERIFIED`: actual NPC buy/sell, economic
planning expressed as a legal action, equipment improvement and autonomous
combat/loot. `NOT_STARTED`: player-to-player trade confirmation and a
distinct equip action. `FAILED`: requested full autonomous veteran progression
loop in live QA. Do not call a goal string evidence of hunting or a shop
intent evidence of a completed transaction. Modern public geography is an
uncertain historical lead, not certified 7.72 parity. No privileged or
off-screen state was used.

Next task, only if separately authorized: inspect the F/G traces and improve
strategic loop avoidance and goal selection without hardcoded waypoints or
changing the final validator. Exact files:
`agent/real33d2d/modules/real33d_agent/agent_aldric.lua`,
`knowledge/world_v1.lua`, and
`evidence/agent/REAL33D-AGENT-VETERAN-PLAY-001.md`.
Reproduce deterministic checks from this worktree with REAL33D2D's bundled
LuaJIT:

```text
luajit tests/real33d_agent_test.lua
luajit tests/real33d_aldric_test.lua
luajit tests/real33d_veteran_knowledge_test.lua
luajit tests/real33d_aldric_live_test.lua evidence/agent/veteran/session_f_trace.jsonl
bash tests/secret_check.sh
```

The new VETERAN branch has no remote push authorization; only the base ALDRIC
branch was explicitly authorized for push. Do not merge main. The complete
evidence report gives trace hashes and the factual session chronology.
