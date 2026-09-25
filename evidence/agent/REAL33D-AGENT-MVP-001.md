# REAL33D-AGENT-MVP-001

State: `PASS` for one live deterministic 2D player. Actual Ollama inference is
`IMPLEMENTED_UNVERIFIED`. Date: 2026-09-24, America/Guatemala.

## Scope and authority

- REAL33D2D `aeef6aa939a764de3728c51a3022953dff97f375` supplied the
  existing OTClient parser, `LocalPlayer`, UIMap, tiles, creatures, containers,
  and `g_game` action bindings. The new module does not import ClientCore or
  WorldState.
- Fusion32 `bd15cc0a49d8182dc1cc3732b8487859f1662044` supplied the
  authoritative game. Inspected `reference/game/src/receiving.cc::CAttack`,
  `CMoveObject`, `CUseObject`; `crcombat.cc::SetAttackDest`, `StopAttack`;
  `moveuse.cc::UseFood`; and `sending.cc::SendClearTarget`. The module changed
  no gameplay or protocol code.
- `REAL33D2D/src/client/game.cpp::walk`, `talk`, `attack`, `follow`, `open`,
  `use`, `move`, and `setChaseMode` send through the ordinary protocol path.

## Local QA precondition

At first, synthetic character A held 111 GM rights including `NO_ATTACK` and
`INVULNERABLE`, so its attack attempt could not prove fair play. B held zero
rights but died after poison during exploratory tests. At the operator's
request to change the GM character, Fusion32 was stopped, the **local QA
SQLite fixture** was backed up to
`/var/lib/fusion32-server-baseline-772-0/querymanager/state/tibia.before-agent-rights.sqlite`,
and A's 111 rights were removed. A and B both read `NONE` afterward. This was
a one-time test-fixture setup operation by the developer, not an action of the
agent; no Fusion32 source, gameplay rule, or protocol was changed. A started
the accepted runs with HP 155/155.

## Named checks

| Check | Preconditions and command | Expected / observed | State |
| --- | --- | --- | --- |
| `REAL33D_AGENT_LUA_POLICY` | Built REAL33D2D LuaJIT; `luajit.exe tests/real33d_agent_test.lua` from fusion32 | Validator rejects hidden/nonmonster attack, blocked move, invalid item/count/action; budget enforces gaps and 24/min; mock prioritizes attack, never cuts off a visible healthy target or a manual target; loot/open/move/use/count progression passes; mocked Ollama callback passes | PASS |
| `REAL33D_AGENT_VIEWPORT` | Same test with a fake UIMap | Offscreen creature 9 and tile x=105 omitted; visible creature 2, own HP, carried item retained | PASS |
| `REAL33D_AGENT_LOGIN_CHAT_MOVE` | Prepared local Fusion32, opt-in REAL33D2D mock, A rights `NONE` | At 16:09:13 A entered; `INTENT say` received server echo; positions changed by server-confirmed steps; HP remained 155/155 during the monitored multi-minute run | PASS |
| `REAL33D_AGENT_COMBAT` | Same session, visible deer 1073743183 | At 16:09:13 `INTENT attack`; next observation `attack=1073743183`; `INTENT combat_mode` enabled normal chase; deer HP 100→92→72→56→40→20→4. Later corrected run attacked visible rabbit 1073743321: HP 73→26→6 with **no agent cancel**; server then sent clear-target | PASS |
| `REAL33D_AGENT_LOOT` | A live, rabbit killed and corpse tile visible | At 16:21:56 `open_container` on visible tile item; `dead rabbit[3577:x1]` opened; at 16:21:59 `move_item c:1:0 → c:0:2`; at 16:22:02 corpse `[]`, bag gained `3577:x1` | PASS |
| `REAL33D_AGENT_EAT` | Same A in next login, previously looted 3577 still visible in bag | At 16:25:48 bag `[3577,3270,3585]`, `INTENT use item=c:0:0`; at 16:25:51 authoritative container update bag `[3270,3585]`. `UseFood` deletes one object on accepted consumption | PASS |
| `REAL33D_AGENT_OLLAMA_ADAPTER` | Mock HTTP callback, no local Ollama model/server | Provider serializes bounded observation and returns intent through the same validator and budget | PASS for adapter; actual inference `IMPLEMENTED_UNVERIFIED` |

The loot and eat checks used the **same character** across two consecutive
sessions. Item ID 3577 was learned from the client's visible `dead rabbit`
container and observed move into A's bag. The mock's subsequent use of that
ID is remembered client-visible knowledge, not a server loot table. The item
count reduction is the authoritative acceptance signal. No food name was
claimed: the shipped 7.72 client returns empty `ThingType` names for these
items, and its `isUsable` flag was false even for the consumed item.

Raw local client logs are retained under ignored `evidence/agent/live/`:
`A_normal_combat_3min_2026-09-24.log`, `A_final_rabbit_inventory_2026-09-24.log`,
`A_loot_no_eat_2026-09-24.log`, and
`A_ate_looted_food_2026-09-24.log`. They are ignored because local chat and
account identifiers may appear. The trace above contains the required
reproducible facts. Fusion32 QA services were stopped cleanly after the runs.

## Fair-play boundaries and remaining work

- Observation reads the current UIMap 11×11 subset on the current floor, sight
  spectators, own stats and position, received chat, equipment, open
  containers, and attack/follow/combat mode. It does not expose map cache,
  unseen creatures, spawn data, other inventories, QA rights, server files or
  database state to a Brain.
- Every intent is revalidated against fresh client state and dispatched by
  ordinary `g_game` functions. The budget allows at most 24 actions per rolling
  minute; mock decisions occur no faster than 2.2 seconds, LLM decisions no
  faster than 15 seconds. No new opcode, packet, or direct server mutation was
  added to the client.
- Follow has a legal validated dispatcher and deterministic test. The accepted
  combat proof used Attack with the client's ordinary chase mode; a separate
  live `follow` intent on a rights-free player remains `IMPLEMENTED_UNVERIFIED`.
- Low-HP cancellation/retreat has policy tests. A's accepted combat runs had
  full HP; survival under sustained incoming damage remains
  `IMPLEMENTED_UNVERIFIED`.
- The provider-independent Ollama Brain is implemented and tested with a mock
  HTTP response. No actual local model was installed or invoked.
