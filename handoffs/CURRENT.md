# HANDOFF

Date/time: 2026-09-16
Agent: Claude
Role: TWO-CLIENT VERTICAL SLICE
Branch: `main`
Starting commit: `da67e78`
Implementation commit: `a048150`
Ending commit: this handoff commit
Worktree: clean after the focused slice commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, `HEAD == origin/main`

## Objective

Complete `TWO-CLIENT-VERTICAL-SLICE-001`: demonstrate end to end that the
original Tibia 7.72 client and Protocol772Core coexist as two independent
clients inside the same Fusion32 world. Do not simulate the second client and do
not substitute another ClientCore for `Tibia.exe`. Do not start Unreal, combat,
chat, inventory or containers.

## Result

`PASS` for one bounded live run, 19 minutes, two distinct synthetic characters:

- Player A: the selected local `Tibia.exe` patched live by the Fusion32 IP
  Changer. Character `Test Player A`, creature id 1001, driven by the operator.
- Player B: Protocol772Core through a temporary non-tracked harness. Character
  `Test Player B`, creature id 1002, driven by commands written into a control
  file.

Every required transition was demonstrated with ids, positions and server
corroboration; the full transcript is in
`evidence/clientcore/TWO-CLIENT-VERTICAL-SLICE-001.md`. Totals: 689 commands
over 657 frames, **zero residual bytes, zero anomalies, zero unsupported
opcodes**.

## Protocol surface

No opcode was added, and none was needed. Appearance
(`ADD_FIELD` with a word-97 descriptor), movement (`MOVE_CREATURE`), turning
(`CHANGE_FIELD`), departure (`DELETE_FIELD`) and refusal (`MESSAGE` plus
`SNAPBACK`) were all already decoded. The run instead corrected two errors in
how the known-creature mirror was maintained.

## Two corrections the live run forced

Both were found against the real server, not by reading, and both now have
deterministic coverage in
`clientcore/tests/movement_tests.cpp::TestSecondPlayerLifecycle`.

1. **`SV_CMD_DELETE_FIELD` must not drop the creature from the mirror.** The
   server sends the same command whether a creature scrolled out of view or was
   destroyed, and `TConnection::KnownCreatureTable` only frees a slot in
   `~TCreature` or when `NewKnownCreature` reuses it. Dropping the entry made a
   later word-98 or word-99 reappearance unrecognisable, which showed up within
   seconds as a nameless rabbit plus an `UnknownCreatureReference` anomaly.
   `DELETE_FIELD` now removes the creature from the map only;
   `visible_creature_ids()` answers what is on the map.
2. **A word-97 whose evicted id equals the introduced id evicts nothing.**
   `~TCreature` frees the slot without clearing its `CreatureID`, and
   `TCreature::SetID` assigns `CreatureID = CharacterID`, so a player keeps its
   id across logins and every relog reuses that very slot. This was being
   flagged as evicting an unknown creature.

## Keepalive, which any sustained session needs

`reference/game/src/connections.cc::TConnection::Process` disconnects a client
whose last command is 90 rounds old, and `reference/game/src/main.cc` advances
one round per second. `SV_CMD_PING` at 30 and 60 seconds does not require a
reply; what resets the timer is any client command, and `ResetTimer` accepts
`CL_CMD_PING`. A listening-only client is dropped after 90 seconds. Player B
pinged every 20 seconds. Note that a ping does not refresh `TimeStampAction`, so
it does not defeat the 15-minute idle warning or 16-minute idle logout.

## Player blocking is part of the evidence

Three of Player B's walk requests were refused, and cross-referencing B's own
log shows Player A was standing on the destination field each time. That is the
7.72 one-SQM rule, and it is the strongest available proof that both clients
inhabit one authoritative world: one client's body constrained the other's
movement, decided entirely by Fusion32. B's anchor and tile set were unchanged
across all three and `viewport_synchronized()` held.

## Changes

- `clientcore/src/movement.cpp`: `DELETE_FIELD` no longer erases from the
  mirror; the self-eviction rule added to `RecordCreature`.
- `clientcore/src/initial_world.cpp`: the same self-eviction rule in
  `ApplyFullScreen`.
- `clientcore/tests/movement_tests.cpp`: added `TestSecondPlayerLifecycle` and
  corrected `TestDeleteFieldRemovesCreature`, which had encoded the wrong
  assumption.
- Added `docs/TWO_CLIENT_SLICE.md` and
  `evidence/clientcore/TWO-CLIENT-VERTICAL-SLICE-001.md`.
- Updated project status, roadmap, parity matrix and this handoff; archived the
  prior Player State handoff.

## Tests/results

Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5, C++17,
warnings as errors:

- CTest: `7/7 PASS`
- ASan/UBSan CTest: `7/7 PASS`
- `verify_classic_client_772.py`: `PASS` (all artifact hashes and the static
  address table)
- `tests/secret_check.sh`: `PASS`
- Live two-client run: `PASS`

No account id, password, modulus or key material was recorded. The account line
the operator needed for the original client was written to a scratch file
outside the repository and deleted along with the harness, the driver script and
the operator launcher. The runtime services were stopped.

## Status and limits

`TWO-CLIENT-VERTICAL-SLICE-001 = PASS` within this bounded scope.

Remaining `UNVERIFIED`:

- One run, one operator, no independent repetition, no retained screenshots.
- Both characters stayed on floor 7, so no floor transition was exercised with
  two clients connected.
- Chat was deliberately avoided because `SV_CMD_TALK` is still undecoded and
  would stop the frame walk.
- Player A's observation of Player B moving is an operator report rather than a
  machine-readable artifact; the server-side effect is corroborated by B's own
  anchor advancing and by the blocking evidence.
- Native Windows execution of Protocol772Core.

Out of scope and untouched: Unreal, combat, chat, NPC interaction, inventory
semantics, containers and trade.

## Operational notes

- WSL2 tears the VM down between separate `wsl.exe` invocations and clears
  `/tmp`, destroying both the sanitized runtime and any build directory there.
  Build under `/root/f32/...`, and run anything spanning prepare, start and a
  live client in a single invocation launched from PowerShell, since Git Bash
  rewrites `/mnt/...` paths.
- The IP Changer needs the client window to already exist. Launching both from
  one script usually loses the race; run `ipchanger.exe fusion32` again after
  the client window is up.
- A live harness driven by a control file should log `SNAPBACK` and `MESSAGE`.
  This one did not, so three legitimate server refusals looked like silence and
  briefly read as a bug. Also give each command a distinct token or a repeat of
  the same command is ignored, and poll the file more often than the read loop's
  worst-case latency.

## Exact next task

`UNREAL-SLICE-001`: protocol coexistence with the original client is now
demonstrated, so the remaining gap in the vertical slice is presentation. Create
the minimal Unreal desktop project that consumes `Protocol772Core` through a
network-thread event queue and applies `WorldState` on the game thread,
rendering ground as planes and creatures as capsules, per `ROADMAP.md` step 8.
The protocol side needs nothing new for it.

If presentation is deferred, the next protocol step is chat and containers,
starting from `reference/game/src/sending.cc::SendTalk` (170), `SendChannels`
(171), `SendOpenChannel` (172), `SendPrivateChannel` (173), `SendContainer`
(110), `SendCloseContainer` (111) and `SendCreateInContainer` (112) through
`SendDeleteInContainer` (114). Chat is what currently forces the operator to
avoid the in-game chat during two-client runs.

Do not begin either automatically.

Commands to reproduce this task's results:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /root/f32/build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /root/f32/build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /root/f32/build --output-on-failure
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_classic_client_772.py /mnt/c/Users/dell/Desktop/fusion32/build/classic-client-772/app/Tibia.exe
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/tests/secret_check.sh /mnt/c/Users/dell/Desktop/fusion32
```
