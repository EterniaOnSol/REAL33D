# REAL33D agent experiment preservation and pause

Date: 2026-09-28, America/Guatemala. Task: `REAL33D-AGENT-PROJECT-CLOSURE-001`.
Agent: Codex; role: repository audit, preservation, offline verification and closeout.

`AGENT_PROJECT_STATUS = PAUSED`
`VETERAN_PLAY_002_STATUS = IMPLEMENTED_UNVERIFIED`

Pause reason: **Project focus returned to REAL33D 3D client.** This is an
operator-directed project pause, not a new technical failure. No autonomous
gameplay session or feature development was performed during this closure.
The existing progression replay failure is preserved below without weakening
its acceptance criteria. Resume the preserved architecture; do not rebuild it.

## Initial repository audit

| Item | Observed value before edits |
| --- | --- |
| Primary checkout | `C:/Users/dell/Desktop/fusion32` |
| Branch | `milestone/real33d-agent-veteran-play-002` |
| HEAD | `77e35b40116e674880fe6bbf9186e15e3fa4f801` |
| Remote | `https://github.com/EterniaOnSol/REAL33D.git` |
| Remote agent branch | Absent: `git ls-remote` returned no matching ref |
| Remote main | `a97cf25e7449a8a3ef35ef2553181c5032c7a247` |
| Worktree | 10 modified files and 8 untracked file/directory entries |
| Existing handoff | VETERAN-PLAY-001; archived intact before replacement |
| VETERAN-PLAY-002 durable status | Not yet recorded; code and session N evidence existed uncommitted |

Modified files at audit: `.gitignore`; `agent/real33d2d/README.md`;
`agent/real33d2d/run_agent.ps1`; module files `agent_aldric.lua`,
`agent_bridge.lua`, `agent_provider_ollama.lua`, `agent_runtime.lua`,
`real33d_agent.otmod`, `knowledge/README.md`; `tests/real33d_aldric_test.lua`.

Untracked entries at audit: module files `agent_preposition.lua`,
`agent_veteran_planner.lua`, `knowledge/real33d_772.lua`;
`evidence/agent/veteran002/session_n_trace.jsonl`;
`scripts/agent/build_knowledge_772.py`; `tests/real33d_veteran_live_test.py`,
`tests/real33d_veteran_planner_test.lua`, `tests/sanitize_veteran_trace.py`.
All are related to the paused experiment and preserved by this closure.

Initial worktree inventory:

| Worktree | Branch | Initial HEAD |
| --- | --- | --- |
| Primary checkout | `milestone/real33d-agent-veteran-play-002` | `77e35b40116e674880fe6bbf9186e15e3fa4f801` |
| `build/integration-item-use` | `main` | `a97cf25e7449a8a3ef35ef2553181c5032c7a247` |
| `build/unreal-item-use-interaction-001` | `milestone/unreal-item-use-interaction-001` | `a97cf25e7449a8a3ef35ef2553181c5032c7a247` |
| `build/unreal-minimap-navigation-001` | `milestone/unreal-minimap-navigation-001` | `ea53eccc58d29fd5fa358108a4278853aa21b130` |

`main` is occupied by another worktree. Per the user's explicit fallback,
leave the primary checkout clean on the published agent branch. No worktree
is removed or modified to make a switch possible. No merge to main occurs.

## Preserved implementation and earlier evidence

The pre-existing VETERAN-PLAY-002 changes add structured model-selected goals,
plans and subgoals, progress/replan diagnostics, bounded current-action
affordances, rejected-step budget handling, a local static 7.72 knowledge
adapter, secondary MCP normalization, and operator-only QA prepositioning.
The Ollama response format, timeout and context settings and launcher options
are preserved as found. Closure changes only documentation, archival inventory
and the sanitized-trace ignore exception; no Lua/runtime behavior is changed.

The earlier implementation, tests, traces, memory system and handoffs remain
in branch history and the tree. Their bounded verdicts remain as documented:

| Milestone | Preserved result and evidence |
| --- | --- |
| MVP-001 | Bounded mock-agent PASS; `REAL33D-AGENT-MVP-001.md` |
| BRIDGE-001 | Bounded bridge PASS, with its existing limitations; `REAL33D-AGENT-BRIDGE-001.md` and `bridge/` |
| MEMORY-001 | Two-session CERTIFIED scope; `REAL33D-AGENT-MEMORY-001-certification.md` and `memory/` |
| ALDRIC-001 | Last completed agent milestone, bounded real-model control CERTIFIED at `49bad6043de685e6b32753c5876ec1cddbe579d2`; `REAL33D-AGENT-ALDRIC-001.md` and `aldric/` |
| VETERAN-PLAY-001 | Implementation IMPLEMENTED_UNVERIFIED; full progression test FAILED; existing report and `veteran/session_f_trace.jsonl`, `session_g_trace.jsonl` |
| VETERAN-PLAY-002 | IMPLEMENTED_UNVERIFIED; project PAUSED; this report and `veteran002/session_n_trace.jsonl` |

All evidence paths above are relative to `evidence/agent/`. The prior substantive
handoff is preserved at
`handoffs/archive/2026-09-28_REAL33D-AGENT-VETERAN-PLAY-001-before-pause.md`.

## Existing live proof and limits

Retained session N, `20260926T225931Z`, contains 55 events, one normal start
and end, four `ollama` / `qwen3:4b` decisions, mock disabled, and 84 loaded
local knowledge records. Personal memory loaded three records and saved seven.
Four accepted `g_game.walk` dispatches each have schema/state/budget acceptance
and a later authoritative position change. The player moved south from
`(32096,32208,7)` to `(32096,32212,7)`. The model goal persisted between decisions.
The trace contains zero `protocol_error` events; this is a trace-scoped count.

The progression gate returns `FAILED`, exit 1, for exactly:
`no observed level, economy, or combat-plus-loot progression`.
There were no attack targets, combat hit, loot gain, economy gain or level gain.
Walking and goal persistence do not establish veteran progression. This does
not change the operator-directed pause into a technical failure verdict.

Other local H-P/preflight/setup traces remain ignored and untouched. Several
have no `session_end` and cannot be called complete certification sessions.
Session P is complete with three model decisions and three movement dispatches;
it does not establish progression. QA prepositioning is explicitly setup only.
`LAST_KNOWN_START_POS = 32096,32208,7` is the preserved test origin, not a promise
about the character's current server position at a future resume.

Public session N SHA-256:
`3ac3176c7070d946fdc954d29b9d9406be27b6a20655e8392f3af3789a3d97fe`.
Re-sanitizing the preserved raw N trace to ignored scratch produced this exact
hash. The original published candidate was not rewritten.

## Local-only preservation, MCP and authority limits

`veteran002/preservation_inventory.json` records paths, sizes and SHA-256 hashes
for 76 local trace, memory and supporting files, including the static index. It
publishes no raw contents. Local-only preservation is not a remote backup:
keep these directories and their private deployment settings when relocating
the project. Model directories are retained in place without copying or hashing
large model payloads.

The local `tools/external/tibia_mcp` clone is clean at
`08329b0a7b376b6cf41c298caa1715fb39e796ea` (`miltonhit/tibia_mcp` as named by the
adapter). Read-only inspection of README, requirements and `src/mcp_server.py`
shows a modern TibiaWiki crawler, PostgreSQL corpus and FastMCP/SSE service.
No generated database files were found in the clone. No crawler/service was
started during closure; the opt-in REAL33D live runtime does not query MCP.
`Real33DKnowledge.normalizeMcp` and `secondary` preserve provenance and mark
modern responses SECONDARY_REFERENCE / UNVERIFIED, with unavailable-provider
handling tested. Live MCP integration remains IMPLEMENTED_UNVERIFIED.

The ignored `agent/real33d2d/knowledge_local/canonical_772.json` has 84 records:
10 NPC homes and 74 historical monster-home regions. Its hash is
`88cdbbbcdf5b038c7a8ca907d65a448415f7a6e3ef60110a4a50d8ab5a828d51`.
`scripts/agent/build_knowledge_772.py` reads selected static archive `npc/*.npc`,
`mon/*.mon` and `dat/monster.db`, preserving source hashes. The generated data
remains local and ignored; it is not republished or regenerated at closure.
The adapter's VERIFIED_772 label is an existing static compatibility claim,
not certification of live occupancy, walkability or the autonomous progression
loop. Fair-play/provenance review of such archive-derived leads must be retained
as a resume concern; current visible observation remains the action gate.

Personal memory remains at
`evidence/agent/live/REAL33D-AGENT-VETERAN-PLAY-002/memory/` and earlier local
session directories. Its certified identity isolation/provenance system and
public MEMORY fixtures are preserved. The planner keeps strategic state within
a running Brain; cross-process restoration of its goal/plan is not proven.
Do not conflate personal memory persistence with planner serialization.

MCP clone/corpus, static index, raw sessions, model payloads, caches and local
credentials remain ignored. Only the explicitly sanitized N JSONL is allowlisted
under `veteran002/`. No generated MCP database, proprietary bulk archive, model,
credential, API key or runtime configuration is intentionally staged.

## Offline verification during closure

Preconditions: existing project files at the audited HEAD plus its pre-existing
uncommitted changes; run from the repository root; no live client or provider.
LuaJIT is the existing REAL33D2D build tool at
`C:/Users/dell/Desktop/REAL33D2D/build/vcpkg_installed/x64-windows/tools/luajit/luajit.exe`.
Python is `C:/Users/dell/AppData/Local/Programs/Python/Python311/python.exe`.

| Check executed | Result |
| --- | --- |
| `tests/real33d_agent_test.lua` | PASS: MVP, bridge, memory regression groups |
| `tests/real33d_aldric_test.lua` | PASS: provider, precedence, tactical and safety |
| `tests/real33d_veteran_knowledge_test.lua` | PASS: static landmarks/current-observation priority |
| `tests/real33d_veteran_planner_test.lua` | PASS: knowledge, plan persistence, replan and safety fixtures |
| `tests/real33d_agent_launcher_test.ps1` | PASS: process-environment regression |
| `tests/real33d_agent_memory_live_test.lua evidence/agent/memory/session_A.jsonl evidence/agent/memory/session_B.jsonl evidence/agent/memory/memory_after_A.json` | PASS: retained two-session replay; no new live run |
| `tests/real33d_aldric_live_test.lua evidence/agent/aldric/certification_trace.jsonl` | PASS: retained bounded ALDRIC replay; no new live run |
| `tests/real33d_veteran_live_test.py evidence/agent/veteran002/session_n_trace.jsonl` | FAILED: progression absent; exact failure retained above |
| `tests/sanitize_veteran_trace.py` raw N to ignored `build/agent/closure_n_sanitized.jsonl` | PASS: 55 events; exact public N hash match |
| `tests/secret_check.sh` after staging closure changes | PASS: tracked files, reachable history, credentials and immutable reference checks |

## Runtime and resume

Windows process inventory found no `otclient.exe`, REAL33D agent launcher,
Ollama runtime or Tibia MCP runtime. Both Ubuntu WSL process inventories found
no autonomous player/provider/MCP process. Ubuntu-26.04 had Fusion32 game,
login and querymanager services; these are shared infrastructure and were left
running. Docker Desktop's Linux engine pipe was absent. The unrelated
`mythera_gl.exe` user application and Blender MCP/Codex processes were left alone.
No project autonomous runtime needed terminating; `AGENT_RUNTIME_ACTIVE = NO`.

Exact resume point, only after a new operator authorization: check out the
published VETERAN-PLAY-002 branch in an isolated worktree; read
`handoffs/CURRENT.md`, this report and the preserved N trace; retain local memory
and knowledge paths. Run the existing offline tests before any client launch.
Inspect `agent_veteran_planner.lua::observe/accept/rejected`,
`agent_aldric.lua::affordances/decide/onRejected`, and the N replay failure.
The next work would have been to diagnose movement-only strategy and establish
observed combat plus loot, level, or economy gain, without scripted progression
or changing the final action validator. No such work is authorized by this pause.

Use the existing `run_agent.ps1 -Mode aldric -Brain ollama -Model qwen3:4b`
entry point only when resumed, with preserved private deployment settings,
`-KnowledgeFile`, `-MemoryDir`, and a fresh ignored `-Trace` path. First observe
the actual character position; the setup origin is not current live state.
Do not automatically run `-Mode prep` or rebuild the architecture.

Publication: closure commit is the branch tip containing this report; resolve
with `git rev-parse milestone/real33d-agent-veteran-play-002`. Push only that
branch without force and verify it against `git ls-remote`. `main` remains
unchanged and occupied by its existing worktree.
