# HANDOFF

Date/time: 2026-09-15T10:06:32-06:00
Agent: Codex orchestrator with independent source auditors/reviewer
Role: ORCHESTRATOR + SOURCE CUSTODIAN
Branch: `main` planned for local baseline
Starting commit: N/A (workspace initially had no Git repository)
Ending commit: resolve with `git rev-parse HEAD`; exact hash is also reported in the session report because a commit cannot embed its own hash
Worktree: `C:\Users\dell\Desktop\fusion32`

## Objective

Complete `BOOTSTRAP-SOURCES-001`: verify archives/bundles, compare loose snapshots, select immutable Game/Login/Query Manager candidates, trace all `TIBIA772` effects and cross-component contracts, classify runtime, materialize safe references, verify builds, and establish local Git hygiene.

## What I inspected

All operational documents; all eight archive hashes; all five bundles with verification, refs, tags, graphs, histories and fsck; every Game loose snapshot against bundled history; all `TIBIA772` occurrences; Login character-list/RSA/framing; Game/Login/Query Manager internal contracts; runtime inventory; IP Changer 7.72/modulus behavior; build systems and WSL toolchain.

## What I discovered

The strongest immutable static baseline is Game `386fa9b...`, Login `f1c839f...`, Query Manager `edea08d...`. Independent review found no static incompatibility. Loose Game snapshots are `COMMIT_PLUS_CHANGES`, not bundle commits. `TIBIA772` only gates Game terminal version plus game-login terminal-field relocation, and Login terminal version; Query Manager has no guard. Static compatibility is not runtime compatibility.

Game/Login track the same compromised reference PEM; it was omitted. A clean environment needs a generated shared key and matching public modulus patched into the exact classic 7.72 client. IP Changer supports this concept, but live addresses/client compatibility are unverified.

## What I changed

Materialized curated byte-identical reference files under `reference/`, explicitly omitting private PEMs. Retained authoritative `config.cfg.dist` templates but documented that sample credentials must be replaced. Added source manifest, runtime classification, provenance, evidence, and reproducible WSL build script. Updated source truth, status and roadmap. Archived the previous handoff. Prepared `.gitignore` against archives, runtime, keys, generated config and builds.

## Files changed

Operational Markdown and `.gitignore`; `SOURCE_MANIFEST.md`; `docs/RUNTIME_CLASSIFICATION.md`; `reference/{game,login,querymanager}` curated trees and provenance; `tests/build_reference_wsl.sh`; three compact build logs; archived and current handoffs. Original archives were untouched.

## Tests executed

- `ARCHIVE-INTEGRITY-002`
- `BUNDLE-INTEGRITY-001` plus `git fsck --full`
- exhaustive snapshot/blob comparison
- complete `TIBIA772` search/trace
- `REFERENCE-BYTE-MATCH-001`
- `SOURCE-SELECTION-STATIC-REVIEW-001` by an independent agent
- `BUILD-GAME-772-001`
- `BUILD-LOGIN-772-001`
- `BUILD-QUERYMANAGER-SQLITE-001`
- staged-content sensitivity scan before commit

## Results

All integrity checks passed. Independent review accepted candidate selection as the strongest static baseline. All three components built with exit 0 on Ubuntu 26.04 WSL2; Game/Login logs show explicit `-DTIBIA772=1`. No server process, historical binary, runtime, database, or classic client was launched.

## Evidence produced

`evidence/protocol/BOOTSTRAP-SOURCES-001.md` and `evidence/build/{game-772-build,login-772-build,querymanager-sqlite-build}.log`.

## What is PASS

Archive/bundle integrity, byte-correct curated materialization, independently reviewed static candidate selection, and three clean component builds. No protocol/gameplay/parity certification.

## What remains UNVERIFIED

Runtime/schema/data compatibility; sanitized configuration and key generation; service startup; world resolution; classic executable/IP Changer addresses; character login and game entry; packet fixtures; all parity and Unreal work.

## Blockers

A legitimate classic Tibia 7.72 client is absent. A sanitized runtime has not been constructed. Runtime compatibility requires live verification.

## Risks

Manual decompilation defects; incomplete 7.72 adaptation despite exhaustive guard search; OpenSSL deprecated RSA API; historical data sensitivity; sample config credentials accidentally reused; classic client/modulus mismatch; IP Changer address dependence; false inference from build success.

## Exact next recommended task

`SERVER-BASELINE-772-001` - construct a minimal sanitized runtime from selected sources and quarantined reference data; generate fresh shared RSA material and credentials; launch Query Manager, Game and Login; then authenticate and enter the world using an exact, legitimate Tibia 7.72 client. If the client remains absent, complete server startup evidence and record the client test as blocked rather than substituting another client.

## Exact files/functions the next agent should inspect

- `SOURCE_MANIFEST.md`, `docs/RUNTIME_CLASSIFICATION.md`, `evidence/protocol/BOOTSTRAP-SOURCES-001.md`
- `reference/querymanager/{README.md,config.cfg.dist,sqlite/schema.sql,sqlite/z-999-initial-data.sql}` and startup/config functions
- `reference/login/{config.cfg.dist,src/main.cc,src/query.cc,src/connections.cc}`
- `reference/game/{README.md,src/config.cc,src/query.cc,src/communication.cc,src/main.cc}`
- Historical archive paths only through selective extraction: required `dat`, `origmap`, `npc`, `mon`; never `usr`, logs, dotfiles, backups or `bin`

## Useful commands

```powershell
git status --short --branch
git rev-parse HEAD
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/tests/build_reference_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
rg -n "TIBIA772" reference/game reference/login reference/querymanager
```

## Critical context for the next agent

Reference files are not implementation workspaces. Build/runtime changes belong in generated areas. Never commit PEM/private keys, generated `config.cfg`, runtime accounts/logs, or historical binaries. Candidate choice has independent static review, but the combination is still `REQUIRES_RUNTIME_VERIFICATION`. A build PASS is not server, protocol, login, or parity PASS.
