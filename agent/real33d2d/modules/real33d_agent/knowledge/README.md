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

The general veteran catalogue intentionally has no coordinates, creature runtime IDs, item
handles, NPC inventories, price tables, map data or server-derived state.
Current `AgentObservation` outranks personal memory and these concepts. Neither
memory nor knowledge is passed to the action validator as current state.

## Static world knowledge v1

`world_v1.lua` is a separate, immutable catalogue of public player-level
landmarks. Every record has a stable ID, source URL, approximate coordinate,
floor, radius, named visual cue, and short description. Sources are public
TibiaWiki player pages for [The Oracle](https://www.tibiawiki.com.br/index.php?stableid=355562&title=The_Oracle),
[Cipfried and Rookgaard temple](https://www.tibiawiki.com.br/index.php?stableid=179425&title=Cipfried),
[Rookgaard Academy](https://www.tibiawiki.com.br/wiki/Rookgaard_Academy),
[Thais and Quentin](https://www.tibiawiki.com.br/Thais), and
[Temple Street](https://www.tibiawiki.com.br/wiki/Temple_Street).
These are current community pages, not a verified 7.72 map snapshot. A
coordinate-only lookup is explicitly tentative. A visible matching named cue
strengthens recognition, but the current observation still decides what is
walkable and what is present. No coordinate or record is an action target.

The catalogue was curated from those public pages. It was not generated from
Fusion32 map, database, source, runtime files, spawn tables, or other privileged
material. It contains no live occupancy, private player state, corpse contents,
NPC stock, or prices. Historical applicability to Fusion32 is `UNVERIFIED` until
ordinary client observations confirm the landmark.

The bridge now also observes an NPC shop only through REAL33D2D's ordinary
`onOpenNpcTrade`, `onPlayerGoods` and `onCloseNpcTrade` client events. Buy/sell
intents require the shop to be open and the current offer, observed price,
money, capacity or goods to pass the validator. They dispatch through
`g_game.buyItem` and `g_game.sellItem` with conservative options and a ten-second
per-action rate limit. A successful dispatch is not proof of a completed
transaction; the next authoritative client observation must show the result.
Player-to-player trade confirmation remains `NOT_IMPLEMENTED`.
