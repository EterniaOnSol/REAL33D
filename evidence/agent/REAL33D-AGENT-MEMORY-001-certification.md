# REAL33D-AGENT-MEMORY-001 live certification

State: `CERTIFIED` for the specified two-session, one-character memory boundary.
Date: 2026-09-24 21:16 -06:00. Branch: `milestone/real33d-agent-memory-001`.
Starting commit: `c321f6f29078e509f34986f2c4aca365d03c4640`.

## Preconditions and fixture

Fusion32 QA login/game/querymanager services ran locally on ports 7171/7172/7173.
The REAL33D2D Release client was a separate fresh process for each session,
using the opt-in bridge, deterministic `mock` Brain, and memory enabled. The
local QA credentials remained in the ignored environment configuration; they
are absent from these artifacts. A read-only `CharacterRights` query returned
no rows for QA character IDs 1001 and 1002. The client used the normal OTClient
parser/game model and `g_game` action path. No LLM or server gameplay change was
used.

Memory began in a new empty directory. After A ended, the single memory JSON
was copied byte-for-byte to `memory_after_A.json` (SHA-256 below). For B, the
server was stopped and the QA character file was backed up, then only its
`CurrentPosition` was set to `(32097,32209,7)`, a walkable tile directly seen
in A. This operator fixture made the remembered bucket invisible at B login;
it was never read or changed by the Brain. After B, the original character file
was restored while the server was stopped. The restored file and untouched
backup had identical SHA-256
`b562fe0c2c969c191ae4846e0cd5ae5121a5fbefca8ed07d6f995339801aef70`.
The three QA services were restarted and verified alive.

## Session A

Fresh client PID 24468, trace session `20260925T025638Z`:
`memory.state=new`, `records=0`, followed by one normal `session_end` with
`memory_saved=true`, `memory_records=20`. Observations recorded 4 place buckets,
5 walked routes, a directly visible Cipfried sighting, chat, and owned item
observations. The saved file has `sessions=1`; each of its 20 records says
`source=direct_observation` and cites observation IDs present in A's trace.
No creature runtime ID or item handle was persisted.

## Session B

Fresh client PID 27420, trace session `20260925T031136Z`:
`memory.state=loaded`, `records=20`, `sessions=2`. The first observation placed
the character at `(32097,32209,7)`. Neither a tile in the remembered
`32095:32215:7` place bucket nor Cipfried appeared in that observation. The
Brain's first memory-guided move used goal `32095:32215:7`, direction 2, from
observation `o-20260925T031136Z-000005`. Schema, state, and budget validations
all accepted it. Dispatch used `g_game.walk`; the correlated result observed
the Fusion32-authoritative y change `32209 -> 32210`. Subsequent observations
showed the character at y=32211, then Cipfried freshly visible at y=32212,
then arrival in the remembered y=32215 bucket. B ended normally with
`memory_saved=true`, `memory_records=26`.

The live evidence test deliberately takes Cipfried's runtime ID from A and
tries `attack` and `follow` against B's initial current observation. The exact
`AgentCore.validate` implementation rejects both with `creature_not_visible`.
The same attack intent becomes valid only after a new B observation includes
that creature ID. A remembered item is rejected as `item_not_visible`, and an
unobserved move as `tile_not_visible_walkable`. This test submits intents to
the validator only; it does not dispatch an attack on Cipfried. A load under
`Test Player B` is rejected as `memory_identity_mismatch`, and the hex identity
keys differ. B observations have no memory field or unobserved map bucket.

## Client and protocol checks

`session_B_client.log` contains the ordinary `g_game` actions and game results.
A case-insensitive scan for `[error]`, `[fatal]`, protocol error, unknown opcode,
parse error, packet error, and disconnect returned zero matches. There were no
`protocol_error` events in B's JSONL. A four-second normal-client startup with
all agent opt-in variables removed produced zero agent events. The existing
opt-in test also confirms no agent timer, observation, or trace when disabled.
No human client parser, model, protocol, or gameplay function was changed.

The mock agent moved about once every 3 seconds in this live run. That can let
monsters move out of reach; movement tuning and survival were not part of this
memory certification and remain unverified.

## Reproduction

From the repository root, run REAL33D2D's bundled LuaJIT:

```text
luajit tests/real33d_agent_test.lua
luajit tests/real33d_agent_memory_live_test.lua evidence/agent/memory/session_A.jsonl evidence/agent/memory/session_B.jsonl evidence/agent/memory/memory_after_A.json
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/real33d_agent_launcher_test.ps1
bash tests/secret_check.sh
```

The live evidence test correlates both sessions, A's saved records and their
source observations, B's absent initial target, guided intent, three validation
stages, normal dispatch, authoritative result, stale-target refusal, fresh
visibility, and character isolation.

| Artifact | SHA-256 |
| --- | --- |
| `memory/session_A.jsonl` | `BE089C44EFF66FC05BD7F20F3C3B258AC35F773DCE157413403EB873B274885F` |
| `memory/session_B.jsonl` | `087CEF634FF825319E9BD5B29D29CCC5CBDD939440262EE0CE9AEC208309A7B0` |
| `memory/memory_after_A.json` | `599BF252FB4A67B1B8B385C1D53F378CD61690169531F68B474DCFC8BAAA1459` |
| `memory/session_B_client.log` | `C8B98DE0768F91CE3C92AEAEAE67D057C440563AEE932148FB238ABACB9ED04C` |

One live defect was found before A: dynamic `Get-Item Env:$name` failed on this
Windows process with a duplicate-key error. The launcher now reads and sets
process environment variables through the .NET API. The dedicated launcher
regression test and both successful fresh client launches cover that fix.

## Dispatch-path attribution correction — 2026-09-25

The retained MEMORY JSONL says `path=g_game.move` for a `move` intent because
the bridge constructed that field from the action name. Read-only source
inspection during ALDRIC recovery showed the actual dispatcher calls
`g_game.walk(direction)` for `move`; `g_game.move` is used for `move_item`.
The observed server position change and MEMORY certification stand. The
historical trace is preserved byte-for-byte, and the ALDRIC bridge now records
the actual API path returned by the dispatcher.
