# CLIENTCORE-GAMELOGIN-772-001

Status: `PASS`

## Implementation

Added `protocol772_gamelogin` behind the validated Transport, Crypto and Login
layers. It builds the source-traced Tibia 7.72 Game Login request, keeps a
persistent `GameLoginSession`, decrypts framed server output and recognizes
only the initial authentication messages needed for entry. Fullscreen/map
bytes and unknown opcodes remain preserved, not interpreted.

## Source traceability

See [`docs/protocol772/GAMELOGIN.md`](../../docs/protocol772/GAMELOGIN.md).
Primary symbols are `reference/game/src/communication.cc::HandleLogin`,
`reference/game/src/communication.cc::WriteToSocket/ReceiveCommand`,
`reference/game/src/connections.cc::JoinGame`,
`reference/game/src/receiving.cc::ReceiveData`,
`reference/game/src/sending.cc::SendInitGame`, `SendRights`, `SendFullScreen`,
and `reference/game/src/connections.hh::ClientCommand/ServerCommand`.

## Tests

Environment: Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5,
C++17, warnings as errors.

- Debug build: `PASS`
- CTest: `4/4 PASS` (Transport, Crypto, Login, Game Login)
- ASan/UBSan CTest: `4/4 PASS`
- Deterministic Game Login RSA/framing fixture: `PASS`
- Init/Rights/combined Fullscreen recognition fixtures: `PASS`
- Unknown and invalid-input preservation/negative cases: `PASS`

## Bounded live smoke

A temporary non-tracked harness read only the local synthetic `ACCOUNT_A`
credential and a temporary public modulus, connected to `127.0.0.1:7172`, sent
the Game Login request, observed `INIT_GAME` and `FULLSCREEN`, and kept the
socket connected for two seconds. Result:

`live_gamelogin_smoke: PASS init=1 fullscreen=1 persistent=1 rights_optional=0`

The server emitted no rights packet because the synthetic account had no
rights; this is source-defined optional behavior. Temporary harness, modulus
file and credentials were removed/not tracked. No secret appears in evidence.

## Scope exclusions

No WorldState, map/fullscreen decoding, movement, gameplay, command encoder or
Unreal integration was started.
