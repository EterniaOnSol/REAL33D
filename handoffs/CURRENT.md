# HANDOFF

Date/time: 2026-09-15T19:08:22-06:00
Agent: Codex orchestrator with independent runtime reviewer
Role: ORCHESTRATOR + SERVER INTEGRATOR
Branch: `main`
Starting commit: `d6201d23f579678fbd60081c8dae205f80dcdb06`
Ending implementation commit: `ef9a09bb80adc7a0baed5899e615dfb966879c0a`; final handoff-metadata commit is the repository HEAD and must be resolved with `git rev-parse HEAD`
Worktree: `C:\Users\dell\Desktop\fusion32`

## Objective

Advance `SERVER-BASELINE-772-001` by constructing a sanitized, resettable Fusion32 7.72 runtime from the immutable Game/Login/Query Manager baseline; generate fresh local secrets and synthetic identities; prove internal startup, world load, service liveness, bounded network behavior and clean shutdown without executing historical binaries or importing historical accounts.

## What I inspected

All required project memory; selected component build/config/startup sources; Query Manager schema, patches, authentication and SQLite initialization; Game `.tibia`, world resolution, map/data and first-login behavior; Login config/RSA/world status; historical archive inventory through bounded extraction only; generated process/log/DB state; Git/staged content and reference-source diffs.

## What I discovered

Query Manager must start from its own cwd with schema/patches and loopback `7173`; Game resolves `Fusion Test` through Query Manager and binds `7172`; Login binds `7171`. Game opens its socket before completing world load, so port availability is not readiness. The clean world loads 9,873 sectors and 8,533,464 objects.

The first runtime design placed secrets on Windows DrvFs, where effective modes were `0777`. Independent review correctly rejected that design, along with self-referential hashes and weak PID/listener correlation. That generated runtime was stopped and deleted. The corrected runtime is WSL-native `/tmp/fusion32-server-baseline-772-$UID`, uses effective root mode `0700` and secret/config/DB modes `0600`, verifies fixed expected archive/data/binary hashes, and correlates each service PID with process start token, executable, cwd and owned socket.

## What I changed

Added reproducible prepare/start/status/smoke/stop/reset scripts. Preparation builds the selected sources with explicit `TIBIA772=1`, verifies expected binaries, extracts only bounded data, initializes a new SQLite schema/seed, creates two synthetic accounts/characters, and generates a fresh shared 1024-bit PKCS#1 RSA key. Added runtime, provenance and classic-client gate documentation plus compact evidence. Updated source truth, manifest, status, roadmap and parity notes. Preserved LF for shell scripts.

## Files changed

- `.gitattributes`
- `PROJECT_STATUS.md`, `ROADMAP.md`, `PARITY_MATRIX.md`, `SOURCE_MANIFEST.md`
- `docs/{SERVER_RUNTIME,RUNTIME_DATA_PROVENANCE,CLASSIC_CLIENT_772,RUNTIME_CLASSIFICATION}.md`
- `docs/protocol772/SOURCE_TRUTH.md`
- `scripts/server/{prepare,start,status,network_smoke,runtime_smoke,stop,reset}_wsl.sh`
- `evidence/runtime/SERVER-RUNTIME-SMOKE-001.md`
- archived prior handoff and this handoff

## Tests executed

- historical runtime archive SHA-256 revalidation
- exact expected data-manifest and three binary hash gates
- `bash -n` on all orchestration scripts
- repeated prepare/start/status/network-smoke/stop cycles
- full destructive generated-state reset followed by start/smoke/stop
- `SERVER-RUNTIME-SMOKE-001`
- `SERVER-RUNTIME-REVIEW-001` (independent `REJECT`, defects fixed)
- `SERVER-RUNTIME-REVIEW-002` (independent fresh prepare/start/smoke/stop, `ACCEPT`)
- staged secret/material scan, `git diff --check`, `git diff -- reference`

## Results

The corrected local run and a fresh independent run both passed. Query Manager, Game and Login were simultaneously alive; internal Game/Login authorization succeeded; exact synthetic DB and world assertions passed; TCP connect/close did not kill services; RSA installation matched without exposing key material; effective secret permissions passed; clean shutdown removed PID/start files and left no listeners on 7171-7173. `reference/` remained unchanged.

## Evidence produced

`evidence/runtime/SERVER-RUNTIME-SMOKE-001.md`, supported by `docs/SERVER_RUNTIME.md` and `docs/RUNTIME_DATA_PROVENANCE.md`.

## What is PASS

- expected archive/data/binary integrity gates
- sanitized SQLite initialization and synthetic seed
- fresh shared Game/Login RSA installation and restrictive effective access modes
- internal component authorization and exact ports
- Game world load: 9,873 sectors / 8,533,464 objects
- bounded network connect/close
- reset and clean shutdown

`SERVER-RUNTIME-SMOKE-001 = CERTIFIED` within this exact internal-runtime scope after independent live repetition.

## What remains UNVERIFIED

Classic Tibia 7.72 executable identity and provenance; exact client patch addresses; fresh-modulus compatibility in a real client; character-list login; Game login; `JoinGame`; initial world display; packet fixtures; gameplay/parity; all Unreal work.

## Blockers

No legitimate classic Tibia 7.72 executable/DAT/SPR/PIC artifact exists in the workspace. Therefore `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001` and `CLASSIC-GAME-ENTRY-772-001` are `BLOCKED_CLIENT_ABSENT`.

## Risks

Manual decompilation defects; classic executable/IP Changer address mismatch; client RSA modulus mismatch; `/tmp` may be cleared when WSL is restarted and then requires preparation again; exact build hashes intentionally bind preparation to the reviewed Ubuntu 26.04 toolchain; world/runtime evidence does not imply external protocol correctness.

## Exact next recommended task

`CLASSIC-CLIENT-772-001`: accept only a legitimate exact Tibia 7.72 client artifact, hash and record its provenance, audit/build the selected IP Changer source or an exact controlled equivalent, patch host/port/fresh public modulus, then prove Login authentication, character list and Game world entry using one generated synthetic identity.

## Exact files/functions the next agent should inspect

- `docs/CLASSIC_CLIENT_772.md`
- `docs/SERVER_RUNTIME.md`
- `evidence/runtime/SERVER-RUNTIME-SMOKE-001.md`
- `scripts/server/*.sh`
- Login `reference/login/src/connections.cc` and `src/query.cc`
- Game `reference/game/src/communication.cc::HandleLogin`, `connections.cc::JoinGame`, and initial send functions
- IP Changer candidate revision/provenance in `SOURCE_MANIFEST.md` and bootstrap evidence; never execute its historical binary

## Useful commands

```powershell
git status --short --branch
git rev-parse HEAD
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/prepare_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/runtime_smoke_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/stop_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
```

If `/tmp/fusion32-server-baseline-772-$UID` already exists, use `reset_wsl.sh` only when intentionally discarding all generated local test state and secrets.

## Critical context for the next agent

The server baseline's internal runtime is certified, not the classic-client protocol. Never weaken this distinction. Runtime credentials and public modulus are generated outside Git under the private WSL runtime; do not print or commit them. A reset changes passwords and RSA modulus. Do not download or run a random classic client or historical IP Changer binary. Do not start Unreal until classic login/world entry is proven or an explicit orchestration decision accepts the client-artifact-only blocker.
