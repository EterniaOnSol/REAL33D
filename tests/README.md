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

`clientcore/tests/movement_tests.cpp` covers cardinal movement in both directions. The fixtures header's server port grew `SendRow`, `SendFloors`, `SendFieldData`, the field commands, `SendMoveCreature`, `SendSnapback` and `SendMessage`, each asserted against hand-computed golden hex before use. Its strongest case walks twelve cardinal steps over a synthetic world and, after every single step, compares the incrementally updated `WorldState` against a freshly emitted `SV_CMD_FULLSCREEN` at the new position.

`clientcore/tests/player_state_tests.cpp` covers the rest of an ordinary session burst: ping, ambience, the effect commands, the six creature attribute updates, player data/skills/state, clear target, inventory, buddy and the first-login outfit chooser. Its strongest case walks a whole simulated login burst, in the order `crplayer.cc` emits it, to exactly zero residual bytes.

`build_clientcore_windows.cmd` builds Protocol772Core natively on Windows with MSVC and runs every suite against it. Until `UNREAL-SLICE-001` the suites had only ever been built by GCC under WSL, and the first native run surfaced four portability defects that GCC accepts silently: a shadowed variable, three narrowing conversions in `std::fill`/`std::make_shared` calls, and an `initializer_list<int>` deduced where `std::uint8_t` was meant. It links the engine's own OpenSSL, so it proves the exact combination the Unreal module links:

```bat
tests\build_clientcore_windows.cmd
```

It compiles the same sources twice. The C++17 build is the one the suites run against and is what keeps the component portable; the C++20 build is archived as `build/clientcore-windows/protocol772core.lib`, because UE 5.8 refuses to compile a module at C++17 and both sides of a static-library link should agree on the standard. Two compilations of one implementation, never two implementations. Both use `/W4 /WX /permissive-`.

Run it from an x64 Native Tools Command Prompt, or call `vcvars64.bat` first. The Unreal module's `Build.cs` fails with these instructions if the archive is missing.

`secret_check.sh` scans the tracked tree and the whole reachable history for private keys, credentials and runtime secrets, and verifies `reference/` was not modified outside its baseline commits. Run it before every push:

```powershell
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/tests/secret_check.sh /mnt/c/Users/dell/Desktop/fusion32
```

`regression_harness.sh` is the one command a 3D milestone runs before it claims anything. It records provenance, runs every check above that needs neither a live Fusion32 nor a human at the GUI, scans existing logs for protocol anomalies, and writes a Markdown report with one status per check:

```bash
bash tests/regression_harness.sh
```

It reports three statuses, never two. `SKIP` means the check could not execute and says why — an absent MSVC, an engine that was not asked for, an empty log scope. A `SKIP` is never rounded up to a `PASS`, because the report exists to be trusted. Overall is `FAIL` if anything failed, `PASS_WITH_SKIPS` if anything was skipped, `PASS` only when every check ran and passed. Exit is `1` on any `FAIL`, or on any `SKIP` under `--strict`.

The checks are `provenance`, `branch-isolation`, `clientcore-tests` (the CTest suites), `real33d-native-tests` (the header-only `tests/real33d_*.cpp`), `secret-check`, `build-sanity-msvc`, `unreal-module-build` and `protocol-log-check`, followed by `output-secret-scan` over everything the run just wrote.

It borrows a toolchain from WSL when Windows has no native compiler, so the same command works from Git Bash, from WSL and from Linux CI. `build-sanity-msvc` needs `cl.exe` on PATH, so run from an x64 Native Tools prompt to get it; the Unreal module build is opt-in with `--unreal` because a full editor build is not a safe CI default.

`protocol-log-check` looks for `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, and for the bridge diagnostics that name a command left unwalked. By default it only scans evidence logs this branch added or changed against `origin/main`, so a milestone is judged on its own sessions rather than re-reporting adjudicated history; `--logs-all` scans everything, and `--logs PATH` adds a file or directory.

The harness is read-only against the repository. It writes only under `--out`, which defaults inside the gitignored `build/` tree, starts no server, launches no client and modifies no gameplay or protocol. Nothing it reports is live proof; live certification remains a separate, operator-driven step.

```bash
# full local run, including the MSVC archive Unreal links and the engine build
bash tests/regression_harness.sh --unreal --objects-srv /path/to/objects.srv

# CI gate: fail on anything that could not be executed
bash tests/regression_harness.sh --no-wsl --allow-main --strict
```

Subsequent protocol tests should keep covering both directions:

- known packet bytes -> expected typed semantic event;
- typed client command -> expected packet bytes.

Every parity test records preconditions, clients A/B, action, expected server/2D/3D results, actual result, evidence path, and one official status. Initial required IDs are `PARITY-LOGIN-001`, `PARITY-GAME-ENTRY-001`, bidirectional visibility, cardinal/blocked movement, and floor transitions.
