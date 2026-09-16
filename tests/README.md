# Tests

`verify_classic_client_772.py` is the first executable artifact verifier. It hashes the selected local EXE/DAT/SPR/PIC set and statically validates the Fusion32 IP Changer's PE version, endpoint ranges and RSA-modulus address without executing the client:

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_classic_client_772.py /mnt/c/Users/dell/Desktop/fusion32/build/classic-client-772/app/Tibia.exe
```

Its expected hashes identify the tested local artifact set; they do not establish historical provenance. Live results and scope limits are recorded in `evidence/client/CLASSIC-CLIENT-772-001.md`.

The first deterministic Protocol772Core suite is under `clientcore/tests/transport_tests.cpp`. It covers 20 TCP/framing/lifecycle cases and runs through CTest without Internet, Fusion32, the classic client or Unreal:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-transport-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-transport-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-transport-build --output-on-failure
```

No crypto or application-packet fixture suite exists yet. Subsequent protocol tests should cover both directions:

- known packet bytes -> expected typed semantic event;
- typed client command -> expected packet bytes.

Every parity test records preconditions, clients A/B, action, expected server/2D/3D results, actual result, evidence path, and one official status. Initial required IDs are `PARITY-LOGIN-001`, `PARITY-GAME-ENTRY-001`, bidirectional visibility, cardinal/blocked movement, and floor transitions.
