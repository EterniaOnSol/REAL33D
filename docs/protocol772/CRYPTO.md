# Protocol772Core crypto

Task: `CLIENTCORE-CRYPTO-772-001`

Status: `PASS`

Scope: Unreal-independent 1024-bit RSA public operation, XTEA key lifecycle, XTEA block processing, encrypted inner-packet construction/validation and deterministic byte fixtures. Login fields, Game Login, opcodes, semantic events, WorldState and Unreal are deliberately absent.

## Layer boundary

```text
TCP byte stream
    -> TcpTransport
    -> FrameDecoder / EncodeFrame
    -> FramedPacket (complete outer payload)
    -> RSA/XTEA crypto
    -> plaintext message + explicit padding
    -> future Protocol772 decoder
```

Crypto never reads from a socket and never adds or removes the outer two-byte frame header. XTEA encryption produces a `FramedPacket`; framing subsequently adds the outer header. XTEA decryption accepts one complete `FramedPacket`, preserves its exact ciphertext in the result, produces a separate decrypted block, and exposes message and padding independently. A malformed encrypted packet returns an explicit error without mutating the input packet.

## Canonical source trace

All server paths below are in the selected immutable `reference/` trees. No rule is taken from OpenTibia or the historical Unreal experiments.

| Component | Source / symbol | Wire or crypto rule | `TIBIA772` effect | Confidence |
| --- | --- | --- | --- | --- |
| Game RSA size | `reference/game/src/crypto.cc::TRSAPrivateKey::initFromFile`, lines 30-54 | Server requires `RSA_size == 128`, conventionally a 1024-bit key | None | High |
| Game RSA operation | `reference/game/src/crypto.cc::TRSAPrivateKey::decrypt`, lines 57-71 | Exactly 128 bytes, `RSA_NO_PADDING` | None | High |
| Login RSA operation | `reference/login/src/crypto.cc::RSADecrypt`, lines 48-65 | Input size must equal key size; private operation uses `RSA_NO_PADDING` | None | High |
| RSA plaintext prefix | Game `communication.cc::HandleLogin`, lines 934-948; Login `connections.cc::ProcessLoginRequest`, lines 602-626 | Decrypted block must start with zero, followed by four XTEA words | Game moves terminal fields outside this block under `TIBIA772`; prefix/key layout is unchanged | High |
| Public exponent | `reference/game/tools/genpem.go::GenerateDefaultKey`, line 27; `tools/pubkey.go`, line 33 | Selected Fusion32 key tooling fixes/prints exponent 65,537 | None | High for selected/generated Fusion32 keys |
| Client public-key configuration | `reference/ipchanger/ipchanger.cc::ChangeIP`, lines 140-236 and 398-407 | 7.72 patch surface accepts a decimal modulus and patches only the modulus field | 7.72 selects its own modulus address, not a different exponent/algorithm | High; exponent conclusion is a cross-source inference because classic client source is unavailable |
| Game XTEA key | `reference/game/src/crypto.cc::TXTEASymmetricKey::init`, lines 76-80 | Four consecutive LE `uint32` words | None | High |
| Game XTEA primitive | `TXTEASymmetricKey::encrypt/decrypt`, lines 83-113 | Two LE `uint32` halves, delta `0x9E3779B9`, 32 rounds | None | High |
| Login XTEA primitive | `reference/login/src/crypto.cc::XTEAEncrypt/XTEADecrypt`, lines 68-102; `common.hh::BufferRead32LE/BufferWrite32LE` | Confirms block/key endian and the same 32-round algorithm | None | High |
| Encrypted output layout | Game `communication.cc::GetPacketSize/WriteToSocket`, lines 250-292; Login `connections.cc::PrepareXTEAResponse/SendXTEAResponse`, lines 503-548 | Encrypted bytes are `uint16 LE message_size + message + padding`; total is rounded to an 8-byte boundary | None | High |
| Encrypted input validation | Game `communication.cc::ReceiveCommand`, lines 1258-1278 | Outer payload must be divisible by eight; decrypted inner length must be nonzero and `length + 2 <= block size` | None | High |

The server sources use `rand_r` for output padding but never interpret padding values. The new client uses OS CSPRNG bytes (`getrandom` on Linux, `BCryptGenRandom` on Windows). That is a client-side security choice; the source-derived wire rule is only the padding count needed for eight-byte alignment.

## RSA contract

- Public modulus input may be decimal, matching the Fusion32 IP Changer/runtime representation, or exactly 128 big-endian bytes.
- Validation follows the server's key-width contract: the modulus occupies exactly 128 bytes and is odd. No runtime modulus is compiled into the library.
- Public exponent is 65,537.
- Plaintext and ciphertext are exactly 128 bytes and interpreted as big-endian integers.
- The operation is raw modular exponentiation with no PKCS#1 encryption padding, matching `RSA_NO_PADDING`.
- The protocol helper additionally requires plaintext byte zero to be `0x00`, as both Game and Login do after private decryption.
- The plaintext integer must be smaller than the modulus; failure is explicit.
- Crypto does not invent or serialize account, password, character, GM flag or other Login/Game Login fields. The future protocol encoder must supply the complete 128-byte plaintext block, including any source-justified unused tail.

OpenSSL `libcrypto` is used only for public big-number modular exponentiation. The client component has no API for loading, storing or applying an RSA private key.

## XTEA contract

- Key size: 16 bytes, represented on wire as four LE words.
- Key generation: 16 bytes from the OS cryptographic random source.
- Key ownership: move-only; destruction and move-source invalidation overwrite the stored words.
- Block size: exactly eight bytes; nonmultiples and empty block calls are rejected rather than partially processed.
- Rounds: 32; delta `0x9E3779B9`; data halves and key words are LE.
- Encode: reject empty/oversized plaintext, write two-byte LE length, append plaintext, add 0–7 random bytes, encrypt every block, then return only the outer payload as `FramedPacket`.
- Decode: retain exact input ciphertext, require a nonempty multiple of eight, decrypt a copy, validate the inner length, and return the complete decrypted block plus separate message/padding vectors.
- Source-exact receive behavior does not impose a maximum padding count after decryption; any bytes remaining after a valid inner length are surfaced as padding rather than silently discarded.

At the established client send limit, 2,046 plaintext bytes become exactly 2,048 encrypted bytes; 2,047 would require 2,056 and is rejected.

## Explicit errors

`CryptoError` distinguishes invalid arguments/configuration, uninitialized keys, random-source failure, invalid block sizes, empty/oversized plaintext, output-limit violation, zero/overrunning inner length, invalid modulus/leading byte, RSA message range and backend failure. Injected random providers that return failure or throw are converted to `RandomGenerationFailed`; partial plaintext buffers are overwritten before returning.

## Golden fixtures

`clientcore/tests/fixtures/crypto_772_vectors.h` contains only public/synthetic test data:

- the public sample modulus already tracked by `reference/ipchanger/ipchanger.cc::CreateSampleServerList`;
- synthetic RSA plaintext bytes `00..7F` and independently calculated raw-RSA ciphertext;
- synthetic XTEA words, LE serialization, one-block ciphertext and an encrypted inner-packet vector.

The RSA expected value was independently calculated as `pow(message, 65537, modulus)` using the exact five decimal source fragments. The XTEA expected values were independently calculated from the selected source equations. Tests compare every byte; no private key, runtime modulus, account or password is present.

## Validation and limitations

Normal and AddressSanitizer/UndefinedBehaviorSanitizer builds pass all 25 Crypto cases and retain all 20 Transport cases. They cover golden RSA/XTEA values, LE key serialization, deterministic and system key generation, move/erase lifecycle, exact limits, padding/no-padding, callback failures, malformed lengths, ciphertext preservation, raw RSA range checks and the full Framing-to-Crypto boundary.

Not yet verified:

- native Windows/MSVC compilation and `BCryptGenRandom` execution;
- a live Login/Game RSA/XTEA exchange by the new client;
- application-specific construction of Login or Game Login RSA plaintext;
- independent reproduction of the fixtures/tests;
- future protocol decode outcomes and semantic event queue.
