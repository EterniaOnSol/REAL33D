# HANDOFF

Date/time: 2026-09-16
Agent: Claude
Role: PROTOCOL772CORE PLAYER STATE IMPLEMENTATION
Branch: `main`
Starting commit: `633a9c2`
Implementation commit: `af3e3ac`
Ending commit: this handoff commit
Worktree: clean after the focused Player State commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, full history pushed to `main`, `HEAD == origin/main`

## Objective

Complete `PLAYERSTATE-772-001`: close the minimal set of 7.72 messages needed to
consume an ordinary session burst without stopping, derived only from the
Fusion32 source. Do not implement combat, inventory semantics, chat, NPC
interaction or Unreal.

## Inspection and source findings

Inspected, in `reference/game/src`:

- `sending.cc::SendPing`, `SendAmbiente`, `SendGraphicalEffect`,
  `SendTextualEffect`, `SendMissileEffect`, `SendMarkCreature`,
  `SendCreatureHealth`, `SendCreatureLight`, `SendCreatureOutfit`,
  `SendCreatureSpeed`, `SendCreatureSkull`, `SendCreatureParty`,
  `SendPlayerData`, `SendPlayerSkills`, `SendPlayerState`, `SendClearTarget`,
  `SendSetInventory`, `SendDeleteInventory`, `SendBuddyData`, `SendBuddyStatus`
  and `SendOutfit(TConnection*)`.
- `crplayer.cc::TPlayer::CheckState` lines 1213-1247 for the player state flag
  table, `SyncState`, and the first-login branch at line 221.
- `connections.cc` lines 25 and 78 and `receiving.cc::CPing` for the keepalive
  contract.
- `enums.hh::InventorySlot` for the slot bounds.

Three findings worth carrying forward:

1. **A character's first login carries `SV_CMD_OUTFIT` (200).** `crplayer.cc`
   line 221 sends the welcome message and the outfit chooser when
   `LastLoginTime` is zero. The first live attempt stopped with exactly 11
   residual bytes, which is precisely that command's length, confirming
   everything before it had been consumed to an exact command boundary.
2. **`SV_CMD_PLAYER_STATE` is conditional.** `CheckState` only emits when the
   computed flags differ from `OldState`, and `SyncState` zeroes `OldState` at
   login, so a character with no active condition receives none.
   `WorldState::state.known` stays false until one arrives. My initial assertion
   that the burst must deliver it was wrong.
3. **`SV_CMD_PING` is server-initiated**, from the connection timer and
   `EmergencyPing`. `CPing` is a no-op that only refreshes the timestamp, so the
   client's own `CL_CMD_PING` is never answered with a ping.

## Changes

- Added `clientcore/include/fusion32/protocol772/player_state.h` and
  `clientcore/src/player_state.cpp`.
- Extended `movement`'s single `DecodeServerUpdate` entry point to dispatch all
  of them, and `ApplyServerUpdate` to apply only the demonstrated ones.
- Extended `worldstate` with `PlayerStats`, `PlayerSkills`, `PlayerState` and
  `AmbientLight`.
- Exposed bounds-checked word, quad, outfit and `SendItem` reads from
  `map_scan`, plus `FailScanner` and `MapDecodeError::InvalidInventorySlot`.
- Added `clientcore/tests/player_state_tests.cpp` and grew the fixtures emitter.
- Added `tests/secret_check.sh`.
- Added `docs/protocol772/PLAYER_STATE.md` and
  `evidence/clientcore/PLAYERSTATE-772-001.md`.
- Updated project status, architecture, roadmap, parity matrix, source truth,
  both READMEs and this handoff; archived the prior Movement handoff.

One retained test changed meaning: `movement_tests` used opcode 141 as its
"unsupported" example, which this task now decodes. It uses `SV_CMD_CONTAINER`
(110) instead, which remains genuinely out of scope.

## Tests/results

Validated in WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5, C++17
with warnings as errors:

- Debug build: `PASS`
- CTest: `7/7 PASS`
- ASan/UBSan CTest: `7/7 PASS`
- Eleven hand-computed golden hex commands, each first reproduced by the literal
  port of the server emitter: `PASS`
- A whole simulated login burst in `crplayer.cc` order walked to exactly zero
  residual bytes: `PASS`
- Negatives - every truncation of each golden command, an inventory slot outside
  `INVENTORY_FIRST..INVENTORY_LAST`, an inventory item naming a server-internal
  container type, an unknown inventory type id, and seven opcodes that remain
  unsupported and consume nothing: `PASS`
- `verify_object_type_invariants.py`: `PASS`
- `tests/secret_check.sh`: `PASS`
- Live smoke: `LIVE PASS`. Login burst 2417 payload bytes as 22 commands, session
  traffic 1073 bytes as 32 commands, **3490 payload bytes and 54 commands with
  zero residual bytes and no unsupported opcode**. Decoded stats matched a fresh
  Rookgaard character: 150/150 hit points, 336 capacity, level 1, magic level 0,
  100 soul points, every weapon skill at 10.

No account id, password, modulus or key material was recorded. The temporary
live harness, its driver script and its binary were deleted and the runtime
services stopped.

## GitHub

`origin` was configured as `https://github.com/EterniaOnSol/REAL33D.git` and the
full existing history, 24 commits, was pushed to `main`. `HEAD == origin/main`.
No history was rewritten and no force push was used.

The first attempt returned HTTP 403:

```text
remote: Permission to EterniaOnSol/REAL33D.git denied to leodavidsoto.
```

`EterniaOnSol` is a personal account, not an organisation, and the credential
stored on this machine belonged to a different personal account,
`leodavidsoto`, whose permissions on the repo were `pull: true, push: false`.
That is a repository permission rather than a scope or URL problem, so it needed
an operator decision. The operator re-authenticated `gh` as `EterniaOnSol`
through the device flow; that account reports `admin: true` and the push then
succeeded unchanged. Both accounts remain registered in `gh`, with
`EterniaOnSol` active.

Worth remembering, because it is a common trap: git identity (`user.name` and
`user.email`) is only a label written into the commit, while the token is what
GitHub checks for write access. Changing the first does nothing for the second.

`tests/secret_check.sh` ran clean immediately before the push, and the check
itself was repaired first: its private key scan had been passing vacuously. See
commit `f205519`.

Reviewed and accepted rather than pushed silently, all recorded in the evidence:

- `reference/login/config.cfg.dist` and
  `reference/querymanager/config.cfg.dist` carry Fusion32's own upstream default
  `QueryManagerPassword`. They are archived third-party source, not a secret of
  this deployment, whose `config.cfg` is generated fresh and gitignored.
- `clientcore/tests/fixtures/crypto_772_vectors.h` holds a public sample modulus
  already present in the selected IP Changer source, labelled as test data.

## Status and limits

`PLAYERSTATE-772-001 = PASS` within this bounded scope.

Remaining `UNVERIFIED`:

- `SV_CMD_PING`, `SV_CMD_CLEAR_TARGET`, `SV_CMD_TEXTUAL_EFFECT`,
  `SV_CMD_MISSILE_EFFECT`, `SV_CMD_MARK_CREATURE`, the buddy status pair and
  four of the six creature attribute updates are fixture-covered only; the quiet
  temple session did not emit them.
- Native Windows execution and independent repetition.

Out of scope and untouched: combat, inventory semantics, containers, chat, NPC
interaction, trade and Unreal. Chat, the channel commands, containers, trade,
the request queue and the text and list editors remain undecoded and still yield
`ServerUpdateKind::Unsupported` with zero bytes consumed. That is correct
behaviour: a caller sees exactly which opcode stopped it.

## Operational note

WSL2 shuts the VM down between separate `wsl.exe` invocations and clears `/tmp`,
destroying both the sanitized runtime and any build directory there. Build under
`/root/f32/...`, and run anything spanning prepare, start and a live client in a
single `wsl.exe` invocation launched from PowerShell, since Git Bash rewrites
`/mnt/...` paths.

## Exact next task

`UNREAL-SLICE-001`: Protocol772Core now decodes an entire ordinary session, so
the remaining vertical-slice gap is presentation. Create the minimal Unreal
desktop project that consumes `Protocol772Core` through a network-thread event
queue and applies `WorldState` on the game thread, rendering ground as planes
and creatures as capsules, per `ROADMAP.md` step 8.

The protocol side needs nothing new for that slice. If presentation is deferred
instead, the next protocol step is chat and containers: start from
`reference/game/src/sending.cc::SendTalk` (170), `SendChannels` (171),
`SendOpenChannel` (172), `SendPrivateChannel` (173), `SendContainer` (110),
`SendCloseContainer` (111) and `SendCreateInContainer` (112) through
`SendDeleteInContainer` (114).

Do not begin either automatically.

Commands to reproduce this task's results:

```powershell
wsl.exe -d Ubuntu-26.04 -- cmake -S /mnt/c/Users/dell/Desktop/fusion32/clientcore -B /root/f32/build -DCMAKE_BUILD_TYPE=Debug
wsl.exe -d Ubuntu-26.04 -- cmake --build /root/f32/build --parallel
wsl.exe -d Ubuntu-26.04 -- ctest --test-dir /root/f32/build --output-on-failure
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_object_type_invariants.py /mnt/c/Users/dell/Desktop/fusion32/tibia-game.tarball.tar.gz
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/tests/secret_check.sh /mnt/c/Users/dell/Desktop/fusion32
```
