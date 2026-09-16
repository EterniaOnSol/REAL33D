# Protocol772Core transport

This directory contains the Unreal-independent transport and outer packet-framing foundation for the new Fusion32 Tibia 7.72 client.

Build and test in the validated WSL toolchain:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-transport-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-transport-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-transport-build --output-on-failure
```

`TcpTransport` owns only TCP lifecycle and byte I/O. `FrameDecoder` and `EncodeFrame` own the source-traced two-byte outer framing. `FramedConnection` composes those layers and returns owned `FramedPacket` payloads. `protocol772_crypto` accepts those packets and owns RSA public operations, XTEA keys/blocks and encrypted inner length/padding. Protocol commands, semantic events, WorldState and Unreal are intentionally absent.

Source traceability and limits are documented in
[`docs/protocol772/TRANSPORT.md`](../docs/protocol772/TRANSPORT.md). Test
results and limitations are recorded in
[`CLIENTCORE-TRANSPORT-772-001.md`](../evidence/clientcore/CLIENTCORE-TRANSPORT-772-001.md).

Crypto source traceability is documented in
[`docs/protocol772/CRYPTO.md`](../docs/protocol772/CRYPTO.md), with executed
evidence in
[`CLIENTCORE-CRYPTO-772-001.md`](../evidence/clientcore/CLIENTCORE-CRYPTO-772-001.md).

The source-traced Login/Character List layer is documented in
[`docs/protocol772/LOGIN.md`](../docs/protocol772/LOGIN.md), with evidence in
[`CLIENTCORE-LOGIN-772-001.md`](../evidence/clientcore/CLIENTCORE-LOGIN-772-001.md).
It deliberately stops before Game Login and world protocol processing.

Game Login, the persistent session and the initial authentication messages are
documented in [`docs/protocol772/GAMELOGIN.md`](../docs/protocol772/GAMELOGIN.md),
with evidence in
[`CLIENTCORE-GAMELOGIN-772-001.md`](../evidence/clientcore/CLIENTCORE-GAMELOGIN-772-001.md).

`protocol772_initial_world` decodes the `FULLSCREEN` world snapshot that follows
Game Login into a minimal `WorldState`, and names every other server command
without parsing it. Because the 7.72 item encoding depends on server object type
flags the wire omits, the decoder takes an explicit `ObjectTypeTable` loaded from
the server's `dat/objects.srv`. See
[`docs/protocol772/INITIAL_WORLD.md`](../docs/protocol772/INITIAL_WORLD.md) and
[`INITIALWORLD-772-001.md`](../evidence/clientcore/INITIALWORLD-772-001.md).
The invariants that table relies on are checked by
[`tests/verify_object_type_invariants.py`](../tests/verify_object_type_invariants.py).
