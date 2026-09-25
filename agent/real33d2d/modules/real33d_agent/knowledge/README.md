# Aldric veteran knowledge v1

`v1.lua` is the static, read-only, auditable player knowledge layer. Each record
has a stable ID, topics, a short concept, and a source ID. The module retrieves
at most seven records per strategic decision. It is never written by play;
personal experiences go only to `AgentMemory` with direct observation IDs.

The source links point to the [official Tibia character manual](https://www.tibia.com/gameguides/?section=characters&subtopic=manual),
[combat manual](https://www.tibia.com/gameguides/?section=combat&subtopic=manual),
[world manual](https://www.tibia.com/gameguides/?section=world&subtopic=manual),
[trading manual](https://www.tibia.com/gameguides/?section=controls_trading&subtopic=manual),
and [communication manual](https://www.tibia.com/gameguides/?section=controls_communication&subtopic=manual).
Those pages describe the current game; they are used here only for durable
player-level concepts. They do not certify a 7.72 item price, spell rule,
market feature, shop stock, location, spawn or protocol behavior. Specific
Fusion32 behavior remains `UNVERIFIED` until the client directly observes it.

The catalogue intentionally has no coordinates, creature runtime IDs, item
handles, NPC inventories, price tables, map data or server-derived state.
Current `AgentObservation` outranks personal memory and these concepts. Neither
memory nor knowledge is passed to the action validator as current state.

The present bridge exposes no NPC buy/sell action. Aldric can plan an economic
goal and converse through `say`, but NPC purchase execution is
`NOT_IMPLEMENTED`. No hidden protocol or server path is used to fill that gap.
