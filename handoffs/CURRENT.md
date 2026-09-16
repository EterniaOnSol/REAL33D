# HANDOFF

Date/time: 2026-09-15
Agent: Codex
Role: PROTOCOL772CORE LOGIN IMPLEMENTATION
Branch: `main`
Starting commit: `35aa40081222da3f199b4b60345ce6ed5343b940`
Ending commit: pending focused Login commit
Worktree: expected clean after commit

## Objective

Complete `CLIENTCORE-LOGIN-772-001`: source-traced Tibia 7.72 Login request,
RSA/XTEA response handling and typed Character List, without Game Login,
WorldState, opcodes beyond the Login records or Unreal.

## Inspection and source findings

Inspected `reference/login/src/connections.cc` (`CheckConnectionInput`,
`ProcessLoginRequest`, `PrepareXTEAResponse`, `SendXTEAResponse`,
`SendLoginError`, `SendCharacterList`), `reference/login/src/common.hh` read/write
buffers and `reference/login/src/query.cc::LoginAccount`. No `reference/` file
was modified.

- Login receives an outer LE U16 payload size and requires exactly 145 payload
  bytes: opcode 1, terminal type/version LE, three LE signatures, 128-byte RSA.
- Under `TIBIA772`, Login accepts terminal types 0..2 with version 772.
- RSA plaintext is zero, four LE XTEA words, LE account ID, LE length-prefixed
  password and uninterpreted remaining bytes.
- Login response is XTEA-encrypted with inner LE message length, optional MOTD
  opcode 20, error opcode 10 or character-list opcode 100. Character entries
  use strings, BE IPv4, LE port; premium days are LE U16.

## Changes

- Added `clientcore/include/fusion32/protocol772/login.h` and `src/login.cpp`.
- Added CMake target `protocol772_login` and deterministic `login_tests.cpp`.
- Added `docs/protocol772/LOGIN.md` and
  `evidence/clientcore/CLIENTCORE-LOGIN-772-001.md`.
- Updated project status, architecture, roadmap, parity matrix and this handoff.

The request builder returns exact 145-byte payload plus outer-framed wire bytes,
retains a move-owned XTEA key, injects deterministic random bytes in tests and
rejects account zero, invalid terminal/version and passwords over 29 bytes. The
response parser exposes `Decoded`, `Incomplete`, `Unsupported`, `Malformed`,
`ProtocolViolation` and `CryptoError`; unknown opcode tails are preserved.

## Tests/results

Validated WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5:

- Debug build: PASS
- CTest: 3/3 PASS (Transport, Crypto, Login)
- Request RSA/framing fixture, MOTD/Character List fixture and negative cases: PASS
- Sanitizer ASan/UBSan build/test: PASS (3/3)
- Live smoke: PASS with one synthetic local account and one parsed character;
  temporary harness and public-modulus/credential material were not tracked.

## Status and limits

`CLIENTCORE-LOGIN-772-001 = PASS` covers deterministic fixtures and the bounded
local Login smoke. Native Windows and independent test reproduction remain
unverified. Game Login, initial world, full opcode decoding, WorldState and
Unreal remain out of scope.
Game Login, initial world, full opcode decoding, WorldState and Unreal remain
out of scope.

## Exact next task

After this commit, review the live smoke result and prepare
`CLIENTCORE-GAMELOGIN-772-001`; do not begin it automatically. Inspect
`reference/game/src/communication.cc::HandleLogin` and `connections.cc::JoinGame`
only when that task starts.
