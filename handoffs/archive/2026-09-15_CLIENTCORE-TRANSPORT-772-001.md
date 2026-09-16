# HANDOFF

Date/time: 2026-09-15T21:29:57-06:00
Agent: Codex
Role: PROTOCOL772CORE TRANSPORT IMPLEMENTATION
Branch: `main`
Starting commit: `0119f589d83d5a5decd5001fd13f84b8ed0ff016`
Implementation commit: `abd2d0a25bd9632f5aa3955e822876268c7ca96c`
Ending metadata commit: repository HEAD after this handoff is committed
Worktree: expected clean after the final metadata commit

## Objective

Close the already verified classic-client documentation/evidence safely, then implement `CLIENTCORE-TRANSPORT-772-001`: the smallest deterministic, Unreal-independent TCP lifecycle and source-traced outer-framing layer required by a real Fusion32 Tibia 7.72 client. Do not implement RSA/XTEA, Login payloads, full protocol decoding, WorldState or Unreal.

## Starting state and classic baseline closeout

The classic closeout began from the earlier evidence work and produced review commit `f65f3a7645ff40b39b7cc8399760fd4f0b69ecee` plus metadata commit `0119f589d83d5a5decd5001fd13f84b8ed0ff016`. Before Transport began, the worktree was clean. No remote exists and nothing was pushed.

The exact classic EXE/DAT/SPR/PIC verifier passed again. The selected client remains functionally compatible: IP Changer patch, Login, character list, Game entry, initial world and a session over 30 minutes are `PASS`. `CLASSIC-CLIENT-772-001` remains `IN_PROGRESS`, historical provenance remains `UNKNOWN`, and independent fresh-runtime/RSA repetition with sanitized screenshots remains `NOT_STARTED`. Those certification items do not block Client Core development.

The prior substantive handoff was archived unchanged as `handoffs/archive/2026-09-15_CLASSIC-CLIENT-772-001.md`.

## Required startup and authoritative inspection

Read `AGENTS.md`, all required project-memory documents, classic evidence/runtime classification, protocol source truth and the prior handoff. Git was present on `main` at `0119f589...`, with a clean worktree and no remotes.

Inspected the exact authoritative transport/framing symbols:

- Game `reference/game/src/communication.cc::GetPacketSize`, `WriteToSocket`, `SendData`, `ReadFromSocket`, `ReceiveCommand`, `HandleLogin`
- Game `reference/game/src/connections.hh::TConnection::InData`, `OutData`
- Game `reference/game/src/sending.cc` ring-buffer capacity enforcement
- Login `reference/login/src/common.hh::BufferRead16LE`, `TConnection::Buffer`
- Login `reference/login/src/connections.cc::CheckConnectionInput`, `PrepareXTEAResponse`, `SendXTEAResponse`, `TERMINALVERSION`
- complete relevant `TIBIA772` guards in selected Game/Login source

Canonical `reference/` content was read only and remains byte-unmodified by this work.

## Source findings

Both Tibia-facing endpoints use a two-byte little-endian outer length that excludes the header. Zero is invalid. Login incrementally reads header/payload and caps client payload at 2,048 bytes. Game caps client payload at `InData[2048]`.

Login responses use a 2,048-byte total buffer including the outer header, yielding a client receive outer-payload maximum of 2,046 bytes. Game's `OutData[16384]` plus `GetPacketSize` padding yields 16,394 total wire bytes and therefore a client receive outer-payload maximum of 16,392 bytes. Separate direction-aware profiles preserve this distinction.

`TIBIA772` changes the accepted version and moves Game terminal type/version outside the RSA block; it does not change outer framing. After Game login, the same outer frame carries XTEA blocks and the decrypted content starts with a second LE length. That inner validation remains the next crypto layer, not Transport.

## Implementation

Added `clientcore/`, a C++17/CMake component with no Unreal or third-party dependency:

- `TcpTransport`: move-only socket ownership; bounded connect; explicit lifecycle; partial reads; full-write loop; timeouts, refusal, FIN and reconnect behavior; Windows/POSIX paths.
- `FrameDecoder`: incremental header/payload buffering, ordered multi-frame extraction, explicit malformed/truncated/finished errors and preservation of unconsumed bytes.
- `EncodeFrame`: endpoint/16-bit validation and exact LE header generation.
- `FramedConnection`: composition boundary yielding owned outer payloads to the future `CryptoStage`.
- deterministic local loopback harness and 20 focused tests.

Transport contains no opcode behavior, logical coordinates, gameplay authority, Actor mutation or Unreal dependency. It cannot discard a future packet remainder merely because an opcode is unsupported.

## Files changed

- `clientcore/CMakeLists.txt`
- `clientcore/README.md`
- `clientcore/include/fusion32/protocol772/framing.h`
- `clientcore/include/fusion32/protocol772/tcp_transport.h`
- `clientcore/include/fusion32/protocol772/framed_connection.h`
- `clientcore/src/framing.cpp`
- `clientcore/src/tcp_transport.cpp`
- `clientcore/src/framed_connection.cpp`
- `clientcore/tests/transport_tests.cpp`
- `docs/protocol772/TRANSPORT.md`
- `evidence/clientcore/CLIENTCORE-TRANSPORT-772-001.md`
- `PROJECT_STATUS.md`
- `ARCHITECTURE.md`
- `ROADMAP.md`
- `PARITY_MATRIX.md`
- `docs/protocol772/SOURCE_TRUTH.md`
- `tests/README.md`
- `handoffs/archive/2026-09-15_CLASSIC-CLIENT-772-001.md`
- `handoffs/CURRENT.md`

## Tests and results

Validated environment: Ubuntu 26.04 under WSL2, Linux x86_64 kernel `6.18.33.2-microsoft-standard-WSL2`, CMake `4.2.3`, GCC/G++ `15.2.0`.

- Debug configure/build with C++17 and `-Wall -Wextra -Wpedantic -Werror`: `PASS`.
- Normal CTest: `100% tests passed, 0 tests failed`.
- Direct case summary: `passed=20 failed=0 total=20`.
- Separate AddressSanitizer + UndefinedBehaviorSanitizer build: `PASS`; all 20 cases passed with no sanitizer diagnostic.
- `git diff --check`: `PASS` after removing Markdown trailing whitespace.
- staged secret-pattern scan: no credential/private-key assignment or PEM marker.
- tracked key-file check: no tracked `.pem`, `.key`, `.pfx` or `.p12` file.
- `reference/` status and diff: empty.

The first compile stopped on one warnings-as-errors diagnostic for a platform-specific test initializer. It was marked `[[maybe_unused]]` and both complete build variants and all tests were rerun.

Cases cover partial header/payload, arbitrary splits, multiple frames, complete-plus-partial, extraction order, buffer preservation, zero/oversize/16-bit-invalid lengths, exact endpoint-profile boundaries, write framing, EOF/truncation, clean/refused/remote lifecycle, loopback I/O and reconnect.

No live Fusion32 smoke was run. It is optional and would not replace deterministic tests. No malformed traffic was sent to Fusion32 and no credentials were used.

## Evidence and status

Primary evidence:

- `evidence/clientcore/CLIENTCORE-TRANSPORT-772-001.md`
- `docs/protocol772/TRANSPORT.md`

`CLIENTCORE-TRANSPORT-772-001 = PASS`. This means the named normal and sanitized deterministic tests were executed and met their expected results. It is not `CERTIFIED`; independent reproduction and native Windows execution are still open.

## Remaining UNVERIFIED work

- native Windows/MSVC compile and loopback test execution;
- optional live Fusion32 transport-only smoke;
- fault injection for a socket failure after a partial write;
- one wall-clock connect deadline spanning every resolved address rather than the current per-candidate timeout;
- RSA public-block construction and XTEA fixtures;
- Login, character list, Game login, opcode decoding, semantic events and WorldState;
- network-thread to consumer/game-thread event-queue implementation;
- all Unreal integration and 2D-to-3D parity;
- independent classic-client certification repetition and historical provenance.

## Blockers

None for `CLIENTCORE-CRYPTO-772-001`. Classic independent certification and client provenance remain separate concerns and do not block Client Core.

## Risks

- The canonical Game source describes itself as manual decompilation with changes; continue citing exact symbols and validate every crypto rule with fixtures.
- Native Windows behavior is implemented but not yet built in this environment.
- Endpoint maximums are source-derived bounds; a live maximum-size Game response was not observed.
- The optional `ALLOW_LOCAL_PROXY` preambles are deployment behavior and intentionally outside direct-client framing.
- The classic client's successful session does not prove the new client or Unreal parity.

## Exact next task

`CLIENTCORE-CRYPTO-772-001`: implement only the source-traced 1024-bit RSA public-block construction and XTEA block processing behind `FramedPacket`, including inner-length/padding validation where the selected source requires it. Create deterministic golden byte fixtures and negative tests before any live Login work. Do not begin Login payloads, Game Login, full opcodes or Unreal.

Inspect at minimum:

- `reference/game/src/crypto.cc` / `crypto.hh`: `TRSAPrivateKey`, `TXTEASymmetricKey`
- `reference/game/src/communication.cc::HandleLogin`, `WriteToSocket`, encrypted branch of `ReceiveCommand`
- `reference/login/src/crypto.cc` / `crypto.hh`: RSA and XTEA helpers
- `reference/login/src/connections.cc::ProcessLoginRequest`, `SendXTEAResponse`
- `clientcore/include/fusion32/protocol772/framing.h`
- `clientcore/include/fusion32/protocol772/framed_connection.h`
- `docs/protocol772/TRANSPORT.md`

Useful verification commands:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-transport-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-transport-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-transport-build --output-on-failure
git diff --check
git status --short --branch
```

## Critical context

Do not collapse the separate Login/Game direction limits into a guessed universal packet maximum. Outer framing and the encrypted inner message length are distinct layers. An owned `FramedPacket` must reach Crypto intact; an unsupported future opcode must not alter TCP synchronization. Fusion32 remains authoritative, and deleting any future 3D presentation/cache must never lose gameplay state.

Generated credentials, RSA private keys, classic-client binaries/data, runtime logs and build products remain ignored/untracked. Never print or commit them. The exact classic artifact hashes in existing evidence identify its passed set; provenance remains `UNKNOWN`.
