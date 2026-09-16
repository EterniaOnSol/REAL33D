# Protocol772Core Login / Character List

Task: `CLIENTCORE-LOGIN-772-001`

Status: `PASS` for deterministic fixtures and one bounded local synthetic-
account Login smoke. This does not change the status of the classic client
baseline or imply Game Login support.

## Boundary

```text
TcpTransport -> FrameDecoder -> FramedPacket -> Crypto -> Login parser
```

This component only performs the Login endpoint exchange through the
character list. It does not implement Game Login, opcodes after the list,
WorldState or Unreal.

## Source truth

All protocol-specific rules below come from the selected Fusion32 Login
source. No OpenTibia or experimental Unreal source is used.

| Rule | Source symbol | Interpretation | Confidence |
| --- | --- | --- | --- |
| Outer request size | `reference/login/src/connections.cc::CheckConnectionInput` and `ProcessLoginRequest` | LE `uint16` payload size followed by exactly 145 payload bytes | High |
| Request fields | `connections.cc::ProcessLoginRequest` | opcode `1`, terminal type/version LE, three LE signature `uint32`s, 128-byte RSA ciphertext | High |
| Terminal version | `connections.cc::TERMINALVERSION` under `TIBIA772` | terminal types 0, 1 and 2 accept version 772 | High |
| RSA plaintext | `connections.cc::ProcessLoginRequest` | zero byte, four LE XTEA words, LE account ID, length-prefixed password; remaining bytes are not interpreted by server | High |
| String encoding | `reference/login/src/common.hh::TReadBuffer::ReadString` and `TWriteBuffer::WriteString` | LE U16 byte length; `0xFFFF` introduces LE U32 length; raw bytes | High |
| Response framing | `connections.cc::PrepareXTEAResponse` / `SendXTEAResponse` | encrypted payload starts with inner LE message length, then message and 8-byte padding; outer LE size excludes its header | High |
| Error/MOTD/list | `connections.cc::SendLoginError` / `SendCharacterList` | opcodes 10, 20 and 100; list count U8, name/world strings, IPv4 U32 BE, port/premium U16 LE | High |
| Account query | `reference/login/src/query.cc::LoginAccount` | Login forwards account/password/IP to Query Manager query 11 and receives endpoint/name/premium fields | High |

`TIBIA772` changes the terminal-version gate for Login. The Game source's
different terminal-field placement is intentionally not implemented here.

## Request implementation

`BuildLoginRequest` generates a move-owned XTEA key, constructs the exact
128-byte RSA plaintext, performs the existing raw RSA public operation, and
returns both the 145-byte payload and its outer-framed wire bytes. Passwords
longer than 29 bytes are rejected because the authoritative server reads into
a 30-byte destination and only accepts lengths below that capacity. Filler
bytes are generated through the injected CSPRNG callback (OS CSPRNG by
default), and are never treated as protocol fields.

## Response safety

`ParseLoginResponse` accepts the already-decrypted message from Crypto and
returns typed characters/endpoints. It recognizes MOTD, error and character
list records. Truncated length-prefixed strings/records return `Incomplete`;
unknown opcodes return `Unsupported` and preserve the bytes beginning at that
opcode in `remaining_bytes`; server errors return `ProtocolViolation` with
the exact message. No branch silently discards an unknown or incomplete tail.

## Verification

`clientcore/tests/login_tests.cpp` contains a byte-for-byte deterministic RSA
request fixture, an outer-framing round trip, a deterministic MOTD/character
list fixture, unknown-opcode preservation, incomplete-record handling and
password validation. It runs with the retained Transport and Crypto tests.

Limitations: native Windows execution and independent reproduction remain
unverified. The runtime's private key and credentials are not part of the
repository.
