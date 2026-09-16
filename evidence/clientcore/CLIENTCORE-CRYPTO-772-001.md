# CLIENTCORE-CRYPTO-772-001

Status: `PASS`

Date: 2026-09-15

Implementation commit: recorded after the focused implementation commit is created

Authority: selected Fusion32 Game/Login/IP Changer source revisions inventoried in `SOURCE_MANIFEST.md`

## Objective and boundary

Implement the Unreal-independent crypto stage immediately behind `FramedPacket`: public RSA required by the selected 7.72 path, XTEA key generation/ownership, XTEA blocks, encrypted inner length/padding and deterministic byte fixtures. This evidence does not claim character-list Login, Game Login, opcodes, semantic events, WorldState, Unreal or gameplay parity.

## Implementation

- `Rsa1024PublicKey`: decimal or 128-byte big-endian public modulus, fixed exponent 65,537, raw 128-byte public operation and source-required leading-zero protocol validation.
- `XteaKey`: four LE words, move-only lifecycle, explicit initialization, serialization and overwrite-on-destruction/move.
- OS secure key/padding generation through Linux `getrandom` or Windows `BCryptGenRandom`.
- exact 32-round XTEA encrypt/decrypt for one or more eight-byte blocks.
- `EncryptXteaPayload`: inner LE length, payload, 0–7 random padding bytes and owned encrypted `FramedPacket`.
- `DecryptXteaPayload`: preserves exact ciphertext, decrypts a copy, validates inner length and exposes both message and padding.
- explicit `CryptoError` outcomes; no logging or silent remainder discard.
- separate `protocol772_crypto` CMake target; no Unreal dependency.

Full source symbols, version-condition findings and confidence are in `docs/protocol772/CRYPTO.md`.

## Source-derived findings

Game and Login both perform a 128-byte RSA private operation with `RSA_NO_PADDING`, verify plaintext byte zero, then read four LE XTEA words. The selected Fusion32 key generator fixes exponent 65,537; the 7.72 IP Changer patches a decimal modulus only. Game/Login XTEA implementations agree on LE halves/key words, 32 rounds and delta `0x9E3779B9`.

Both response paths construct `uint16 LE data_length + data + padding`, with the encrypted region rounded to an eight-byte multiple. Game receive rejects encrypted sizes not divisible by eight, inner length zero, or inner length extending beyond the decrypted block. No `TIBIA772` guard changes RSA/XTEA/framing; the Game guard only relocates terminal type/version around the RSA block.

Padding byte values themselves are opaque. The server uses `rand_r`; the client uses an OS CSPRNG without changing wire semantics.

## Golden fixtures

Fixture file: `clientcore/tests/fixtures/crypto_772_vectors.h`.

The modulus is the already-public sample in the selected IP Changer source, not a generated runtime modulus. RSA plaintext and every XTEA value are synthetic. Independent Python integer/XTEA calculations generated expected ciphertexts; C++/OpenSSL output must match exactly.

During development, the first external RSA calculation accidentally used an incompletely transcribed modulus and the golden test failed. Recalculation from the exact five source fragments established a 309-digit/1024-bit modulus and matched the C++ result byte-for-byte. The erroneous value was removed and never treated as PASS evidence.

## Build environment

- Ubuntu 26.04 under WSL2, Linux x86_64
- CMake `4.2.3`
- GCC/G++ `15.2.0`
- OpenSSL `3.5.5`, `libcrypto` public BIGNUM operation only
- C++17, `-Wall -Wextra -Wpedantic -Werror`
- separate ASan/UBSan build with frame pointers

## Commands and results

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-crypto-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-crypto-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-crypto-build --output-on-failure
wsl.exe -d Ubuntu-26.04 -- /tmp/fusion32-clientcore-crypto-build/protocol772_crypto_tests
```

Normal result: configure/build `PASS`; CTest `2/2 PASS`; Crypto executable `25/25 PASS`; existing Transport executable `20/20 PASS`.

Sanitizer result: separate `/tmp/fusion32-clientcore-crypto-sanitize` configure/build `PASS`; CTest `2/2 PASS`; no ASan/UBSan diagnostic.

## Positive coverage

- XTEA key LE serialization/reconstruction;
- deterministic injected and OS key generation;
- move construction/assignment invalidating the source key;
- single golden XTEA block and multiblock roundtrip;
- exact encrypted-packet golden encode/decode;
- message and padding extraction;
- no-padding path without random callback;
- 2,046-byte exact client-send boundary;
- RSA public-key parsing from decimal/big-endian forms;
- fixed exponent and golden raw-RSA output;
- complete outer framing encode/decode around the encrypted payload.

## Negative/error coverage

- uninitialized keys and null arguments;
- zero/nonmultiple XTEA block sizes;
- empty/oversized plaintext and invalid configured limit;
- encrypted output exceeding endpoint limit;
- random provider returning failure or throwing;
- empty/misaligned ciphertext with exact input preservation;
- zero and overrun inner lengths with decrypted bytes retained;
- malformed, short, oversized, leading-zero, nondigit and even RSA moduli;
- RSA plaintext leading-byte violation and integer not below modulus;
- uninitialized RSA key.

The source-permitted case where a valid inner length leaves more than seven trailing bytes is accepted and every trailing byte is returned as padding, matching the server's actual validation rather than inventing a stricter rule.

## Security and repository boundary

- No RSA private-key API or private material exists in Client Core.
- No runtime modulus, password, account, credential or generated XTEA key is recorded.
- The committed modulus is public fixture data already present in canonical IP Changer source.
- XTEA key words are overwritten on destruction and when moved from.
- Temporary random key bytes and failed plaintext assembly buffers are overwritten.
- Crypto never logs keys, plaintext, ciphertext or credentials.
- No file under `reference/` is modified and all build output is outside Git in WSL `/tmp`.

## Limitations and unverified assumptions

- Native Windows/MSVC/OpenSSL linkage and `BCryptGenRandom` execution are `UNVERIFIED`.
- No live new-client RSA/XTEA exchange was run; deterministic fixtures are the required evidence for this task.
- The fixed exponent conclusion is strongly supported by selected Fusion32 key tooling, runtime generation and modulus-only classic patching, but classic client source is unavailable.
- Login/Game Login-specific plaintext field serialization and unused RSA-tail content remain intentionally unimplemented.
- OpenSSL remains a build dependency for public big-number modular exponentiation.
- Independent reproduction remains pending; this task is not `CERTIFIED`.

## Result

`CLIENTCORE-CRYPTO-772-001 = PASS`: implementation and source trace exist; golden, negative and boundary tests pass in normal and sanitized builds; prior Transport tests remain green; no secret/reference mutation is present. The result is bounded to Crypto and is not Login or parity evidence.

Next bounded task: `CLIENTCORE-LOGIN-772-001`.
