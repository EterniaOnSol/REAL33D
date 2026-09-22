# FUSION32_772_TO_860_EVOLUTION

Milestone: `FUSION32-772-TO-860-EVOLUTION-RESEARCH-001`
Mode: **RESEARCH ONLY / READ ONLY**
Primary subject: `C:\Users\dell\Desktop\fusion32` (Fusion32 / Tibia 7.72)
Reference corpus: `C:\Users\dell\Desktop\distribuciones` (operator-indicated)
Date: 2026-09-21
Status: **scratch, uncommitted, outside every project tree.**

Evidence labels: `DEMONSTRATED` (quoted from source present on disk) · `STRONG_INFERENCE` · `HYPOTHESIS` · `NOT_PROVEN`.

---

## 0. Two findings that reframe the whole question

Before the structured answer, two things emerged that change what "going to 8.6" is even worth.

### 0.1 The 7.72 and 8.60 opcode spaces are the same lineage

Fusion32 declares its commands in **decimal**; TFS 8.60 dispatches in **hex**. Converted, they are the same numbers with the same meanings.

| Fusion32 (decimal) | hex | TFS 8.60 handler |
| --- | --- | --- |
| `CL_CMD_LOGOUT = 20` | `0x14` | `case 0x14: logout` |
| `CL_CMD_PING = 30` | `0x1E` | `case 0x1E: playerReceivePing` |
| `CL_CMD_GO_PATH = 100` | `0x64` | `case 0x64: parseAutoWalk` |
| `CL_CMD_GO_NORTH = 101` | `0x65` | `DIRECTION_NORTH, // 0x65` |
| `CL_CMD_MOVE_OBJECT = 120` | `0x78` | `case 0x78: parseThrow` |
| `CL_CMD_USE_TWO_OBJECTS = 131` | `0x83` | `case 0x83: parseUseItemEx` |
| `CL_CMD_LOOK_AT_POINT = 140` | `0x8C` | `case 0x8C: parseLookAt` |
| `CL_CMD_TALK = 150` | `0x96` | `case 0x96: parseSay` |
| `CL_CMD_ATTACK = 161` | `0xA1` | `case 0xA1: parseAttack` |
| `CL_CMD_SET_OUTFIT = 211` | `0xD3` | `case 0xD3: parseSetOutfit` |

`DEMONSTRATED` — `reference/game/src/connections.hh:12-138` against `forgottenserver-downgrade-1.8-8.60-main/src/protocolgame.cpp:677-905`.

Roughly **50 of Fusion32's 58 client commands** and **~45 of its 62 server commands** occupy the identical opcode number in 8.60. 8.60 is not a different protocol; it is 7.72 **plus additions**, minus the rule-violation queue block (`0xAE-0xB1`, Fusion32 `174-177`), which was relocated to `0xF2`.

The practical consequence is large: the gap between the two is a *set of additional opcodes*, not a redesign. Fusion32's dispatch already ignores unknown inbound opcodes (established in the previous milestone at `receiving.cc:1786`), and 8.60's additions sit almost entirely in ranges Fusion32 leaves free.

### 0.2 The 8.60 viewport is identical to 7.72 — migrating buys zero extra world

```
Greed-TFS-1.5-Downgrades-8.60/src/map.h:177   static constexpr int32_t maxClientViewportX = 8;
Greed-TFS-1.5-Downgrades-8.60/src/map.h:178   static constexpr int32_t maxClientViewportY = 6;
protocolgame.cpp:1730  GetMapDescription(pos.x - maxClientViewportX, pos.y - maxClientViewportY, pos.z,
                                         (maxClientViewportX * 2) + 2, (maxClientViewportY * 2) + 2, msg);
```

`(8 × 2) + 2 = 18`, `(6 × 2) + 2 = 14`. `DEMONSTRATED`.

Fusion32: `TerminalOffsetX = 8; TerminalOffsetY = 6; TerminalWidth = 18; TerminalHeight = 14;` (`connections.cc:219-222`).

**The values are identical.** The 8.6 era did not widen the client window. If wide world is a motivation for migration, migration does not deliver it — the previous milestone's conclusion stands unchanged: the wide *visual* world is a client-side sector-streaming problem requiring no server change at all.

---

## 1. What "going to 8.6" could mean — three modes

### MODE A — 7.72 protocol + 8.6-like content
Keep protocol, architecture and `Tibia.exe` 7.72 compatibility; import later content only where the existing object/monster/NPC/map systems can represent it.

| Dimension | Assessment |
| --- | --- |
| Feasibility | **High** for map/monsters/NPCs/quests; **medium** for items; **low** for spells |
| Reuse of Fusion32 | ~100% — no code change for most of it |
| Protocol impact | **None** |
| Save impact | None (new content uses existing `QuestValues`, existing `.usr` fields) |
| Map impact | New `.sec` files within existing sector bounds |
| Item impact | Bounded by `objects.srv` and the 4-slot instance attribute limit (§5) |
| Client impact | **`CLIENT_ASSET_REQUIRED` is the real gate** — a 7.72 `Tibia.dat/.spr` cannot render IDs it does not have |
| Risk | **Low** |

The binding constraint in Mode A is not the server. It is that the classic client renders from its own `.dat/.spr`; any item, outfit, effect or projectile absent from the 7.72 asset set is unrenderable regardless of what the server sends. REAL33D is not bound this way (it renders from its own catalogue), which makes Mode A **asymmetric**: REAL33D can see more Mode-A content than `Tibia.exe` can.

### MODE B — 7.72 core + post-7.72 features
Add later *systems* (shops, cooldowns, conditions icons, modal windows, addons), extending Fusion32 and REAL33D while keeping `Tibia.exe` working via capability gating.

| Dimension | Assessment |
| --- | --- |
| Feasibility | **Medium-high** — the opcode numbers to use are known from §0.1 |
| Reuse | ~90% of Fusion32 |
| Protocol impact | Additive, REAL33D-gated via `TerminalType` |
| Save impact | **This is where `.usr` positional parsing bites** (§9) |
| Map/item impact | As Mode A |
| Client impact | REAL33D implements new opcodes; `Tibia.exe` never receives them |
| Risk | **Medium** — every new server→client opcode must be gated or a classic client desyncs |

### MODE C — true 8.6 compatibility
Evolve Fusion32 to accept a real 8.6 client.

| Dimension | Assessment |
| --- | --- |
| Feasibility | **Lower than the opcode overlap suggests** — see below |
| Reuse | Game logic yes; login/RSA/framing, item encoding, map encoding all change |
| Protocol impact | Total |
| Save impact | High |
| Map impact | Item IDs must be remapped to the 8.6 `.dat` identity space |
| Client impact | Replaces `Tibia.exe` 7.72 entirely |
| Risk | **High** |

The opcode overlap (§0.1) makes Mode C look cheaper than it is. What overlaps is the *command vocabulary*. What differs is the *encoding inside each command*: 8.60 outfits carry `lookAddons` and `lookMount` (`protocolgame.cpp:2261-2275`); 8.60 has a shop window protocol 7.72 lacks entirely; the map description is produced by a different stack walker; and 7.72's login block layout differs. Mode C means rewriting every encoder while keeping every game rule. `STRONG_INFERENCE`.

**Mode C also delivers nothing the project currently wants.** It does not widen the viewport (§0.2), and the content it unlocks is gated on client assets either way.

---

## 2. Fusion32 7.72 inventory

| Subsystem | Status | Evidence |
| --- | --- | --- |
| Map | `EXISTING` | `.sec`, 32×32 tiles/sector, `map.hh:74-79`; hot-reload via `RefreshSector` |
| Items / object types | `EXISTING` | 5,003 object types (project tooling, `PROJECT_STATUS.md`); 66 flags, 62 type attrs, 18 instance attrs (`enums.hh`) |
| Containers | `EXISTING` | `containers.hh`; `OpenContainer[16]`, `MAX_OBJECTS_PER_CONTAINER 36` |
| NPCs | `EXISTING` | `.npc` files + own behaviour language, `crnonpl.cc:3124-3141` |
| **NPC shop window** | **`ABSENT`** | no shop/purchase/sale opcode anywhere in `connections.hh` — 7.72 trades via chat |
| Monsters | `EXISTING` | `.mon` files, `crmain.cc:1688-1705` |
| Monster raids / events | `EXISTING` | `.evt`, `LoadMonsterRaids` `crmain.cc:1901` |
| Spells | `PARTIAL` | **100 hardcoded** `CreateSpell(...)` calls, ids ≤ 102, `magic.cc:4416+` |
| Combat | `EXISTING` | `crcombat.cc`; damage types, protection, `AVOIDDAMAGETYPES` |
| **Conditions / buffs / debuffs** | `PARTIAL` | `SV_CMD_PLAYER_STATE` carries a state byte; no general condition framework |
| **Cooldowns** | `PARTIAL` | exhaustion exists (`EXHAUSTED`, `sending.cc:336`); no per-spell cooldown display |
| Quests | `EXISTING` | `QuestValues[500]` (`cr.hh:146,916`) + `MOVEUSE_CONDITION_HASQUESTVALUE` |
| Houses | `EXISTING` | `houses.cc` 54 KB, own data files, own QueryManager RPCs, auctions |
| Depots | `EXISTING` | `Depot[MAX_DEPOTS]` (`cr.hh:150`), `GetDepotSize` premium-aware |
| Trade (player↔player) | `EXISTING` | `receiving.cc:290-383`, four commands |
| Party | `EXISTING` | five commands `163-167`; `SV_CMD_CREATURE_PARTY` |
| **Shared party experience** | `ABSENT` | 8.60 `0xA8`; no Fusion32 equivalent |
| Guilds | `PARTIAL` | guild/rank/title strings from `loginGame`; no in-game guild system |
| Buddy / VIP | `EXISTING` | `220/221`, `SV_CMD_BUDDY_*` |
| Outfits | `PARTIAL` | lookType + 4 colours (`sending.cc:162-169`); **no addons, no mount** |
| Effects | `EXISTING` | `SendGraphicalEffect`, byte type |
| Projectiles | `EXISTING` | `SendMissileEffect` |
| Player skills | `EXISTING` | fist/club/axe/sword/distance/shielding/magic/fishing (`createHighscores`) |
| Player stats | `EXISTING` | `SV_CMD_PLAYER_DATA`, `SV_CMD_PLAYER_SKILLS` |
| Persistence | `EXISTING` | `.usr` text files + SQL via QueryManager |
| Admin / GM | `EXISTING` | rights bitmask, per-reason banishment matrix, `TerminalType 2` |
| **Modal windows** | `ABSENT` | 8.60 `0xFA`/`0xF9` |
| **Map markers** | `ABSENT` | 8.60 `sendAddMarker` |
| **Battle-list look** | `ABSENT` | 8.60 `0x8D` |

---

## 3. The 8.6 reference — provenance, and an important negative result

### 3.1 What is NOT here

**No `BronsonServer86`. No `cryingdamson`. No CipSoft 8.6 original.** Exhaustive filename search across `C:\Users\dell` returned nothing matching those names. `DEMONSTRATED` (negative result).

This matters: everything below compares Fusion32 against **OTServ-lineage reimplementations**, not against an authoritative CipSoft 8.6 server. Where the previous milestone could say "this is what the original does", this one can only say "this is what a widely-used 8.60 reimplementation does". Treat every 8.60 claim as *reference behaviour*, not canon.

### 3.2 Reference A — `forgottenserver-downgrade-1.8-8.60`

| Attribute | Value |
| --- | --- |
| Provenance | `Mateuzkl/forgottenserver-downgrade-1.8-8.60`, GitHub |
| Engine | TFS 1.8 |
| Protocol | `CLIENT_VERSION_MIN/MAX = 860` (`src/definitions.h:12-14`) |
| Language | C++23 + Lua 5.4 |
| DB | MariaDB |
| Map format | OTBM (`world.otbm`, 4.1 MB) + `world-spawn.xml` + `world-house.xml` |
| Item format | `items.otb` (2.1 MB) + `items.xml` (3.1 MB) |
| **License** | **GPL-2.0** |

### 3.3 Reference B — `Greed-TFS-1.5-Downgrades-8.60`

| Attribute | Value |
| --- | --- |
| Engine | TFS 1.5 |
| Protocol | `CLIENT_VERSION_STR = "8.60"` (`src/definitions.h:27-29`) |
| Map | `forgotten.otbm` (3.4 MB), Remere's Map Editor 3.7.0 |
| Items | 5,999 entries, **max id 12,660** |
| Spells | 167 instant + 33 rune = **200**, XML-declared |
| Monsters | 15 XML | NPCs | 21 |

### 3.4 The content-era trap — quantified

Reference A's *engine* is 8.60. Its *content* is not.

| Measurement | Reference A | Reference B (Greed) | Fusion32 7.72 |
| --- | --- | --- | --- |
| Item entries | 17,308 | 5,999 | 5,003 object types |
| Max item id | **51,747** | **12,660** | 7.72 range |
| Monster files | 1,705 Lua | 15 XML | 159 races |
| Max monster `lookType` | **1,869** | — | 7.72 outfit range |
| Monsters with `lookType > 400` | **770 of 1,615** | — | n/a |
| Monsters carrying `Bestiary` blocks | **763** | — | n/a |
| Bundled tooling | `imbuments_scroll.gif`, `forge.gif` (93 MB) | — | n/a |

`DEMONSTRATED` by direct counts.

Bestiary, imbuements and forge are systems from far later eras; `lookType 1869` and item id `51747` are far outside anything an 8.6 client can render. **Reference A is an 8.60-protocol engine carrying modern-era content.** Importing its `data/` wholesale would inject thousands of IDs unrenderable by *either* a 7.72 or a true 8.6 client.

Reference B is materially more era-appropriate (max item id 12,660) and is the better content comparator.

Neither ships the real Tibia world map: Reference A's spawns sit at `centerx="830" centery="1046"`, Reference B's at `centerx="149" centery="574"` — custom maps, not the ~32,768-centred Tibia world Fusion32 runs (`audit_wide_world_sources.py` defaults to `32330, 32226`). `DEMONSTRATED`.

### 3.5 Licensing — a hard constraint, stated plainly

Both references are **GPL-2.0**. Fusion32's own `LICENSE.txt` places it in the public domain, and REAL33D is a private repository. Copying TFS **source** into Fusion32 or REAL33D would attach GPL-2.0 obligations to the combined work.

`DEMONSTRATED` for the licence text; the legal consequence is a well-established reading of GPL-2.0 but this is **not legal advice**, and data/content files may carry different terms than source — `NOT_PROVEN` per file.

**Practical guidance:** use the references as *specifications to read*, not as code to copy. §0.1's opcode correspondence is a fact about the Tibia protocol, observable from either side, and is the kind of knowledge that transfers cleanly. Lua scripts, C++ files and generated `items.otb` do not.

---

## 4. Content delta 7.72 → 8.6

| Category | Classification | Reasoning |
| --- | --- | --- |
| **Map areas / cities / islands** | `CAN_IMPORT_DIRECTLY` after format conversion | `.sec` is a text script; geometry is era-neutral. Constraint is sector bounds and object budget, not era. Requires an OTBM→`.sec` converter (§6) |
| **Monsters** | `NEEDS_ID_REMAP` + `NEEDS_CLIENT_ASSET` | `.mon` format expresses health/speed/attacks/loot/immunities already. Blocker is `lookType` — 770/1,615 in Reference A exceed the 7.72 outfit range |
| **NPCs** | `CAN_IMPORT_DIRECTLY` (dialogue) / `NEEDS_SERVER_CODE` (shops) | 7.72 NPC language handles conversation; shop *windows* need protocol (§10) |
| **Items (representable)** | `NEEDS_ID_REMAP` | Fusion32's 66 flags cover most 8.6 item kinds (§5); IDs must map into the 7.72 `.dat` space |
| **Items (new mechanics)** | `NEEDS_SERVER_CODE` or `NOT_COMPATIBLE` | imbuements, forge, charms have no representation |
| **Quests** | `CAN_IMPORT_DIRECTLY` | `QuestValues[500]` + `moveuse.dat` conditions cover classic quest logic; 500 slots is the budget |
| **Spells** | `NEEDS_SERVER_CODE` | hardcoded in `magic.cc`; every addition is a recompile (§8) |
| **Bosses** | `CAN_IMPORT_DIRECTLY` | a boss is a monster plus a raid entry; both are data |
| **Houses** | `CAN_IMPORT_DIRECTLY` | `houses.dat`/`houseareas.dat` are data; auctions already exist |
| **Hunting areas** | `CAN_IMPORT_DIRECTLY` | map + spawn data |
| **Outfits** | `NEEDS_PROTOCOL_EXTENSION` + `NEEDS_CLIENT_ASSET` | addons need a wire field 7.72 lacks |
| **Effects** | `NEEDS_CLIENT_ASSET` | byte id on the wire; renderability is a client-asset question |
| **Projectiles** | `NEEDS_CLIENT_ASSET` | same |

**The recurring gate is `NEEDS_CLIENT_ASSET`, not `NEEDS_SERVER_CODE`.** For `Tibia.exe` 7.72 that gate is absolute — the binary renders only what its `.dat/.spr` contains. For REAL33D it is an art-pipeline question, and the project has already hit exactly this wall once: `UNREAL-WIDE-WORLD-001` blocked on a single missing stair mesh (TypeID 469).

---

## 5. Item system delta

### 5.1 What Fusion32 7.72 can already represent

`enums.hh` declares 66 `FLAG` values, 62 `TYPEATTRIBUTE` values and 18 `INSTANCEATTRIBUTE` values. `DEMONSTRATED`.

| 8.6-era item concept | Fusion32 support |
| --- | --- |
| Stacking | `CUMULATIVE` flag + `AMOUNT` instance attr |
| Containers | `CONTAINER`, `CHEST`, `CAPACITY` |
| Doors (key/name/level/quest) | `KEYDOOR`, `NAMEDOOR`, `LEVELDOOR`, `QUESTDOOR` + matching targets |
| Keys | `KEY`, `KEYNUMBER`, `KEYHOLENUMBER` |
| Text items | `TEXT`, `WRITE`, `WRITEONCE`, `TEXTSTRING`, `EDITOR`, `MAXLENGTH` |
| Fluids | `LIQUIDCONTAINER`, `LIQUIDSOURCE`, `LIQUIDPOOL`, `CONTAINERLIQUIDTYPE` |
| Runes | `RUNE`, `CHARGES`, `REMAININGUSES`, `TOTALUSES` |
| Weapons | `WEAPON`, `WEAPONTYPE`, `WEAPONATTACKVALUE`, `WEAPONDEFENDVALUE` |
| Bows / ammo / throwables | `BOW`, `AMMO`, `THROW` + range/attack/missile attrs |
| Wands | `WAND` + range/mana/attack/variation/damage-type/missile |
| Armor / shields | `ARMOR`, `ARMORVALUE`, `SHIELD`, `SHIELDDEFENDVALUE` |
| Decay | `EXPIRE`, `EXPIRESTOP`, `TOTALEXPIRETIME`, `EXPIRETARGET`, `REMAININGEXPIRETIME` |
| Wear-out | `WEAROUT`, `TOTALUSES`, `WEAROUTTARGET` |
| Transformations | `CHANGEUSE`/`CHANGETARGET`, `ROTATE`/`ROTATETARGET`, `DESTROY`/`DESTROYTARGET`, `DISGUISE`/`DISGUISETARGET` |
| Skill boost | `SKILLBOOST`, `SKILLNUMBER`, `SKILLMODIFICATION` |
| Protection | `PROTECTION`, `PROTECTIONDAMAGETYPES`, `DAMAGEREDUCTION` |
| Light | `LIGHT`, `BRIGHTNESS`, `LIGHTCOLOR` |
| Magic fields | `MAGICFIELD`, `AVOIDDAMAGETYPES` |
| Teleports | `TELEPORTABSOLUTE`/`RELATIVE`, `ABSTELEPORTDESTINATION` |
| Beds | `BED` |
| Level/vocation restriction | `RESTRICTLEVEL`, `RESTRICTPROFESSION`, `MINIMUMLEVEL`, `PROFESSIONS` |

This is a genuinely capable item model. **Most 8.6-era item behaviour is expressible in `objects.srv` data alone.**

### 5.2 The hard serialization limit

```c
struct TObject { uint32 ObjectID; Object NextObject; Object Container;
                 ObjectType Type; uint32 Attributes[4]; };
```
`map.hh:62-68`. And `Object::getAttribute` resolves a per-type offset then bounds-checks it against `NARRAY(TObject::Attributes)` (`map.cc:98-113`). `DEMONSTRATED`.

**An item instance can carry at most four instance attributes simultaneously.** The set is chosen per object type by which flags are set. Any imported item needing a fifth concurrent instance attribute cannot be represented without widening `TObject`, which changes the swap-file format (`SwapObject` does `File->writeBytes((const uint8*)Entry, sizeof(TObject))`, `map.cc:624`) and the in-memory object budget.

### 5.3 Classification

| Change | Class |
| --- | --- |
| New item with existing flag combinations | **data only** (`objects.srv`) |
| New item needing a new *type* attribute | extend `TYPEATTRIBUTE` — recompile, no serialization break |
| New item needing a 5th concurrent *instance* attribute | **breaks serialization** — widens `TObject`, invalidates `.swp` |
| New flag | extend `FLAG` enum — recompile |
| Item id outside the 7.72 `.dat` range | **requires new client** or ID remap (§12) |
| Imbuements / forge / charms | `NOT_COMPATIBLE` — no representation |

**Do not assume ID equivalence across versions.** The project has already proved this is dangerous in its own tree: `UNREAL-WIDE-WORLD-001` documents `objects.srv` declaring TypeID 451 as a disguised stair whose `DisguiseTarget` is 469, so the *map* ID and the *visible* ID differ within a single version. Cross-version ID mapping must be derived, never assumed. `DEMONSTRATED`.

---

## 6. Map format / world migration

| Aspect | Fusion32 `.sec` | 8.60 OTBM |
| --- | --- | --- |
| Encoding | Text script | Binary node tree (`0xFE` node markers) |
| Granularity | One file per 32×32×1 sector, coords in filename | One file, whole world |
| Coordinates | Global `(x,y,z)`, sector = coord/32, bounds `SectorXMin..Max` ≤ 2047 | Global `(x,y,z)`, header-declared width/height |
| Tile content | Recursive `Content` with attributes | Node tree with item attribute records |
| Spawns | `.mon` + monster homes | `world-spawn.xml` |
| Houses | `houses.dat`, `houseareas.dat` | `world-house.xml` + tile house ids |
| Zones | Tile flags (`Refresh`, `NoLogout`, `ProtectionZone`) | `world-zones` + tile flags |

`DEMONSTRATED` — `map.cc:1011-1045`, `UNREAL-WIDE-WORLD-001`, and the OTBM headers read above.

### 6.1 Is `OTBM → .sec` possible?

**Yes, structurally.** Both are tile-stack models over global 3D coordinates with per-tile flags and recursive container content. The conversion is a tree walk. `STRONG_INFERENCE`.

| Information | Transfers | Notes |
| --- | --- | --- |
| Coordinates | ✅ | if inside `SectorXMin..Max`; **references A and B use custom low coordinates** and would need translation |
| Floors | ✅ | both 0–15 |
| Tile stacks | ✅ | ordering must be re-derived from Fusion32 priorities (`GetObjectPriority`), not copied |
| Item IDs | ⚠️ **remap required** | different identity spaces; no assumed equivalence |
| Zone flags | ✅ | PZ / no-logout / refresh have direct counterparts |
| Houses | ✅ | house id per tile + separate house table |
| Spawns | ⚠️ | XML→`.mon`/monster-home conversion; monster must exist in `.mon` |
| NPC placement | ⚠️ | NPC must exist as `.npc` |
| Teleports | ✅ | `TELEPORTABSOLUTE` + `ABSTELEPORTDESTINATION` |
| Doors | ✅ | four door flavours exist |
| Quest objects | ⚠️ | quest ids must map into the 500-slot array |

### 6.2 What would be lost

- **Any item whose ID has no 7.72 counterpart** — the dominant loss.
- **Attributes beyond four concurrent instance slots** (§5.2).
- **OTBM-only constructs** (`NOT_PROVEN` in detail — the OTBM node vocabulary was not exhaustively enumerated).
- **Content outside the sector bounds** or beyond the object budget.
- Modern-era systems entirely (imbuement slots, charm data).

**Rating: `MODERATE`.** A converter is ordinary engineering; the hard part is the ID mapping table, not the tree walk.

---

## 7. Monsters / NPCs

### 7.1 Monsters

| Property | Fusion32 `.mon` | 8.60 reference |
| --- | --- | --- |
| Health / speed / experience | ✅ | ✅ |
| Attacks / defenses | ✅ | ✅ |
| Immunities | ✅ (`AVOIDDAMAGETYPES`, `PROTECTIONDAMAGETYPES`) | ✅ |
| Loot | ✅ | ✅ |
| Summons | ✅ (`TOOMANYSLAVES`) | ✅ |
| Voices | ✅ | ✅ |
| Spells | ⚠️ from the fixed spell table | ✅ arbitrary Lua |
| Target strategy | ✅ | ✅ `strategiesTarget` |
| AI / pathfinding | server-side, shared | server-side, shared |
| **Bestiary / charms** | ❌ | present in Reference A (post-8.6) |

**Classification: `DATA_ONLY`** for monsters whose behaviour is expressible in `.mon`, **provided** `lookType` is within the 7.72 outfit range and loot item IDs exist. Monsters needing novel spell behaviour become `SERVER_CODE` because of §8.

### 7.2 NPCs

Fusion32 NPCs have their own scripted behaviour language loaded from `.npc` (`crnonpl.cc:3124-3141`). Dialogue, quest gating and travel are expressible.

**Shops are the exception and it is a big one.** 7.72 has no shop protocol; 8.60 has four client opcodes (`0x79` look-in-shop, `0x7A` purchase, `0x7B` sale, `0x7C` close) plus `sendShop`/`sendSaleItemList`/`sendCloseShop`. `DEMONSTRATED`. In 7.72 a shop is a conversation. Importing an 8.6 NPC shop means either re-expressing it as dialogue (`DATA+SCRIPT`) or adding the shop window (`SERVER_CODE` + `PROTOCOL_CHANGE`).

---

## 8. Spell / combat delta

Fusion32: **100 spells**, ids ≤ 102, built by literal `CreateSpell(...)` calls in `InitSpells()` (`magic.cc:4416+`). `DEMONSTRATED`.
Reference B: **200 spells** (167 instant + 33 rune) declared in `spells.xml`. `DEMONSTRATED`.

The delta is roughly 2× in count, but the structural gap is larger than the count: one is code, the other is data.

### What needs generalizing before a larger library is practical

| Need | Current state | Work |
| --- | --- | --- |
| Spell declaration | hardcoded C++ | **externalize to a data file** — the highest-leverage change in this whole report |
| Area patterns | embedded in `magic.cc` impact logic | needs a pattern/shape descriptor |
| Damage types | `AVOIDDAMAGETYPES`/`PROTECTIONDAMAGETYPES` exist | largely present |
| Buffs / debuffs | no general framework | new subsystem |
| Conditions | `SV_CMD_PLAYER_STATE` byte only | new subsystem + protocol for display |
| Cooldowns | exhaustion only | new state + `0xA4`/`0xA5`/`0xA6` for display |
| Persistent effects | magic fields exist (`MAGICFIELD`) | partial |
| Spell lookup | `magic.cc:3842` — *"We're iterating over all spells for each level. This is bad."* | index needed before growing the table |

The source's own comment at `magic.cc:3842` is a direct warning that the spell table does not scale as written. `DEMONSTRATED`.

**Rating: the spell system is the single largest `SERVER_CODE` obstacle to content parity.** Externalizing `InitSpells` into a data file — mirroring what `moveuse.dat` already does for item behaviour — converts an unbounded recompile treadmill into content work. It changes no protocol and no save format.

---

## 9. Player state delta

| Feature | Where it can live | Risk |
| --- | --- | --- |
| Quest / achievement flags | **`QuestValues[500]`** | Low — already persisted, already readable by `moveuse.dat` |
| Counters (kills, uses) | `QuestValues` | Low, but 500 slots is the budget |
| Outfit addons | **new `.usr` field** | **High — see below** |
| Unlocked outfits | `QuestValues` bitfield | Low |
| Spell cooldowns | runtime only (or new `.usr` field) | Low if transient |
| Conditions / buffs | runtime only | Low if transient |
| Account-wide currency | **QueryManager DB** | Medium — new RPC, id space has gaps |
| Bestiary / charms | not representable | — |
| Cosmetic unlocks | `QuestValues` or DB | Low |
| Rich per-item metadata | **breaks `.usr` and `TObject`** | High |

### The `.usr` risk, restated because Mode B depends on it

`LoadPlayerData` reads keys **positionally and discards the identifier**, with the source comment *"Data is expected to be in an exact order and we don't check identifiers"* (`crplayer.cc`, confirmed in the previous milestone). `DEMONSTRATED`.

Consequences for this milestone specifically:
- Inserting any new field mid-file invalidates **every existing character**.
- Append-at-end may work but the loader's EOF tolerance is `NOT_PROVEN`.
- `QuestValues[500]` is the **only** safe expansion surface today.

**This is the gating prerequisite for Mode B.** Converting the loader to identifier-keyed dispatch with defaults — it already reads the identifier — makes the format permanently additive. Until then, every feature needing new persisted per-character state is blocked or forced into quest slots.

---

## 10. Protocol delta matrix

`SAME` = same opcode, same role. Encoding may still differ in detail.

### 10.1 Client → server

| Feature | 7.72 | 8.60 ref | Status |
| --- | --- | --- | --- |
| Logout | `20` | `0x14` | **SAME** |
| Ping | `30` | `0x1E` | **SAME** |
| Extended opcode (OTClient) | — | `0x32` | **NEW** |
| New ping | — | `0x40` | **NEW** |
| Auto-walk | `100` | `0x64` | **SAME** |
| Walk N/E/S/W | `101-104` | `0x65-0x68` | **SAME** |
| Stop auto-walk | `105` | `0x69` | **SAME** |
| Walk diagonals | `106-109` | `0x6A-0x6D` | **SAME** |
| Turn N/E/S/W | `111-114` | `0x6F-0x72` | **SAME** |
| Move object | `120` | `0x78` | **SAME** |
| **Shop: look/buy/sell/close** | — | `0x79-0x7C` | **NEW** |
| Trade request | `125` | `0x7D` | **SAME** |
| Look in trade | `126` | `0x7E` | **SAME** |
| Accept trade | `127` | `0x7F` | **SAME** |
| Close trade | `128` | `0x80` | **SAME** |
| Use item | `130` | `0x82` | **SAME** |
| Use item ex | `131` | `0x83` | **SAME** |
| Use on creature | `132` | `0x84` | **SAME** |
| Rotate item | `133` | `0x85` | **SAME** |
| Close container | `135` | `0x87` | **SAME** |
| Up container | `136` | `0x88` | **SAME** |
| Text window | `137` | `0x89` | **SAME** |
| House window | `138` | `0x8A` | **SAME** |
| Look at | `140` | `0x8C` | **SAME** |
| **Look in battle list** | — | `0x8D` | **NEW** |
| **Join aggression** | — | `0x8E` | **NEW** |
| Say | `150` | `0x96` | **SAME** |
| Request channels | `151` | `0x97` | **SAME** |
| Open/close channel | `152/153` | `0x98/0x99` | **SAME** |
| Open private channel | `154` | `0x9A` | **SAME** |
| GM request queue | `155-157` | — | **REPLACED** (→ `0xF2`) |
| **Close NPC channel** | — | `0x9E` | **NEW** |
| Fight modes | `160` | `0xA0` | **SAME** |
| Attack / follow | `161/162` | `0xA1/0xA2` | **SAME** |
| Party invite/join/revoke/pass/leave | `163-167` | `0xA3-0xA7` | **SAME** |
| **Shared party experience** | — | `0xA8` | **NEW** |
| Private channel create/invite/exclude | `170-172` | `0xAA-0xAC` | **SAME** |
| Cancel attack+follow | `190` | `0xBE` | **SAME** |
| Update tile | `201` | `0xC9` | **SAME** |
| Update container | `202` | `0xCA` | **SAME** |
| Request/set outfit | `210/211` | `0xD2/0xD3` | **SAME** |
| Add/remove VIP | `220/221` | `0xDC/0xDD` | **SAME** |
| Bug report | `230` | `0xE6` | **SAME** |
| Rule violation | `231` | `0xE7` / `0xF2` | **EXTENDED** |
| Error file entry | `232` | — | **UNKNOWN** |
| **Get object info** | — | `0xF3` | **NEW** |
| **Modal window answer** | — | `0xF9` | **NEW** |

### 10.2 Server → client

| Feature | 7.72 | 8.60 ref | Status |
| --- | --- | --- | --- |
| Init game | `10` | `0x0A` | **SAME** |
| Login error/premium/waitlist | `20/21/22` | `0x14/0x15/0x16` | **SAME** |
| **DLL check / challenge** | — | `0x1F` | **NEW** |
| **Relogin window** | — | `0x28` | **NEW** |
| Ping | `30` | `0x1E` | **SAME** |
| Full map | `100` | `0x64` | **SAME** — *same 18×14* |
| Map rows N/E/S/W | `101-104` | `0x65-0x68` | **SAME** |
| Field data / add / change / delete | `105-108` | `0x69-0x6C` | **SAME** |
| Move creature | `109` | `0x6D` | **SAME** |
| Container open/close/add/update/remove | `110-114` | `0x6E-0x72` | **SAME** |
| Inventory set / delete | `120/121` | `0x78/0x79` | **SAME** |
| **Shop / sale list** | — | `0x7A/0x7B` | **NEW** |
| Trade own/partner/close | `125-127` | `0x7D-0x7F` | **SAME** |
| World light | `130` | `0x82` | **SAME** |
| Magic effect | `131` | `0x83` | **SAME** |
| Animated text | `132` | `0x84` | **SAME** |
| Distance shot | `133` | `0x85` | **SAME** |
| Creature square | `134` | `0x86` | **SAME** |
| Creature health/light/outfit/speed/skull/shield | `140-145` | `0x8C-0x91` | **SAME** (outfit payload **EXTENDED**) |
| **Creature walkthrough** | — | `0x92` | **NEW** |
| Text / house window | `150/151` | `0x96/0x97` | **SAME** |
| Player stats | `160` | `0xA0` | **EXTENDED** |
| Player skills | `161` | `0xA1` | **EXTENDED** |
| Player state / icons | `162` | `0xA2` | **EXTENDED** |
| Cancel target | `163` | `0xA3` | **SAME** |
| **Spell / group / item cooldown** | — | `0xA4-0xA6` | **NEW** |
| Creature say | `170` | `0xAA` | **SAME** |
| Channels dialog / open / private | `171-173` | `0xAB-0xAD` | **SAME** |
| GM request queue | `174-177` | — | **REPLACED** |
| Own channel / close channel | `178/179` | `0xB2/0xB3` | **SAME** |
| Text message | `180` | `0xB4` | **SAME** |
| Cancel walk (snapback) | `181` | `0xB5` | **SAME** |
| Floor up / down | `190/191` | `0xBE/0xBF` | **SAME** |
| Outfit window | `200` | `0xC8` | **EXTENDED** (addons) |
| VIP data / online / offline | `210-212` | `0xD2-0xD4` | **SAME** |
| **Tutorial hint** | — | `0xDC` | **NEW** |
| **Map marker** | — | `0xDD` | **NEW** |
| **Modal window** | — | `0xFA` | **NEW** |
| **Extended opcode** | — | `0xFF` | **NEW** |

**Reading of the matrix:** the overwhelming majority is `SAME`. The genuinely new surface is: shops, cooldowns, modal windows, markers, tutorial, walkthrough, shared-party-XP, battle-list look, challenge/relogin, extended opcode. Roughly **fifteen features**, each with a known opcode number.

That is a tractable list — and every one of those opcode numbers is currently unused by Fusion32.

---

## 11. Can we keep the 7.72 client?

| Feature | Classic 7.72 verdict |
| --- | --- |
| New map areas | `CLASSIC_772_CAN_USE_WITH_DOWNGRADE` — renders if IDs exist in its `.dat` |
| New monsters (in-range `lookType`) | `CLASSIC_772_CAN_USE_WITH_DOWNGRADE` |
| New monsters (out-of-range) | `CLASSIC_772_NEEDS_MAPPING` |
| New items (in-range) | `CLASSIC_772_CAN_USE_WITH_DOWNGRADE` |
| New items (out-of-range) | `CLASSIC_772_NEEDS_MAPPING` |
| New quests | `CLASSIC_772_CAN_IGNORE` — pure server logic |
| New bosses | as monsters |
| New houses | `CLASSIC_772_CAN_IGNORE` |
| New spells (existing effects) | `CLASSIC_772_CAN_IGNORE` |
| Shop windows | `CLASSIC_772_NEEDS_MAPPING` — degrade to NPC dialogue |
| Spell cooldowns | `CLASSIC_772_CAN_IGNORE` — enforce silently server-side |
| Condition icons | `CLASSIC_772_CAN_IGNORE` |
| Modal windows | `CLASSIC_772_NEEDS_MAPPING` — degrade to text message |
| Map markers | `CLASSIC_772_CAN_IGNORE` |
| Outfit addons | `CLASSIC_772_NEEDS_MAPPING` — send base outfit |
| Mounts | `CLASSIC_772_INCOMPATIBLE` |
| Shared party XP | `CLASSIC_772_CAN_IGNORE` — server-side rule |
| Battle-list look | `CLASSIC_772_CAN_IGNORE` |
| Bestiary / charms / imbuements | `CLASSIC_772_INCOMPATIBLE` |

**How much can `TerminalType` solve?** Nearly all of the gating, and the previous milestone established why: `TerminalType` is client-declared, server-validated against `TERMINALVERSION[]`, persisted on `TConnection` for the session, and already branched on (`JoinGame` accepts only 1 and 2). Adding a fourth entry gives a per-session capability flag with no new machinery. `DEMONSTRATED`.

What `TerminalType` **cannot** solve is asset availability. Gating decides *whether* a packet is sent; it cannot make `Tibia.exe` render a sprite it does not have. That is the `NEEDS_MAPPING` column, and it requires §12.

---

## 12. ID translation

The proposed shape:

```
                 canonical server item identity
                              │
              ┌───────────────┴───────────────┐
              ▼                               ▼
      7.72 mapping table              REAL33D mapping table
              ▼                               ▼
    legacy-compatible TypeID        full-fidelity identity
```

### Does the architecture support this?

**Partially, and there is already a working precedent in-tree.**

`sending.cc:173` sends `ObjType.getDisguise().TypeID` — **not** the item's own TypeID. `objects.hh::ObjectType::getDisguise()` substitutes `DisguiseTarget` when the `DISGUISE` flag is set. The server already has a mechanism that decouples stored identity from transmitted identity, and the project has already observed it in action (map ID 451 → visible ID 469). `DEMONSTRATED`.

That precedent is encouraging but not sufficient:

| Aspect | Assessment |
| --- | --- |
| Per-connection translation point | `SendItem` takes a `TConnection*` — the hook exists |
| Translation cost | one table lookup per item per tile per packet; `SendFullScreen` sends 2,016 map points |
| Disguise interaction | disguise is per-type and global; a per-client layer must compose with it |
| Ambiguity | many-to-one collapse (several modern items → one legacy sprite) loses distinguishability for the classic player |
| Look/inspect text | names come from `GetInfo`/`GetName` server-side, so text can stay truthful even when the sprite is approximated |
| Stack/attribute encoding | `SendItem` chooses payload length from **flags of the type it sends**; a substituted type with different flags changes packet length and desyncs |

That last row is the real hazard and it is demonstrable: `ARCHITECTURE.md` records that `SendItem` "decides an item's on-wire length from server object type flags the protocol never carries". A translation table that maps to a type with different `LIQUIDCONTAINER`/`CUMULATIVE` flags silently changes the byte length of the message.

**Verdict: `PRACTICAL` but only under a strict constraint** — the legacy target must share the *wire-relevant flag shape* of the canonical item (`CUMULATIVE`, `LIQUIDCONTAINER`, `LIQUIDPOOL`). A mapping table that ignores this is `HIGH_RISK`. Enforced as a validated table with a build-time check, it is sound; left to hand-maintenance, it will silently corrupt sessions.

---

## 13. 8.x-era feature matrix

Only features **observed in the references on disk**. No dates or features invented.

| Feature | In 7.72 | In ref 8.60 | Server | Data | Protocol | REAL33D | Classic | Complexity |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NPC shop window | ❌ | ✅ `0x79-0x7C` | Yes | Yes | **Yes** | Yes | degrade to dialogue | **High** |
| Spell cooldown display | ❌ | ✅ `0xA4-0xA6` | Yes | — | **Yes** | Yes | ignorable | Medium |
| Spell group cooldown | ❌ | ✅ | Yes | — | Yes | Yes | ignorable | Medium |
| Item use cooldown | ❌ | ✅ | Yes | — | Yes | Yes | ignorable | Medium |
| Condition icons | partial | ✅ `sendIcons` | Yes | — | Extend `0xA2` | Yes | ignorable | Medium |
| Modal windows | ❌ | ✅ `0xFA`/`0xF9` | Yes | Yes | **Yes** | Yes | degrade to message | Medium |
| Map markers | ❌ | ✅ `0xDD` | Yes | — | Yes | Yes | ignorable | Low |
| Tutorial hints | ❌ | ✅ `0xDC` | Yes | Yes | Yes | Yes | ignorable | Low |
| Creature walkthrough | ❌ | ✅ `0x92` | Yes | — | Yes | Yes | ignorable | Low |
| Shared party experience | ❌ | ✅ `0xA8` | Yes | — | Yes | Yes | **ignorable** | Low |
| Look in battle list | ❌ | ✅ `0x8D` | Yes | — | Yes | Yes | ignorable | Low |
| Outfit addons | ❌ | ✅ | Yes | Yes | **Yes** | Yes | needs mapping | Medium |
| Outfit mount field | ❌ | ✅ (ref-specific) | Yes | Yes | Yes | Yes | incompatible | Medium |
| Guild emblem | ❌ | ✅ `sendCreatureEmblem` | Yes | Yes | Yes | Yes | ignorable | Medium |
| Extended opcode channel | ❌ | ✅ `0xFF`/`0x32` | Yes | — | Yes | Yes | ignorable | Low |
| Challenge / DLL check | ❌ | ✅ `0x1F` | Yes | — | Yes | Yes | ignorable | Low |
| Relogin window | ❌ | ✅ `0x28` | Yes | — | Yes | Yes | ignorable | Low |
| Data-driven spells | ❌ | ✅ XML | **Yes** | **Yes** | No | No | **transparent** | **High value** |
| Data-driven monsters | ✅ (`.mon`) | ✅ | No | Yes | No | No | transparent | — |
| Condition framework | ❌ | ✅ 30+ params | Yes | Yes | partial | Yes | mostly ignorable | High |
| Bestiary / charms | ❌ | ref A only (post-8.6) | Yes | Yes | Yes | Yes | incompatible | Very high |
| Imbuements / forge | ❌ | ref A only (post-8.6) | Yes | Yes | Yes | Yes | incompatible | Very high |

---

## 14. Migration strategies

### STRATEGY A — Fusion32 7.72 + imported later content, no protocol change
- **Reuse:** ~100% of server; ~100% of REAL33D.
- **Content gained:** map areas, monsters, NPCs, quests, bosses, hunting grounds, houses — bounded by 7.72-renderable IDs for the classic client, and by the REAL33D art pipeline otherwise.
- **Risk:** **Low.** No code change for most of it.
- **Complexity:** Medium — dominated by the OTBM→`.sec` converter and the ID mapping table.
- **Classic compatibility:** Preserved.
- **Maintenance:** Low.
- **Difficulty:** Content pipeline work, not engine work.

### STRATEGY B — Fusion32 + extended systems + classic compatibility + REAL33D enhancements
- **Reuse:** ~90% of server.
- **Content gained:** everything in A, plus the ~15 features in §10.
- **Risk:** **Medium.** Gating discipline; `.usr` blocker must be cleared first.
- **Complexity:** High but incremental — one feature at a time, each with a known opcode.
- **Classic compatibility:** Preserved *if* gating holds.
- **Maintenance:** Medium — two wire dialects forever.

### STRATEGY C — Fusion32 game logic + Protocol772 adapter + Protocol860 adapter
- **Reuse:** game logic yes; all encoders rewritten.
- **Risk:** **High.** The previous milestone established that game logic emits protocol directly (`cract.cc::NotifyGo` writes `SV_CMD_ROW_*` itself), so a clean adapter split is a major refactor of a decompiled codebase with no server test suite.
- **Mitigating fact:** §0.1's opcode correspondence means a 8.60 adapter is mostly *re-encoding*, not *re-designing*.
- **Content gained:** none by itself — it is plumbing.
- **Classic compatibility:** preserved in principle, at risk throughout.

### STRATEGY D — Migrate content/logic from a real 8.6 server into REAL33D architecture
- **Reuse:** would discard Fusion32.
- **Risk:** **Very high**, and it has a specific blocker the others do not: **both available references are GPL-2.0** (§3.5). Absorbing TFS source into a private REAL33D would attach GPL obligations.
- **Additional problem:** neither reference ships the real Tibia world map, and Reference A's content is modern-era, not 8.6.
- **Content gained:** large on paper; much smaller once era-filtered.
- **It also discards the project's principal asset** — a faithful decompilation of the original server, with a client core already source-traced against it.

---

## 15. Easy wins — content addable with zero or minimal protocol change

1. **New map areas / cities / islands** — new `.sec` files within existing sector bounds. No protocol change.
2. **New hunting grounds** — map + spawn data.
3. **New monsters** with `lookType` inside the 7.72 range — `.mon` files only.
4. **New bosses** — a monster plus a raid `.evt` entry; the raid system already does timed world spawns.
5. **New monster raids / world events** — `.evt` data.
6. **New NPCs** with dialogue, travel and quest gating — `.npc` files.
7. **New quests** — `moveuse.dat` rules + `QuestValues` slots; conditions `HASQUESTVALUE`/`HASLEVEL`/`HASRIGHT`/`HASPROFESSION`/`TESTSKILL` already exist.
8. **New quest doors and keys** — `KEYDOOR`/`NAMEDOOR`/`LEVELDOOR`/`QUESTDOOR` all exist.
9. **New houses** — `houses.dat` / `houseareas.dat`; auctions already work end-to-end.
10. **New items reusing existing flag combinations** — `objects.srv` data.
11. **New runes** — `RUNE` + `CHARGES`/`REMAININGUSES`.
12. **New weapons / armor / shields / wands / bows / ammo** — full attribute coverage exists.
13. **New fluids** — `LIQUIDCONTAINER`/`LIQUIDSOURCE`/`LIQUIDPOOL`.
14. **New text/readable items** — `TEXT`/`WRITE`/`WRITEONCE`.
15. **New teleports** — `TELEPORTABSOLUTE`/`RELATIVE`.
16. **New magic fields** — `MAGICFIELD` + `AVOIDDAMAGETYPES`.
17. **New decay / transformation chains** — `EXPIRE`→`EXPIRETARGET`, `CHANGEUSE`→`CHANGETARGET`.
18. **New crafting recipes** — `moveuse.dat` `multiuse` already expresses A+B→C.
19. **Achievements / dailies** — `QuestValues` + `SendMessage`; classic client sees plain text.
20. **Shared party experience as a silent server rule** — no client display needed; `0xA8` only adds the *toggle*.

Every one of these is `CONTENT EXPANSION` or `DATA FORMAT MIGRATION`, none is `PROTOCOL EXPANSION`, and all are invisible to `Tibia.exe` in the sense that it needs no change.

---

## 16. Real blockers — things that look simple but are not

1. **"Just add a few spells."** Hardcoded `CreateSpell` calls + an O(all spells) scan the source itself flags as bad (`magic.cc:3842`). Every spell is a recompile. → `SERVER_CODE`, and externalization should precede any content push.
2. **"Just import the 8.6 item list."** Item ids up to 12,660 (ref B) or 51,747 (ref A) against a 7.72 client `.dat`. → `CLIENT_ASSET_REQUIRED` + ID remap with a wire-flag-shape constraint (§12).
3. **"Just add outfit addons."** One extra wire byte that 7.72 cannot parse. → `PROTOCOL_CHANGE` + gating + client assets.
4. **"Just add NPC shops."** Four client opcodes, three server opcodes, a shop state machine, and a degradation path for classic. → `PROTOCOL_CHANGE` + `SERVER_CODE`.
5. **"Just store one more thing per character."** `.usr` positional parsing invalidates all saves on a mid-file insert. → `SAVE MIGRATION` first.
6. **"Just add a fifth attribute to this item."** `TObject::Attributes[4]` is fixed and `SwapObject` writes `TObject` raw to `.swp`. → item schema extension **breaks serialization**.
7. **"Just import the 8.6 map."** OTBM→`.sec` is tractable, but stack ordering must be re-derived from `GetObjectPriority`, IDs remapped, and content must fit `SectorXMin..Max` and the non-growable object pool (`ResizeHashTable` → `abort()`).
8. **"Just add more objects."** Object storage is fixed at startup and refuses to grow. → world size and content density share one budget.
9. **"Just add condition icons."** No condition framework exists; `SV_CMD_PLAYER_STATE` is a single byte. → **new combat state** subsystem.
10. **"Just support the 8.6 client."** Same opcodes, different encodings, different login block, shop protocol absent, outfit payload wider. → `PROTOCOL REWRITE` of every encoder, with no server test suite to catch regressions, for **zero viewport gain** (§0.2).

---

## 17. Migration ladder

Each stage has an exit criterion that must be **demonstrated** before the next begins.

### STAGE 0 — Current Fusion32 7.72
*Exit criterion:* already met — server runs, two clients coexist (`UNREAL-SLICE-001`).

### STAGE 1 — Later content representable under 7.72
Scope: new map areas, monsters, NPCs, quests, bosses, houses, hunting grounds; `objects.srv`-expressible items.
*Would demonstrate:* an OTBM→`.sec` converter producing a loadable sector; a validated ID mapping table with the wire-flag-shape constraint enforced; both `Tibia.exe` and REAL33D entering the new area without protocol anomalies or residual bytes.
*Blocked by:* nothing. **This is the stage that can start.**

### STAGE 2 — Data-driven spells and expanded monster/item systems
Scope: externalize `InitSpells` to a data file; index the spell lookup; grow the monster and item libraries.
*Would demonstrate:* spells loaded from data with behaviour identical to the current 100 hardcoded ones (regression fixtures), then N new spells added with **no recompile**.
*Blocked by:* nothing technical; it is contained work in `magic.cc`.

### STAGE 3 — `.usr` keyed loading, then REAL33D-only optional features
Scope: identifier-keyed character loading with defaults; then `TerminalType`-gated additions — cooldown display, condition icons, markers, tutorial hints, battle-list look, shared-party-XP toggle.
*Would demonstrate:* every existing `.usr` file loads byte-identically under the keyed loader; then a REAL33D session receives an extended opcode while a concurrent `Tibia.exe` session shows zero anomalies over a sustained window.
*Blocked by:* Stage 3a (`.usr`) gates Stage 3b. Do not invert.

### STAGE 4 — Dual-protocol experiments
Scope: branch inside `Send*` on `TerminalType`; heavier features (shops, modal windows, outfit addons) with classic degradation paths.
*Would demonstrate:* one feature delivered in two dialects simultaneously — rich to REAL33D, degraded to `Tibia.exe` — with `Connection->Overflow` instrumented and never firing.
*Blocked by:* Stage 3.

### STAGE 5 — Optional 8.6-compatible endpoint
Scope: a real 8.6 client speaking to Fusion32.
*Would demonstrate:* an 8.6 client completing login, entering the world, walking, and seeing correct items — with the 7.72 client still working.
*Blocked by:* Stage 4, and by an explicit decision that it is worth it. **On current evidence it is not**: it costs a full encoder rewrite and delivers no viewport gain and no content the other stages do not already reach.

---

## 18. Results

# FUSION32_772_TO_860_EVOLUTION = COMPLETE

**CONTENT_MIGRATION_FEASIBILITY** — High for everything the existing data formats express: map, monsters, NPCs, quests, bosses, houses, hunting grounds are all file-driven and era-neutral. The gate is almost never the server; it is whether the target client owns the sprite, which binds `Tibia.exe` absolutely and REAL33D only through its art pipeline.

**MAP_MIGRATION_FEASIBILITY** — `MODERATE`. OTBM and `.sec` are both tile-stack models over global 3D coordinates, so a converter is a tree walk. The real work is the ID mapping table and re-deriving stack order from `GetObjectPriority`; the real ceiling is the non-growable object pool, which world size and content density must share.

**ITEM_MIGRATION_FEASIBILITY** — Better than expected, with one hard wall. Fusion32's 66 flags and 62 type attributes already express weapons, armor, wands, ammo, runes, charges, decay, doors, keys, fluids, text and transformations. But `TObject::Attributes[4]` caps an instance at four concurrent attributes, and `SwapObject` writes `TObject` raw to disk — so a fifth breaks serialization, not just compilation.

**MONSTER_NPC_MIGRATION_FEASIBILITY** — `DATA_ONLY` for monsters whose behaviour fits `.mon` and whose `lookType` is 7.72-renderable; in Reference A that excludes 770 of 1,615. NPC dialogue, travel and quest gating import cleanly; NPC **shops** do not, because 7.72 has no shop protocol at all and trades through conversation.

**QUEST_MIGRATION_FEASIBILITY** — High and genuinely cheap. `QuestValues[500]` is a persisted generic int store already readable by the `moveuse.dat` condition engine, and all four door flavours exist. The only budget is 500 slots, which should be governed by a registry before it is spent casually.

**COMBAT_MIGRATION_FEASIBILITY** — The weakest area. 100 spells are hardcoded C++ against ~200 declared in data in the reference, and the source itself warns the spell lookup scans every spell per level. Damage types and protections exist; conditions, buffs and cooldowns do not. Externalizing spell declaration is the highest-leverage single change in this report.

**TRUE_860_PROTOCOL_FEASIBILITY** — Technically nearer than expected, strategically unattractive. ~50 of 58 client and ~45 of 62 server opcodes already occupy the identical number in 8.60, so the vocabulary largely matches; but the encodings inside them differ, the shop protocol is absent, the outfit payload is wider, and every encoder would be rewritten against a codebase with no server tests. It would also deliver **zero extra viewport** — 8.60 uses the same `8/6 → 18×14` geometry.

**CLASSIC_772_COEXISTENCE** — Strong, and cheaper than the feature list suggests. `TerminalType` is already a validated, session-persistent, branched-upon capability flag, and most 8.6-era additions are ignorable by a classic client because they are display-only. The exceptions needing real degradation paths are shops, modal windows and outfit addons. What gating cannot fix is sprite availability — that is what ID translation is for.

**REAL33D_COMPATIBILITY** — Excellent. ClientCore is already a source-traced semantic layer that injects `ObjectTypeTable` from the server's own `objects.srv`, so new content flows without parser changes, and `WorldView` already converts state into engine-agnostic events. REAL33D is unbound by the 7.72 `.dat`, so it is the client that benefits most from Stage 1 — and its real constraint is the art pipeline, exactly as `UNREAL-WIDE-WORLD-001` found.

---

### TOP_20_CONTENT_FEATURES_WE_CAN_ADD_WITHOUT_PROTOCOL_CHANGE

1. New cities / islands / continents (`.sec` within bounds)
2. New hunting grounds
3. New dungeons / cave systems (new floors within z 0–15)
4. New monsters with 7.72-renderable `lookType`
5. New bosses
6. New monster raids and timed world events (`.evt`)
7. New NPCs with dialogue and travel (`.npc`)
8. New quests (`moveuse.dat` + `QuestValues`)
9. New quest doors, keys, levers, chests
10. New items reusing existing flag combinations (`objects.srv`)
11. New runes with charges
12. New weapons / armor / shields / wands / bows / ammo
13. New fluids and fluid containers
14. New readable / writable items
15. New teleports and travel networks
16. New magic fields
17. New decay and transformation chains
18. New crafting recipes (`multiuse`)
19. Achievements and daily objectives on `QuestValues` + `SendMessage`
20. New houses and guildhalls with the existing auction system

### TOP_20_FEATURES_REQUIRING_SERVER_CHANGE

1. Data-driven spell declaration (externalize `InitSpells`)
2. Spell lookup index (fix the per-level scan)
3. New spell effects / area patterns
4. Condition / buff / debuff framework
5. Per-spell and per-group cooldown state
6. Item use cooldowns
7. NPC shop state machine
8. Shared party experience rule
9. Guild system beyond login-supplied strings
10. Outfit addon ownership and validation
11. Account-wide progression (new QueryManager RPC)
12. Identifier-keyed `.usr` loading
13. Expanded `QuestValues` beyond 500 slots
14. New `FLAG` / `TYPEATTRIBUTE` values for imported items
15. Per-client ID translation layer
16. OTBM→`.sec` converter pipeline
17. Runtime sector addition beyond startup enumeration
18. Growable object storage (replace the `abort()` in `ResizeHashTable`)
19. Announce-radius parameterization (prerequisite for any wide live viewport)
20. Bestiary-style kill tracking

### TOP_20_FEATURES_REQUIRING_PROTOCOL_CHANGE

1. NPC shop window (`0x79-0x7C`, `0x7A/0x7B` out)
2. Spell cooldown display (`0xA4`)
3. Spell group cooldown (`0xA5`)
4. Item use cooldown (`0xA6`)
5. Condition icons (extend `0xA2`)
6. Modal windows (`0xFA` / `0xF9`)
7. Map markers (`0xDD`)
8. Tutorial hints (`0xDC`)
9. Creature walkthrough (`0x92`)
10. Shared party experience toggle (`0xA8`)
11. Look in battle list (`0x8D`)
12. Outfit addons (wider outfit payload)
13. Outfit mount field
14. Guild emblem (`sendCreatureEmblem`)
15. Extended opcode channel (`0xFF` / `0x32`)
16. Challenge / DLL check (`0x1F`)
17. Relogin window (`0x28`)
18. Extended player stats (`0xA0`)
19. Extended skills (`0xA1`)
20. Get object info (`0xF3`)

### TOP_10_MAJOR_MIGRATION_BLOCKERS

1. **Client asset availability** — `Tibia.exe` renders only its own `.dat/.spr`; the single most pervasive gate.
2. **`.usr` positional parsing** — blocks all new per-character state; must be fixed before Mode B.
3. **Hardcoded spells** — every spell is a recompile; the spell table does not scale as written.
4. **`TObject::Attributes[4]`** — a fifth concurrent instance attribute breaks the swap-file format.
5. **Non-growable object storage** — `ResizeHashTable` aborts; world size and content density share one fixed budget.
6. **No shop protocol in 7.72** — NPC commerce is conversational, not windowed.
7. **Cross-version ID identity** — no safe assumed equivalence; disguise already proves identity ≠ appearance within one version.
8. **GPL-2.0 on both references** — source cannot be absorbed into REAL33D; read them as specs, not as code.
9. **Reference content is not 8.6-era** — Reference A carries modern content (max lookType 1,869, max item id 51,747, Bestiary, imbuements).
10. **No server test suite** — any encoder rewrite (Strategy C / Stage 5) has no regression net.

---

## 19. Questions still unanswered

1. Does `LoadPlayerData` tolerate trailing unknown sections? Decides whether append-only `.usr` extension works. `NOT_PROVEN`.
2. What does `Tibia.exe` 7.72 do on an unknown **server** opcode? Still assumed fatal. `NOT_PROVEN`.
3. Exact OTBM node vocabulary — needed to bound converter information loss. `NOT_PROVEN`.
4. Is `lookMount` in the reference's 8.60 `AddOutfit` canonical 8.6 or a TFS-ism? `HYPOTHESIS` — it reads as the latter.
5. Do the 8.60 encodings of `SAME`-numbered opcodes differ in payload? Sampled only for outfit and map description. `NOT_PROVEN` in general.
6. What is the true 7.72 `.dat` item id ceiling? Needed to size the ID mapping table. `NOT_PROVEN` — the client is a binary and was not analyzed.
7. Do the reference `items.otb` ids correspond to client `.dat` ids 1:1 in 8.6? `NOT_PROVEN`.
8. How many of Fusion32's 5,003 object types have no 8.6 counterpart (reverse mapping)? `NOT_MEASURED`.
9. Licence terms of the reference **data** files as distinct from source. `NOT_PROVEN`.
10. Whether 500 quest slots suffice for a Stage-1 content push. `NOT_MEASURED`.

---

---

# ADDENDUM — Reference C: `BronsonServer86`

**Correction to §3.1.** The earlier report stated no `BronsonServer86` existed on the machine. That was true when the filesystem sweep ran and is now wrong: `Desktop\distribuciones\BronsonServer86.rar` (26 MB) is timestamped `Sep 21 13:30`, after the sweep. `htmlbronson.rar` (6.4 MB, website) sits alongside it. The negative result in §3.1 is withdrawn; §3.4's content-era conclusions are revised below, and they improve materially.

## C.1 Provenance

| Attribute | Value |
| --- | --- |
| Self-description | "Bronson Server 8.6 Rebuild" (`README.md`) |
| Lineage | **Nekiro's TFS 1.3/1.4 downgrade to 8.6** → `github.com/nekiro/TFS-1.5-Downgrades/tree/8.60` |
| Upstream base | `otland/forgottenserver` commit `17bf638815fa7c04d5b723baa8e0bfbdaad341f2`, Dec 21 2021 |
| Protocol | `CLIENT_VERSION_MIN/MAX = 860` (`src/definitions.h:27-29`) |
| Engine | TFS, Lua 5.1 (`lua51.dll`), MariaDB, pugixml, cryptopp |
| **Licence** | **GPL-2.0** |
| Ships binaries | `theforgottenserver-x64.exe`, `-ubunto`, 70 MB `.pdb`, `key.pem` |

`DEMONSTRATED`. Still TFS-lineage, not CipSoft — **§3.5's licensing constraint is unchanged and now applies to all three references.** Read as specification; do not copy source into REAL33D.

The author's own caveat is worth quoting: *"This downgrade is not download and run distribution, monsters and spells are probably not 100% correct."* Treat its content as era-*appropriate*, not era-*authoritative*.

## C.2 Why this reference is materially better than A and B

| Measure | Ref A (fs-downgrade) | Ref B (Greed) | **Ref C (Bronson)** | Fusion32 7.72 |
| --- | --- | --- | --- | --- |
| Map file | 4.1 MB | 3.4 MB | **53.9 MB** | `.sec` set |
| Map dimensions | — | — | **2500 × 2500** (OTBM v2, items 3.20) | sector-bounded |
| Spawn file | 79 KB | 67 KB | **1.38 MB** | `.mon` + monster homes |
| Spawn points | — | — | **9,271** | — |
| Monster placements | — | — | **10,825** | — |
| Monster definitions | 1,705 (Lua) | 15 | **201** (XML) | 159 races |
| Max monster `lookType` | **1,869** | — | **558** | 152 outfit identities |
| Monsters with `lookType > 400` | **770 / 1,615** | — | **4 / 197** | — |
| NPCs | 43 | 21 | **228** | 337 |
| Items declared | 17,308 | 5,999 | **4,608** | 5,003 types (ids 0–5090) |
| Max item id | **51,747** | 12,660 | **20,035** | **5,090** |
| Spells | — | 200 | **152** (116 instant + 36 rune) | **100** (hardcoded) |
| Post-8.6 systems present | Bestiary, imbuements, forge | — | **none observed** | — |

`DEMONSTRATED` by direct counts over the extracted trees.

**Reference C is the era-accurate content donor the other two were not.** Only 4 of 197 monster outfits exceed 400 (Ref A: 770 of 1,615). No Bestiary blocks, no imbuement or forge tooling. The named content above `lookType 250` — Frost Giant, Chakoya, Wyrm, Elder Wyrm, Grim Reaper, Barbarian tiers, Dragon Hatchlings — is recognisably 8.0–8.6-era Tibia, which is exactly what Mode A asks for.

## C.3 The quantified importability answer

This is what the previous report could only estimate. Thresholds applied to Reference C's 197 monster look types and 4,608 declared item ids:

**Monsters**

| Renderable by | Threshold | Count | Share |
| --- | --- | --- | --- |
| 7.72 client (conservative) | `lookType ≤ 160` | **120 / 197** | **61%** |
| 7.72 client (optimistic) | `lookType ≤ 200` | 128 / 197 | 65% |
| 8.6 client | `lookType ≤ 360` | **191 / 197** | **97%** |
| REAL33D | any — art-gated only | 197 / 197 | 100% |

The 7.72 thresholds are `STRONG_INFERENCE`, not `DEMONSTRATED`: the project's own inventory records **152 outfit identities** in 7.72 (`visual/manifests/summary.json`), and the client `.dat` is listed there as unavailable with `UNKNOWN` provenance, so the exact ceiling was not parsed. `160` is a reasonable working bound; `152` is the safe one.

**Items**

| Inside id range | Count | Share |
| --- | --- | --- |
| `≤ 5,003` (≈ Fusion32 type count) | **1,727 / 4,608** | **37%** |
| `≤ 7,000` | 2,436 | 53% |
| `≤ 9,000` | 3,274 | 71% |
| `≤ 12,000` | 4,295 | 93% |
| `≤ 20,035` (max) | 4,608 | 100% |

Fusion32's object TypeIDs run **0–5090** with 5,003 declared (`visual/manifests/objects.csv`). `DEMONSTRATED`.

So roughly **63% of Reference C's items carry IDs outside anything Fusion32/7.72 can represent** — the sharpest single number in this study, and it confirms §16's blocker #1 with a figure rather than an assertion.

**NPCs** — **128 of 228 (56%) use shop modules.** `DEMONSTRATED`. This quantifies §7.2: more than half of importable NPCs depend on the one feature 7.72 lacks entirely. It is a *degradation*, not a wall — 7.72 traded through conversation — but it means a Stage-1 NPC import touches the shop question for the majority of NPCs, not a minority.

**Spells** — 152 vs Fusion32's 100. A smaller gap than Reference B suggested (200), and it strengthens §8: the obstacle is that Fusion32's are code while these are data, not that there are vastly more.

## C.4 Map coordinates — a new constraint, and a favourable one

Reference C's spawns span `x 101–2050`, `y 30–1602`; the OTBM header declares `2500 × 2500`. `DEMONSTRATED`.

This is **not** the real Tibia coordinate space. Fusion32 runs the genuine world — the project's own audit script defaults to `32330, 32226` (Thais) and the previous milestone confirmed sector `1009-1005-06.sec`. Reference C's map is relocated to a low-coordinate origin.

Two consequences:

1. **It is not a drop-in replacement for Fusion32's world**, and it is not the same map. Importing it means importing *areas*, translated into Fusion32's coordinate frame — or hosting it as a separate world.
2. **It fits Fusion32's addressing comfortably.** `x 101–2050` is sectors 3–64; `y 30–1602` is sectors 0–50. Fusion32 validates `SectorXMax < 2047` (`map.cc:446-451`) and allocates the sector matrix as pointers with lazy per-sector allocation (`map.cc:1724`, `ExitMap` frees non-NULL only). The range is well inside the limit. `STRONG_INFERENCE`.

The live constraint remains the **object budget**, not the coordinate space: `ResizeHashTable` aborts and `ObjectBlock` is fixed at startup. 9,271 spawns and a 2500×2500 populated map is a large object count, and whether it fits the configured `Objects` value is `NOT_MEASURED`.

## C.5 Revised ladder impact

Stage 1 is unchanged in shape but now has a named, measured donor.

- **Stage 1 content source:** Reference C, read as a specification. It is the only one of the three that is simultaneously era-appropriate, fully populated, and 8.60-protocol.
- **Stage 1 realistic yield for the classic client:** ~61% of monsters, ~37% of items directly; more with a validated ID remap under §12's wire-flag-shape constraint.
- **Stage 1 yield for REAL33D:** effectively all of it, gated on art — which is the project's known bottleneck, not a new one.
- **The shop question moves earlier.** With 56% of NPCs shop-driven, deciding *now* whether Stage 1 NPCs degrade to conversation or wait for Stage 4's shop window is a real sequencing choice, not a deferred detail.

## C.6 What this does not change

- §0.1 — the opcode lineage finding stands; Reference C is the same TFS dispatch family and corroborates it.
- §0.2 — **the viewport finding stands unchanged.** Reference C is protocol 8.60 and inherits the same `8/6 → 18×14` geometry. Migration still buys no extra world.
- §3.5 — GPL-2.0 applies here too.
- §16 — every blocker remains; blockers #1 (client assets) and #6 (no shop protocol) are now quantified rather than merely asserted.

---

*End of report. No project file was created, modified, or committed. Reference archives were extracted read-only into the session scratchpad; the originals in `Desktop\distribuciones` were not altered.*
