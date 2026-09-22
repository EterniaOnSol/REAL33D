# FUSION32_EXPANDABILITY_RESEARCH

Milestone: `FUSION32-EXPANDABILITY-RESEARCH-001`
Mode: **RESEARCH ONLY / READ ONLY**
Workspace: `C:\Users\dell\Desktop\fusion32`
Date: 2026-09-21
Status of this file: **scratch, uncommitted, outside the project tree.**

Evidence labels used throughout:

- `DEMONSTRATED` — quoted from source/config present in this tree.
- `STRONG_INFERENCE` — follows necessarily from demonstrated code, but the exact path was not executed.
- `HYPOTHESIS` — plausible reading, not established.
- `NOT_PROVEN` — asserted nowhere; would need measurement or new work.

---

## 1. Executive Summary

Fusion32 is a decompilation of the original CipSoft 7.7 game server, with a 7.72 protocol mode enabled by a compile flag. Its reputation as a rigid legacy monolith is only half right, and the half that is wrong is the half that matters most to REAL33D.

Four findings dominate everything else in this report.

**The viewport is already a per-connection variable, not a constant.** `TConnection` carries `TerminalOffsetX/Y` and `TerminalWidth/Height`, and every map-sending routine computes its rectangle from those fields. They are assigned the literals `8, 6, 18, 14` at exactly one place, `connections.cc:219-222`. The server was written to support a variable window; nobody ever varied it. `DEMONSTRATED`.

**But the viewport is not the only thing that decides what you see.** Change notifications are broadcast through `TFindCreatures` searches whose radii are hardcoded literals — `(16, 14)` in three places in `operate.cc`, `(12, 10)` in two more. The source itself carries a TODO at `operate.cc:25-30` observing that these constants "are commonly around 16 and 14" and relating them to "the regular client's viewport dimensions of 15x11 visible fields or 18x14 total fields". Widening the window without widening these radii produces a client that can see a tile but is never told when it changes. This coupling — *viewport is a variable, interest management is a constant* — is the single most important technical fact in this report. `DEMONSTRATED`.

**Unknown opcodes are ignored, not fatal.** `receiving.cc:1786` handles the `default:` case with `print(3, "Unbekanntes Kommando %d.\n", Command)` and `break`. The connection survives. Protocol extension in the client→server direction is therefore additive and safe by construction. `DEMONSTRATED`.

**Three database backends already exist.** `reference/querymanager/Makefile` selects `sqlite`, `postgres`, or `mariadb` at compile time, and `database_postgres.cc` is 109 KB of real implementation with its own `schema.sql`. "Migrate to PostgreSQL" is not a project; it is a make variable. `DEMONSTRATED`.

Against those, the genuine walls are: object storage is fixed at startup and refuses to grow (`ResizeHashTable` calls `abort()`); character files are parsed **positionally with identifiers ignored**, so the save format cannot be extended in the middle; spells are ~740 lines of hardcoded C++ constructor calls, not data; and the whole game runs on one thread driven by a signal-based beat loop.

The overall verdict: Fusion32 can become the backend of a modern 3D MMORPG without losing 7.72 identity, and most of the distance can be covered *additively* — new opcodes, new QueryManager RPCs, sidecar services, and the existing 500-slot per-player quest array. The invasive work is concentrated in exactly two places: interest management (for a wide live world) and the character save format (for rich new per-character data).

---

## 2. Current Architecture

### 2.1 Processes

Three separate programs, three separate repos of origin, joined by TCP.

| Process | Source | Entry point | Binds |
| --- | --- | --- | --- |
| Game | `reference/game/src/` | `main.cc:531 main()` | `GamePort`, from DB world config |
| Login | `reference/login/src/` | `main.cc:633 main()` | `LoginPort`, default `7171` |
| QueryManager | `reference/querymanager/src/` | `querymanager.cc` | `QueryManagerPort`, default `7173`, **loopback only** |

`DEMONSTRATED`. Login server defaults at `login/src/main.cc:646,652`.

### 2.2 The real dependency graph

The diagram in the brief has the arrows right but the ownership wrong. QueryManager is not upstream of Login and Game in a pipeline sense; it is a *shared datastore RPC service* that both call, and it is the only hard dependency.

```
                 ┌──────────────────────────┐
  Tibia.exe ────▶│  Login  (7171)           │──┐
  (charlist)     │  character list, MOTD    │  │
                 └──────────────────────────┘  │
                                               ▼
                 ┌──────────────────────────┐ ┌──────────────────────────┐
  Tibia.exe ────▶│  Game   (GamePort)       │▶│ QueryManager (7173, lo)  │
  (world)        │  authoritative world     │ │ accounts, houses, bans,  │
                 │                          │ │ highscores, playerlist   │
                 └──────────────────────────┘ └──────────────────────────┘
                      │              │                    │
                      ▼              ▼                    ▼
              map/*.sec        usr/NN/ID.usr        SQLite | Postgres | MariaDB
              (world objects)  (character state)    (accounts & metadata)
```

The split of persistence is the part most people get wrong, and it matters for every later section: **character state does not live in the database.** `crplayer.cc:2138` writes `"%s/%02u/%u.usr"` — a text script file sharded by `CharacterID % 100`. The SQL database holds accounts, banishments, house ownership, highscores and the online list. `DEMONSTRATED`.

`README.md:43` states the dependency rule explicitly: "The game server won't boot up if it's not able to connect to the query manager which makes it the only real hard dependency."

### 2.3 Game server: lifecycle and threading

`main.cc:245 InitAll()` runs a fixed init order — `ReadConfig`, `LoadWorldConfig` (over QueryManager), `InitSHM`, `LockGame`, then subsystem inits ending in `InitTime` and `ApplyPatches`. `ExitAll()` unwinds in reverse. `DEMONSTRATED`.

Threading is unusual and is the root of several constraints:

- **One main game thread** running `LaunchGame()` (`main.cc:456`), which blocks in `sigsuspend` and wakes on two signals.
- **One communication thread per connection.** `communication.cc:24-25` declares `MAX_COMMUNICATION_THREADS 1100` and `COMMUNICATION_THREAD_STACK_SIZE ((int)KB(64))`, with `CommunicationThreadStacks[1100][65536]` — a **72 MB static array**. `DEMONSTRATED`.
- **One acceptor thread**, **one reader thread**, **one writer thread**, **one protocol/log thread**.

Cross-thread signalling is done with `tgkill` and real POSIX signals rather than queues:

| Signal | Direction | Meaning | Site |
| --- | --- | --- | --- |
| `SIGUSR1` | comm → game | a packet is parsed and ready | `communication.cc:653` |
| `SIGUSR1` | game → comm | parse done, resume reading | `receiving.cc:1807` |
| `SIGUSR2` | game → comm | output buffer has data | `sending.cc:26` |
| `SIGALRM` | timer → game | beat tick | `main.cc:140` |
| `SIGHUP` | game → comm | disconnect | `connections.cc:319` |

`DEMONSTRATED`. This is why the README calls Linux-only non-negotiable, and it is the reason "just make it async" is a rewrite rather than a refactor.

### 2.4 The beat loop

`AdvanceGame(Delay)` (`main.cc:312`) is the entire game tick. It runs four staggered sub-schedules and one per-second block:

```
CreatureTimeCounter >= 1750  →  ProcessCreatures()
CronTimeCounter     >= 1500  →  ProcessCronSystem()
SkillTimeCounter    >= 1250  →  ProcessSkills()
OtherTimeCounter    >= 1000  →  RoundNr++, ProcessConnections, ProcessMonsterhomes,
                                ProcessMonsterRaids, ProcessCommunicationControl,
                                ProcessReaderThreadReplies, ProcessWriterThreadReplies,
                                ProcessCommand, ambience diff, NetLoadCheck
```

then `MoveCreatures(Delay)` and `SendAll()`. `DEMONSTRATED`.

Two behaviours worth flagging. There is an explicit lag bail-out: if `Delay >= 1000`, creature movement is **skipped entirely** for that tick (`main.cc:439-447`) and an error is logged. And there is a scheduled daily restart driven by `RebootTime`, with 5/3/1-minute broadcast warnings (`main.cc:392-427`).

### 2.5 Failure modes

- `LockGame()` (`main.cc:196`) refuses to start if `save/game.pid` exists — a stale file after a hard crash blocks startup. `DEMONSTRATED`, and `README.md:61` documents it as a known operational footgun.
- `ResizeHashTable()` (`map.cc:510-520`) logs a fatal error and calls `abort()`. Object capacity is a hard ceiling. `DEMONSTRATED`.
- `SwapSector()` calls `abort()` if no sector can be evicted (`map.cc:665-666`). `DEMONSTRATED`.
- Character saves are individual file writes with no transaction. The source comments on this itself at `crplayer.cc:2432`: "This is probably one of the sources of whole day rollbacks." `DEMONSTRATED`.

---

## 3. Module Map

Classification per the requested scale, with concrete anchors.

| Subsystem | Class | Anchor evidence |
| --- | --- | --- |
| Networking / sockets | `MODERATELY_COUPLED` | `communication.cc`; thread-per-connection + signals; swappable only by rewriting the thread model |
| Packet framing / length | `MODULAR` | `communication.cc:1218-1276`, self-contained size + `%8` + plaintext-length checks |
| Crypto (RSA/XTEA) | `MODULAR` | `crypto.hh` — two small structs, OpenSSL-backed; already reimplemented independently in `clientcore/src/crypto.cpp` |
| Opcode dispatch (in) | `MODULAR` | `receiving.cc:1726-1790`, flat switch, one free function per command |
| Packet serialization (out) | `MODULAR` | `sending.cc:75-160` primitives; ~60 `Send*` free functions taking `TConnection*` |
| Login / auth | `STRONGLY_COUPLED` | `communication.cc:898 HandleLogin` does crypto + version gate + waitlist + registration + packet rewriting in one function |
| Session lifecycle | `MODERATELY_COUPLED` | `TConnection` state machine `connections.cc:191-320`; clean states, but `JoinGame` constructs `TPlayer` inline |
| Players | `STRONGLY_COUPLED` | `crplayer.cc` 87 KB; `TPlayer` owns connection ptr, skills, depot, quests, save/load |
| Creatures (base) | `MODERATELY_COUPLED` | `cr.hh` `TCreature`; real inheritance to player/NPC/monster |
| Monsters | `MODERATELY_COUPLED` | `crnonpl.cc`; behaviour in C++, stats/loot in `.mon` files |
| NPCs | `MODULAR` (data side) | `crnonpl.cc:3124-3141` loads `*.npc` from `NPCPATH`; NPCs have their own scripted behaviour language |
| Movement | `STRONGLY_COUPLED` | `cract.cc::TCreature::NotifyGo` emits `SV_CMD_ROW_*` directly from movement logic |
| Combat | `STRONGLY_COUPLED` | `crcombat.cc` 25 KB, entangled with skills and magic |
| Spells | `HARD_CODED` | `magic.cc:4416 InitSpells()` — literal `CreateSpell(...)` calls with inline `Mana`/`Level`/`Comment`, running to ~line 5150 |
| Items / object model | `MODERATELY_COUPLED` | `map.cc` `Object`/`TObject`; 4 attribute slots per object (`map.hh:67`) |
| Item types | `MODULAR` | `dat/objects.srv`, loaded as data; ClientCore reuses the same file |
| Containers | `MODERATELY_COUPLED` | `containers.hh`; `OpenContainer[16]` per creature (`cr.hh:917`) |
| Inventory | `MODERATELY_COUPLED` | body positions in `info.cc` |
| Trade | `MODULAR` | `receiving.cc:290-383`, four self-contained commands |
| Chat | `MODULAR` | `receiving.cc:750 CTalk`; three `SendTalk` overloads |
| Parties | `MODULAR` | `receiving.cc:1157-1255`, five commands; `Parties` counter in `operate.cc:20` |
| Guilds | `HARD_CODED` (thin) | guild/rank/title are strings carried on `TPlayerData` from `loginGame`; no in-game guild system |
| Houses | `MODULAR` | `houses.cc` 54 KB, own data files + own QueryManager RPCs |
| Quests | `MODULAR` | `cr.hh:146,916` `int QuestValues[500]`, generic int store, persisted |
| Map / sectors | `MODERATELY_COUPLED` | `map.cc`; global `Sector` matrix3d + global object hash table |
| Floor transitions | `STRONGLY_COUPLED` | `sending.cc:517 SendFloors` hardcodes the 7/8 boundary and the `[10,6]`/`[7,0]` client-cache assumptions |
| Effects / projectiles | `MODULAR` | `SendGraphicalEffect`/`SendMissileEffect`, byte type ids |
| Persistence (character) | `HARD_CODED` | `crplayer.cc:2420 SavePlayerData` + positional reader (see §8) |
| Persistence (world) | `MODULAR` | `.sec` files, `SaveMap`/`LoadMap` |
| Database access | `MODULAR` | QueryManager only; game never touches SQL |
| Configuration | `MODULAR` | `config.cc:137-260`, keyword/value script |
| Timers / scheduling | `MODERATELY_COUPLED` | beat loop + `CronCheck`/`CronExpire` object cron in `map.hh:116-120` |
| Logging | `MODULAR` | `writer.cc` `Log(ProtocolName, ...)`, dedicated thread |
| Error handling | `STRONGLY_COUPLED` | `const char*` throws everywhere; README:13 calls exceptions "too engraved into the codebase" to remove |
| Admin / GM | `MODERATELY_COUPLED` | rights bitmask via `CheckRight`; SHM command channel; `TerminalType 2` gamemaster client |

---

## 4. Coupling Analysis

### 4.1 What is genuinely loose

`TConnection*` is the currency of the entire output path. Every `Send*` function in `connections.hh:236-309` takes a `TConnection*` and writes bytes into that connection's ring buffer. Nothing in the game logic knows about sockets. That is a real seam: an alternate transport that presents a `TConnection`-shaped object gets the whole existing protocol for free. `DEMONSTRATED`.

QueryManager is a clean typed RPC facade — `query.hh:43-140` declares ~50 methods, each a request/response pair over a length-prefixed byte protocol. The game never issues SQL. `DEMONSTRATED`.

### 4.2 What is genuinely tight

**Game logic emits protocol directly.** This is the deepest coupling in the tree and the one that decides §11. Movement does not raise an event that a protocol layer then encodes; `cract.cc::TCreature::NotifyGo` advances the player one axis and emits `SV_CMD_ROW_*` itself. The project's own `ARCHITECTURE.md` documents this from the client side: "`SV_CMD_ROW_*` and `SV_CMD_FLOOR_UP/DOWN` carry no coordinates at all". The wire format is a direct consequence of the traversal order in the loop that produces it. `DEMONSTRATED`.

**The protocol is implicitly sized.** `SendFullScreen` (`sending.cc:421`) writes the opcode, the player position, and then a bare stream of map points in nested `z/x/y` order. The dimensions are never transmitted. The client knows the rectangle is 18×14 because it is compiled to know. Any server-side change to `TerminalWidth` desynchronizes a classic client immediately and silently. `DEMONSTRATED`.

**Interest management is constant while the viewport is variable.** Covered in §6.1 — the decisive coupling.

**Global mutable singletons.** `map.cc` holds file-scope `Sector`, `ObjectBlock`, `HashTableData/Type/Size/Mask`, `FirstFreeObject`; `crmain.cc:15` holds `FirstChainCreature`; `sending.cc:14-15` holds `static int Skip` and `FirstSendingConnection`. `static int Skip` in particular is shared mutable state used to compress empty tiles across the map-sending functions. There is exactly one world per process, and that is structural. `DEMONSTRATED`.

---

## 5. Protocol Extensibility

### 5.1 How the protocol actually works

**Inbound.** `ReceiveCommand` (`communication.cc:1122`) reads a 2-byte LE length, rejects `Size == 0 || Size > sizeof(InData)` (2048), reads the body, requires `Size % 8 == 0` for encrypted packets, XTEA-decrypts in 8-byte blocks, reads an inner plaintext length and rejects `PlainSize == 0 || (PlainSize + 2) > Size`. Then `CallGameThread` raises `SIGUSR1` and the main thread runs `ReceiveData(Connection)`. `DEMONSTRATED`.

That validation is better than the codebase's age suggests. Length handling is bounded at every step.

One structural limit: `receiving.cc:1688` notes "I thought the client would actually pack multiple commands in the same packet. Perhaps not until some later version?" — the parser reads exactly **one command per packet**. Batching is not supported. `DEMONSTRATED`.

**Outbound.** `BeginSendData` / `SendByte|Word|Quad|Bytes|String` / `FinishSendData` write into `OutData[16384]`, used as a ring buffer with commit/send watermarks. On overflow, `Connection->Overflow` is set and `FinishSendData` **silently drops the whole packet** with only a level-2 print (`sending.cc:62-65`). `DEMONSTRATED`. This 16 KB per-connection buffer is a real capacity limit for any wide-viewport design (§6.3).

### 5.2 A. Strict Tibia.exe 7.72 compatibility

**Can capability be added invisibly to the classic client? Yes, substantially.** `STRONG_INFERENCE`, resting on demonstrated facts:

- Anything that never changes bytes on the wire is free: new gameplay rules, new `moveuse.dat` behaviour, new monsters, new NPC scripts, quest-array bookkeeping, telemetry, external services.
- Anything expressible in existing opcodes is free: new item types via `SendItem`, new effect ids via `SendGraphicalEffect`, messages via `SendMessage`, new map content.
- **Never add a server→client opcode for a classic client.** The 7.72 client's own dispatcher will not recognize it. Nothing in this tree proves its behaviour on an unknown opcode; treat it as fatal desync. `NOT_PROVEN` that it is survivable — and this is the correct conservative default.

### 5.3 B. REAL33D extended client

Client→server extension is demonstrably safe. `receiving.cc:1786` ignores unknown commands. The `ClientCommand` enum has large unused ranges — `0-9`, `12-19`, `21-29`, `31-99`, `110`, `115-119`, `121-124`, `129`, `134`, `139`, `141-149`, `158-159`, `168-169`, `173-189`, `191-200`, `203-209`, `212-219`, `222-229`, `233-255`. `DEMONSTRATED` from `connections.hh:12-74`.

`ServerCommand` has comparable gaps: `0-9`, `12-19`, `23-29`, `31-99`, `115-119`, `122-124`, `128-129`, `135-139`, `146-149`, `152-159`, `164-169`, `182-189`, `192-199`, `201-209`, `213-255`. `DEMONSTRATED` from `connections.hh:76-138`.

`CommandAllowed(Connection, Command)` gates every inbound command before dispatch (`receiving.cc:1720`) — the natural place to reject extended opcodes from non-REAL33D sessions.

### 5.4 C. Two clients on one world — strategy comparison

The single most valuable discovery for this question: **a capability field already exists and is already validated.**

```
communication.cc:28   static const int TERMINALVERSION[] = {772, 772, 772};
communication.cc:970  if(TerminalType < 0 || TerminalType >= NARRAY(TERMINALVERSION)
                          || TERMINALVERSION[TerminalType] != TerminalVersion){  → reject
connections.cc:207    this->TerminalType = (int)Buffer->readWord();
connections.cc:214    if(this->TerminalType != 1 && this->TerminalType != 2){ → reject
```

`TerminalType` is sent by the client, validated at login, **persisted on `TConnection` for the life of the session**, and already used to distinguish an ordinary client from a gamemaster client. `DEMONSTRATED`.

| Strategy | Change required | Classic risk | Verdict |
| --- | --- | --- | --- |
| **Terminal-type capability flag** | Add a 4th `TERMINALVERSION` entry; allow the new type in `JoinGame`; branch in `Send*`/`CommandAllowed` on `TerminalType` | Very low — classic clients keep sending 1 or 2 and take unchanged paths | **Lowest-cost, uses existing machinery** |
| Protocol version flag | `TerminalVersion` is already checked against a per-type table; a REAL33D type could carry its own version | Very low | Natural companion to the above |
| Optional extended opcode | Free inbound (`default:` ignores); outbound must be gated by capability | Low if gated, fatal if not | Good, but needs the flag above to be safe |
| Explicit handshake after login | New client→server opcode, server replies with capability set | Low | Most flexible; more state; redundant with `TerminalType` for v1 |
| Separate port | `OpenSocket`/`AcceptorThreadLoop` assume one acceptor; `GamePort` is a single DB value | Low for classic, high for effort | Poor value |
| Separate Game endpoint / gateway | A second Game process is a second world — the object store, sector matrix and creature grid are process-global singletons | N/A — different world | **Not viable for a shared world** |
| Side-channel connection | Second TCP connection correlated to the session; carries bulk/non-authoritative data only | None to classic | **Strong for bulk data**; see §10 |

Recommendation deferred to §18 per the brief. The comparison, however, is decisive on one point: a second Game process cannot share a world, because `map.cc` and `crmain.cc` hold the world in process-global state. `DEMONSTRATED`.

---

## 6. World / Map Extensibility

### 6.1 The 18×14 question, answered precisely

The brief asks whether 18×14 is a client, protocol, server-logic or performance limit. It is **all four, in different amounts**, and the parts must be separated.

**Server logic: NOT a limit.** Four map-sending functions and the visibility predicate all compute from per-connection fields:

| Site | Code |
| --- | --- |
| `connections.cc:372-375` | `MinX = (PlayerX - TerminalOffsetX) + (PlayerZ - z); MaxX = MinX + TerminalWidth - 1;` |
| `sending.cc:428-431` | `SendFullScreen` rectangle |
| `sending.cc:470-473` | `SendRow` rectangle |
| `sending.cc:561-564` | `SendFloors` rectangle |
| `receiving.cc:526-527` | `CGoPath` distance check against `TerminalOffsetX/Y` |

Assigned once, at `connections.cc:219-222`. `DEMONSTRATED`.

**Protocol: a hard limit, by omission.** `SendFullScreen` transmits no dimensions (`sending.cc:445-457`). Geometry is a shared compile-time assumption. `DEMONSTRATED`.

**Client: a hard limit.** The classic `Tibia.exe` is a binary; its window is fixed. `DEMONSTRATED` in the trivial sense that no source exists to change.

**Server logic, second order: a REAL limit — and this is the one that bites.** Even with `TerminalWidth` raised, the player would see a stale world. Change announcements never consult the connection's viewport size to decide *whom to search for*; they use literals:

```
operate.cc:100   AnnounceChangedField      → TFindCreatures Search(16, 14, ...)
operate.cc:218   AnnounceGraphicalEffect   → TFindCreatures Search(16, 14, ...)
operate.cc:243   AnnounceTextualEffect     → TFindCreatures Search(16, 14, ...)
operate.cc:1004  (object announce)         → TFindCreatures Search(12, 10, ...)
operate.cc:1072  (container announce)      → TFindCreatures Search(12, 10, ...)
operate.cc:45    AnnounceMovingCreature    → 16 + |Δx|/2 + 1 , 14 + |Δy|/2 + 1
operate.cc:273   AnnounceMissile           → 16 + |Δx|/2 + 1 , 14 + |Δy|/2 + 1
```

The `IsVisible` call inside each loop is a *filter on an already-truncated candidate set*, so it cannot recover players the search never yielded. The source's own TODO at `operate.cc:25-30` names the relationship between these constants and the 18×14 window. `DEMONSTRATED`.

**Performance: not currently a limit, and there is a spatial index.** `TFindCreatures` walks a 16×16-tile bucket grid (`crmain.cc:74-95`, `FirstChainCreature->boundedAt(blockx, blocky)`), allocated over `[SectorXMin*2, SectorXMax*2+1]` (`crmain.cc:2120`). Spectator search is proportional to buckets in range, not to total creature count. Widening radii grows it quadratically in radius but from a small base. `DEMONSTRATED` for the mechanism; the cost is `NOT_MEASURED`.

**The three hard ceilings any wide viewport hits:**

1. `OutData[16384]` per connection, with silent packet drop on overflow (`sending.cc:62-65`, `connections.hh:199`). 18×14×8 floors = 2,016 map points today. 32×32 would be ~4.2×, 64×64 ~16.6×. `STRONG_INFERENCE` that 64×64 full-screen cannot fit one 16 KB buffer.
2. `KnownCreatureTable[150]` per connection (`connections.hh:225`). `NewKnownCreature` evicts by scanning for a creature that is no longer `IsVisible` (`connections.cc:421-431`); a wider window makes fewer creatures evictable and can exhaust the table, which logs "KnownCreatureTable ausgelastet" and returns 0. `DEMONSTRATED`.
3. `MAX_OBJECTS_PER_POINT 10` (`sending.cc:11`) caps stack depth per tile.

### 6.2 Scenario evaluation

| Scenario | Server change | CPU | RAM | Bandwidth | Gameplay | Classic compat | Class |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 32×32 live, all clients | viewport init + all 7 radii + buffer + known-creature table | ~4× map serialization | buffer growth × connections | ~4× snapshot, ~2× rows | **changes balance** — wider aggro awareness, ranged targets | **breaks** | `HARD` |
| 64×64 live, all clients | same, plus buffer redesign | ~17× | large | ~17× | severely changes balance | **breaks** | `ARCHITECTURAL_REWRITE` |
| Variable per client | above + gate every radius on the *receiver's* viewport | scales per client | per client | per client | asymmetric fairness question | preserved if classic stays 18×14 | `HARD` |
| Classic 18×14 + REAL33D extended | as above, gated on `TerminalType` | bounded to REAL33D sessions | bounded | bounded | same asymmetry | **preserved** | `HARD` but **correct shape** |
| Static sector streaming (client-side) | **none** | none | client-side | none (reads files) | none — cosmetic only | **preserved** | `MODERATE`, client-only |
| Delta streaming | already how the server works (`ADD/CHANGE/DELETE_FIELD`) | n/a | n/a | n/a | n/a | n/a | already present |
| True interest management | replace literal radii with a per-observer registry | redesign | new index | better at scale | none if radii preserved | preserved | `ARCHITECTURAL_REWRITE` |

The row that matters: **static sector streaming requires zero server change.** The world outside the live window is already on disk in `.sec` files, and the project has already traced this. `UNREAL-WIDE-WORLD-001` established the boundary — "A static tile is suppressed for *every* coordinate inside the live window, including empty tiles; WorldState owns that whole rectangle" — and found the blocker is **art, not protocol**: visible stair TypeID 469 is absent from the frozen V08 catalogue. `DEMONSTRATED` by that evidence file.

This is the single highest-value finding for REAL33D's wide world: the desired visual result does not require touching Fusion32 at all.

### 6.3 Sectors, addressing, persistence

- **Dimensions.** `TSector { Object MapCon[32][32]; uint32 TimeStamp; uint8 Status; uint8 MapFlags; }` (`map.hh:74-79`). 32×32 tiles, one floor. `DEMONSTRATED`.
- **Addressing.** Filenames `%04d-%04d-%02d.sec` (`operate.cc:2894`). Bounds `SectorXMin/Max`, `SectorYMin/Max`, `SectorZMin/Max` from map config, defaults `1000..1015` (`map.cc:345-346`), validated `> 0` and `< 2047` (`map.cc:446-451`). `DEMONSTRATED`.
- **Startup load.** `LoadMap` (`map.cc:1011`) `opendir(MAPPATH)`, loads **every** `.sec` file matching `%d-%d-%d.sec`. Whole world in RAM at boot. `DEMONSTRATED`.
- **Caching / swap.** A real demand-paging system exists but is driven by *object slot exhaustion*, not memory pressure or access locality: `GetFreeObjectSlot` calls `SwapSector()` when the free list empties (`map.cc:571-574`). `SwapSector` linearly scans all sectors for the oldest `TimeStamp`, writes it to `SAVEPATH/%08u.swp`, and marks hash entries `STATUS_SWAPPED` (`map.cc:639-697`). `AccessObject` transparently un-swaps (`map.cc:1808-1810`). `DEMONSTRATED`.
- **Object capacity is fixed and non-growable.** `ObjectBlock` is `malloc`ed once for `OBCount` blocks of 32,768 `TObject` (`map.cc:1730-1733`, `map.hh:70-72`); the hash table is sized from the `Objects` config value and must be a power of two (`map.cc:497`); `ResizeHashTable` `abort()`s (`map.cc:520`). `DEMONSTRATED`.
- **Hot reload EXISTS.** This is an underappreciated seam. `RefreshSector` (`operate.cc:2813`) reloads a sector's refreshable content from `ORIGMAPPATH` **while the server runs**, gated by `SectorRefreshable` (`operate.cc:2785`) which returns false if any player `CanSeeFloor(SectorZ)` within radius 31. It is driven asynchronously through the reader thread (`reader.hh` `READER_ORDER_LOADSECTOR` → `READER_REPLY_SECTORDATA` → `ProcessReaderThreadReplies(RefreshSector, ...)`, wired at `main.cc:351`), triggered every minute by `RefreshCylinders()` (`main.cc:377`). `DEMONSTRATED`.

### 6.4 Map capability ratings

| Capability | Rating | Reason |
| --- | --- | --- |
| On-demand sector load | `MODERATE` | `LoadSector` is a standalone function and the reader thread already does async sector I/O; but bounds and object pool are fixed at startup |
| Streaming (server-side) | `MODERATE` | swap machinery exists; eviction policy is exhaustion-driven and its scan is O(all sectors) |
| Hot-load of existing sectors | `EASY` | `RefreshSector` already does exactly this, live, every minute |
| New sector **file** at runtime | `HARD` | `LoadMap` only enumerates at boot; `Sector` matrix3d is allocated for fixed bounds at `map.cc:1724` |
| Expanding world bounds | `ARCHITECTURAL_REWRITE` | `SectorXMin/Max` fix the sector matrix *and* `FirstChainCreature` at init; also hard-capped at 2047 |
| Dynamic / procedural sectors | `HARD` | must fit existing bounds and object budget; `.sec` is a text script, generatable offline |
| Instanced maps / dungeons | `ARCHITECTURAL_REWRITE` | coordinates are global `(x,y,z)` with a single `Sector` matrix; no instance dimension exists anywhere |
| Separate dimensions / worlds | `ARCHITECTURAL_REWRITE` | one world per process by construction |
| World expansion without restart | `HARD` | possible only inside pre-allocated bounds and object budget |

---

## 7. Gameplay Extensibility

### 7.1 Content: what is data and what is code

| Content | Source | Class |
| --- | --- | --- |
| Monsters | `MONSTERPATH/*.mon` (`crmain.cc:1688-1705`) | `DATA_DRIVEN` |
| Monster raids | `MONSTERPATH/*.evt` (`magic.cc:1962`), `LoadMonsterRaids` (`crmain.cc:1901`) | `DATA_DRIVEN` |
| NPCs | `NPCPATH/*.npc` (`crnonpl.cc:3124-3141`) | `DATA_DRIVEN` — own behaviour language |
| Item types | `dat/objects.srv` | `DATA_DRIVEN` + `CLIENT_ASSET_REQUIRED` |
| Item behaviour | `DATAPATH/moveuse.dat` (`moveuse.cc:2981`) | `DATA_DRIVEN` — rule engine with `use`/`multiuse`/`movement`/`collision`/`separation` events, ~15 condition types incl. `HASQUESTVALUE`, `HASLEVEL`, `HASRIGHT`, `TESTSKILL` |
| Houses | `DATAPATH/houses.dat`, `houseareas.dat`, `owners.dat` (`houses.cc:1528-1883`) | `DATA_DRIVEN` |
| Map / cities / islands / floors | `MAPPATH/*.sec` | `DATA_DRIVEN` within fixed bounds |
| Quest state | `QuestValues[500]` (`cr.hh:146,916`) | `DATA_DRIVEN` via moveuse conditions |
| **Spells** | `magic.cc:4416 InitSpells()` | **`CODE_REQUIRED`** — literal `CreateSpell(id, "ex","ura","")` + `Mana`/`Level`/`Flags`/`RuneGr`/`RuneNr`/`Comment` assignments |
| Effects / projectiles | byte type ids on the wire | `CLIENT_ASSET_REQUIRED` |

The asymmetry is striking: item *behaviour* got a full rule-engine DSL, while spells were left as hardcoded constructor calls. `DEMONSTRATED`.

New cities, islands, floors, quests, NPCs, monsters, bosses and houses are therefore **data work plus client art**, provided they fit the existing sector bounds and object budget. New spells are a recompile.

### 7.2 Modern features

`QuestValues[500]` deserves emphasis. It is a generic, persisted, per-character array of 500 integers with get/set accessors (`cr.hh:856-857`) already readable from the moveuse rule engine. A large fraction of "modern progression" is expressible in it with **no protocol change, no DB change and no save-format change**, surfaced to the player through `SendMessage`.

| Feature | Classification | Basis |
| --- | --- | --- |
| Achievements | `SERVER_ONLY` / `POSSIBLE_WITH_CLASSIC_FALLBACK` | quest slots + `SendMessage`; rich UI needs `SERVER+PROTOCOL_EXTENSION` |
| Daily quests | `SERVER_ONLY` | quest slots + cron; `ProcessCronSystem` exists |
| Account-wide progression | `SERVER+DATABASE_CHANGE` | per-character today; account scope needs a QueryManager RPC |
| Crafting | `SERVER_ONLY` | `moveuse.dat` `multiuse` already expresses A+B→C |
| Cooldown systems | `SERVER_ONLY` | exhaustion already modelled (`EXHAUSTED` result, `sending.cc:336`) |
| Extended character stats | `SERVER+PROTOCOL_EXTENSION` | `SV_CMD_PLAYER_DATA`/`PLAYER_SKILLS` are fixed-shape |
| Skill trees | `SERVER+PROTOCOL_EXTENSION` + save format | needs new persisted per-character data → §8 blocker |
| Server-side cosmetics | `POSSIBLE_WITH_CLASSIC_FALLBACK` | outfit id + 4 colours already on the wire (`sending.cc:162-169`); new looks need client art |
| Transmog | `SERVER_ONLY` (mechanically) | `ObjectType::getDisguise()` already redirects visible TypeID — the mechanism exists |
| Mounts | `CLASSIC_CLIENT_INCOMPATIBLE` | no outfit-mount field in 7.72; speed-only version is `SERVER_ONLY` |
| Pets / companions | `SERVER_ONLY` | summons exist; `TOOMANYSLAVES` result at `sending.cc:343` |
| Modern party features | `SERVER+PROTOCOL_EXTENSION` | party opcodes exist (`SV_CMD_CREATURE_PARTY`) but are minimal |
| Instances / raids (instanced) | `CLASSIC_CLIENT_INCOMPATIBLE` + rewrite | no instance dimension in coordinates |
| Shared world bosses | `SERVER_ONLY` | raid system (`.evt`) already does timed world spawns |
| Dynamic events | `SERVER_ONLY` | raids + cron |
| Seasons | `SERVER_ONLY` (+ ops) | data swap + restart |
| Weather | `SERVER+PROTOCOL_EXTENSION` for real weather; `POSSIBLE_WITH_CLASSIC_FALLBACK` via ambience | `SV_CMD_AMBIENTE` carries brightness+colour only |
| Day/night | **already exists** | `GetAmbiente` diffed every second, `main.cc:355-367` |
| Auction house | `SERVER+DATABASE_CHANGE` | house auctions already exist end-to-end (`QUERY_START_AUCTION`, `finishAuctions`) — a template |
| Matchmaking | `SERVER+PROTOCOL_EXTENSION` or external | no such concept |
| Modern quest log | `SERVER+PROTOCOL_EXTENSION` | state exists, no delivery opcode |
| Extended item metadata | `SERVER+PROTOCOL_EXTENSION` | `SendItem` is minimal by design |

---

## 8. Persistence

### 8.1 Where each thing lives

| Data | Store | Owner |
| --- | --- | --- |
| Accounts, passwords | SQL | QueryManager |
| Banishments, notations, namelocks | SQL | QueryManager |
| House ownership, auctions, transfers | SQL | QueryManager |
| Highscores, playerlist, kill statistics, census | SQL | QueryManager |
| Online flags | SQL | QueryManager |
| **Character state** (skills, items, position, quests, depot) | `usr/NN/ID.usr` text files | Game |
| World objects | `map/*.sec` text files | Game |
| Swapped sectors | `save/*.swp` binary | Game |
| Runtime status | SysV shared memory | Game |

`DEMONSTRATED`: `crplayer.cc:2138`, `map.cc:1011`, `map.cc:675`, `shm.cc`.

### 8.2 Save timing

- Characters: every 15 minutes via `SavePlayerDataOrder()` when `Minute % 15 == 0` (`main.cc:381-383`), plus on logout.
- Playerlist: every 5 minutes. Kill statistics: at `Minute == 55`. `DEMONSTRATED` (`main.cc:378-389`).
- Map: on graceful shutdown (`SaveMap`, `main.cc:424`) and via `ExitMap(SaveMapOn)`, where `SaveMapOn` is set only for `SIGQUIT`/`SIGTERM`/`SIGPWR` (`main.cc:87`).

**Crash recovery is weak.** A `SIGKILL` or hard crash loses up to 15 minutes of character progress and the entire map delta since boot. There are no transactions and no write-ahead log on the file side. `STRONG_INFERENCE` from the above; the source itself flags the rollback consequence at `crplayer.cc:2432`.

### 8.3 The save-format blocker

This is the most important rigidity in the codebase after interest management.

`LoadPlayerData` (`crplayer.cc:~2168`) carries the comment: *"Data is expected to be in an exact order and we don't check identifiers"*, and the code proves it:

```
Script.readIdentifier(); // "id"
Script.readSymbol('=');
Script.readNumber();

Script.readIdentifier(); // "name"
Script.readSymbol('=');
strcpy(Slot->Name, Script.readString());
```

The identifier is read and **discarded**. Parsing is purely positional. `DEMONSTRATED`.

Consequences:

- Inserting any new field mid-file invalidates every existing `.usr` file.
- The only backward-compatible extension is appending after the final section, with a reader that tolerates EOF.
- Reordering for readability is a save-wipe.

Any roadmap item needing new per-character persisted state — skill trees, achievements beyond 500 ints, cosmetic unlocks, account-wide currency — hits this. The mitigation is cheap and should be recorded now: **keyed loading** (match on the identifier already being read and discarded, with defaults for absent keys) would make the format additive forever, and is a contained change to one function pair.

### 8.4 Migration options

| Target | Cost | Risk | Benefit | Verdict |
| --- | --- | --- | --- | --- |
| **PostgreSQL** | `make DATABASE=postgres` + `postgres/schema.sql` | **Low** | concurrency, real backups, external read access | **Already supported.** `database_postgres.cc` is 109 KB; migration scripts `z-002`/`z-003` exist. `DEMONSTRATED` |
| MariaDB | same switch | Low | — | Already supported, though `database_mariadb.cc` is only 377 bytes — likely a stub. `DEMONSTRATED` (file size) |
| Modern SQLite schema | schema edit + QueryManager change | Low | — | Fine; SQLite is the default and adequate for a single world |
| Character data → SQL | rewrite `Save/LoadPlayerData` + new RPCs | **High** | transactions, no rollbacks, external tooling | Real benefit, real risk; the `.usr` reader must be rewritten anyway (§8.3) |
| Redis cache | new dependency | Medium | little — world state is already in RAM | **Low value.** The game is not read-bound on the DB |
| Event log / analytics DB | sidecar consuming logs | Low | high for live ops | **Best value per unit risk** |
| Cloud persistence | — | High | — | Contradicts loopback-only QueryManager and file-backed world |
| Backups / snapshots | external, filesystem-level | Low | high | The map is text files; snapshotting is an ops task, not a code task |
| Horizontal services | — | High | — | Blocked by single-process world (§12) |

On "modern is not automatically better": the `.sec` and `.usr` text formats are diffable, greppable, hand-editable and trivially backed up. The audit script in `scripts/client/audit_wide_world_sources.py` reads `.sec` files directly — a capability a binary or SQL world format would have cost. Moving the world into a database would *reduce* this project's tooling leverage.

---

## 9. API / Services / Sidecars

### 9.1 The existing zero-coupling seam

`shm.cc` already exposes a live telemetry surface over SysV shared memory:

```c
struct TSharedMemory {
    int Command;  char CommandBuffer[256];
    uint32 RoundNr, ObjectCounter, Errors;
    int PlayersOnline, NewbiesOnline;
    int PrintBufferPosition;  char PrintBuffer[200][128];
    GAMESTATE GameState;  pid_t GameProcessID, GameThreadID;
};
```

The source notes: *"This looks like an interface to external tools. Looking at the `bin` directory this program was in, there are other programs that probably use this interface."* `DEMONSTRATED`.

There is also a **write** path: `SetCommand`/`GetCommand` (`shm.cc:261,273`), polled each second by `ProcessCommand()` (`main.cc:294`), with command `1` = broadcast message. An external process can already push a broadcast into a running server without any code change. `DEMONSTRATED`.

Read-only SHM consumption is the lowest-coupling integration available: zero server change, zero protocol change, no risk to the game loop.

### 9.2 Placement recommendations

| Integration | Best placement | Why |
| --- | --- | --- |
| Metrics / telemetry | **SHM reader sidecar** | already exposed; no server change |
| World status / online count | **SHM reader** or `getPlayersOnline` RPC | both already exist |
| Highscores | **database reader** | `createHighscores` already writes them |
| Character lookup | **database reader** + `.usr` reader | both are readable formats |
| Web account management | **existing pattern** | `QUERY_CREATE_ACCOUNT`, `GET_ACCOUNT_SUMMARY` are already web-facing query types |
| REST API | **sidecar over DB + SHM** | never inside the game process |
| WebSocket events | **sidecar tailing the log/event stream** | the game thread must not block on a socket |
| Discord integration | **sidecar** | consumes the same event stream |
| Logging service | **sidecar tailing `LOGPATH`** | `WriteProtocol` already writes per-protocol log files from a dedicated thread |
| Analytics | **event stream → separate DB** | keeps the OLTP path clean |
| Anti-cheat signals | **emit from server, judge outside** | detection needs in-process data; decisions do not |
| Moderation | **hybrid** | banishment RPCs exist (`banishAccount`, `setNamelock`, `banishIPAddress`); a dashboard can call QueryManager directly |
| Economy monitoring | **sidecar** over event stream | |
| Backups | **filesystem, external** | text world files |
| Live ops / broadcast | **SHM command channel** | already implemented |
| Patch / update service | **external** | |
| Matchmaking | **external** | no in-game concept to couple to |
| AI / NPC service | **inside**, or a carefully bounded sidecar | NPC behaviour is on the game thread; an external call would block the world. `HYPOTHESIS` that an async request/reply through the reader-thread pattern could work |
| gRPC | **sidecar only** | a C++11 codebase with `-pedantic` and custom allocators is a poor host for it |

The governing rule, and it is demonstrated rather than stylistic: **anything that can block must not live on the game thread.** `main.cc:434-447` already degrades — skipping all creature movement — when a tick exceeds 1000 ms. An in-process HTTP server is a latent world-freeze.

---

## 10. REAL33D Integration

The project's stack is already well-factored, and `ARCHITECTURE.md` documents three structural guards worth preserving: ClientCore is *linked* not copied (`REAL33D.Build.cs` raises a `BuildException` if `protocol772core.lib` is absent); exactly one file may include a protocol header (`Real33DBridge.cpp`); and the WorldState→events diff lives in ClientCore (`protocol772_worldview`), not in a Tick function. `DEMONSTRATED` by that file.

Routing recommendations for future features:

| Feature | Route | Justification |
| --- | --- | --- |
| Authoritative movement, combat, items, containers, trade, chat | **stay on 7.72** | already decoded; changing gains nothing |
| Wide *static* world | **from files/map** | `.sec` + `objects.srv` read directly; zero server change; already scoped by `UNREAL-WIDE-WORLD-001` |
| Distant creatures | **extend the protocol** (REAL33D-gated) | authoritative data; must come from the server; needs both viewport and announce radii (§6.1) |
| Weather | **extend the protocol** | no 7.72 carrier |
| Lighting / ambience | **stay on 7.72** | `SV_CMD_AMBIENTE` already decoded and diffed per second |
| Ambient world state | **presentation only** | no authority needed |
| Dynamic events | **7.72 messages** + sidecar detail | raids already exist server-side |
| Richer NPC data | **extend the protocol** or read `.npc` files | static NPC data is already on disk |
| Quest UI | **extend the protocol** | `QuestValues[500]` exists but has no delivery opcode |
| Houses | **from files** | `houses.dat`/`houseareas.dat` are static |
| Minimap / world map | **from files** | `.sec` is the whole world |
| Inspection / look | **stay on 7.72** | `CL_CMD_LOOK_AT_POINT` + `GetInfo` |
| Extended item metadata | **from `objects.srv`** first, protocol only if dynamic | ClientCore already injects `ObjectTypeTable` from the server's own file |
| Achievements | **sidecar API** | non-authoritative presentation data |
| Cosmetics | **7.72 outfit fields** where possible | `SendOutfit` carries id + 4 colours |

The pattern that emerges: **static → files; authoritative-dynamic → 7.72; new-authoritative-dynamic → gated protocol extension; everything else → sidecar.** The side-channel connection from §5.4 is the right home for bulk non-authoritative data because it cannot stall the game thread's send path or overflow `OutData[16384]`.

---

## 11. Multi-Protocol Feasibility

Can `Game Logic → Protocol Adapter 772 | Protocol Adapter REAL33D` be reached without a full rewrite?

**Verdict: `FEASIBLE_WITH_MAJOR_REFACTOR` for a true adapter split; `FEASIBLE_INCREMENTALLY` for the capability-gated variant that delivers most of the value.**

Where the coupling actually is:

| Layer | Coupled? | Evidence |
| --- | --- | --- |
| Player session | **No** | `TConnection` is a clean state machine; `TerminalType` already distinguishes client kinds |
| Networking | Partly | thread-per-connection + signals; a second adapter inherits the model |
| Serialization | **No** | all `Send*` take `TConnection*` and use five primitives; substitutable |
| World model | **No** | `Object`/`TObject`/`TCreature` know nothing about protocol |
| **Game logic** | **Yes — decisively** | logic calls `Send*`/`Announce*` directly. `cract.cc::NotifyGo` emits `SV_CMD_ROW_*`; `operate.cc` announce functions call `SendAddField` etc. inside spectator loops |

A true adapter split requires interposing an event layer between game logic and every `Send*` call site — dozens of sites across `operate.cc`, `cract.cc`, `crplayer.cc`, `moveuse.cc`, `magic.cc`. That is a major refactor of a decompiled codebase whose fidelity to the original is its main asset.

The incremental path that avoids it: branch **inside** the `Send*` functions on `Connection->TerminalType`. The call sites in game logic never change; the encoder chooses its wire format per connection. This yields two wire protocols over one game logic with no event layer, no new threading, and a change surface confined to `sending.cc`. `STRONG_INFERENCE`.

Its limit is equally clear: it can only express things game logic already announces. Genuinely new server→client information (distant creatures, weather) still needs new announce paths — and those must respect §6.1's radii.

---

## 12. Performance / Scale

No benchmarks exist in this tree. Every number below is a structural bound or a code-derived hotspot, not a measurement.

**Declared ceilings** (`DEMONSTRATED`):

- `MAX_CONNECTIONS 1100` (`connections.hh:143`), `MAX_COMMUNICATION_THREADS 1100`.
- 72 MB static thread-stack array.
- `MaxPlayers` / `PremiumPlayerBuffer` / `MaxNewbies` from world config.
- `KnownCreatureTable[150]`, `OutData[16384]`, `InData[2048]` per connection.
- `MAX_OBJECTS_PER_POINT 10`, `MAX_OBJECTS_PER_CONTAINER 36`, `OpenContainer[16]`, `QuestValues[500]`.
- Object pool: `OBCount × 32768`, fixed, non-growable.
- Map: sector coordinates bounded to 2047.

**Probable hotspots, code-backed:**

1. **Single game thread.** Everything except socket I/O and the reader/writer threads runs on it. `NOT_MEASURED`, but the degradation path is explicit at `main.cc:439-447`.
2. **`SwapSector()` is O(all sectors)** — a triple nested loop over the entire sector matrix on every object-pool exhaustion (`map.cc:649-662`). If the pool is chronically tight this becomes a per-allocation full-world scan. `DEMONSTRATED` structurally.
3. **Spell iteration.** `magic.cc:3842`: *"We're iterating over all spells for each level. This is bad."* — the source's own assessment. `DEMONSTRATED`.
4. **`SendFullScreen`** writes 2,016 map points per call, each doing object-type lookups; called on login, teleport and every `SendRefresh`.
5. **Announce fan-out.** Every field change runs a spectator search plus a per-player `IsVisible`. Bounded by the 16×16 bucket grid, so acceptable today; grows quadratically with any radius increase.
6. **`NewKnownCreature`** does up to three linear scans of 150 entries (`connections.cc:400-431`).
7. **Thread-per-connection context switching** — the README (line 102) identifies this as the main scaling difference vs OpenTibia.
8. **`TConnection::KnownCreature`** linear scan per creature descriptor sent.

**What to benchmark first**, in value order:

1. Tick duration distribution (p50/p95/p99/max) under synthetic load — directly observable, and the `lag` log already records `Delay > Beat`.
2. `SendFullScreen` cost per call vs. viewport size — the gating number for any wide-viewport decision.
3. Announce fan-out cost vs. player density at radii 16, 24, 32, 48.
4. `OutData` overflow frequency vs. viewport size — currently a *silent* failure, so instrument before experimenting.
5. Object pool occupancy and `SwapSector` invocation rate over a long session.
6. Memory and context-switch rate at 50 / 200 / 500 connections.
7. `.usr` save duration at `Minute % 15` — a synchronous stall candidate.

**Horizontal scaling: `NOT_PRACTICAL` without a rewrite.** The world is process-global (`map.cc` statics, `crmain.cc:15`), the object id space is a single hash table, and creature ids come from one counter (`crmain.cc` `NextCreatureID = 0x40000000`). Multiple worlds are supported (the DB has a world table and `getWorlds`); multiple *processes serving one world* are not.

---

## 13. Security / Hardening

Architectural review only; no exploitation was attempted and none should be inferred.

| Area | Finding | Class |
| --- | --- | --- |
| Packet length validation | Bounded at every stage: `Size > sizeof(InData)` rejected, `% 8` enforced, inner `PlainSize` checked (`communication.cc:1218-1276`) | `LOW_RISK` |
| Command buffer bounds | `TReadBuffer` throws on over-read; every handler is wrapped in try/catch (`receiving.cc:1791`) | `LOW_RISK` |
| Unknown opcodes | Ignored, connection survives | `LOW_RISK` |
| **RSA key size** | 1024-bit enforced (`crypto.cc:RSA_size(m_RSA) != 128`) | `HARDENING_RECOMMENDED` — below modern norms; fixed by the protocol |
| **No packet authentication** | `communication.cc:929-931`: *"Without a checksum, there is no way of validating the asymmetric data. The best we can do is to verify that the first plaintext byte is ZERO"* | `HARDENING_RECOMMENDED` |
| XTEA session crypto | Period-appropriate, not modern | `HARDENING_RECOMMENDED` |
| **QueryManager transport** | **Unencrypted**, plaintext password compare (`connections.cc:321 StringEq(g_Config.QueryManagerPassword, Password)`); mitigated by loopback-only bind with an explicit `IMPORTANT` comment | `LOCAL_ONLY_ACCEPTABLE` — **`INTERNET_BLOCKER` if ever exposed** |
| Config secrets | Passwords in a plaintext config file, obfuscated by `DisguisePassword(..., PasswordKey)` (`config.cc:193`) — obfuscation, not encryption | `HARDENING_RECOMMENDED` |
| Account passwords at rest | `sha256.cc` + `tools/pwdhash.go` present | `LOW_RISK` for the era; no salt/KDF evidence found — `NOT_PROVEN` either way |
| **Proxy header spoofing** | `ALLOW_LOCAL_PROXY` accepts HAProxy headers; guarded by `strcmp(IPAddress, "127.0.0.1")`, and the source itself says *"Ideally we'd have a list of trusted proxies"* | `HARDENING_RECOMMENDED` if enabled |
| **Connection slot exhaustion** | 1100 slots, each pre-allocating a 64 KB stack; no per-IP cap or rate limit found | `INTERNET_BLOCKER` |
| **Login cost amplification** | Every login does an RSA private-key decrypt under a global `RSAMutex` (`communication.cc:936-944`) before any cheap rejection | `INTERNET_BLOCKER` — serialized expensive operation reachable pre-auth |
| Crash surfaces | `abort()` in `ResizeHashTable` and `SwapSector`; `error()`-then-continue is the dominant idiom | `HARDENING_RECOMMENDED` |
| Stale PID lock | `game.pid` blocks restart after hard crash | `HARDENING_RECOMMENDED` (operational) |
| GM capabilities | Rights bitmask via `CheckRight`, per-reason/per-action banishment matrix (`sending.cc:386-405`) — genuinely fine-grained | `LOW_RISK` |
| Session trust | `CharacterID` fixed at login, server-side; no client-supplied identity after handshake | `LOW_RISK` |
| DB trust boundary | Game cannot issue SQL; QueryManager is the only writer | `LOW_RISK` — good design |

**Summary for internet exposure:** the two blockers are the pre-auth RSA-under-global-mutex amplification and unbounded connection-slot consumption. Both are classic DoS shapes, both are fixable with a rate limiter and a per-IP cap in front of the acceptor, and neither requires touching game logic. The QueryManager must remain loopback-only or be tunnelled; exposing it would be a credential-disclosure blocker.

---

## 14. Testability

**Today: effectively zero for the server.** `reference/game` and `reference/querymanager` contain no test directory. `tests/` at the repo root holds build scripts, a secret checker, and Python validators for client-side artefacts. `DEMONSTRATED`.

**The contrast is instructive.** `clientcore/tests/` has eight deterministic suites with checked-in byte fixtures (`crypto_772_vectors.h`, `fullscreen_772_vectors.h`), built under MSVC at `/W4 /WX /permissive-` and under GCC with ASan/UBSan. `PROJECT_STATUS.md` records that the Windows build "found four real portability defects that GCC accepts silently". The project already knows how to test protocol code — it just tests the client half.

**What blocks server tests:**

| Obstacle | Evidence |
| --- | --- |
| Global mutable state | `map.cc` statics, `crmain.cc:15`, `sending.cc:14-15` |
| Init order is a fixed chain | `InitAll()` runs 17 subsystem inits; no subset is independently constructible |
| QueryManager is a hard boot dependency | `LoadWorldConfig` throws if it cannot connect (`main.cc:221-243`) |
| Filesystem coupling | every subsystem loads from a configured directory at init |
| Signal-driven control flow | `sigsuspend` + `tgkill` cannot run in a normal test harness |
| SysV shared memory | `InitSHM` at boot |
| `abort()` on capacity errors | kills the test runner |
| `const char*` exceptions | no typed errors to assert on |

| Test kind | Feasibility | Note |
| --- | --- | --- |
| Protocol fixtures | **Easy** | already proven on the client side; server encoders are pure functions of `TConnection` + world state |
| Unit tests (leaf functions) | Moderate | `info.cc` geometry, `strings.cc`, `utils.cc` are largely pure |
| Integration tests | Moderate | `scripts/server/*_wsl.sh` already do prepare/start/smoke/stop |
| Headless world tests | Hard | requires decoupling init from SHM/QueryManager/signals |
| Deterministic simulation | Hard | `srand(time(NULL))` at `main.cc:253`; seedable in principle |
| Replay tests | Moderate | the project already keeps `movement_journal.jsonl` evidence |
| Load tests | Moderate | a synthetic client exists — ClientCore itself |
| **Fuzzing** | **Easy and high-value** | `ReceiveData(TConnection*)` takes a buffer and is already exception-guarded; a harness needs only a fake `TConnection` |
| CI | Moderate | Linux-only build is already scripted |

The highest-value, lowest-cost item is fuzzing the inbound parser: it is the only pre-auth attack surface, the entry point is a single function, and the codebase's own try/catch discipline makes crashes meaningful signals rather than noise.

---

## 15. Hot Reload / Live Ops

| Asset | Reloadable live? | Evidence |
| --- | --- | --- |
| **Map sectors** | **Yes** | `RefreshSector` (`operate.cc:2813`) reloads from `ORIGMAPPATH` while running, gated by `SectorRefreshable`; driven by the reader thread and `RefreshCylinders()` each minute (`main.cc:377`) |
| **Whole map** | **Yes, at reboot** | `RefreshMap()` (`operate.cc:2884`) walks every sector; called just before `SaveMap` on scheduled reboot (`main.cc:422-424`) |
| Broadcast messages | **Yes** | SHM command channel, `ProcessCommand` each second |
| Server state (running/closing/ending) | **Yes** | `CloseGame`/`EndGame` via SHM |
| Monsters (`.mon`) | No | `LoadRaces()` only at `InitCr` |
| NPCs (`.npc`) | No | `InitNPCs` only at boot |
| Items (`objects.srv`) | No | `InitObjects` only at boot |
| Item behaviour (`moveuse.dat`) | No | `LoadDataBase()` only at `InitMoveUse` |
| Houses | No | boot only |
| Spells | No | compiled in |
| Config | No | `ReadConfig` first in `InitAll` |

`DEMONSTRATED` throughout.

`SIGHUP` is bound to a **no-op** handler (`main.cc:67-69`) — the conventional reload signal is free and already wired.

| Capability | Difficulty | Note |
| --- | --- | --- |
| Safe reload of `.mon` / `.npc` / `moveuse.dat` | `MODERATE` | loaders are already file-scanning functions; the hard part is live creatures referencing an old race table |
| Selective reload via `SIGHUP` | `MODERATE` | handler exists, empty; SHM command channel could carry the selector |
| Rolling restart | `HARD` | one process owns the world; no handoff mechanism |
| World snapshot + resume | `MODERATE` | `SaveMap` + `.usr` files already constitute a snapshot; resume is just a restart — this is effectively the existing daily-reboot flow |
| Config reload | `MODERATE` | many values are captured into globals at init and used unguarded afterwards |

The daily scheduled reboot with graduated warnings (`main.cc:392-427`) is, in practice, the live-ops model this server was designed around. Working *with* it is far cheaper than engineering around it.

---

## 16. Feature Feasibility Matrix

`CUR` = current support. `SRV`/`PROTO`/`DB`/`R33D` = change needed. `772` = classic-client compatibility. `CX` = complexity. `RISK` = risk.

| Feature | CUR | SRV | PROTO | DB | R33D | 772 | CX | RISK | Recommended architecture |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Wide **visual** world (static) | none | **No** | **No** | No | Yes | **Preserved** | Med | **Low** | Client sector streamer from `.sec` + `objects.srv`; already scoped by `UNREAL-WIDE-WORLD-001` |
| Larger **live** viewport | partial (per-conn fields exist) | **Yes** | Yes | No | Yes | Broken unless gated | **High** | **High** | `TerminalType` gate + viewport fields + **all 7 announce radii** + buffer + known-creature table |
| Extended REAL33D protocol | none | Yes | Yes | No | Yes | Preserved | Med | Low | New opcodes in free ranges, gated on `TerminalType`; `CommandAllowed` enforces |
| Second protocol adapter | none | Yes | Yes | No | Yes | Preserved | High | Med | Branch inside `Send*` on `TerminalType`; not a full event layer (§11) |
| REST API | none | **No** | No | No | No | Preserved | Low | **Low** | Sidecar over DB + SHM |
| WebSocket events | none | **No** | No | No | No | Preserved | Low | **Low** | Sidecar tailing log/event stream |
| Telemetry | **partial (SHM)** | **No** | No | No | No | Preserved | **Low** | **Low** | SHM reader sidecar |
| PostgreSQL | **already supported** | No | No | Build flag | No | Preserved | **Low** | **Low** | `make DATABASE=postgres` + `postgres/schema.sql` |
| Dynamic maps | partial (`RefreshSector`) | Yes | No | No | No | Preserved | Med | Med | Extend the existing refresh path within fixed bounds |
| Instances | none | Yes | Yes | No | Yes | Broken | **Very High** | **Very High** | Needs an instance dimension in coordinates — rewrite |
| Raids (world) | **exists** (`.evt`) | No | No | No | No | Preserved | **Low** | **Low** | Data only |
| Achievements | none | Yes | Optional | Optional | Optional | Fallback OK | Low | Low | `QuestValues[500]` + `SendMessage`; rich UI via extension |
| Mounts | none | Yes | Yes | No | Yes | **Broken** | Med | Med | REAL33D-only; speed-only variant is classic-safe |
| Pets | partial (summons) | Yes | No | No | No | Preserved | Low | Low | Extend the summon/slave system |
| Crafting | partial (`multiuse`) | **No** | No | No | No | Preserved | **Low** | **Low** | `moveuse.dat` rules |
| Auction house | partial (houses) | Yes | Yes | Yes | Yes | Broken for UI | High | Med | Mirror the house-auction RPC pattern; UI outside the game |
| Account-wide data | none | Yes | No | **Yes** | No | Preserved | Med | Med | New QueryManager RPC; account scope already exists |
| Modern quest log | state exists | Yes | Yes | No | Yes | Fallback via messages | Med | Low | Extension opcode reading `QuestValues` |
| Modern inventory metadata | none | Yes | Yes | No | Yes | Broken for new fields | Med | Med | Extension opcode; `SendItem` stays untouched for classic |
| Server-side cosmetics | partial (outfits) | Yes | No | Maybe | Art | Preserved | Low | Low | Outfit id + colours; new looks need client art |
| Extended effects | partial | Yes | No | No | Art | Preserved | **Low** | **Low** | New effect ids over `SendGraphicalEffect` |
| Weather | none | Yes | Yes | No | Yes | Fallback via ambience | Med | Low | Extension opcode; ambience approximation for classic |
| Day / night | **exists** | No | No | No | No | Preserved | **None** | **None** | `GetAmbiente`, already diffed per second |
| World events | partial (raids) | Yes | No | No | No | Preserved | Low | Low | Raids + cron + broadcast |
| Admin dashboard | partial | **No** | No | No | No | Preserved | Low | **Low** | SHM reader + QueryManager RPCs |
| Live ops | partial (SHM cmd) | Minor | No | No | No | Preserved | Low | Low | Extend the SHM command vocabulary |
| Horizontal scaling | none | Rewrite | — | — | — | — | **Very High** | **Very High** | Not practical (§12) |

---

## 17. Extension Seams

### EXTENSION_SEAM_01 — Terminal type as a capability flag
- **Area:** session establishment / capability negotiation
- **Files:** `communication.cc:28,970`; `connections.cc:207-222`; `connections.hh:217-218`
- **Symbols:** `TERMINALVERSION[]`, `TConnection::TerminalType`, `TConnection::TerminalVersion`, `TConnection::JoinGame`
- **Current responsibility:** validate client version; distinguish ordinary (1) from gamemaster (2) clients
- **Why useful:** a client-declared, server-validated, session-persistent capability field **already exists and is already branched on**. Nothing needs inventing.
- **Safe to add:** a 4th `TERMINALVERSION` entry for REAL33D; per-type viewport defaults; per-type opcode gating
- **Must not couple here:** gameplay rules. A capability flag must never become a gameplay advantage flag.
- **Risk:** **Low.** `NARRAY(TERMINALVERSION)` bounds the check automatically; classic clients keep sending 1 or 2.

### EXTENSION_SEAM_02 — Inbound opcode dispatch
- **Area:** client→server protocol
- **Files:** `receiving.cc:1720-1790`
- **Symbols:** `ReceiveData(TConnection*)`, `CommandAllowed`, the `switch(Command)`, `default:`
- **Current responsibility:** route one command per packet to a handler
- **Why useful:** flat switch, one free function per command, unknown opcodes ignored not fatal, large free id ranges, already exception-guarded
- **Safe to add:** new `CL_CMD_*` in free ranges with handlers gated by `TerminalType` in `CommandAllowed`
- **Must not couple here:** blocking work. Handlers run on the game thread.
- **Risk:** **Very low** — the only demonstrably additive seam in the protocol.

### EXTENSION_SEAM_03 — Per-connection viewport fields
- **Area:** world visibility
- **Files:** `connections.cc:219-222` (assignment); `connections.cc:372-375`, `sending.cc:428-431,470-473,561-564`, `receiving.cc:526-527` (consumers)
- **Symbols:** `TerminalOffsetX/Y`, `TerminalWidth/Height`, `TConnection::IsVisible`
- **Current responsibility:** define the rectangle each connection sees
- **Why useful:** already a per-connection variable; a single assignment site controls all five consumers
- **Safe to add:** per-`TerminalType` values
- **Must not couple here:** **anything, until SEAM_04 is addressed.** Changing this alone produces a silently stale world.
- **Risk:** **High if used alone. Low in combination with SEAM_04.**

### EXTENSION_SEAM_04 — Announce radii (the gating seam)
- **Area:** interest management
- **Files:** `operate.cc:45, 100, 218, 243, 273, 1004, 1072`
- **Symbols:** `AnnounceChangedField`, `AnnounceGraphicalEffect`, `AnnounceTextualEffect`, `AnnounceMovingCreature`, `AnnounceMissile`, `TFindCreatures`
- **Current responsibility:** decide which players are told about a change
- **Why useful:** all radii are literals in seven adjacent call sites in one file — a small, enumerable change surface for such a structural property. The source's own TODO (`operate.cc:25-30`) documents the intent.
- **Safe to add:** named constants; then radii derived from the maximum viewport in play
- **Must not couple here:** per-connection logic inside the *search*. The search selects candidates; `IsVisible` filters. Keep that split or the spatial index stops helping.
- **Risk:** **Medium** — quadratic cost growth, and getting it wrong yields a stale world rather than a loud failure.

### EXTENSION_SEAM_05 — QueryManager RPC catalogue
- **Area:** persistence and external data
- **Files:** `query.hh:43-140`; `querymanager.hh:1112-1154`
- **Symbols:** `TQueryManagerConnection`, the `QUERY_*` enum
- **Current responsibility:** all durable account/house/statistics operations
- **Why useful:** typed request/response facade, connection pooling already implemented (`TQueryManagerConnectionPool`), and the id space has gaps: `22`, `24`, `34`, `49`, `104-149`, `153+`
- **Safe to add:** new query types for account-wide data, achievements, analytics
- **Must not couple here:** anything latency-sensitive on the game thread. QueryManager calls are blocking I/O.
- **Risk:** **Low** — both sides are ours; unknown query ids are rejected cleanly.

### EXTENSION_SEAM_06 — Shared memory telemetry and command channel
- **Area:** external integration
- **Files:** `shm.cc` (esp. `:261, :273`); `main.cc:294-310`
- **Symbols:** `TSharedMemory`, `SetCommand`, `GetCommand`, `GetCommandBuffer`, `ProcessCommand`
- **Current responsibility:** expose live counters; accept external commands (1 = broadcast)
- **Why useful:** **zero-coupling** read path and an existing write path, polled once per second. The source explicitly identifies it as a tool interface.
- **Safe to add:** new read-only counters; new command codes
- **Must not couple here:** anything large or latency-critical — the struct is fixed-size and the poll is 1 Hz
- **Risk:** **Very low** for reads; low for commands.

### EXTENSION_SEAM_07 — `QuestValues[500]`
- **Area:** per-character persistent state
- **Files:** `cr.hh:146, 856-857, 916`; `crplayer.cc:520-524, 589-591`; `moveuse.cc:207` (`MOVEUSE_CONDITION_HASQUESTVALUE`)
- **Symbols:** `GetQuestValue`, `SetQuestValue`, `TPlayerData::QuestValues`
- **Current responsibility:** quest progress flags
- **Why useful:** a generic, persisted, 500-slot int store that **already survives the save format** and is already readable from the data-driven rule engine. Achievements, daily quests, unlock flags and counters all fit without touching §8.3.
- **Safe to add:** a documented slot registry (the scarce resource is slots, not capability)
- **Must not couple here:** high-frequency counters — this is saved with the character every 15 minutes
- **Risk:** **Low**, provided slot allocation is governed.

### EXTENSION_SEAM_08 — `moveuse.dat` rule engine
- **Area:** item behaviour
- **Files:** `moveuse.cc:2977-3017` (`LoadDataBase`), `:90-222` (conditions)
- **Symbols:** `MOVEUSE_EVENT_USE|MULTIUSE|MOVEMENT|COLLISION|SEPARATION`, `MOVEUSE_CONDITION_*`
- **Current responsibility:** data-driven item interaction
- **Why useful:** a real condition/action DSL with `HASQUESTVALUE`, `HASLEVEL`, `HASRIGHT`, `HASPROFESSION`, `TESTSKILL`. Crafting, gated content and quest chains are **data**, not code.
- **Safe to add:** new rules; new condition/action types (code, but localized)
- **Must not couple here:** anything needing per-tick evaluation
- **Risk:** **Very low** for data; low for new condition types.

### EXTENSION_SEAM_09 — Outbound encoder functions
- **Area:** server→client serialization
- **Files:** `sending.cc:75-160` (primitives), `connections.hh:236-309` (~60 declarations)
- **Symbols:** `BeginSendData`, `SendByte/Word/Quad/Bytes/String`, `FinishSendData`, all `Send*`
- **Current responsibility:** encode game events into 7.72 bytes
- **Why useful:** uniform `TConnection*` signature and five primitives. A per-`TerminalType` branch **inside** these functions gives two wire protocols without touching a single game-logic call site (§11).
- **Safe to add:** capability-gated alternate encodings; new `Send*` for new opcodes
- **Must not couple here:** game logic decisions. These functions must stay encoders.
- **Risk:** **Medium** — `OutData[16384]` overflow is silent (`sending.cc:62-65`). Instrument before expanding payloads.

### EXTENSION_SEAM_10 — Reader thread async work channel
- **Area:** background I/O
- **Files:** `reader.hh` (full); `main.cc:351`
- **Symbols:** `TReaderThreadOrderType`, `TReaderThreadReplyType`, `LoadSectorOrder`, `ProcessReaderThreadReplies`
- **Current responsibility:** async sector and character loading, results applied on the game thread
- **Why useful:** **the only existing pattern for doing blocking work without stalling the world.** Order in, reply out, applied at a safe point in the tick. Any future integration needing I/O should copy this shape.
- **Safe to add:** new order/reply types for background loads
- **Must not couple here:** anything expecting a synchronous answer
- **Risk:** **Low** — the pattern is proven in-tree by `RefreshSector`.

### EXTENSION_SEAM_11 — `SIGHUP` no-op handler
- **Area:** live ops
- **Files:** `main.cc:67-69, 112`
- **Symbols:** `SigHupHandler`
- **Current responsibility:** nothing — it is empty
- **Why useful:** the conventional reload signal is already registered and deliberately inert
- **Safe to add:** a flag set here, acted on at a safe point in `AdvanceGame` (never work in the handler itself)
- **Must not couple here:** the reload work itself — this is a signal handler
- **Risk:** **Low** for the mechanism; the reload semantics are the hard part (§15).

### EXTENSION_SEAM_12 — `.usr` identifier-keyed loading (a seam that must be *created*)
- **Area:** character persistence
- **Files:** `crplayer.cc:2420` (`SavePlayerData`), `:~2168` (`LoadPlayerData`)
- **Current responsibility:** positional read/write of character state
- **Why useful:** the loader **already reads the identifier and discards it**. Matching on it instead — with defaults for missing keys — converts a rigid format into a permanently additive one, in one function pair.
- **Safe to add:** keyed dispatch with defaults; new fields thereafter
- **Must not couple here:** nothing until this is done. Every new persisted field is blocked behind it.
- **Risk:** **Medium** to change (it is the character save path — a bug is a data-loss bug), but it is the **single highest-leverage prerequisite** for §7's roadmap.

---

## 18. Architecture Options A–D

### OPTION A — Conservative Fusion32
*Server essentially untouched; REAL33D speaks stock 7.72.*

- **Benefits:** zero risk to the authoritative server; decompilation fidelity preserved; REAL33D keeps progressing on presentation, which is where its current blockers actually are (`UNREAL-WIDE-WORLD-001` is blocked on art, not protocol); classic compatibility guaranteed by construction; fully reversible.
- **Risks:** a hard ceiling on authoritative new information. Distant creatures, weather and richer NPC data are unreachable. Static-world streaming is available; dynamic wide-world content is not.
- **Migration effort:** none.
- **Classic compat:** perfect. **REAL33D freedom:** presentation-only. **Maintainability:** highest. **Performance:** unchanged. **Testability:** unchanged (poor server-side). **Reversibility:** total.

### OPTION B — Fusion32 Extended
*Optional capabilities gated on `TerminalType`; `Tibia.exe` keeps working.*

- **Benefits:** uses machinery that already exists (SEAM_01, 02, 09); inbound extension is demonstrably safe; classic clients take unchanged code paths; per-feature reversibility.
- **Risks:** two wire formats to keep correct forever; `OutData` overflow is silent; **any viewport work drags in SEAM_04**, which is the real cost and is easy to underestimate; a gating bug sends an extended packet to a classic client and desyncs it.
- **Migration effort:** low per feature, cumulative over time.
- **Classic compat:** preserved **if** gating is disciplined — this is the standing risk. **REAL33D freedom:** high. **Maintainability:** good if extensions stay in dedicated files. **Performance:** proportional to what is added. **Testability:** good — extensions can be fixture-tested like ClientCore. **Reversibility:** per feature.

### OPTION C — Fusion32 + Sidecar Platform
*Gameplay stays authoritative in Fusion32; new services live outside.*

- **Benefits:** the game thread cannot be destabilized by anything built here — the dominant operational risk in §12 and §13; SHM and the DB already expose enough for telemetry, dashboards, highscores and account management; sidecars are testable, restartable and written in whatever language suits; PostgreSQL is already available if a service wants real concurrent reads.
- **Risks:** cannot affect gameplay at all; eventual-consistency between sidecar views and world truth; more processes to operate.
- **Migration effort:** low, and entirely outside the server.
- **Classic compat:** perfect. **REAL33D freedom:** high for non-authoritative data. **Maintainability:** excellent — strongest isolation of any option. **Performance:** no game-thread impact. **Testability:** excellent. **Reversibility:** total.

### OPTION D — Protocol / Server Refactor
*Separate world/game logic from transport, convert to a multiprotocol backend.*

- **Benefits:** the only option that makes genuinely new authoritative capability cheap in the long run; would enable real interest management, instances and testable headless worlds.
- **Risks:** game logic emits protocol directly at dozens of sites (`cract.cc::NotifyGo`, the `operate.cc` announce family) — interposing an event layer touches nearly every gameplay file; it is a **decompiled** codebase whose value is fidelity to the original, and a refactor forfeits the ability to diff against original behaviour; there are no server tests to catch regressions (§14); `README.md:13` warns that exceptions are "too engraved into the codebase" to unpick.
- **Migration effort:** very high. **Classic compat:** at risk throughout. **REAL33D freedom:** eventually highest. **Maintainability:** better long-term, much worse during. **Performance:** could improve; could regress. **Testability:** the main prize. **Reversibility:** very low.

**Comparative note, grounded rather than intuited.** These options are not mutually exclusive, and the architecture found here does not force a single choice. A and C are additive and compose with anything. B is the only option that delivers new authoritative capability at moderate cost, and its cost is dominated by one thing — SEAM_04 — which this report has now enumerated precisely. D's cost is set by the game-logic-emits-protocol coupling, which is demonstrated and extensive. That is the comparison; the decision is the project owner's.

---

## 19. Major Blockers

1. **Interest-management constants vs. variable viewport** (`operate.cc`, 7 literal radii). Gates every wide-live-world scenario. `DEMONSTRATED`.
2. **Positional `.usr` parsing** (`crplayer.cc`, identifiers read and discarded). Gates all new per-character persisted state. `DEMONSTRATED`.
3. **Non-growable object storage** (`ResizeHashTable` → `abort()`). Caps total world objects at a boot-time constant. `DEMONSTRATED`.
4. **Single-process world** (`map.cc`/`crmain.cc` globals). Blocks horizontal scaling and instances. `DEMONSTRATED`.
5. **Game logic emits protocol directly** (`cract.cc::NotifyGo`, `operate.cc` announces). Sets the price of Option D. `DEMONSTRATED`.
6. **`OutData[16384]` with silent drop** (`sending.cc:62-65`). Caps payload growth and fails invisibly. `DEMONSTRATED`.
7. **`KnownCreatureTable[150]` with visibility-based eviction** (`connections.cc:400-431`). Interacts badly with a wider viewport. `DEMONSTRATED`.
8. **Hardcoded spells** (`magic.cc:4416+`). Every spell change is a recompile. `DEMONSTRATED`.
9. **Fixed world bounds** (`SectorXMin/Max` at init, capped at 2047). Blocks runtime world growth. `DEMONSTRATED`.
10. **Pre-auth RSA under a global mutex + unbounded connection slots** (`communication.cc:936-944`). Internet-exposure blocker. `DEMONSTRATED` structurally; exploitability `NOT_PROVEN` and not tested.

---

## 20. Low-Risk Opportunities

1. **PostgreSQL** — a make flag and an existing schema. `DEMONSTRATED`.
2. **SHM telemetry sidecar** — zero server change. `DEMONSTRATED`.
3. **Static wide-world streaming in Unreal** — zero server change; already scoped; blocked only on art. `DEMONSTRATED`.
4. **Achievements / dailies on `QuestValues[500]`** — no protocol, DB or save-format change. `DEMONSTRATED`.
5. **Crafting via `moveuse.dat` `multiuse`** — data only. `DEMONSTRATED`.
6. **New monsters / NPCs / raids** — data files, restart only. `DEMONSTRATED`.
7. **Fuzz the inbound parser** — single entry point, already exception-guarded, highest security value per hour. `STRONG_INFERENCE`.
8. **Named constants for the announce radii** — behaviour-preserving, and a prerequisite for any viewport work. `DEMONSTRATED`.
9. **Instrument `Connection->Overflow`** — converts a silent failure into a signal before anyone enlarges a payload. `DEMONSTRATED`.
10. **Admin dashboard over SHM + QueryManager** — read-only, no game-thread contact. `DEMONSTRATED`.
11. **Extra effect ids** over `SendGraphicalEffect` — classic-safe, art-gated. `DEMONSTRATED`.
12. **New `CL_CMD_*` opcodes for REAL33D** — ignored by an unmodified server, gated by `CommandAllowed`. `DEMONSTRATED`.

---

## 21. Questions Still Unanswered

1. **What does `Tibia.exe` 7.72 do on an unknown server opcode?** Assumed fatal. `NOT_PROVEN` — the client is a binary and was not analyzed. This assumption underwrites every "classic compatible" claim in §16.
2. **What is the real tick budget?** No benchmarks exist. `NOT_MEASURED`.
3. **At what viewport size does `OutData[16384]` first overflow?** Computable, not computed. `NOT_MEASURED`.
4. **Is `TerminalType 0` reachable?** `TERMINALVERSION` has three entries but `JoinGame` accepts only 1 and 2 — so type 0 passes `HandleLogin` and fails later. Whether that is vestigial or meaningful is `NOT_PROVEN`.
5. **Is `database_mariadb.cc` (377 bytes) a real backend or a stub?** File size suggests a stub. `HYPOTHESIS`.
6. **Does `LoadPlayerData` tolerate trailing unknown sections?** Determines whether append-only extension works without rewriting the loader. `NOT_PROVEN` — only the head of the function was read.
7. **How are account passwords hashed?** `sha256.cc` and `pwdhash.go` exist; salting/KDF `NOT_PROVEN`.
8. **What is the actual object-pool headroom in the live Thais map?** Determines how close `SwapSector`/`ResizeHashTable` are to being hit. `NOT_MEASURED`.
9. **Can NPC behaviour scripts call out asynchronously?** Would decide whether an AI/NPC sidecar is viable. `HYPOTHESIS` only; `crnonpl.cc` was not read in full.
10. **Do the 7.72 protocol changes (`-DTIBIA772=1`) alter anything beyond the login block?** Only the `HandleLogin` terminal-type relocation and `TERMINALVERSION` were confirmed. Other `#if TIBIA772` sites were not enumerated. `NOT_PROVEN`.
11. **What is `MapFlags` on `TSector` used for?** Present in the struct; its consumers were not traced. `NOT_PROVEN`.
12. **Would raising announce radii break 7.72 gameplay balance?** Wider awareness changes monster aggro and ranged engagement. `HYPOTHESIS` — needs a design decision, not just a code change.

---

*End of report. No project file was created, modified, or committed. This document lives in the session scratchpad.*
