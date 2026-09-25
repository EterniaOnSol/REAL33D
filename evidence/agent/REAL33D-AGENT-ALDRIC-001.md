# REAL33D-AGENT-ALDRIC-001

Branch: `milestone/real33d-agent-aldric-001`, based on
`ef57363dd7c25f588fff4266eb0578b389c6c5e0`.

## Design and fair play

This opt-in mode uses the existing REAL33D2D OTClient parser and game model.
The current `AgentObservation` is the only live world-state input. Personal
`AgentMemory` consists of earlier direct observations, and the read-only
`real33d.agent.veteran_knowledge/1` records provide durable player-level
concepts. The prompt explicitly ranks current observation above memory above
static knowledge. Neither memory nor knowledge contains a live creature ID or
authorizes an action. Each proposed intent passes schema, fresh state, and
budget validation before dispatch through the ordinary `g_game.*` function.
The server remains the sole gameplay authority. No protocol or server gameplay
code changed.

The static knowledge source and limits are documented in
`agent/real33d2d/modules/real33d_agent/knowledge/README.md`. It intentionally
contains no map coordinates, spawn occupancy, exact prices, NPC availability,
or server-derived state. Conceptual economic planning is possible; NPC buy and
sell execution is not exposed by the audited action bridge.

`AgentAldric` is the strategic Brain. Its identity explicitly names Tibia 7.72,
Fusion32 and REAL33D2D, and asks for survival, hunting, looting, supplies,
economy, equipment and progression. It does not specify a hunt, waypoint or
action sequence. The
model returns a short goal, summary, one intent, and a movement horizon at
most four tiles. The tactical continuation only repeats the model-selected
direction through freshly visible walkable tiles and stops on material state
change. The model is never called for each SQM within that short horizon.
Invalid provider output or a provider outage results in no dispatched action,
an explicit log event and bounded backoff. Aldric mode rejects mock Brain
configuration and has no mock fallback.

## QA setup

The operator QA server is the unchanged Fusion32 7.72 runtime in WSL, with
login/game/query ports 7171/7172/7173. A separate ordinary-rights QA account
and character named Aldric were provisioned while the server was stopped.
The credentials remain in ignored runtime configuration and are absent from
this repository. A read-only query found zero `CharacterRights` rows for
Aldric. The executable REAL33D2D checkout is separate from this repository;
its pre-existing unrelated edits were preserved. The opt-in modules were
copied there for live QA. A provider-offline probe launched REAL33D2D and
showed a new personal memory state, repeated provider errors with bounded
backoff, no intent, no dispatch, and no mock action. A normal client with the
agent opt-in removed remained running and generated no agent trace.

The local provider runtime is official Ollama 0.33.1 Windows portable package
(`ollama-windows-amd64.zip`, SHA-256
`321BF63E36871FB31EE44CF4987150D770C679AC3E233115540D83468D675B06`).
Its API is the [official Ollama chat API](https://github.com/ollama/ollama/blob/main/docs/api.md).
The model source is the [official Qwen3-4B Q4_K_M GGUF release](https://huggingface.co/Qwen/Qwen3-4B-GGUF);
the local GGUF SHA-256 is `7485FE6F11AF29433BC51CAB58009521F205840F5B4AE3A32FA7F92E8534FDF5`.
Ollama's local `/api/tags` listed `qwen3:4b`, and `/api/chat` returned a
nonempty completed response with that exact model name. Binaries, weights and
credentials remain ignored.

## Recovery and live certification — 2026-09-25

The recovery audit found branch `milestone/real33d-agent-aldric-001` at the
certified MEMORY commit `ef57363` with unstaged ALDRIC implementation and no
ALDRIC commit or remote branch. The executable REAL33D2D checkout had an older
copy of two ALDRIC modules; these were reconciled from the source checkout after
diff review. Pre-shutdown traces remain local historical/debug evidence. The
first post-reboot session was retained as recovery evidence. A second session
with the explicit Tibia identity exposed a trace attribution defect: the
bridge labeled a movement action `g_game.move` although the actual call was
`g_game.walk`. That complete session is retained locally as diagnostic evidence.
The bridge now records the API path returned by the dispatcher, without
changing the gameplay call. The final session below ran after that correction.

Before the final session, deterministic MVP/BRIDGE/MEMORY and ALDRIC tests,
the two-session MEMORY replay, launcher regression, and `secret_check.sh` all
passed. The local model API was verified. Fusion32 was stopped and started
cleanly; `status_wsl.sh` verified Query Manager, Game and Login process
identity and ownership of ports 7173, 7172 and 7171. A stale Game PID file
left by the power loss was archived after its PID was confirmed absent. A
read-only SQLite query returned zero `CharacterRights` rows for Aldric's
character ID 1003. No QA database or server file was read by the Brain.

Final fresh REAL33D2D process PID 9044 used `-Mode aldric -Brain ollama
-Model qwen3:4b -Character C`, with an isolated trace path and existing
observation-derived Aldric memory. Its trace session ID is
`20260925T191803Z`; `session_start` reports `mock_disabled=true`,
`memory.state=loaded`, 32 records and 8 cumulative memory sessions. The
session ran only after the reboot, final prompt and dispatch-trace correction.
A clean Fusion32
shutdown caused `onGameEnd`; the trace ends with `session_end`,
`memory_saved=true` and 32 records. The client was then closed while offline.

The complete local trace has 62 events: 6 real-model decisions with 6 distinct
goal strings, 6 LLM intents, 3 tactical continuations, 26 validation events,
5 accepted `g_game.walk` calls and 4 correlated observations of changed
authoritative position. One state refusal was
`tile_not_visible_walkable`; three budget refusals were `action_cooldown`.
One goal naming The Oracle was checked against the *same decision's*
current visible-creature list. No mock intent or provider failure appears in
this session. The retained trace has no `protocol_error` event. The final
OTClient log was 2,460 bytes; a case-insensitive scan for error/fatal,
protocol error, unknown opcode, parse error and packet error found zero
matches. No new protocol parity is claimed here.

The committed [sanitized JSONL](aldric/certification_trace.jsonl) removes local
file paths, raw viewport tile lists and chat while retaining every event and
the visible creatures needed for replay. The raw trace and before/after memory
files remain in ignored `evidence/agent/live/REAL33D-AGENT-ALDRIC-001/`.
`tests/sanitize_aldric_trace.py` refuses to overwrite output and verifies a
single complete, contiguous session. The replay test passes on both raw and
sanitized traces:

```text
REAL33D_ALDRIC_LIVE decisions=6 goals=6 llm_actions=6 tactical_actions=3
dispatches=5 moves=4 opened_container=0 npc_goals=1 provider_failures=0 PASS
```

| Artifact | SHA-256 |
| --- | --- |
| Raw final `session.jsonl` (ignored) | `80D31BE65C8214BC88CFD3C37427685910E99AB93AD478766FE38E04D266CC36` |
| Sanitized `certification_trace.jsonl` | `DD16D8D84DB1B4340B4E8E3E89E4E2C3B04F3391752C4FF70BC18298025B6E52` |
| Final client log (ignored) | `64C5A2BBED4DDC8623934FEBF4BD2E3B4EDE301B36F0F4EAFD12234AD9E5F6B6` |
| Memory before final session (ignored) | `EA7E5F777152E5D38723DF9B9E77152CAA8A2F18B32FFB54187706A9B998FC7D` |
| Memory after final session (ignored) | `9D84946D9839D0396B9315C29A84D36A166A5A8DBD64AE6A584B629F1C2C86EE` |
| Final `agent_aldric.lua` | `D0C9E5BA2C543004D44349EA3F251820B3A7DDA5D8D119B0403E19452B3F047C` |
| Final `agent_bridge.lua` | `5C25B5D415F6024FA00AD1F5FC2BD1EC487B3C3E12C8E81C290B7A74826FBFF8` |
| Final `agent_runtime.lua` | `54BE6324CA4701F07A4134FBCDA30E19618DC52DADC17525E60536818263CC28` |

`REAL33D-AGENT-ALDRIC-001 = CERTIFIED` for bounded real-LLM control of
ordinary-rights Aldric through current observations, personal memory,
veteran concepts and the unchanged action gate. The live run proves movement,
not autonomous combat, looting, supply use, trading or equipment upgrades.
Those player skills are represented as versioned conceptual knowledge and
remain `IMPLEMENTED_UNVERIFIED` as live behaviors. NPC buy/sell execution is
`NOT_STARTED` because the audited bridge has no such action.

## Reproduction

From the repository root, run REAL33D2D's bundled LuaJIT and PowerShell:

```text
luajit tests/real33d_agent_test.lua
luajit tests/real33d_aldric_test.lua
luajit tests/real33d_agent_memory_live_test.lua evidence/agent/live/REAL33D-AGENT-MEMORY-001-cert/session_A_clean.jsonl evidence/agent/live/REAL33D-AGENT-MEMORY-001-cert/session_B_clean.jsonl evidence/agent/live/REAL33D-AGENT-MEMORY-001-cert/memory_after_A.json
luajit tests/real33d_aldric_live_test.lua evidence/agent/aldric/certification_trace.jsonl
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/real33d_agent_launcher_test.ps1
bash tests/secret_check.sh
```

The live replay checks model identity and mock exclusion, personal memory load
and save, multiple distinct model goals, intent correlation, validation gates,
normal `g_game.*` dispatch, and Fusion32-observed movement. It does not turn a
model goal into a PASS label for combat, chat, loot, economy, or equipment
unless the actual trace demonstrates that behavior.

The pre-shutdown provider-outage JSONL is intentionally ignored and incomplete;
its local-only replay passed with eight failures and zero intents/dispatches.
It is historical diagnostics, not part of the final certification. The
deterministic Aldric test independently covers provider error and timeout.
