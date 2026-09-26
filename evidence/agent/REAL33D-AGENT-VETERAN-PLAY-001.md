# REAL33D-AGENT-VETERAN-PLAY-001

Base: certified ALDRIC commit `49bad6043de685e6b32753c5876ec1cddbe579d2`.
Branch: `milestone/real33d-agent-veteran-play-001`, never merged to `main`.
State: `IMPLEMENTED_UNVERIFIED` for the new knowledge/actions; full autonomous
veteran progression `FAILED` in the live sessions below. The earlier ALDRIC
bounded-control certification is unchanged.

## Base push and source audit

Before creating this branch, `milestone/real33d-agent-aldric-001` was clean at
the exact base commit. It was pushed and `git ls-remote --heads origin
milestone/real33d-agent-aldric-001` returned the same full hash. The new branch
was created from that commit. No main merge or Fusion32 gameplay/protocol edit
occurred. The separate REAL33D2D checkout already held the previous opt-in
ALDRIC module; only reviewed module files were synchronized for live QA.

REAL33D2D's `src/client/game.cpp::Game::talk`, `buyItem`, `sellItem`, `move`,
`equipItem`, `requestTrade`, `acceptTrade` and `rejectTrade` were inspected
alongside `src/client/luafunctions.cpp` bindings, the normal NPC trade UI in
`modules/game_npctrade`, and the trade callbacks in
`src/client/protocolgameparse.cpp::parseOpenNpcTrade/parsePlayerGoods` and
`src/client/game.cpp::processOpenNpcTrade/processPlayerGoods`. The resulting
agent shop view is created only from ordinary `onOpenNpcTrade`,
`onPlayerGoods`, and `onCloseNpcTrade` callbacks. Buy/sell intents use current
offer handles with observed prices and goods, conservative options, and a
ten-second per-action cooldown. A dispatch is only an attempted trade; a
subsequent client observation must confirm the result. Opening a shop uses
ordinary conversation through `say`. Player trade confirmation remains
`NOT_STARTED` because the agent has no complete observed trade-offer state.
Existing validated `move_item` already moves observed items to equipment
slots through `g_game.move`; no distinct equip action was added because the
version-specific `g_game.equipItem` path was not verified for 7.72.

## Knowledge and fair play

`knowledge/world_v1.lua` contains public, static landmark leads for
Rookgaard and Thais, including academy, temples, known merchants and a
Rookgaard hunting-area entrance. Each record has a source ID, URL, approximate
coordinate, floor, radius and visible cue where applicable. The source pages
are [The Oracle](https://www.tibiawiki.com.br/index.php?stableid=355562&title=The_Oracle),
[Cipfried](https://www.tibiawiki.com.br/index.php?stableid=179425&title=Cipfried),
[Rookgaard Academy](https://www.tibiawiki.com.br/wiki/Rookgaard_Academy),
[Obi](https://www.tibiawiki.com.br/wiki/Obi),
[Tom](https://www.tibiawiki.com.br/wiki/Tom),
[Al Dee](https://www.tibiawiki.com.br/wiki/Al_Dee),
[Rookgaard Troll Cave](https://www.tibiawiki.com.br/wiki/Troll_Cave_%28Rookgaard%29),
[Thais](https://www.tibiawiki.com.br/Thais), and
[Temple Street](https://www.tibiawiki.com.br/wiki/Temple_Street).
These are public modern community pages, not a certified 7.72 map snapshot.
Their applicability to Fusion32 is a lead until current client observations
support it. The Oracle's current visible name near its sourced coordinate
supported recognition in the live trace. A coordinate-only match is marked
tentative. Regional bearings are approximate and never authorize a move.

The Brain receives current observation, personal memory, static world leads,
then general veteran concepts in that priority. Its compact 7x7 map contains
only tiles from the current visible viewport; `?` marks unobserved cells.
Current-session recent positions and decisions are the agent's own history.
None of the static catalogue was generated from Fusion32 map, database,
runtime files, spawn state or private player data. It contains no live
creature, corpse, NPC offer, price or player position. The existing schema,
fresh-state validator and budget remain the final action gate.

## Fresh live QA and what Aldric actually did

All sessions below used ordinary-rights character Aldric, REAL33D2D's normal
client path, Ollama `qwen3:4b`, `mock_disabled=true`, and isolated trace/memory
files. Fusion32 Query Manager/Game/Login were cleanly restarted with verified
process identities and ports 7173/7172/7171. `/api/tags` listed the model and
`/api/chat` returned a completed response. Earlier A-D sessions diagnosed a
false freshness rejection: the shop trace projection mutated an empty offer
table from `{}` to `[]`. The projection now copies the table; a deterministic
regression test covers it. Those traces remain ignored local diagnostics.
Session E confirmed accepted movement after that repair but concentrated on
The Oracle; it too remains local diagnostics.

Session F (`20260925T201237Z`) is a complete later run. Chronologically:

1. Aldric started at `(32099,32192,6)`, level 1, HP 150/150. Personal memory
   loaded 32 observation-derived records. The visible Oracle and public
   Rookgaard landmark references were available. No shop was open.
2. The first model goal was to approach the Oracle area. Its decision cited
   seven personal-memory records, seven static-world records including Oracle
   and Rookgaard Troll Cave, and seven general veteran concepts. It chose
   `move(direction=2)` with a short summary. Schema, state and budget gates
   accepted it; `g_game.walk` dispatched. The next observation moved from
   `(32099,32192,6)` to `(32099,32193,6)`. A bounded continuation reached
   `(32099,32194,6)` through a second accepted walk. Trace events 2-17 show
   the observation, goal, references, intent, validations, dispatches and
   authoritative results without chain-of-thought.
3. Over the complete run the model made 346 decisions with 101 distinct goal
   strings, all proposing movement. Forty-one bounded tactical continuations
   occurred. There were 287 accepted `g_game.walk` dispatches and 277
   correlated authoritative changes. The client observed 38 distinct
   positions on floors 6 and 7. Aldric ended at `(32101,32200,7)`, still
   level 1 with HP 150/150. His goals repeatedly returned to academy/Oracle
   exploration; he did not select a hunt, attack, loot action, conversation,
   shop interaction, purchase, sale or equipment upgrade.
4. Fresh-state and rate gates rejected 50 materially changed observations,
   21 blocked/not-currently-walkable moves and 29 cooldown attempts. The
   provider returned 60 invalid summaries and 5 invalid goals, all without
   dispatch. No `protocol_error` trace event occurred. Clean session end
   saved 54 memory records.

The model's tendency to backtrack led to a final, short session G after
adding current-session position/decision history and approximate public
landmark bearings. G (`20260926T001257Z`) loaded F's 54 records. From
`(32101,32200,7)` it chose to move north toward the academy twice. Both
intents passed schema but were rejected by the fresh state gate as
`tile_not_visible_walkable`; no dispatch occurred. The user ended the test.
Fusion32 was stopped cleanly, the client closed offline, and G ended with
`memory_saved=true` and 54 records. This is evidence of safe rejection, not
evidence of autonomous progression.

## Tests and retained evidence

Bundled LuaJIT `tests/real33d_agent_test.lua` (MVP/BRIDGE/MEMORY),
`tests/real33d_aldric_test.lua`, and
`tests/real33d_veteran_knowledge_test.lua`: PASS. The two-session MEMORY replay
and PowerShell launcher regression: PASS. `bash tests/secret_check.sh`: PASS.
The existing bounded `tests/real33d_aldric_live_test.lua` replay on F's
sanitized trace: PASS for real LLM control and correlated walking; it does not
certify veteran progression. No live buy/sell or equipment transaction was
observed.

The committed `veteran/session_f_trace.jsonl` and
`veteran/session_g_trace.jsonl` are complete contiguous sessions sanitized by
`tests/sanitize_aldric_trace.py`. The sanitizer removes local paths, raw
viewport tiles and chat; the raw traces and memory files remain ignored local
diagnostics. Reproduce with:

```text
luajit tests/real33d_agent_test.lua
luajit tests/real33d_aldric_test.lua
luajit tests/real33d_veteran_knowledge_test.lua
luajit tests/real33d_aldric_live_test.lua evidence/agent/veteran/session_f_trace.jsonl
bash tests/secret_check.sh
```

| Artifact | SHA-256 |
| --- | --- |
| Raw F trace (ignored) | `5F366C4D05E1CF56B2598F498B264A494C5C4C244CAF4088DF673AFF48AC26F2` |
| Sanitized F trace | `4077489711C55CE4720942B99F14595ABE40880B2B48321692CA4DD7591B6989` |
| Raw G trace (ignored) | `BAB5B3BE028F695F3FE718EBCB8326C121A2F457AD73150CB4C802B648E9F82D` |
| Sanitized G trace | `B109C938606A5C63692229E024401BF6A1E89B5419E369D3AB122A389B945DD1` |

The full veteran-play goal remains `FAILED` in live QA. Landmark recognition,
knowledge provenance, memory use, ordinary walking, and final validation have
evidence. Combat, looting, economic reasoning expressed as an action, NPC
interaction, buy/sell, and equipment improvement have no observed PASS here.
