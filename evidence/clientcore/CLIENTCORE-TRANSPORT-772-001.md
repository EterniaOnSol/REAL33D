# CLIENTCORE-TRANSPORT-772-001

Status: `PASS`

Date: 2026-09-15

Implementation commit: `abd2d0a25bd9632f5aa3955e822876268c7ca96c`

Authority: selected Fusion32 Game/Login source revisions inventoried in `SOURCE_MANIFEST.md`

## Objective and boundary

Implement the smallest Unreal-independent TCP and outer-framing foundation for the new Tibia 7.72 client. This evidence does not claim RSA/XTEA, Login, Game Login, opcode decoding, WorldState, Unreal integration, gameplay parity or certification.

## Implementation

- `TcpTransport`: move-only socket ownership, bounded connect, explicit lifecycle, partial-read delivery, complete-write loop, timeout/refusal/remote-close reporting and reconnect support.
- `FrameDecoder`: incremental two-byte LE framing across arbitrary TCP segmentation; ordered extraction of multiple frames; preservation of incomplete and malformed input.
- `EncodeFrame`: validates endpoint limit and the 16-bit wire limit, then writes the source-derived outer header.
- `FramedConnection`: composes transport and framing and leaves an owned `FramedPacket` for the future crypto stage.
- C++17/CMake, standard library plus OS socket APIs only; no Unreal dependency and no third-party dependency.

Detailed source symbols, line locations, calculations and confidence are recorded in `docs/protocol772/TRANSPORT.md`.

## Source-derived findings

Game `communication.cc::ReceiveCommand` and Login `connections.cc::CheckConnectionInput` agree on a two-byte little-endian outer length that excludes the header. Both reject zero. Game client input is capped by `InData[2048]`; Login client input is capped by `TConnection::Buffer[2048]`.

Game `GetPacketSize`, `WriteToSocket`, `SendData` and `OutData[16384]` yield a source-derived maximum client receive outer payload of 16,392 bytes. Login `PrepareXTEAResponse` and `SendXTEAResponse` use a 2,048-byte total response buffer, yielding a maximum client receive outer payload of 2,046 bytes. These are implemented as separate direction-aware profiles rather than one guessed global maximum.

The `TIBIA772` conditions alter accepted version and the Game login placement of terminal type/version. They do not alter this outer framing. Post-login XTEA block alignment and inner plaintext length were traced but left behind the future crypto boundary.

## Build environment

- Host: Windows workspace; build executed in Ubuntu 26.04 under WSL2
- Kernel: Linux `6.18.33.2-microsoft-standard-WSL2`, x86_64
- CMake: `4.2.3`
- Compiler: `c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0`
- Configuration: Debug, C++17, `-Wall -Wextra -Wpedantic -Werror`
- Sanitized configuration: AddressSanitizer plus UndefinedBehaviorSanitizer and frame pointers

## Commands and results

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-transport-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-transport-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-transport-build --output-on-failure
wsl.exe -d Ubuntu-26.04 -- /tmp/fusion32-clientcore-transport-build/protocol772_transport_tests
```

Final normal result: build succeeded with warnings treated as errors; CTest `100% tests passed, 0 tests failed`; executable summary `passed=20 failed=0 total=20`.

The first build correctly stopped on one GCC unused-variable warning for the Windows-only socket initializer in the cross-platform test harness. The declaration was made explicitly `[[maybe_unused]]`; the complete build and all tests were then rerun successfully.

Sanitizer commands used a separate `/tmp/fusion32-clientcore-transport-sanitize` build with `-fsanitize=address,undefined -fno-omit-frame-pointer`. Result: CTest `100% tests passed, 0 tests failed`; no sanitizer diagnostic.

## Deterministic cases

1. partial header;
2. partial payload;
3. frame split across several reads;
4. multiple frames and extraction order;
5. complete frame plus partial next frame;
6. zero length and preservation of later bytes;
7. declared size beyond configured maximum;
8. buffer preservation until completion;
9. correct little-endian write framing;
10. rejected zero/configuration/16-bit-overflow write sizes;
11. exact Login/Game endpoint profile boundaries and multi-byte header encoding;
12. truncated EOF and bytes after clean EOF;
13. successful local connection and clean local disconnect;
14. deterministic refused connection using a reserved unlistened loopback port;
15. clean remote disconnect;
16. loopback partial header and payload with explicit synchronization;
17. multiple frames in one loopback write;
18. loopback verification of exact write bytes;
19. reconnect lifecycle;
20. remote disconnect during a partial frame.

No test requires Internet access, Fusion32 services, credentials, the classic client or Unreal. No malformed traffic was sent to Fusion32.

## Negative/error coverage

Zero length, endpoint-size violation, 16-bit header overflow, incomplete EOF, input after EOF, refused connection, remote FIN and remote FIN during a partial frame are asserted. Offending/remaining bytes are preserved at the framing boundary. A partial write error closes the transport by contract, avoiding an ambiguous stream state; deterministic fault injection for that rare socket condition is not yet implemented.

## Security and repository checks

- No credentials, generated password, RSA private key, private modulus or runtime configuration is part of the implementation.
- No file under `reference/` is modified.
- Build products are outside the repository in WSL `/tmp`.
- The implementation does not log payloads or credentials.

## Limitations and unverified assumptions

- Native Windows/MSVC compilation and loopback execution are `UNVERIFIED`; the Windows socket path is implemented but only Linux/WSL was executed.
- Live Fusion32 smoke was not run because it is supplementary and would not replace deterministic framing tests.
- The Game maximum receive profile is a conservative selected-source derivation from the maximum ring-buffer contribution and padding formula; no live maximum-size packet was observed.
- Connect timeout currently applies per resolved address candidate, not as a single wall-clock deadline across all candidates.
- Optional local-proxy preambles are server deployment behavior and are outside normal direct-client transport scope.
- Crypto, protocol decode outcomes and thread/event queues are only preserved as architectural boundaries, not implemented.

## Result

`CLIENTCORE-TRANSPORT-772-001 = PASS`: implementation exists, source traceability is recorded, the deterministic and negative tests pass in normal and sanitized builds, no secret/reference mutation is present, and the task remains bounded. This is not `CERTIFIED`; independent reproduction and native Windows verification remain open.

Next bounded task: `CLIENTCORE-CRYPTO-772-001`.
