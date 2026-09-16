# HANDOFF

Date/time: 2026-09-16
Agent: Codex
Role: PROTOCOL772CORE GAME LOGIN IMPLEMENTATION
Branch: `main`
Starting commit: `a56add56e7ad5a11fd2e28d642bada4390b16756`
Ending commit: `3db23f9`
Worktree: clean after focused Game Login commit

## Objective

Complete `CLIENTCORE-GAMELOGIN-772-001`: Character List endpoint to Game
`:7172`, source-traced Tibia 7.72 Game Login, RSA/XTEA handoff, initial
authentication messages and persistent session. Do not implement WorldState,
movement, gameplay or Unreal.

## Inspection and source findings

Inspected selected immutable Game symbols:

- `reference/game/src/communication.cc::HandleLogin` under `TIBIA772`:
  command 10, terminal type/version outside RSA, raw 128-byte RSA, then RSA
  plaintext zero/XTEA/GM/account/name/password.
- `reference/game/src/connections.cc::JoinGame` and
  `reference/game/src/receiving.cc::ReceiveData`: authenticated handoff and
  client command 11.
- `reference/game/src/sending.cc::SendInitGame`, `SendRights`,
  `SendFullScreen`, and `connections.hh::ServerCommand`.
- `communication.cc::WriteToSocket/ReceiveCommand`: encrypted inner size,
  XTEA blocks, outer LE size and persistent receive lifecycle.

`SendInitGame`, `SendRights` and `SendFullScreen` append to the server output
ring and can share one encrypted frame. `SendRights` is optional: the source
emits it only when at least one action is present.

## Changes

- Added `clientcore/include/fusion32/protocol772/gamelogin.h` and
  `clientcore/src/gamelogin.cpp`.
- Added `protocol772_gamelogin` CMake target and deterministic
  `clientcore/tests/gamelogin_tests.cpp`.
- Added `docs/protocol772/GAMELOGIN.md` and
  `evidence/clientcore/CLIENTCORE-GAMELOGIN-772-001.md`.
- Updated project status, architecture, roadmap, parity matrix, source truth
  and this handoff; archived the prior Login handoff.

`GameLoginSession` preserves pending frames when one TCP read returns several
frames. The parser decodes INIT_GAME and optional RIGHTS, recognizes
FULLSCREEN as intentionally unparsed, and preserves unknown/tail bytes.

## Tests/results

Validated WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5:

- Debug build: PASS
- CTest: 4/4 PASS (Transport, Crypto, Login, Game Login)
- ASan/UBSan CTest: 4/4 PASS
- Deterministic RSA/framing and initial-message fixtures: PASS
- Unknown/invalid/tail preservation: PASS
- Live smoke: PASS with synthetic `ACCOUNT_A`; Game `INIT_GAME` and
  `FULLSCREEN` observed, socket remained connected for two seconds. Rights
  packet was absent because the synthetic account has no rights, which is
  source-defined optional behavior.

No credential, public modulus, private key or runtime secret was recorded. The
temporary live harness and temporary modulus file were removed.

## Status and limits

`CLIENTCORE-GAMELOGIN-772-001 = PASS` within this bounded scope. Native Windows
and independent repetition remain unverified. Fullscreen/map data is preserved
but not parsed; WorldState, movement, gameplay, command encoding and Unreal
remain out of scope.

## Exact next task

`WORLDSTATE-INIT-772-001`: plan and source-trace the minimal semantic state
needed after the preserved initial messages. Do not begin it automatically.
