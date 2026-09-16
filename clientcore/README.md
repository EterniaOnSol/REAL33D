# Protocol772Core transport

This directory contains the Unreal-independent transport and outer packet-framing foundation for the new Fusion32 Tibia 7.72 client.

Build and test in the validated WSL toolchain:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-transport-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-transport-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-transport-build --output-on-failure
```

`TcpTransport` owns only TCP lifecycle and byte I/O. `FrameDecoder` and `EncodeFrame` own the source-traced two-byte outer framing. `FramedConnection` composes those layers and returns owned `FramedPacket` payloads. Crypto, protocol commands, semantic events, WorldState and Unreal are intentionally absent.

Source traceability and limits are documented in
[`docs/protocol772/TRANSPORT.md`](../docs/protocol772/TRANSPORT.md). Test
results and limitations are recorded in
[`CLIENTCORE-TRANSPORT-772-001.md`](../evidence/clientcore/CLIENTCORE-TRANSPORT-772-001.md).
