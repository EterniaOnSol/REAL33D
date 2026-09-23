# UNREAL-COMBAT-FOLLOW-001

Date: 2026-09-23 (America/Guatemala)
Base: `0cbcaf771091b18cd5c92afbe07333dfda918473`
Result: `CERTIFIED_PASS`

## Authority inspected

| Concern | Fusion32 source of truth | Observed contract |
| --- | --- | --- |
| Opcodes | `reference/game/src/connections.hh` | tactics 160, attack 161, follow 162, cancel 190; clear target 163 |
| Tactics | `reference/game/src/receiving.cc::CSetTactics` and `enums.hh` | attack 1/2/3, chase 0/1, secure 0/1, exactly three body bytes |
| Attack/follow | `receiving.cc::CAttack`; `crcombat.cc::TCombat::SetAttackDest` and `CanToDoAttack` | one little-endian creature quad; the handler's `Follow` flag selects authoritative chase without attacks |
| Cancel | `receiving.cc::CCancel` | no body; stops combat and the current to-do action |
| Revoke/reject | `crcombat.cc::TCombat::StopAttack`; `sending.cc::SendClearTarget` | one-byte server command; accepted targets have no positive acknowledgement |

The classic client is available only as a binary and was not used to invent a
gesture. The original Shift-click proposal was removed. Battle List rows use a
normal attack toggle plus an explicit visible Follow/Stop button; world
right-click uses the same ClientCore/WorldState state.

## Deterministic evidence

`tests/build_clientcore_windows.cmd` was run from the VS 2022 x64 environment.
Both the C++17 library and the C++20 Unreal archive built at `/W4 /WX
/permissive-`. All eight suites passed. The new assertions include exact bytes
for attack, follow, cancel and all tactics fields; attack/follow replacement;
server clear; semantic diff; and reset.

```
transport 22/22   crypto 25/25   login PASS   gamelogin PASS
initial_world PASS   movement PASS   player_state PASS   worldview PASS
WINDOWS CLIENTCORE: PASS
```

The final Unreal command was:

```
Build.bat REAL33DEditor Win64 Development -Project=.../REAL33D.uproject -WaitMutex
Result: Succeeded
```

## Live acceptance

The retained Unreal logs and EndPlay snapshots were generated against real
Fusion32 creatures. Selected lines are shown with Unreal timestamps:

```
21:48:52 combat input 61 from battle list: attack creature 1073743327
21:48:52 combat state: target=1073743327 action=attack
21:48:55 combat input 62 from battle list: attack creature 1073743319
21:48:55 server message [FailureMessage]: Target lost.
21:48:55 combat state: target=0 action=none
21:49:05 combat input 65 from world: follow creature 1073743327
21:49:06 combat state: target=1073743327 action=follow
21:49:10 combat input 66 from world: attack creature 1073743327
21:49:10 combat state: target=1073743327 action=attack
21:49:15 combat input 67 from battle list: attack creature 1073743327
21:49:15 combat state: target=0 action=none
22:05:11 combat input 2 from world right-click: attack creature 1073765524
22:05:11 combat state: target=1073765524 action=attack
22:05:29 combat state: target=0 action=none
22:05:31 use 5 from world: object 4173 ... stack 2
```

The operator then confirmed that Battle List attack/follow and their switching
and cancellation worked, that the fight-stance controls loaded and worked, and
that right-click world interaction was correct. Server-authored movement while
following remained the only movement authority; no presentation-side chase was
added.

`evidence/clientcore/unreal-combat-follow/B/unreal_slice_evidence.json` is the
final 117.2-second run: 246 frames, 496 commands, 8 attack requests, 4 tactics
requests, 6 server target clears, 30 Unreal walk requests, 54 separately
counted external relocations, synchronised WorldState/presentation counts, and:

```
residual_bytes       0
unsupported_opcodes  0
protocol_anomalies   0
```

It also retains the unaffected equipment/inventory slots and the server-opened
container `{type_id: 4173, name: "dead rabbit", capacity: 5}`. The corpse is
currently presented by the existing generic physical placeholder because no
approved corpse mesh exists; its server type and container semantics are
correct, and the excluded V08 system was not changed.

## Acceptance verdict

| Criterion | Result |
| --- | --- |
| Attack request reaches Fusion32; valid target active | PASS |
| Attack target switching and cancel | PASS |
| Server rejection/cancellation | PASS |
| Follow request and server-authoritative following | PASS |
| Follow switching and cancel | PASS |
| Target leaving/removal clears shared state | PASS |
| Battle List and world actor share target/feedback | PASS |
| Logout/session reset clears state | PASS |
| Attack/balanced/defensive and stand/follow modes | PASS |
| Movement and inventory/containers remain correct | PASS |
| Build and protocol counters | PASS / 0 |

Out of scope remained untouched: V08, WideWorld, REAL33D2D, shops, action bars,
automap, reconnect handling, opcode 50 and protocol extensions.
