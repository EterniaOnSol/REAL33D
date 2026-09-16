# CLIENTCORE-LOGIN-772-001

Status: `PASS` (offline deterministic scope)

## Implementation

Added `protocol772_login`, an Unreal-independent Login layer behind the
existing Transport/Framing/Crypto components. It builds the source-traced
145-byte Tibia 7.72 Login payload, wraps it with the existing two-byte outer
frame, retains the generated XTEA key for the response, and parses decrypted
MOTD/error/character-list messages into typed records.

## Source truth

See [`docs/protocol772/LOGIN.md`](../../docs/protocol772/LOGIN.md). Primary
symbols are `reference/login/src/connections.cc::ProcessLoginRequest`,
`PrepareXTEAResponse`, `SendXTEAResponse`, `SendLoginError`,
`SendCharacterList`, `TERMINALVERSION`, `reference/login/src/common.hh` string
buffers, and `reference/login/src/query.cc::LoginAccount`. No reference file
was modified.

## Tests

Environment: Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5,
C++17, warnings as errors.

- Debug build: `PASS`
- CTest: `3/3 PASS` (Transport, Crypto, Login)
- Login fixture: exact 145-byte request and deterministic raw-RSA ciphertext
- Response fixture: MOTD, one character, BE IPv4, LE port and premium days
- Negative cases: invalid account/password length, unknown opcode preservation,
  incomplete records
- Framing round trip: request outer header and payload extraction `PASS`
- Sanitized ASan/UBSan build/test: `3/3 PASS`

## Bounded live smoke

After deterministic and sanitized tests, a temporary non-tracked harness used
only the local Fusion32 runtime and its synthetic `ACCOUNT_A` credential. It
loaded only the public modulus derived in a temporary file, connected to
`127.0.0.1:7171`, sent one valid Login request, decrypted the response and
parsed one character. Result: `live_login_smoke: PASS characters=1`.
The harness, public-modulus temporary file and all credential material were
removed/not tracked; no password, modulus or private key is recorded here.
The smoke does not implement or test Game Login.

Native Windows execution remains unverified.

## Scope exclusions

Game Login, initial world, opcodes beyond the character list, WorldState and
Unreal are not implemented.
