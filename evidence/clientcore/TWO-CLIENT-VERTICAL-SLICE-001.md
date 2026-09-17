# TWO-CLIENT-VERTICAL-SLICE-001

Status: `PASS`

## What was run

The original Tibia 7.72 client and Protocol772Core connected two distinct
synthetic characters to the same sanitized Fusion32 world at the same time:

- Player A: the selected local `Tibia.exe`, SHA-256
  `1A82FE97A39E536C29327D6D5DDE316775F537DCC01506831429B1A0433B76F8`, patched
  live by the Fusion32 IP Changer. Character `Test Player A`, creature id 1001.
- Player B: Protocol772Core through a temporary non-tracked harness. Character
  `Test Player B`, creature id 1002.

Neither client was simulated and no second `ClientCore` stood in for the
original. The operator drove Player A by hand; every Player B action was a
command written into a control file the harness polled.

Preconditions verified before the run:

```text
CLIENT_ARTIFACT Tibia.exe sha256=1A82FE97...B76F8 PASS
CLIENT_ARTIFACT Tibia.dat/spr/pic PASS
VERSION_ADDRESS 0x55C65D value='Version 7.72 ... CipSoft GmbH' PASS
ENDPOINT_0..4 PASS
RSA_ADDRESS 0x55B620 chars=309 PASS
FUSION32_IPCHANGER_STATIC_ADDRESS_TABLE PASS
```

IP Changer against the running process: `Client is now configured to connect to
127.0.0.1:7171.`

## Transition evidence

All timestamps from the Player B harness log. Player B's anchor started at
`(32097,32219,7)`; its viewport spans `x` 32089..32106 and `y` 32213..32226.

### 1. B online, alone

```text
22:39:57.197  B ONLINE character=[Test Player B] creature_id=1002 at (32097,32219,7) tiles=408
22:39:57.198    INITIAL creature id=1073741827 name=[Cipfried] at (32097,32217,7)
```

### 2. A appears, detected semantically

```text
22:43:59.922  EVENT creature APPEARED id=1001 name=[Test Player A] at (32098,32219,7) stack=1
```

One SQM east of B. Server log corroboration:

```text
16.09.2026 22:39:57 (25):  Spieler Test Player B loggt ein an Socket 18 von 127.0.0.1.
16.09.2026 22:43:59 (268): Spieler Test Player A loggt ein an Socket 16 von 127.0.0.1.
```

Both accounts logged in for the first time, so both received the outfit chooser,
which `PLAYERSTATE-772-001` already decodes.

```text
22:44:21.213  STATUS anchor=(32097,32219,7) synchronized=1 tiles=408 things=588
              visible_creatures=3 mirror=4 frames=145 commands=167 residual=0
  SEEN id=1001 name=[Test Player A]        at (32098,32219,7)
  SEEN id=1002 (self) name=[Test Player B] at (32097,32219,7)
  SEEN id=1073741827 name=[Cipfried]       at (32097,32218,7)
```

### 3. A moves, B tracks every step

```text
22:47:50.036  MOVED id=1001 (32098,32219,7) -> (32099,32219,7)
22:47:50.535  MOVED id=1001 (32099,32219,7) -> (32100,32219,7)
22:47:59.035  MOVED id=1001 (32100,32219,7) -> (32100,32218,7)
22:47:59.536  MOVED id=1001 (32100,32218,7) -> (32099,32218,7)
22:48:00.037  MOVED id=1001 (32099,32218,7) -> (32099,32217,7)
22:48:00.536  MOVED id=1001 (32099,32217,7) -> (32098,32217,7)
22:48:01.037  MOVED id=1001 (32098,32217,7) -> (32098,32216,7)
22:48:01.538  MOVED id=1001 (32098,32216,7) -> (32098,32215,7)
22:48:02.085  MOVED id=1001 (32098,32215,7) -> (32098,32214,7)
22:48:02.587  MOVED id=1001 (32098,32214,7) -> (32097,32214,7)
22:48:03.087  MOVED id=1001 (32097,32214,7) -> (32097,32213,7)
22:48:03.586  GONE  id=1001 last seen at (32097,32213,7)
```

Eleven consecutive steps, each exactly one SQM on exactly one axis. The
disappearance is not incidental: `y=32213` is `anchor.y - kTerminalOffsetY`, the
topmost visible row, so the next northward step left the window. The viewport
geometry is correct to the field.

### 4. A returns to view without a name, still resolved

```text
22:48:20.379  EVENT creature APPEARED id=1001 name=[Test Player A] at (32095,32213,7) stack=1
```

The server reannounced A with a descriptor carrying no name, because its
known-creature table for B still held the entry. The retained mirror supplied
the name. Before the fix recorded below, this produced `name=[]` and an
`UnknownCreatureReference` anomaly.

### 5. B moves, observable from A

```text
22:52:15.080  B WALK requested South (32097,32219,7) -> expecting (32097,32220,7)
22:52:29.082  STATUS anchor=(32097,32220,7) synchronized=1 ... residual=0
```

The operator confirmed seeing `Test Player B` move on the original client's
screen. A second request later in the run:

```text
22:57:19.018  B WALK requested West (32097,32220,7) -> expecting (32096,32220,7)
22:57:47.012  STATUS anchor=(32096,32220,7) synchronized=1 ... residual=0
```

### 6. Blocked movement, no desynchronisation

Three of B's requests were refused. Cross-referencing B's own log for A's
position at each moment:

| Time | B requested | A was standing at |
| --- | --- | --- |
| 22:48:51 | East to `(32098,32219,7)` | `(32098,32219,7)` |
| 22:49:35 | East to `(32098,32219,7)` | `(32098,32219,7)` |
| 22:51:15 | North to `(32097,32218,7)` | `(32097,32218,7)` |

Every refusal corresponds exactly to Player A occupying the destination field.
This is the 7.72 one-SQM blocking rule, and it is the clearest evidence that the
two clients share one authoritative world: one client's body constrained the
other's movement, decided entirely by Fusion32.

In all three cases B's anchor stayed at its previous value and
`synchronized=1` held. The refusal arrives as `SV_CMD_MESSAGE` plus
`SV_CMD_SNAPBACK`, neither of which touches map state.

### 7. A disconnects, no ghost

```text
22:53:26.102  EVENT creature GONE id=1001 name=[Test Player A] last seen at (32097,32218,7)
22:54:13.038  HEARTBEAT anchor=(32097,32220,7) synchronized=1 visible_creatures=2 residual=0
```

`visible_creatures` fell from 3 to 2. B and Cipfried remained.

### 8. A reconnects, no duplicate

```text
22:56:45.161  EVENT creature APPEARED id=1001 name=[Test Player A] at (32097,32218,7) stack=1
```

Name present, exactly one entry, no anomaly. This is the self-evicting word-97
case described below.

### 9. Final state

```text
22:58:47.248  FINAL anchor=(32096,32220,7) synchronized=1 tiles=408
              visible_creatures=3 mirror=4 frames=657 commands=689 residual=0
  FINAL SEEN id=1001 name=[Test Player A]        at (32097,32217,7)
  FINAL SEEN id=1002 (self) name=[Test Player B] at (32096,32220,7)
  FINAL SEEN id=1073741827 name=[Cipfried]       at (32097,32218,7)
22:58:47.260  B OFFLINE
```

Across the whole 19-minute session: **689 commands over 657 frames, zero
residual bytes, zero anomalies, zero unsupported opcodes.**

`mirror=4` against `visible_creatures=3` is the rabbit that wandered out of view
and never returned, retained exactly as the server's own table retains it.

## Corrections this run forced

Both were found against the live server, not by reading, and both now have
deterministic coverage in
`clientcore/tests/movement_tests.cpp::TestSecondPlayerLifecycle`.

1. **`SV_CMD_DELETE_FIELD` must not drop the creature from the mirror.** The
   server sends it both for "left the viewport" and for "destroyed", and
   `TConnection::KnownCreatureTable` only frees a slot in `~TCreature` or when
   `NewKnownCreature` reuses it. Dropping the entry made a later word-98 or
   word-99 reappearance unrecognisable. Observed within seconds of connecting,
   on a wandering rabbit.
2. **A word-97 whose evicted id equals the introduced id evicts nothing.**
   `~TCreature` frees the slot without clearing its `CreatureID`, and a player
   keeps its `CreatureID` because `TCreature::SetID` assigns
   `CreatureID = CharacterID`. Every relog therefore reuses that slot and
   reports `removed_creature_id == creature_id`, which was being flagged as
   evicting an unknown creature.

## Keepalive

`TConnection::Process` disconnects a client whose last command is 90 rounds old,
and `main.cc` advances one round per second. Player B sent `CL_CMD_PING` every
20 seconds and held the session for 19 minutes. A purely listening client would
have dropped after 90 seconds.

## Tests

Environment: Ubuntu 26.04 under WSL2, CMake 4.2.3, GCC 15.2.0, OpenSSL 3.5.5,
C++17, warnings as errors.

- CTest: `7/7 PASS`
- ASan/UBSan CTest: `7/7 PASS`
- `TestSecondPlayerLifecycle`: appearance, movement, scroll-out and return,
  disconnect, self-evicting relog, and a genuine eviction still reported: `PASS`
- `verify_classic_client_772.py`: `PASS`
- `tests/secret_check.sh`: `PASS`

## Sanitisation

No account id, password, modulus or key material appears here. The account line
the operator needed for the original client was written to a scratch file
outside the repository and deleted with the harness. The temporary harness,
its driver script and the operator launcher were deleted; the runtime services
were stopped.

## Remaining UNVERIFIED

- One run, one operator, no independent repetition, no retained screenshots.
- Both characters stayed on floor 7, so no floor transition was exercised with
  two clients connected.
- Chat was deliberately avoided because `SV_CMD_TALK` is still undecoded.
- Player A's observation of Player B's movement is an operator report, not a
  machine-readable artifact. The server-side effect is corroborated by B's own
  anchor advancing and by the blocking evidence in section 6.
- Native Windows execution of Protocol772Core remains unverified.

## Scope exclusions

No Unreal, combat, chat, inventory or container work was started. This slice
proves protocol coexistence and world consistency, not rendering, and no
2D-to-3D parity cell changes on its account.
