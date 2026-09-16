# HANDOFF

Date/time: 2026-09-15T20:24:11-06:00
Agent: Codex
Role: CLASSIC CLIENT INTEGRATION EVIDENCE
Branch: `main`
Starting commit: `7d0202f9190febc001b9e0be2fc3a094a8197e5d`
Ending review commit: `f65f3a7645ff40b39b7cc8399760fd4f0b69ecee`; final metadata commit is repository HEAD
Worktree: `C:\Users\dell\Desktop\fusion32`

## Objective

Resume project inspection, validate the operator-reported successful Tibia 7.72 Login/Game session against the selected Fusion32 baseline, preserve compact reproducible evidence, remove the obsolete `BLOCKED_CLIENT_ABSENT` state, and state precisely what remains before certification. Do not execute the client as part of inspection, expose generated secrets, or claim historical provenance that is unavailable.

## Required startup and authoritative inspection

Read `AGENTS.md`, `PROJECT_STATUS.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `PARITY_MATRIX.md` and the prior handoff. Git was present on `main`, HEAD `7d0202f...`, with no remotes. The starting worktree had one pre-existing untracked file, `tests/verify_classic_client_772.py`; it was inspected, hardened and adopted rather than discarded.

Inspected the exact task sources and evidence boundaries:

- `reference/ipchanger/ipchanger.cc::ChangeIP` and its 7.72 `TibiaVersion` entry
- `reference/login/src/connections.cc::ProcessLoginRequest`, `SendCharacterList`, XTEA response framing
- `reference/login/src/query.cc` character/world response path
- `reference/game/src/communication.cc::HandleLogin`, outer framing and XTEA receive path
- `reference/game/src/connections.cc::JoinGame`
- runtime status scripts, sanitized Login/Game logs and live socket ownership
- PE structure, resources, hashes and address ranges of the selected ignored local client set

## Discoveries

The client was not absent. An exact EXE/DAT/SPR/PIC set existed under ignored `build/classic-client-772/app/`. The operator reported that it was a pre-existing copy on the same computer; no original URL, archive, installer, acquisition date or chain of custody is available. Windows had no `Zone.Identifier`. PE resources identify `Tibia Player` version `7.72`, company `CipSoft GmbH`; this supports identity, not historical provenance or licensing.

The selected executable hash is `1A82FE97A39E536C29327D6D5DDE316775F537DCC01506831429B1A0433B76F8`. Its x86/PE32 layout contains the `Version 7.72` marker and all Fusion32 source-defined endpoint/RSA address ranges. The locally built official Fusion32 IP Changer successfully patched host, port and the fresh generated public modulus for this exact process image.

Game logged `Test Player A` entering and taking the first-login outfit path. Two client map caches were produced. Repeated inspection showed the Windows Tibia process connected `ESTABLISHED` to local Game port `7172`; the observed interval exceeded 30 minutes from client start. Query Manager, Game and Login remained alive with verified identity and listener ownership.

Several stale `CLOSE-WAIT` sockets were visible between Game/Login and Query Manager. They did not interrupt the client session, but cause and long-run descriptor impact remain `UNKNOWN`.

## Changes

- Added reproducible client artifact/PE verification and exact companion hashes.
- Added sanitized classic-client evidence with preconditions, commands, results and certification boundary.
- Replaced all obsolete `CLIENT_ABSENT` statements with bounded live results.
- Advanced the server baseline to `PASS` and the aggregate classic-client gate to `IN_PROGRESS`.
- Marked classic Login, character list, Game entry, live IP Changer patch and sustained session as `PASS`.
- Kept historical client provenance explicitly `UNKNOWN` and independent live repetition `NOT_STARTED`.
- Advanced the recommended implementation task to `PROTOCOL-LOGIN-001`.
- Archived the prior substantive handoff unchanged.

## Files changed

- `PROJECT_STATUS.md`
- `ROADMAP.md`
- `PARITY_MATRIX.md`
- `SOURCE_MANIFEST.md`
- `docs/CLASSIC_CLIENT_772.md`
- `docs/SERVER_RUNTIME.md`
- `tests/README.md`
- `tests/verify_classic_client_772.py`
- `evidence/client/CLASSIC-CLIENT-772-001.md`
- `handoffs/archive/2026-09-15_SERVER-RUNTIME-IPCHANGER-001.md`
- `handoffs/CURRENT.md`

No file under `reference/` was changed. Client binaries/data, generated credentials, `servers.txt`, modulus and runtime logs remain untracked/uncommitted.

## Tests and results

- `CLASSIC-CLIENT-772-STATIC-001`: exact EXE/DAT/SPR/PIC hashes, x86 PE32, embedded 7.72 marker, five endpoint ranges and RSA field validation: `PASS`.
- `IPCHANGER-772-LIVE-001`: selected Fusion32 address table and fresh-modulus patch reached local Login and completed the RSA/XTEA transition to Game: `PASS`.
- `CLASSIC-LOGIN-772-001`: generated synthetic identity authenticated through Login: `PASS`.
- `CLASSIC-CHARLIST-772-001`: classic client traversed the source-defined character-list response and selected `Test Player A`: `PASS`.
- `CLASSIC-GAME-ENTRY-772-001`: Game logged `Test Player A`, first-login processing ran, client map caches appeared and TCP was established to `7172`: `PASS`.
- `CLASSIC-SESSION-SUSTAIN-001`: repeated live checks observed the session after more than 30 minutes from client process start: `PASS`.
- Query Manager/Game/Login identity, cwd, process start and owned listener checks: `PASS` during inspection.
- Client acquisition provenance: `UNKNOWN`.
- Independent live client repetition: `NOT_STARTED`.

## Evidence

Primary evidence is `evidence/client/CLASSIC-CLIENT-772-001.md`. Supporting source/build/runtime evidence remains:

- `evidence/build/ipchanger-772-windows-build.md`
- `evidence/runtime/SERVER-RUNTIME-SMOKE-001.md`
- `docs/CLASSIC_CLIENT_772.md`
- `docs/SERVER_RUNTIME.md`

No secret value or full noisy runtime log was retained.

## What is PASS

- exact selected client artifact identity and static PE/address-table validation
- live Fusion32 IP Changer compatibility for this exact executable hash and fresh generated modulus
- classic Login authentication and character-list flow
- Game `JoinGame`, initial world processing and an established sustained session
- the selected sanitized server baseline as a usable classic-client reference

## What remains UNVERIFIED

- original source, acquisition date, chain of custody and licensing of the operator-supplied local client copy
- independent repetition with sanitized character-list and initial-world screenshots
- packet captures/golden byte fixtures and complete RSA/XTEA/framing specification
- the cause and resource impact of Query Manager-related `CLOSE-WAIT` sockets
- gameplay semantics beyond bounded entry/stability
- all Unreal implementation and 2D-to-3D parity

## Certification boundary and blockers

There is no longer a client-absence blocker to `PROTOCOL-LOGIN-001` or the vertical-slice implementation path.

Functional classic compatibility can become `CERTIFIED` after an independent reviewer performs a fresh runtime/modulus cycle, repeats static verification and the live Login -> character list -> world -> sustained-session procedure, and retains sanitized screenshots/log/socket evidence.

Historical client provenance cannot be certified from the current copy because its original source record is absent. Either obtain an authorized copy with verifiable provenance or define certification narrowly as functional compatibility of the recorded hashes while provenance remains `UNKNOWN`.

## Risks

- Manual-decompilation defects and unverified packet semantics remain.
- The selected absolute IP Changer addresses are proven only for the recorded executable hash.
- Resetting the WSL runtime changes passwords and RSA modulus; regenerate `servers.txt` before the next client run.
- `/tmp` runtime state can disappear on WSL restart.
- Repeated `CLOSE-WAIT` sockets may indicate a descriptor-lifecycle defect; investigate before long soak tests.
- A classic client PASS is not Unreal parity.

## Exact next recommended task

`CLIENTCORE-TRANSPORT-772-001`: implement the smallest Unreal-independent TCP lifecycle, byte buffering and source-traced outer packet framing foundation with deterministic loopback and negative tests. Do not implement RSA/XTEA, Login payloads or Unreal. In parallel or afterward, request an independent reviewer for `CLASSIC-CLIENT-772-REVIEW-001`.

## Exact files/functions for the next agent

- `reference/login/src/connections.cc::ProcessLoginRequest`
- `reference/login/src/connections.cc::PrepareXTEAResponse`, `SendXTEAResponse`, `SendCharacterList`
- `reference/login/src/query.cc::LoginAccount` and character/world decoding path
- `reference/game/src/communication.cc::HandleLogin`, `ReceiveCommand`
- `reference/game/src/connections.cc::JoinGame`
- the selected read/write buffer, RSA and XTEA implementations used by Login and Game
- `docs/protocol772/SOURCE_TRUTH.md`, `ARCHITECTURE.md`, `evidence/client/CLASSIC-CLIENT-772-001.md`

## Useful commands

```powershell
git status --short --branch
git diff --check
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_classic_client_772.py /mnt/c/Users/dell/Desktop/fusion32/build/classic-client-772/app/Tibia.exe
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/status_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
```

For a fresh independent live repetition, stop the current session first, intentionally reset only the validated generated WSL runtime, regenerate the IP Changer configuration, and never print or commit the generated credentials/modulus.

## Critical context

The client artifacts are intentionally ignored and must not be staged. Their exact hashes, not their mutable path or filename alone, identify the passed set. The operator's statement supports `USER-SUPPLIED_LOCAL_COPY` only; it is not evidence for an original download source. Keep functional certification separate from provenance certification. The services and classic client were still running during evidence collection; do not reset or stop them unless the operator intends to end that live session.
