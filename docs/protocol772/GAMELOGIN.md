# Protocol772Core Game Login

Task: `CLIENTCORE-GAMELOGIN-772-001`

Status: `PASS` for deterministic fixtures and one bounded local synthetic-
account smoke. This task stops at authenticated session/initial message
recognition; it does not implement WorldState, movement, gameplay or Unreal.

## Boundary

```text
Character List endpoint -> TcpTransport :7172 -> FrameDecoder
  -> RSA/XTEA -> initial Game messages -> future Protocol772/WorldState
```

`GameLoginSession` owns the persistent socket, XTEA key and pending framed
packets. It never creates Actors or applies gameplay state.

## Source truth

| Rule | Source symbol | Interpretation | Confidence |
| --- | --- | --- | --- |
| Request command | `reference/game/src/connections.hh::CL_CMD_LOGIN_REQUEST` | payload opcode `10` | High |
| 7.72 terminal placement | `reference/game/src/communication.cc::HandleLogin`, `TIBIA772` lines 920-927 | terminal type/version are LE words before RSA ciphertext | High |
| RSA plaintext | `HandleLogin` lines 934-956 | zero, four LE XTEA words, GM byte, LE account ID, character/password strings, unused tail | High |
| Terminal acceptance | `communication.cc::TERMINALVERSION` and `HandleLogin` lines 970-976 | terminal types 0..2 accept version 772; `JoinGame` accepts types 1/2 | High |
| Login handoff | `communication.cc::HandleLogin`, `connections.cc::JoinGame`, `receiving.cc::ReceiveData` | server authenticates character, rewrites internal state, then expects client command 11 | High |
| Initial messages | `crplayer.cc` login path and `sending.cc::SendInitGame/SendRights/SendFullScreen` | init command 10, optional rights command 11 (32 bytes), then fullscreen command 100 | High |
| Server command values | `reference/game/src/connections.hh::ServerCommand` | 10 init, 11 rights, 20-22 login notices, 30 ping, 100 fullscreen | High |
| Encrypted framing | `communication.cc::WriteToSocket/ReceiveCommand` | outer LE size, encrypted inner LE data size, XTEA blocks and padding | High |

`SendInitGame`, `SendRights` and `SendFullScreen` append to the server output
ring and may arrive in one encrypted frame. Rights are optional: the source
only emits the rights command when at least one right/action exists.

## Request

`BuildGameLoginRequest` constructs a 133-byte payload: opcode 10, terminal
type/version LE, and 128-byte raw RSA ciphertext. The decrypted RSA block is
zero, the generated XTEA key, GM flag, account ID, character name and password
as source-traced length-prefixed strings. The existing frame encoder adds the
two-byte outer size.

## Initial message parser

`ParseGameInitialMessage` decodes `INIT_GAME` (creature ID, beat and bug-report
flag) and `RIGHTS` (32 action bytes). If the same frame continues with
`FULLSCREEN`, it marks it `FullScreenUnparsed` and preserves the entire tail;
the map payload is intentionally not decoded. Login errors, premium notices,
waiting-list notices and ping are parsed only as bounded initial records.
Unknown and malformed data remains visible in `unparsed_bytes` or explicit
status values; it is never silently discarded.

## Verification

`clientcore/tests/gamelogin_tests.cpp` covers exact deterministic RSA request
bytes, outer framing, init/rights records, combined init+fullscreen output,
unknown-opcode preservation and invalid terminal/name cases. Debug and
ASan/UBSan CTest suites pass 4/4, including retained Transport/Crypto/Login
tests.

The bounded live harness used the local synthetic `ACCOUNT_A` only. It entered
Game on `127.0.0.1:7172`, observed `INIT_GAME` and `FULLSCREEN` in a persistent
connection for two seconds, and passed. No credentials, runtime modulus or
private key were recorded.
