# FUSION32 Agent Contract

## Mission

Build a real Tibia 7.72 client with an Unreal Engine 3D presentation that connects to the same authoritative Fusion32 server as the classic 2D client. The target is logical parity, not an independent simulation or a map viewer.

## Scope

Current scope is Fusion32, protocol 7.72, the classic Tibia 7.72 desktop client, and a new Unreal desktop client. Mobile, console, and web abstractions are out of scope.

## Source authority

The local artifacts and their provenance are inventoried in `docs/protocol772/SOURCE_TRUTH.md`. Treat all server/runtime material as `REFERENCE / READ ONLY` until a canonical revision is selected. The game source describes itself as a manual decompilation with changes, so claims must cite file, symbol, version guard, and observed behavior where available. Never substitute OpenTibia behavior or memory from another project.

## Required startup procedure

Before editing, every agent must:

1. Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `PARITY_MATRIX.md`, and `handoffs/CURRENT.md`.
2. Run `git status`, `git branch --show-current`, `git rev-parse HEAD`, and `git remote -v` if Git exists. If it does not, record that fact.
3. Inspect the exact authoritative files/functions named by the current task.
4. Preserve unrelated and pre-existing work.
5. State the task ID, allowed scope, deliverables, tests, and evidence before substantial implementation.

## Rules

- Fusion32 is authoritative for all gameplay and state. Unreal sends commands and presents server state.
- Never invent opcodes, packet sizes, field order, framing, crypto, stack positions, or semantics. Use `UNKNOWN` or `UNVERIFIED` when evidence is insufficient.
- Keep protocol, semantic events, logical WorldState, Unreal adapter, and rendering separate.
- A WorldState thing is not an Unreal Actor. Network threads must not mutate Actors directly.
- Preserve tile thing order and stack position; never reduce a tile to one object.
- Do not silently alter, reformat, move, or delete reference sources, runtime data, binaries, or another agent's work.
- Do not execute legacy binaries. Do not expose secrets found in runtime configuration. The bundled PEM is compromised reference material, not a production key.
- Make small, traceable changes. Do not push, create remotes, or force Git operations without explicit authorization.
- Add proportionate tests and reproducible, compact evidence. Compilation proves only build success.
- Use only these states: `NOT_STARTED`, `IN_PROGRESS`, `IMPLEMENTED_UNVERIFIED`, `BLOCKED`, `FAILED`, `PASS`, `CERTIFIED`.
- Update `PROJECT_STATUS.md`, `PARITY_MATRIX.md` where relevant, and `handoffs/CURRENT.md` before ending substantial work.

## Definition of PASS

`PASS` means a named test was executed with documented preconditions and produced its expected result. Absence of visible errors, compiling, connecting, or displaying placeholders is not gameplay parity. `CERTIFIED` requires enough reproducible evidence for an independent reviewer to repeat and confirm the target behavior.

## Handoff protocol

Archive the prior handoff only when replacing a substantive one, then update `handoffs/CURRENT.md` with date/time, agent, role, branch, starting/ending commit, worktree, objective, inspection, discoveries, changes, files, tests/results, evidence, PASS, remaining UNVERIFIED work, blockers, risks, exact next task/files/functions/commands, and critical context. The repository must be sufficient without chat history.
