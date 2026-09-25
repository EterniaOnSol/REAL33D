# HANDOFF

Date/time: 2026-09-24 21:16 -06:00, America/Guatemala
Task: `REAL33D-AGENT-MEMORY-001`
Agent / role: Codex, live certification
Branch: `milestone/real33d-agent-memory-001` (not merged to `main`)
Starting commit: `c321f6f29078e509f34986f2c4aca365d03c4640`
Ending commit: certification commit on this branch (see `git rev-parse HEAD`)
Worktree: `C:\Users\dell\Desktop\fusion32`; separate executable REAL33D2D
checkout `C:\Users\dell\Desktop\REAL33D2D` was used for live QA. That
checkout's existing unrelated dirty state was preserved.

## Objective and result

`REAL33D-AGENT-MEMORY-001 = CERTIFIED` for the requested two independent 2D
sessions. The MVP and BRIDGE live PASS results remain separate. Session A made
new observation-derived memories, persisted them, and ended normally. A fresh
Session B loaded them, began with the remembered place and creature invisible,
walked toward the remembered place through a newly validated `g_game.move`,
and observed Fusion32's position result. A stale creature ID was rejected
until a fresh B observation showed it. No LLM, protocol extension, Fusion32
gameplay modification, 3D/Unreal path, or hidden map source was used.

## Startup and inspection

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
`PARITY_MATRIX.md`, and prior `handoffs/CURRENT.md`. Inspected Git status,
branch, HEAD and remotes, and the exact `AgentMemory.load`/`observe`/`remember`,
`AgentCore.validate`/mock navigation, runtime load/save/observation/dispatch,
launcher environment lookup, schema projection, and QA rights paths. The
prior substantive handoff is archived as
`handoffs/archive/2026-09-24_REAL33D-AGENT-MEMORY-001-implementation.md`.

## Changes and files

- `agent/real33d2d/run_agent.ps1`: fixed the one live defect, a duplicate-key
  failure from dynamic `Get-Item Env:$name`, using the .NET Process environment
  API. No Brain, memory, action, protocol, or gameplay feature changed.
- `tests/real33d_agent_launcher_test.ps1`: regression for that launcher path.
- `tests/real33d_agent_memory_live_test.lua`: reproducible two-session evidence
  and stale-target validator checks against the retained live observations.
- `evidence/agent/memory/`: A/B JSONL traces, exact memory after A, and B client
  log. `evidence/agent/REAL33D-AGENT-MEMORY-001-certification.md` explains the
  preconditions, fixture, correlation, checks and hashes.
- `PROJECT_STATUS.md`, `PARITY_MATRIX.md`, memory progress snapshot, and this
  handoff: current states and limits.

## Live run and evidence

QA rights query returned no `CharacterRights` rows for IDs 1001/1002. Fresh
client PIDs 24468 (A) and 27420 (B) used `-Mode bridge -Brain mock -Memory`.
A started `memory.state=new records=0`; it directly observed places/routes and
Cipfried, then ended with `memory_saved=true records=20`. B started
`memory.state=loaded records=20 sessions=2` at `(32097,32209,7)`. The remembered
`32095:32215:7` place bucket and Cipfried were absent from B's first
observation. The first memory-guided move passed schema/state/budget validation,
used `g_game.move`, and the correlated result observed y `32209 -> 32210`.
Further B observations freshly showed Cipfried at y=32212 and arrival in the
remembered bucket at y=32215. B ended with `memory_saved=true records=26`.

The only server-side file edit was an operator QA fixture while services were
stopped: the ordinary test character was placed at a tile directly observed
in A. Its pre-certification file was restored after B; the SHA-256 equals the
untouched backup. QA services were restarted and verified alive. This fixture
was never exposed to or used by the Brain. Protocol error scan: zero matches
in the retained B client log; no protocol-error JSONL event.

## Tests and results

- REAL33D2D bundled LuaJIT `tests/real33d_agent_test.lua`: MVP, BRIDGE and
  MEMORY deterministic sections PASS.
- LuaJIT `tests/real33d_agent_memory_live_test.lua` with the retained A/B
  traces and A memory file: PASS for two sessions, provenance, memory-guided
  navigation, stale attack/follow refusal, fresh visibility, unobserved-map
  boundary, and character isolation.
- PowerShell `tests/real33d_agent_launcher_test.ps1`: PASS. The same launcher
  also started both fresh live clients successfully.
- Normal-client opt-in-off startup: no agent events during the four-second
  smoke interval; existing opt-in deterministic test PASS.
- `tests/secret_check.sh`: see the certification commit's check before push.

## Remaining work and risks

No work remains for this milestone's requested certification. The mock agent
walked at roughly one step per three seconds in this run, so faster monsters
can leave its reach; movement tuning and sustained survival require a separate
task. Earlier low-HP survival and cross-floor death recovery remain
`IMPLEMENTED_UNVERIFIED` and were not promoted by this run. Existing memory
files using the old pre-`c321f6f` underscore naming are not migrated. No LLM
provider was connected.

Exact next task: none within `REAL33D-AGENT-MEMORY-001`; stop after committing,
pushing this milestone branch, verifying remote HEAD, and checking a clean
worktree. For independent replay, run the commands in the certification
evidence document with the retained artifacts. Do not infer any 3D or protocol
parity from these 2D results.
