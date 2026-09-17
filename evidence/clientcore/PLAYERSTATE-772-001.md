# PLAYERSTATE-772-001

Status: `PASS`

## Implementation

Closed the set of 7.72 server commands an ordinary session emits, so a caller
can walk a decrypted payload to its end instead of stopping at the first opcode
it cannot size:

- `player_state` - byte-exact decoders for ping, ambience, the four effect
  commands, the six creature attribute updates, player data, skills and state,
  clear target, the inventory pair, the buddy trio and the first-login outfit
  chooser.
- `movement` - the single `DecodeServerUpdate` entry point now dispatches all of
  them, so there is still exactly one place a caller walks a frame through.
- `worldstate` - `PlayerStats`, `PlayerSkills`, `PlayerState` with its traced
  flag table, and `AmbientLight`.
- `map_scan` - exposed the bounds-checked word, quad, outfit and `SendItem`
  reads the new decoders share with the map path.

## Source traceability

See [`docs/protocol772/PLAYER_STATE.md`](../../docs/protocol772/PLAYER_STATE.md).
Primary symbols are `reference/game/src/sending.cc::SendPing`, `SendAmbiente`,
`SendGraphicalEffect`, `SendTextualEffect`, `SendMissileEffect`,
`SendMarkCreature`, `SendCreatureHealth`, `SendCreatureLight`,
`SendCreatureOutfit`, `SendCreatureSpeed`, `SendCreatureSkull`,
`SendCreatureParty`, `SendPlayerData`, `SendPlayerSkills`, `SendPlayerState`,
`SendClearTarget`, `SendSetInventory`, `SendDeleteInventory`, `SendBuddyData`,
`SendBuddyStatus` and `SendOutfit(TConnection*)`;
`reference/game/src/crplayer.cc::TPlayer::CheckState` and `SyncState` and the
first-login branch at line 221; `reference/game/src/connections.cc`
lines 25 and 78; and `reference/game/src/receiving.cc::CPing`.

Two findings came out of the live run rather than the reading:

1. **A character's first login carries `SV_CMD_OUTFIT`.** `crplayer.cc` line 221
   sends the welcome message and the outfit chooser when `LastLoginTime` is
   zero. The first live attempt stopped with exactly 11 residual bytes, which is
   precisely that command's length, so the decoder had consumed everything else
   to an exact command boundary. Implementing it took the residual to zero.
2. **`SV_CMD_PLAYER_STATE` is not always sent.** `CheckState` only emits when the
   computed flags differ from `OldState`, and `SyncState` zeroes `OldState` at
   login, so a character with no active condition receives none. The initial
   assertion that the burst must deliver it was wrong and was corrected;
   `WorldState::state.known` stays false until one arrives.

`SV_CMD_PING` is server-initiated from the connection timer and `EmergencyPing`;
`CPing` is a no-op, so the client's own ping is never answered with one. It was
therefore not observed in this short session and is fixture-covered only.

## Tests

Environment: Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5,
C++17, warnings as errors.

- Debug build: `PASS`
- CTest: `7/7 PASS`
- ASan/UBSan CTest: `7/7 PASS`
- Eleven hand-computed golden hex commands, byte for byte, each first reproduced
  by the literal port of the server emitter: `PASS`
- Every creature attribute variant with its exact length: `PASS`
- The player state bit table from `CheckState`: `PASS`
- Application touching exactly the four pieces of player condition plus the
  creature mirror, with an eight-command run of effects, inventory, buddy,
  clear-target and ping leaving tile, thing and creature counts untouched:
  `PASS`
- A whole simulated login burst in `crplayer.cc` order walked to exactly zero
  residual bytes across 13 commands: `PASS`
- Negatives - every truncation of each golden command, an inventory slot outside
  `INVENTORY_FIRST..INVENTORY_LAST`, an inventory item naming a server-internal
  container type, an unknown inventory type id, and seven opcodes that remain
  unsupported and consume nothing: `PASS`
- `verify_object_type_invariants.py`: `PASS`
- `tests/secret_check.sh`: `PASS`

## Bounded live smoke

A temporary non-tracked harness used only the local synthetic `ACCOUNT_A` and
the runtime's public modulus, both read at run time and never recorded:

```text
object type table: declared=5003
login burst: 2 frame(s), 2417 payload bytes, 22 commands, 0 residual bytes
  AddField x1
  Ambient x1
  CreatureAttribute x2
  FullScreen x1
  GraphicalEffect x1
  InitGame x1
  Inventory x4
  Message x1
  MoveCreature x2
  OutfitDialog x1
  PlayerData x6
  PlayerSkills x1
player (32097,32219,7) creature_id=1001 tiles=408
stats known=1 hp=150/150 mana=0/0 cap=336 level=1@0% exp=0 ml=0 soul=100
skills known=1 fist=10 club=10 sword=10 axe=10 dist=10 shield=10 fish=10
state known=0 flags=0; ambient known=1 brightness=204 color=208
player state command absent, as CheckState skips a zero state
session traffic: 6 steps, 16 frames, 1073 payload bytes, 32 commands, 0 residual bytes
  DeleteField x1
  Message x1
  MoveCreature x24
  Row x6
consumed 3490 payload bytes as 54 commands with zero residual bytes
LIVE PASS
```

What this demonstrates:

- the login burst and the walking session were both consumed to **zero residual
  bytes**, with no opcode falling through to `Unsupported`;
- the decoded stats match a freshly created Rookgaard character: 150/150 hit
  points, 336 capacity, level 1, magic level 0, 100 soul points, and every
  weapon skill at 10;
- `PLAYER_DATA` arrives six times during login, once per change the server makes
  while equipping and settling the character, and each one decoded at its exact
  21-byte length;
- `SV_CMD_OUTFIT` appeared exactly once, on the character's first ever login;
- no `SV_CMD_PLAYER_STATE` arrived, which the source explains rather than
  contradicts;
- no world state anomaly was reported in either phase.

The temporary harness source, driver script and binary were deleted and the
runtime services stopped after the run. No account id, password, modulus or key
material appears in this evidence.

## Secret and sensitive-file check

`tests/secret_check.sh` was added and run before the first push:

```text
ok   : no secret-bearing file extensions tracked
ok   : runtime and scratch trees untracked
ok   : no private key blocks in any reachable commit
ok   : no generated credential values tracked
ok   : reference/ untouched outside its baseline commits
ok   : archived upstream config templates present and accepted (see evidence)
secret_check: PASS
```

Reviewed and accepted, rather than silently pushed:

- `reference/login/config.cfg.dist` and
  `reference/querymanager/config.cfg.dist` carry Fusion32's own upstream default
  `QueryManagerPassword`. They are archived third-party source material, not a
  secret of this deployment: `scripts/server/prepare_wsl.sh` generates a fresh
  password into `config.cfg`, which `.gitignore` excludes.
- `clientcore/tests/fixtures/crypto_772_vectors.h` holds a public sample modulus
  already present in the selected IP Changer source, labelled as test data. It
  is a public key modulus, never a private key.
- `scripts/server/prepare_wsl.sh` contains credential *variable names* only; the
  values it generates land in the gitignored runtime.

No private key block exists in any reachable commit, and no history was
rewritten.

## Remaining UNVERIFIED

- `SV_CMD_PING`, `SV_CMD_CLEAR_TARGET`, `SV_CMD_TEXTUAL_EFFECT`,
  `SV_CMD_MISSILE_EFFECT`, `SV_CMD_MARK_CREATURE`, the buddy status pair and
  four of the six creature attribute updates are fixture-covered only; the quiet
  temple session did not emit them.
- Native Windows execution and independent repetition remain unverified.

## Scope exclusions

No combat, inventory semantics, containers, chat, NPC interaction or Unreal
integration was started. Chat, the channel commands, containers, trade, the
request queue and the text and list editors remain undecoded and still yield
`ServerUpdateKind::Unsupported` with zero bytes consumed.
