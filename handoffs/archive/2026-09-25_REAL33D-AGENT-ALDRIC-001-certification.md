# HANDOFF

Date/time: 2026-09-25 13:11 -06:00, America/Guatemala
Task: `REAL33D-AGENT-ALDRIC-001`
Agent / role: Codex, power-loss recovery and live certification
Branch: `milestone/real33d-agent-aldric-001` (not merged to `main`, not pushed)
Starting commit: `ef57363dd7c25f588fff4266eb0578b389c6c5e0` (MEMORY certified)
Ending commit: ALDRIC certification commit on this branch (see `git rev-parse HEAD`)
Worktree: `C:\Users\dell\Desktop\fusion32`; executable mirror:
`C:\Users\dell\Desktop\REAL33D2D`.

## Objective and result

`REAL33D-AGENT-ALDRIC-001 = CERTIFIED` for bounded real-LLM control of the
ordinary-rights 2D Aldric character. A post-reboot, post-final-prompt session
used Ollama `qwen3:4b` with mock disabled, loaded and saved personal memory,
consulted static veteran knowledge, and made five validated walk calls, four
of which changed position in later Fusion32 observations. The Brain explicitly
knows it is playing Tibia 7.72 on Fusion32 through REAL33D2D. Current
observation outranks memory and knowledge, and the existing schema/state/budget
gate remains final. No server gameplay, protocol, Unreal, Musebook, Web3, or
multi-agent path changed.

## Recovery audit and inspection

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
`PARITY_MATRIX.md`, `handoffs/CURRENT.md` and source provenance. Inspected
Git status, staged/untracked/ignored files, branch, HEAD, remotes, recent log
and reflog, exact ALDRIC modules, tests, evidence, server start/stop/status
scripts, `reference/game/src/main.cc::LockGame`, and the separate REAL33D2D
checkout. The previous handoff was archived byte-for-byte at
`handoffs/archive/2026-09-24_REAL33D-AGENT-MEMORY-001-certification.md`.

The branch already existed at `ef57363`. No ALDRIC commit, staging or remote
ALDRIC branch existed. ALDRIC implementation, deterministic tests and partial
logs were preserved. REAL33D2D held an older prompt/horizon variant of
`agent_aldric.lua` and `agent_runtime.lua`; the diff showed a linear local
adjustment, so those files were synchronized from fusion32. A later audit found
that the bridge labeled a walk as `g_game.move`; its metadata now records the
actual API path returned by the dispatcher, and `agent_bridge.lua` plus the
updated runtime were synchronized too. All other mirror edits were preserved.
Pre-shutdown JSONL, the first post-reboot recovery session and the next session
before this trace correction remain historical/debug evidence, never counted
as the final certified session.

No services were running after reboot. Fusion32's first start failed because
`state/save/game.pid` held PID 1086 from before the power loss. The PID was
absent; the file was renamed to `game.pid.recovery-20260925` without deleting
it, then all three services started and passed `status_wsl.sh`. No legacy
binary or reference source was executed or changed.

## Changes and files

- `agent/real33d2d/modules/real33d_agent/agent_aldric.lua`: provider-
  independent strategic Brain, compact current observation, memory/knowledge
  retrieval, bounded tactical continuation, backoff and explicit Tibia 7.72
  identity. `agent_provider_ollama.lua` provides the local API adapter;
  `knowledge/v1.lua` and its README provide versioned player-level concepts.
- `agent_bridge.lua`, `agent_runtime.lua`, `agent_schema.lua`, `real33d_agent.otmod`,
  `run_agent.ps1`, and `agent/real33d2d/README.md`: opt-in integration,
  observation capacity, model selection, trace metadata, validation, launch and
  operating documentation. Normal client remains opt-in-off.
- `tests/real33d_aldric_test.lua`, `real33d_aldric_live_test.lua`,
  `real33d_aldric_failure_live_test.lua`: deterministic safety/provider
  checks, trace replay, and provider outage replay.
- `tests/sanitize_aldric_trace.py`, `.gitignore`, and
  `evidence/agent/aldric/certification_trace.jsonl`: reproducible sanitization
  and a committed 62-event trace without local paths, raw viewport tiles or
  chat. The raw trace and memory snapshots remain ignored locally.
- `evidence/agent/REAL33D-AGENT-ALDRIC-001.md`, `PROJECT_STATUS.md`,
  `PARITY_MATRIX.md`, archived prior handoff, and this handoff: exact
  preconditions, results, hashes, states and limits.

## Tests and live evidence

Before the final session: bundled LuaJIT MVP/BRIDGE/MEMORY deterministic suite
PASS; Aldric deterministic suite PASS; two-session MEMORY replay PASS;
PowerShell launcher regression PASS; `tests/secret_check.sh` PASS.
Provider availability: local Ollama 0.33.1, `/api/tags` lists `qwen3:4b`,
and `/api/chat` returned a completed response with matching model. The
Qwen3-4B Q4_K_M GGUF hash is in the evidence document. A read-only QA query
found zero `CharacterRights` rows for Aldric ID 1003.

Fusion32 was stopped and restarted cleanly before final client PID 9044
connected. Query Manager/Game/Login process identities and ports 7173/7172/7171
were verified. Session `20260925T191803Z` began with
`mode=aldric brain=ollama mock_disabled=true memory.state=loaded
memory.records=32`. It produced six real-model decisions, six LLM intents,
three tactical intents, 26 gate events, five accepted `g_game.walk` calls
and four correlated authoritative coordinate changes. One Oracle goal
was grounded in the same observation's visible creature list. One state
refusal and three cooldown refusals dispatched nothing. A clean Fusion32 stop
caused `session_end memory_saved=true records=32`; client closed offline.
Raw and sanitized trace replays both PASS. No `protocol_error` event is present.
The final OTClient log was 2,460 bytes; a separate error/protocol-pattern scan
found zero matches. Raw and sanitized artifact hashes are in the evidence report.

## PASS, limits, risks, exact next task

The named live replay `REAL33D_ALDRIC_LIVE` is PASS and the bounded milestone
is CERTIFIED. The static knowledge covers experienced-player concepts but the
certified live run proves movement only. Combat, loot, supply use, buying,
selling and equipment upgrades are `IMPLEMENTED_UNVERIFIED` as autonomous
behavior; NPC buy/sell execution remains `NOT_STARTED` because the audited
bridge exposes no safe buy/sell action. Exact 7.72 prices, NPC offerings,
spawns and geography are not asserted from current manuals. No off-screen or
privileged server data reaches the Brain. No protocol code was changed.

Next task within ALDRIC: none. Do not merge `main` or push without explicit
authorization. For independent replay from the repository root, run the
commands in `evidence/agent/REAL33D-AGENT-ALDRIC-001.md` with REAL33D2D's
bundled LuaJIT. Any broader gameplay competence must be its own bounded task
with fresh live evidence. Fusion32 services were stopped cleanly after the run;
the REAL33D2D client exited and Ollama was stopped.
