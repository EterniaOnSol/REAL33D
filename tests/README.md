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

`clientcore/tests/crypto_tests.cpp` and `clientcore/tests/fixtures/crypto_772_vectors.h` now provide 25 RSA/XTEA/key/padding/error cases with byte-for-byte public fixtures. CTest executes both Transport and Crypto:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /tmp/fusion32-clientcore-crypto-build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /tmp/fusion32-clientcore-crypto-build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /tmp/fusion32-clientcore-crypto-build --output-on-failure
```

`clientcore/tests/initial_world_tests.cpp` and `clientcore/tests/fixtures/fullscreen_772_vectors.h` cover the `FULLSCREEN` world snapshot. The fixtures header carries a literal port of the server emitter (`SendFullScreen`, `SendMapPoint`, `SkipFlush`, `SendMapObject`, `SendItem`, `SendOutfit`), which is first asserted to reproduce three hand-computed golden hex messages byte for byte and only then used to build the structural and negative cases.

`verify_object_type_invariants.py` proves the assumptions that decoder relies on against the shipped `dat/objects.srv`: that the creature markers 97/98/99 cannot collide with a map item, that no type id reaches the `0xFF00` skip-marker page, that at most one extra byte follows a type id, and that every disguise target shares its source's wire-relevant flags:

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_object_type_invariants.py /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz
```

Client command encoding is still untested in either direction. Subsequent protocol tests should cover both:

- known packet bytes -> expected typed semantic event;
- typed client command -> expected packet bytes.

Every parity test records preconditions, clients A/B, action, expected server/2D/3D results, actual result, evidence path, and one official status. Initial required IDs are `PARITY-LOGIN-001`, `PARITY-GAME-ENTRY-001`, bidirectional visibility, cardinal/blocked movement, and floor transitions.
