# HANDOFF

Date/time: 2026-09-15T09:38:47-06:00
Agent: Codex orchestrator with three read-only audit agents
Role: ORCHESTRATOR + SOURCE AUDITOR
Branch: N/A (no Git repository)
Starting commit: N/A
Ending commit: N/A
Worktree: `C:\Users\dell\Desktop\fusion32`

## Objective

Audit the untouched workspace, establish initial source truth and operational memory, and determine the shortest evidence-based path to `TWO-CLIENT-VERTICAL-SLICE-001` without implementing gameplay or graphics.

## What I inspected

All eight top-level archives; archive hashes/listings; the three loose game snapshots; the five Git bundle heads and recent histories via temporary extraction; game/login/querymanager source locations; legacy runtime top-level contents/config without exposing its secret; Git availability; Unreal/classic-client presence.

## What I discovered

There was no Git repository or extracted tree. The game source is a manual decompilation with known changes. 7.72 is an optional `TIBIA772` compile guard and is absent from the default Makefile. Querymanager is required to boot game; login handles character lists. The runtime is large, stale, sensitive, and includes untrusted binaries. No classic client or Unreal project is present. Detailed source locations are in `docs/protocol772/SOURCE_TRUTH.md`.

## What I changed

Created only the project contract, status/architecture/roadmap/parity documents, initial source-truth inventory, compact audit evidence policy/report, test policy, retention guard, and this handoff. Original archives and reference content were not changed or extracted into the workspace. Git was not initialized because canonical source selection remains unresolved.

## Files changed

`.gitignore`, `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `PARITY_MATRIX.md`, `docs/protocol772/SOURCE_TRUTH.md`, `evidence/README.md`, `evidence/protocol/PHASE-0-AUDIT.md`, `tests/README.md`, `handoffs/CURRENT.md`, `handoffs/archive/README.md`.

## Tests executed

Read-only Git probes; recursive archive listings/counts; SHA-256 of every top-level archive; bundle head/log inspection in `%TEMP%`; source search and spot tracing for version guards, framing, crypto, opcodes, login, map/things, movement, gameplay systems, and build configuration.

## Results

Inventory and hashes reproduced. Game bundle head observed as `386fa9b...`; login `f1c839f...`; querymanager `edea08d...`; web `c61e291...`; ipchanger `8215db...`. No build, server startup, packet fixture, live login, Unreal, or parity test was executed.

## Evidence produced

`evidence/protocol/PHASE-0-AUDIT.md`.

## What is PASS

`PHASE-0-ARCHIVE-HASH-001`: SHA-256 recording and independent reproduction for all eight top-level archives only.

## What remains UNVERIFIED

Canonical revisions and archive/bundle relationship; complete character-list protocol; exact all-opcode payloads; public RSA modulus and fixtures; runtime compatibility and safe minimum dataset; archived binary protocol version; build/startup; classic client availability; all Unreal work; all parity.

## Blockers

No canonical materialized source, classic Tibia 7.72 client, Unreal project, or running sanitized Fusion32 environment. Git initialization remains deferred; `.gitignore` now guards known archives and future runtime extraction but must be reviewed when source layout is selected.

## Risks

Legal/provenance ambiguity; decompilation mistakes; default 7.70 build mistaken for 7.72; stale user/log/config data; compromised reference PEM; untrusted binaries; revision drift; one-command parser assumption; stack-position complexity.

## Exact next recommended task

`BOOTSTRAP-SOURCES-001` — verify bundles completely, diff selected histories/snapshots, propose a canonical immutable revision set and safe directory/ignore policy, materialize reference source trees, then initialize local Git with the scaffold and metadata only. Do not import runtime user/log/backups or execute binaries.

## Exact files/functions the next agent should inspect

Bundle `game`: `README.md`, `Makefile`, `src/communication.cc::{HandleLogin,ReceiveCommand,WriteToSocket}`, `src/crypto.cc`, `src/connections.hh`, `src/receiving.cc::ReceiveData`, `src/sending.cc`, `src/map.cc::PlaceObject`.
Bundle `login`: README/config and the connection/login handlers that parse client login and serialize character list.
Bundle `querymanager`: README/config/schema/bootstrap path and game/login query contracts.
Loose `game-db505.../src/operate.cc` versus content-identical master/3fd1 version.

## Useful commands

```powershell
git status --short --branch
Get-FileHash -Algorithm SHA256 *
tar -tf .\tibiacacaca.zip
git bundle verify <bundle>
git bundle list-heads <bundle>
rg -n "TIBIA772|HandleLogin|ReceiveCommand|ReceiveData|ClientCommand|ServerCommand" <materialized-source>
```

## Critical context for the next agent

Do not infer that bundle HEAD is canonical merely because it is newer. Do not copy the legacy runtime wholesale. Keep secrets and personal data out of Git/evidence. The shortest vertical slice requires character-list login before game login and requires a real classic 7.72 client that is currently absent. Any protocol claim must include source-to-handler-to-wire traceability and a fixture before `PASS`.
