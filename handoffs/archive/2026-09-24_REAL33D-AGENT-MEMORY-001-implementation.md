# HANDOFF

Date/time: 2026-09-24 20:36 -06:00, America/Guatemala
Task: `REAL33D-AGENT-MEMORY-001`
Agent / role: Codex, review and regression fixes in shared worktree
Branch: `milestone/real33d-agent-memory-001` (not merged to `main`)
Starting commit: `56f1815b207061e16d2351e4908a723dc1234b91`
Ending commit: the commit containing this handoff
Worktrees: `C:\Users\dell\Desktop\fusion32` (writable source); separate
`C:\Users\dell\Desktop\REAL33D2D` checkout was read for live evidence but
not modified during this continuation.

## Objective and result

The operator's `REAL33D-AGENT-MVP-001` remains `PASS` for one autonomous 2D
mock player live against Fusion32; `REAL33D-AGENT-BRIDGE-001` remains `PASS`.
Repeated `keep going` prompted the opt-in memory follow-on. Its current state
is `IMPLEMENTED_UNVERIFIED`: pure Lua tests pass, a live session saved and a
later session loaded memory, but prolonged survival and a correlated live
memory-guided action are not proved. This work does not add ClientCore,
WorldState, a protocol extension, Fusion32 gameplay logic or Unreal changes.

## Startup and inspection

The required repository contract, status, architecture, roadmap, parity matrix
and prior handoff were read earlier in the task. Current branch, HEAD, status
and remotes were inspected. Exact functions reviewed here:
`AgentMemory.checkRecord`, `Store:remember`, `AgentMemory.load`,
`AgentMemory.observe`, `Store:routeTo`, `Store:huntingGround`,
`AgentCore.mockBrain:decide`, `AgentCore.validate`, and runtime
`loadMemory`/`saveMemory`/`applyIntent`/`tick`. The prior substantive handoff
is archived as `handoffs/archive/2026-09-24_REAL33D-AGENT-BRIDGE-001.md`.

## Changes and files

The shared branch already contained uncommitted memory work in
`agent/real33d2d/modules/real33d_agent/` (`agent_bridge.lua`,
`agent_core.lua`, `agent_runtime.lua`, `agent_schema.lua`,
`real33d_agent.otmod`, and `agent_memory.lua`),
`agent/real33d2d/run_agent.ps1` and
`tests/real33d_agent_test.lua`. Preserve that work. This review added:

- `agent_memory.lua`: category field types, required coordinates for places,
  copy-on-write failed-update handling, and cross-floor hunting-ground guard
  requiring an agent-observed route; identity key encoding now prevents space
  versus underscore collisions between character names.
- `agent_runtime.lua`: a rejected memory file is not overwritten on a later
  periodic save; memory file names use the same collision-free identity.
- `tests/real33d_agent_test.lua`: malformed fields, missing coordinates,
  failed-update immutability, and cross-floor route regression cases.
- `evidence/agent/REAL33D-AGENT-MEMORY-001-progress.md`,
  `PROJECT_STATUS.md`, `PARITY_MATRIX.md`, and this handoff: precise state and
  live limitations.

## Tests and evidence

- LuaJIT `tests/real33d_agent_test.lua`: MVP, BRIDGE and MEMORY sections PASS.
- `git diff --check`: exit 0, with CRLF conversion warnings only.
- `bash tests/secret_check.sh`: PASS.
- Read-only review of `C:\Users\dell\Desktop\REAL33D2D\real33d2d.log`:
  first memory-enabled session `memory=loaded` at 20:26:28, observed combat,
  inventory, movement, chat, HP declining to 2/160, then `GAME_END
  memory_saved=true records=56` at 20:29:39; next session `memory=loaded` at
  20:29:42 on floor 7, then repeated four nearby temple tiles. See progress
  evidence for the exact scope. The floor and validation fixes were made after
  this run; no new live result is claimed for them.

## Remaining work and risks

The separate REAL33D2D checkout has not been synced to these final fixes.
Existing memory files created with the old underscore file-name scheme are not
automatically migrated to the new hex identity path.
Retain a JSONL trace and memory file across two sessions, confirm one action
was guided by a remembered place and still passed fresh-state validation, and
show the server result. Then test the cross-floor guard live. The mock policy
still does not demonstrate sustained low-HP survival. `saveMemory` writes the
valid memory file directly, so interruption during a write can truncate it;
consider an atomic replacement strategy before relying on it for long runs.
Do not mark MEMORY `PASS` from the text log alone. No merge to `main` or push
has been performed.

Exact next files: `agent/real33d2d/modules/real33d_agent/agent_memory.lua`,
`agent_runtime.lua`, `agent_core.lua`, `tests/real33d_agent_test.lua`, and
`evidence/agent/REAL33D-AGENT-MEMORY-001-progress.md`. Run LuaJIT suite and
`git diff --check` before any live retry. Keep the ordinary-rights QA client
and server path; do not alter server gameplay or give the character GM rights.
