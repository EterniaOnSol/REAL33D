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

The checks are `provenance`, `branch-isolation`, `clientcore-tests` (the CTest suites), `real33d-native-tests` (the header-only `tests/real33d_*.cpp`), `outfit-color-mapping`, `outfit-palette`, `secret-check`, `build-sanity-msvc`, `unreal-module-build` and `protocol-log-check`, followed by `output-secret-scan` over everything the run just wrote.

`real33d-native-tests` names its suites explicitly rather than globbing, so a suite that is deleted or renamed fails the check instead of quietly dropping out of it. A new suite has to be added to that list; `real33d_outfit_selection_tests` was the first one that proved the list can be forgotten. The eight suites are `real33d_mouse_input_tests`, `real33d_minimap_view_tests`, `real33d_item_use_policy_tests`, `real33d_stackable_sprite_tests`, `real33d_chat_views_tests`, `real33d_outfit_selection_tests`, `real33d_stack_move_tests` and `real33d_walk_direction_tests`.

`real33d_walk_direction_tests` fixes how this client numbers the eight walk directions it can ask for: the cardinals keep the server's own `enums.hh` values 0..3, the four diagonals sit past them at 4..7 in `DiagonalDirection` order, the two halves never overlap, every direction has a distinct name, and every offset is the one `receiving.cc::ReceiveData` hands to `CGoDirection` — a diagonal moving both axes and a cardinal exactly one. The numbering is this client's own, for its own events and logs; it is never put on the wire. The header is `unreal/REAL33D/Source/REAL33D/Public/Real33DWalkDirection.h`, and it exists because the direction used to be printed from two separate copies of a four-entry table.

`real33d_stack_move_tests` fixes the stack-movement gesture and its arithmetic: a plain drag moves the whole stack, Shift+drag moves one, Ctrl+drag asks, a non-stackable object moves whole whatever is held down, and every answer is clamped to 1..the stack count and to the one count byte `CMoveObject` reads. The rule lives in `unreal/REAL33D/Source/REAL33D/Public/Real33DStackMove.h` and all four drag routes go through it, so the gesture cannot mean one thing in the inventory panel and another on the ground. It proves nothing about what Fusion32 then does with the count; that is a live question.

`outfit-palette` runs `verify_outfit_palette.py`, the sibling of the check below: that one proves *which* body part each of the four outfit colour bytes tints, this one proves *what colour* each byte value is. The server has no palette at all — it validates `Colors[n] > 132` and stores the byte — and the classic client holds no colour table either: it **computes** every swatch. So this reproduces the computation rather than hardcoding its output, asserting the 133-swatch loop, the `index -> row*19 + col` split, the per-row saturation/value pairs and the six-sector HSV conversion against the hashed image, then checks the generated palette against invariants the binary guarantees. `--dump` prints all 133 colours. A hardcoded table would be a guess that nothing would notice drifting; this fails closed instead.

`audit_outfit_asset_regions.py` is not a harness check but the acceptance gate for outfit colour *rendering*. It reads a delivered creature `.glb` and reports whether the four 7.72 regions are addressable at all — by node, by material slot, or by a mask in `COLOR_0`, a second UV set or a texture. Exit 3 means the asset cannot represent them. It currently exits 3 for all 16 delivered outfits, none of which has any feet geometry or any region mask; see `evidence/clientcore/UNREAL-OUTFIT-COLOR-RENDERING-001.md`.

`outfit-color-mapping` runs `verify_outfit_color_mapping.py`, which is the only claim in this client that rests on the classic 7.72 client rather than on Fusion32. The server cannot say which body part each of the four outfit colour bytes tints — `reference/game/src/cr.hh` declares `uint8 Colors[4]` and every use addresses them by index — so the proof is a read-only static walk of the local `Tibia.exe`, asserting the four-entry jump table that selects each colour row's caption, the store that writes `colours[slot]`, and the four reads the set-outfit sender makes in wire order. It `SKIP`s rather than passing when the client artifact is absent, because that file is ignored by Git and is not on a clean checkout. The binary is read, never executed.

It borrows a toolchain from WSL when Windows has no native compiler, so the same command works from Git Bash, from WSL and from Linux CI. `build-sanity-msvc` needs `cl.exe` on PATH, so run from an x64 Native Tools prompt to get it; the Unreal module build is opt-in with `--unreal` because a full editor build is not a safe CI default.

`protocol-log-check` looks for `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, and for the bridge diagnostics that name a command left unwalked.

### The two Windows checks, and why exit status is not enough

`build-sanity-msvc` and `unreal-module-build` launch a Windows build. Both also require the success line that build prints for itself — `WINDOWS CLIENTCORE: PASS` and `Result: Succeeded` respectively. Exit 0 without that line is `FAIL`, reported as *"exit 0 but the log does not contain '…'; the build did not run"*.

That is not belt-and-braces. Until 2026-10-15 both checks were written as `cmd.exe /c "<command>"`, and from Git Bash that ran nothing: MSYS rewrites an argument that looks like an absolute Unix path, so the `/c` **switch** arrived as the path `C:/`, cmd started interactively, read EOF from the redirected stdin, printed its banner and exited 0. Both checks recorded `PASS` against a four-line log containing no build output.

Launch Windows commands through `run_windows_command`, which writes the line to a temporary `.cmd` and runs it by path with argument conversion disabled and stdin closed. Keep the switch as `/c`: `//c` also works under Git Bash, but only *because* MSYS collapses it back to `/c`, so it breaks as soon as conversion is disabled and is wrong on a non-MSYS host. Disabling conversion **and** writing `//c` cancel out and silently restore the original bug.

`regression_harness_selftest.sh` is the guard. It drives the real harness against a throwaway fixture repository whose batch files it writes, so it exercises the real `cmd.exe` rather than a mock, and asserts all four outcomes: the command actually executes (the fixture leaves a sentinel on disk), a no-op exit 0 is `FAIL`, a real success is `PASS`, a non-zero exit is `FAIL`, and an absent toolchain is still `SKIP`.

```bash
bash tests/regression_harness_selftest.sh
```

It skips cleanly where there is no `cmd.exe`. Run it from an x64 Native Tools prompt to exercise the `build-sanity-msvc` cases too; without `cl.exe` on PATH those report `SKIP` instead of asserting.

### Log scope policy

This is the rule the check exists to enforce, and it is not a preference:

- **The default scope is milestone-scoped.** Only evidence logs the current branch added or changed against `origin/main` are scanned — machine-written `.json`, `.jsonl`, `.txt` and `.log` files. A milestone is judged on the sessions it actually produced.
- **`--logs-all` is historical audit only.** It is a census of the whole `evidence/` tree, useful for asking how widespread a known gap is. It is never the gate for a milestone and its result is never a milestone's result.
- **A historical anomaly is not attributable to the current milestone unless that milestone's own logs reproduce it.** Nine logs under `evidence/` carry the long-known `SV_CMD_TALK` chat gap. A `--logs-all` run will keep reporting them for as long as they exist. Quoting that FAIL as if the current work caused it is a misattribution, and the reason the default scope is narrow: a check that re-reports adjudicated history on every run stops being read.
- **The `PASS` / `FAIL` / `SKIP` semantics do not change with scope.** An empty scope is `SKIP` — no logs were examined, so nothing is claimed. It is never a `PASS`. Widening the scope widens what is examined; it never relaxes what a status means.

`--logs PATH` adds a file or directory on top of the default scope. Explicitly named paths are scanned whatever their extension, because naming a file is the caller saying they meant that file.

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
