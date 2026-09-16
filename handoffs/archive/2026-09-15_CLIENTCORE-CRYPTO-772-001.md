# HANDOFF

Date/time: 2026-09-15T22:12:35-06:00
Agent: Codex
Role: PROTOCOL772CORE CRYPTO IMPLEMENTATION
Branch: `main`
Starting commit: `4f5ec30faaee701fde51b2227419c21cce68718c`
Implementation commit: `64e9217ef64181d44cdce815a36b6bb1d2aa9038`
Ending metadata commit: repository HEAD after this handoff is committed
Worktree: expected clean after the final metadata commit

## Objective

Implement `CLIENTCORE-CRYPTO-772-001` behind the existing `FramedPacket`: source-traced 1024-bit RSA public processing, XTEA encode/decode, XTEA key generation/ownership, exact inner lengths/padding/endian, explicit errors and deterministic golden fixtures. Do not implement application Login, Game Login, opcodes, WorldState or Unreal. Do not commit runtime secrets or any private key.

## Starting state

Read all required project memory and the Transport handoff before editing. Git was present on `main` at `4f5ec30faaee701fde51b2227419c21cce68718c`, with a clean worktree and no remotes. `CLIENTCORE-TRANSPORT-772-001 = PASS`; the next documented task was Crypto.

The prior substantive handoff was archived byte-identically as `handoffs/archive/2026-09-15_CLIENTCORE-TRANSPORT-772-001.md`.

## Authoritative inspection

Inspected the selected read-only source symbols:

- Game `reference/game/src/crypto.cc::TRSAPrivateKey::initFromFile/decrypt`
- Game `reference/game/src/crypto.cc::TXTEASymmetricKey::init/encrypt/decrypt`
- Game `reference/game/src/communication.cc::GetPacketSize`, `WriteToSocket`, `HandleLogin`, encrypted `ReceiveCommand` branch
- Login `reference/login/src/crypto.cc::RSADecrypt`, `XTEAEncrypt`, `XTEADecrypt`
- Login `reference/login/src/common.hh::BufferRead32LE`, `BufferWrite32LE`, read/write buffers
- Login `reference/login/src/connections.cc::PrepareXTEAResponse`, `SendXTEAResponse`, `ProcessLoginRequest`
- Game key tools `reference/game/tools/genpem.go::GenerateDefaultKey` and `pubkey.go`
- official IP Changer `reference/ipchanger/ipchanger.cc::ChangeIP`, 7.72 modulus patch surface and public sample modulus
- every relevant Game/Login `TIBIA772` guard

No file under `reference/` was modified.

## Source findings

Game requires `RSA_size == 128`; both Game and Login use a private 128-byte operation with `RSA_NO_PADDING`. Both require decrypted byte zero to be zero and then read four consecutive LE XTEA words. Selected Fusion32 key tooling fixes exponent 65,537. The classic 7.72 patch accepts/patches only a decimal modulus. Because classic client source is unavailable, the exponent conclusion is a high-confidence cross-source inference, also consistent with the already passed live classic path.

Game and Login XTEA implementations agree on two LE 32-bit halves, four LE key words, delta `0x9E3779B9` and 32 rounds. Encrypted payload is two-byte LE message length, message, and padding to an eight-byte boundary. Game input requires nonempty inner length fitting inside the decrypted block. It does not enforce a maximum padding count after decryption, so all remaining bytes must be surfaced rather than silently dropped.

`TIBIA772` changes accepted client version and relocates Game terminal type/version outside the RSA block. It does not change RSA, XTEA, inner length or padding.

Server output uses `rand_r` padding bytes but never interprets their values. Client use of an OS CSPRNG is an implementation security choice that preserves the source-derived wire layout.

## Implementation

Added the separate `protocol772_crypto` CMake target and API:

- `Rsa1024PublicKey`: decimal/big-endian public modulus, fixed exponent 65,537, raw 128-byte big-endian modular exponentiation through OpenSSL BIGNUM, range validation and source-required leading-zero protocol helper.
- No private-key loading, storage or operation exists in Client Core.
- `XteaKey`: move-only four-word key, explicit initialized state, LE serialization, overwrite on destruction and move-source invalidation.
- secure generation through Linux `getrandom` or Windows `BCryptGenRandom`.
- exact XTEA block encode/decode for any positive eight-byte multiple.
- `EncryptXteaPayload`: inner length, plaintext, 0–7 random padding bytes, encryption and owned `FramedPacket` output.
- `DecryptXteaPayload`: exact ciphertext preservation, decrypted copy, explicit message/padding and malformed-result bytes.
- `CryptoError`: distinct configuration, key, random, block, inner-length, modulus, RSA range/leading-byte and backend errors.
- callbacks that return false or throw become explicit random-generation failures; failed plaintext assembly is overwritten.

Crypto is independent of sockets, Unreal, gameplay and protocol opcodes. The integration fixture proves that framing adds the outer header only after Crypto and that decoding returns the same complete encrypted payload to Crypto.

## Golden fixtures

`clientcore/tests/fixtures/crypto_772_vectors.h` contains only public/synthetic data:

- the already-public 1024-bit sample modulus present in selected IP Changer source;
- synthetic RSA plaintext bytes and raw-RSA ciphertext;
- synthetic XTEA words, LE bytes, block cipher and inner-packet cipher.

Expected RSA was independently calculated with Python integer `pow(message, 65537, modulus)` using the exact five decimal source fragments. XTEA was independently calculated from the selected source equations.

The first external RSA calculation accidentally used an incomplete manual modulus transcription and produced a failing fixture. The test rejected it. Recalculation using all five exact source fragments produced a 309-digit/1024-bit modulus and matched C++/OpenSSL byte-for-byte; the erroneous expected value was removed before PASS.

## Files changed

- `clientcore/CMakeLists.txt`
- `clientcore/README.md`
- `clientcore/include/fusion32/protocol772/crypto.h`
- `clientcore/src/crypto.cpp`
- `clientcore/tests/crypto_tests.cpp`
- `clientcore/tests/fixtures/crypto_772_vectors.h`
- `docs/protocol772/CRYPTO.md`
- `docs/protocol772/TRANSPORT.md`
- `docs/protocol772/SOURCE_TRUTH.md`
- `evidence/clientcore/CLIENTCORE-CRYPTO-772-001.md`
- `PROJECT_STATUS.md`
- `ARCHITECTURE.md`
- `ROADMAP.md`
- `PARITY_MATRIX.md`
- `tests/README.md`
- `handoffs/archive/2026-09-15_CLIENTCORE-TRANSPORT-772-001.md`
- `handoffs/CURRENT.md`

## Tests and results

Validated environment: Ubuntu 26.04 under WSL2, Linux x86_64, CMake `4.2.3`, GCC/G++ `15.2.0`, OpenSSL `3.5.5`.

- Debug configure/build, C++17, `-Wall -Wextra -Wpedantic -Werror`: `PASS`.
- Normal CTest: `2/2 PASS`.
- Crypto direct summary: `25/25 PASS`.
- Retained Transport direct suite: `20/20 PASS` through CTest.
- Separate AddressSanitizer + UndefinedBehaviorSanitizer configure/build: `PASS`.
- Sanitized CTest: `2/2 PASS`; no sanitizer diagnostic.

Positive cases include key endian/generation/move lifecycle, golden one/multiblock XTEA, golden inner packet, exact 2,046-to-2,048 boundary, public modulus parsing, exponent, golden raw RSA and complete Framing-to-Crypto roundtrip.

Negative cases include uninitialized/null state, invalid block sizes, empty/oversized plaintext, endpoint overflow, random failure/exception, malformed ciphertext, zero/overrun inner lengths, malformed/short/large/even RSA moduli, invalid RSA leading byte and RSA message range. Ciphertext/decrypted bytes are retained according to the stage reached.

No live test was run. It is not a substitute for deterministic byte fixtures and application Login is outside this task.

## Security and repository checks

- No runtime modulus, generated key, password, account or credential was used or recorded.
- No RSA private key or private operation exists in new code.
- The fixture modulus is public and already tracked in canonical IP Changer source.
- staged secret-marker scan found no PEM/private-exponent/password assignment.
- no tracked `.pem`, `.key`, `.pfx` or `.p12` file exists.
- `reference/` status/diff is empty.
- builds remain outside the repository under WSL `/tmp`.

## Evidence and status

Primary evidence:

- `evidence/clientcore/CLIENTCORE-CRYPTO-772-001.md`
- `docs/protocol772/CRYPTO.md`
- `clientcore/tests/fixtures/crypto_772_vectors.h`

`CLIENTCORE-CRYPTO-772-001 = PASS`. This is a bounded executed-test result, not `CERTIFIED`, application Login, world entry or parity evidence.

## Remaining UNVERIFIED work

- native Windows/MSVC/OpenSSL build and `BCryptGenRandom` execution;
- live new-client RSA/XTEA exchange;
- exact application Login request/response serialization;
- exact Game Login request serialization, including 7.72 terminal-field placement;
- source-justified unused RSA plaintext tail policy in the future application encoder;
- opcodes, semantic events, WorldState and threading/event queues;
- all Unreal and 2D-to-3D parity;
- independent repetition of Crypto fixtures/tests;
- independent classic-client certification and historical provenance.

## Blockers

None for `CLIENTCORE-LOGIN-772-001`. Native Windows verification is recommended before integration shipping but does not block deterministic Login implementation.

## Risks

- The Game source identifies itself as a manual decompilation with changes; continue exact symbol/fixture traceability.
- Classic client source is unavailable, so exponent evidence is cross-source rather than direct client-code evidence.
- OpenSSL `libcrypto` is now a Client Core build dependency for public BIGNUM RSA.
- Do not invent RSA unused-tail bytes when implementing Login; trace what the server consumes and mark any client-only filler assumption explicitly.
- A Crypto PASS does not prove authentication or gameplay.

## Exact next task

`CLIENTCORE-LOGIN-772-001`: implement only the source-traced character-list Login exchange using the existing Transport, Framing and Crypto layers. Build the exact 145-byte Login request and RSA plaintext fields justified by selected source, decode MOTD/error/character-list responses into typed Login results, and create golden request/response plus malformed-input tests before a bounded live smoke. Do not begin Game Login, world opcodes, WorldState or Unreal.

Inspect at minimum:

- `reference/login/src/connections.cc::ProcessLoginRequest`, `PrepareXTEAResponse`, `SendXTEAResponse`, `SendLoginError`, `SendCharacterList`
- `reference/login/src/query.cc` Login account/world response path
- `reference/login/src/common.hh` read/write buffers and string encodings
- `reference/game/src/communication.cc::HandleLogin` only to keep Game Login out of the Login task
- `clientcore/include/fusion32/protocol772/framing.h`
- `clientcore/include/fusion32/protocol772/crypto.h`
- `docs/protocol772/TRANSPORT.md`
- `docs/protocol772/CRYPTO.md`

Useful commands:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-crypto-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-crypto-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-crypto-build --output-on-failure
git diff --check
git status --short --branch
```

## Critical context

The RSA operation is raw and the application supplies an exact 128-byte block; Crypto only enforces the leading zero required by both servers. XTEA key bytes are four LE words. The encrypted inner length is not the outer frame length. Keep `Transport -> Framing -> Crypto -> Protocol772`; do not merge Login fields into Crypto or let malformed/unsupported application data corrupt stream synchronization.

Generated credentials, runtime modulus/private key, classic-client artifacts and build output remain ignored/untracked. Never print or commit them. Fusion32 remains authoritative and Unreal remains out of scope.
