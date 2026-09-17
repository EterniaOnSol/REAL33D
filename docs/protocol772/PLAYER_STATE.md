# Protocol772Core Player State and Session Commands

Task: `PLAYERSTATE-772-001`

Status: `PASS` for deterministic byte fixtures, negative cases, ASan/UBSan and
one bounded local synthetic-account live smoke that consumed an entire session
with zero residual bytes. Combat, inventory semantics, containers, chat, NPC
interaction and Unreal remain out of scope.

## Purpose

Before this task a caller walking a decrypted payload stopped at the first
opcode the decoder could not size, because guessing a length would
desynchronize the stream. The login burst always hit that wall at
`SV_CMD_GRAPHICAL_EFFECT`. This task closes the set of commands an ordinary
session actually emits, so a frame can be walked to its end.

## Wire layouts

All traced from `reference/game/src/sending.cc`.

```text
SV_CMD_PING            30   opcode only
SV_CMD_SET_INVENTORY  120   opcode, u8 slot, SendItem object
SV_CMD_DELETE_INVENTORY 121 opcode, u8 slot
SV_CMD_AMBIENTE       130   opcode, u8 brightness, u8 colour
SV_CMD_GRAPHICAL_EFFECT 131 opcode, u16 x, u16 y, u8 z, u8 effect
SV_CMD_TEXTUAL_EFFECT 132   opcode, u16 x, u16 y, u8 z, u8 colour, SendString
SV_CMD_MISSILE_EFFECT 133   opcode, origin position, destination position, u8 effect
SV_CMD_MARK_CREATURE  134   opcode, u32 creature id, u8 colour
SV_CMD_CREATURE_HEALTH 140  opcode, u32 creature id, u8 health percent
SV_CMD_CREATURE_LIGHT 141   opcode, u32 creature id, u8 brightness, u8 colour
SV_CMD_CREATURE_OUTFIT 142  opcode, u32 creature id, SendOutfit
SV_CMD_CREATURE_SPEED 143   opcode, u32 creature id, u16 speed
SV_CMD_CREATURE_SKULL 144   opcode, u32 creature id, u8 playerkilling mark
SV_CMD_CREATURE_PARTY 145   opcode, u32 creature id, u8 party mark
SV_CMD_PLAYER_DATA    160   opcode, u16 hp, u16 max hp, u16 capacity,
                            u32 experience, u16 level, u8 level percent,
                            u16 mana, u16 max mana, u8 magic level,
                            u8 magic level percent, u8 soul points   (21 bytes)
SV_CMD_PLAYER_SKILLS  161   opcode, then level and percent for fist, club,
                            sword, axe, distance, shielding, fishing (15 bytes)
SV_CMD_PLAYER_STATE   162   opcode, u8 flags
SV_CMD_CLEAR_TARGET   163   opcode only
SV_CMD_OUTFIT         200   opcode, SendOutfit, u16 first outfit, u16 last outfit
SV_CMD_BUDDY_DATA     210   opcode, u32 character id, SendString name, u8 online
SV_CMD_BUDDY_ONLINE   211   opcode, u32 character id
SV_CMD_BUDDY_OFFLINE  212   opcode, u32 character id
```

`SendItem` and `SendOutfit` are the same encodings the map already uses, so they
come from the shared scanner rather than a second implementation. In particular
an inventory item's on-wire length still depends on the object type flags the
protocol never carries, which is why `DecodeInventory` needs the object type
table.

## Player state flags

Source: `reference/game/src/crplayer.cc::TPlayer::CheckState`, lines 1213-1247.

| Bit | Meaning | Condition in the source |
| --- | --- | --- |
| `0x01` | Poisoned | `SKILL_POISON` timer running |
| `0x02` | Burning | `SKILL_BURNING` timer running |
| `0x04` | Electrified | `SKILL_ENERGY` timer running |
| `0x08` | Drunk | `SKILL_DRUNKEN` timer running and not suppressed |
| `0x10` | Mana shield | `SKILL_MANASHIELD` timer or value active |
| `0x20` | Slowed | `SKILL_GO_STRENGTH` delta below zero |
| `0x40` | Hasted | `SKILL_GO_STRENGTH` delta above zero |
| `0x80` | Logout blocked | still inside `EarliestLogoutRound` |

`CheckState` only sends when the computed flags differ from `OldState`, and
`SyncState` zeroes `OldState` at login. A character with no active condition
therefore receives **no** `SV_CMD_PLAYER_STATE` at all, which the live run
confirmed. `WorldState::state.known` stays false until one actually arrives.

## The first-login outfit chooser

`reference/game/src/crplayer.cc` line 221 sends a welcome message and
`SendOutfit(TConnection*)` when `PlayerData->LastLoginTime` is zero and the
character has no gamemaster outfit right. The selectable range in
`reference/game/src/sending.cc` lines 1671-1681 is 128..131 for male and
136..139 for female characters, extended by three with a premium account.

This is why a first-ever login carries `SV_CMD_OUTFIT`, an opcode that never
appears again for that character. It was found by the live smoke rather than
predicted, and it is exactly the kind of burst member this task exists to cover.

## What reaches WorldState

Only where the semantics are demonstrated:

| Command | Effect |
| --- | --- |
| `SV_CMD_PLAYER_DATA` | replaces `WorldState::stats` |
| `SV_CMD_PLAYER_SKILLS` | replaces `WorldState::skills` |
| `SV_CMD_PLAYER_STATE` | replaces `WorldState::state` |
| `SV_CMD_AMBIENTE` | replaces `WorldState::ambient_light` |
| 140-145 | updates the named creature in the known-creature mirror |

Everything else is decoded, typed and surfaced but stores nothing. The
graphical, textual and missile effects and `SV_CMD_MARK_CREATURE` are
presentation events with no world state of their own. `SV_CMD_SET_INVENTORY`,
`SV_CMD_DELETE_INVENTORY`, the buddy commands and `SV_CMD_OUTFIT` belong to
features this task did not claim, so they are decoded for length and structure
and deliberately not applied.

A creature attribute update naming a creature the mirror has never met is
reported as `UnknownCreatureReference` and applied to nothing:
`AnnounceChangedCreature` only reaches connections that already know the
creature, so a miss means the local mirror is behind rather than that a new
creature should be invented.

## Keepalive

`SV_CMD_PING` is server-initiated. `reference/game/src/connections.cc` line 25
sends it from the connection timer and line 78 from `EmergencyPing` when
`NetLoadCheck` detects lag. `reference/game/src/receiving.cc::CPing` is a no-op
that only refreshes `Connection::TimeStamp`, so the client's own
`CL_CMD_PING` is never answered with a ping. `BuildPingCommand` exists for the
client side of that contract.

## Verification

`clientcore/tests/player_state_tests.cpp` covers:

* eleven hand-computed golden hex commands, each first reproduced byte for byte
  by the literal port of the server emitter;
* every creature attribute variant with its exact length;
* the player state bit table;
* application updating exactly the four pieces of player condition and the
  creature mirror, and an eight-command run of effects, inventory, buddy,
  clear-target and ping leaving the tile count, thing count and creature count
  untouched;
* a whole simulated login burst in the order `crplayer.cc` emits it, walked
  command by command to exactly zero residual bytes;
* negatives: every truncation of each golden command, an inventory slot outside
  `INVENTORY_FIRST..INVENTORY_LAST`, an inventory item naming a server-internal
  container type, an inventory item absent from the type table, and seven
  opcodes that remain unsupported and consume nothing.

WSL Ubuntu 26.04, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5: CTest 7/7 `PASS`
normally and under ASan/UBSan.

Live smoke: a real login burst of 2417 payload bytes decoded as 22 commands and
ordinary session traffic of 1073 bytes as 32 commands, **3490 payload bytes and
54 commands with zero residual bytes and no unsupported opcode**. Details in
`evidence/clientcore/PLAYERSTATE-772-001.md`.

## Limits

The set is closed for what a quiet session emits, not for the protocol. Chat
(`SV_CMD_TALK`, the channel commands), containers, trade, the request queue and
the text and list editors remain undecoded and still yield
`ServerUpdateKind::Unsupported` with zero bytes consumed. That is the correct
behaviour, not a gap to paper over: a caller can see exactly which opcode
stopped it.
