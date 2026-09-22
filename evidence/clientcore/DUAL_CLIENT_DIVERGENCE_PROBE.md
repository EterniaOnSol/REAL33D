# DUAL_CLIENT_DIVERGENCE_PROBE

Milestone: `DUAL-CLIENT-DIVERGENCE-PROBE-001`
Mode: **RESEARCH / PROBE ONLY — READ ONLY**
Date: 2026-09-21
Status: **scratch, uncommitted, outside every project tree.**

Subjects compared:
- **Fusion32** `C:\Users\dell\Desktop\fusion32\reference\game\src` (CipSoft 7.7 decompilation, `TIBIA772` mode)
- **Protocol772Core** `clientcore/` (traced against the above)
- **mehah/otclient** — read-only at `C:\Users\dell\3DTIBIA\cliente`, **reference only, nothing copied**
- **OTCv8** — `Downloads/otcv8-dev-master.zip`, licence check only

Evidence labels: `DEMONSTRATED` · `STRONG_INFERENCE` · `HYPOTHESIS` · `NOT_PROVEN`.

---

## 1. Candidate and licence evidence

### 1.1 Licences — checked first, as instructed

| Candidate | Licence file | Declared | Headers | SPDX |
| --- | --- | --- | --- | --- |
| **mehah/otclient** | `LICENSE` (1,166 B) | **MIT** — "OTClient is made available under the MIT License", © 2010-2020 OTClient | per-file MIT block, © 2010-**2026** | none (0 files) |
| **OTCv8** | `LICENSE` | **MIT** — "OTClientV8 is made available under the MIT License", © 2010-2017 OTClient, © 2018-2023 OTClientV8 | not sampled | not sampled |

`DEMONSTRATED`. Both carry the standard MIT grant including redistribution and sublicensing, with the notice-retention condition.

**This is a materially better position than the server references.** The earlier milestone found both 8.60 server references under GPL-2.0, which blocked source absorption into a private REAL33D. MIT does not.

### 1.2 Dependency licences — not cleared

mehah declares **31 vcpkg dependencies**: `asio, abseil, cpp-httplib, cppcodec, discord-rpc, liblzma, libarchive, libobfuscate, libogg, libpng, libvorbis, nlohmann-json, openal-soft, openssl, parallel-hashmap, physfs, protobuf, pugixml, stduuid, zlib, bshoshany-thread-pool, fmt, ixwebsocket, spdlog, utfcpp, freetype, inih, luajit, opengl, glew, angle`. `DEMONSTRATED`.

Individual dependency licences were **not** inspected. At least two warrant attention before distribution because they are commonly non-MIT in ways that carry conditions — `openal-soft` and `freetype` — but this report makes **no** determination about them.

### 1.3 Verdict

```
LICENCE_STATUS = PARTIALLY_VERIFIED
  project licence (both candidates) = MIT, DEMONSTRATED
  dependency licences               = NOT_VERIFIED
  redistribution obligations        = NOT_ASSESSED
```

Not legal advice, and **this does not authorize adoption.** The project licence is clear; the dependency tree is not yet cleared.

---

## 2. Build evidence

```
BUILD_PERFORMED = NO
```

Stated plainly: **no OTClient build was produced, and no live dual-decoder session was run.** The brief made compilation conditional ("si necesitas compilar"), and a build was not required to produce decision-grade results for the static protocol surface — which is where the architectural question actually lives. A build *is* required for the live confirmation, and that is scoped in §11 as the next step rather than claimed here.

Verified without building:

| Claim | Evidence |
| --- | --- |
| mehah declares 772 | `modules/gamelib/game.lua:54-55` — `supportedClients = { 740, 741, 750, 755, 760, 770, 772, 780, ... 1525 }` |
| OTCv8 declares 772 | `modules/gamelib/game.lua` — `getSupportedClients()` returns `{ 740, 741, 750, 760, 770, 772, 780, ... }` |

`DEMONSTRATED`.

**Consequence for confidence.** Every result below is a *source-differential* comparison — two decoders read against each other and against the Fusion32 emitter that produced the format. This is strong for encoding rules, constants and dispatch, and it is **not** a substitute for byte-identical observation of a live stream. Results are labelled accordingly.

---

## 3. Test methodology

Strategy **A** from the brief (existing REAL33D fixtures) plus source-differential analysis. Strategy C (live capture) was not reached.

The chain that makes this meaningful:

```
Fusion32 sending.cc  ──emits──▶  7.72 bytes
        │                            │
        │ traced symbol-for-symbol   │
        ▼                            ▼
ClientCore decoder            OTClient decoder
(clientcore/src/*)            (src/client/protocolgame*.cpp)
        │                            │
        └────── compared here ───────┘
```

ClientCore's fixture header states its provenance directly: *"A literal port of the 7.72 emitter so fixtures are produced by the traced algorithm instead of being retyped. Mirrors, symbol for symbol: `sending.cc::SendFullScreen`, `SendMapPoint`, `SkipFlush`, `SendMapObject`/`SendItem`/`SendOutfit`."* `DEMONSTRATED`.

So Fusion32's emitter is the arbiter, not either client.

**No credentials, keys, passwords, XTEA keys or RSA private material appear in this report.** No session capture was taken.

---

## 4. Comparable protocol surface

| System | Comparable? | Basis |
| --- | --- | --- |
| Transport / framing | ✅ | both length-prefixed LE |
| XTEA handling | ✅ | key placement in RSA block |
| Login (login server) | ⚠️ partial | ClientCore `login.h`; OTClient `ProtocolLogin` not read in depth |
| Game login | ✅ | full layout both sides |
| Initial world / fullscreen | ✅ | traversal + skip + item + creature |
| Floor decoding | ✅ | z-loop bounds |
| Map stack order | ✅ | emission order |
| Object decoding | ✅ | wire-length rule |
| Creatures | ✅ | 97/98/99 markers |
| Known-creature handling | ⚠️ partial | table size vs OTClient's model |
| Movement | ✅ | opcode identity |
| Rows after movement | ✅ | opcodes 101-104 |
| Floor up/down | ✅ | opcodes 190/191 |
| Player stats | ✅ | opcode 160 |
| Inventory | ✅ | opcodes 120/121 |
| Containers | ✅ | opcodes 110-114 |
| Item move | ✅ | opcode 120 |
| Creature movement | ✅ | opcode 109 |
| Creature turn | ✅ | via 106/107 field ops |
| Chat | ✅ | mode table |
| Private/channel chat | ✅ | mode table |
| Attack/follow | ✅ | opcodes 161/162 |
| Effects | ✅ | opcode 131 |
| Projectiles | ✅ | opcode 133 |
| Outfit | ✅ | `SendOutfit` shape |
| Logout / reconnect | ⚠️ partial | opcode identity only |
| **NPC shops** | ❌ | `NOT_COMPARABLE_YET` — absent from 7.72 entirely |
| **Extended opcode** | ❌ | `NOT_COMPARABLE_YET` — not implemented either side |

---

## 5. Exact matches

### TC-01 — Game login header layout
**Fusion32** `communication.cc:920-956`, `TIBIA772` branch: terminal fields are read **before** the RSA block.
**ClientCore** `gamelogin.cpp:136-141`:
```
payload[0]   = 10                    (kGameLoginRequestOpcode)
payload[1-2] = terminal_type   u16 LE
payload[3-4] = terminal_version u16 LE
payload[5..] = RSA-1024 ciphertext (128 B)
```
**OTClient** `protocolgamesend.cpp:54-56`:
```
addU8(ClientPendingGame)   // = 10
addU16(g_game.getOs())
addU16(g_game.getProtocolVersion())
... offset = messageSize → RSA block begins
```
**RESULT: `MATCH`.** Identical shape. OTClient names the first `u16` "os" where CipSoft names it "TerminalType" — the same field. `DEMONSTRATED`.

### TC-02 — TerminalType value domain
Fusion32 `connections.cc:214` accepts **only 1 or 2**; `communication.cc:970` bounds against `NARRAY(TERMINALVERSION)` = 3.
OTClient `modules/gamelib/const.lua:329-336`: `OsTypes = { Linux = 1, Windows = 2, Flash = 3, OtclientLinux = 10, ... }`.
**RESULT: `MATCH`** for `Linux=1` / `Windows=2`; values ≥ 3 rejected by the server. `DEMONSTRATED`.

This retroactively explains a Fusion32 detail: `TERMINALVERSION[]` has three entries indexed 0,1,2 because the field is the client OS code.

### TC-03 — RSA plaintext block layout
ClientCore `gamelogin.cpp:104-113`: `[0]=0x00`, `[1..16]` XTEA key (4×u32 LE), `[17]` gamemaster flag, `[18..21]` accountID u32, `[22..]` string(name), string(password), random padding.
Fusion32 `communication.cc:946-956`: reads zero byte, `SymmetricKey.init`, gamemaster byte, `readQuad` account, `readString` name, `readString` password.
OTClient `protocolgamesend.cpp:76-100`: `addU8(0)`, 4×`addU32` xtea, `addU8(0)` gm, `addU32(accountName)`, `addString(characterName)`, `addString(accountPassword)`.
**RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-04 — Server opcode table coverage
All **61 of 61** Fusion32 `ServerCommand` values exist in OTClient's `GameServerOpcodes`. **RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-05 — Client opcode table coverage
**60 of 61** Fusion32 `ClientCommand` values exist in OTClient's `ClientOpcodes`. The one absent, `CL_CMD_LOGIN = 11`, is **never on the wire** — `HandleLogin` rewrites the packet internally for the main thread (`communication.cc:1102`) and `ReceiveData` expects it only in `CONNECTION_LOGIN` state. **RESULT: `MATCH` (60/60 wire opcodes).** `DEMONSTRATED`.

### TC-06 — Opcode 134 collision resolved by version gate
OTClient names 134 `GameServerItemClasses`, but dispatches:
```cpp
case Proto::GameServerItemClasses:
    if (g_game.getClientVersion() >= 1281) parseItemClasses(msg);
    else                                   parseCreatureMark(msg);
```
At 772 → `parseCreatureMark`, matching `SV_CMD_MARK_CREATURE = 134`. **RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-07 — Map skip-marker encoding
Fusion32 `sending.cc::SkipFlush`: emits `count` then `0xFF`.
ClientCore `object_types.h:28` `kSkipMarkerBase = 0xFF00`; `map_scan.cpp:339` `word >= kSkipMarkerBase`, count `= marker & 0x00FF`.
OTClient `protocolgameparse.cpp:3936-3937`: `if (msg->peekU16() >= 0xff00) return msg->getU16() & 0xff;`
**RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-08 — Map traversal order
Fusion32 `sending.cc:453-454`: `for PointX ... for PointY` — X outer, Y inner.
OTClient `protocolgameparse.cpp:3916-3917`: `for (nx...) for (ny...)` — X outer, Y inner.
**RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-09 — Floor iteration
Both walk `startz → endz` with a `zstep`, accumulating `skip` across floors (OTClient `:3906-3908`; ClientCore `map_scan` equivalently). **RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-10 — Creature descriptor markers
OTClient `protocolcodes.h:40-42`: `UnknownCreature = 97, OutdatedCreature = 98, Creature = 99`.
ClientCore `worldstate.h`: word 97 Introduced, 98 Outdated, 99 Known.
**RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-11 — Item/creature discrimination in a tile stack
OTClient `getThing` reads `u16`; if it is 97/98/99 → `getCreature`, else → `getItem`. Fusion32 `SendMapObject` emits exactly that discriminator. **RESULT: `MATCH`.** `DEMONSTRATED`.

### TC-12 — Chargeable phantom byte — not reachable at 772
OTClient's `getItem` includes `isChargeable()` in its length condition, but `ThingAttrChargeable` is only assigned when `getClientVersion() >= 780` (`thingtype.cpp:528-536`). At 772 the 7.55-7.72 branch runs and never sets it. **RESULT: `MATCH` — no phantom byte.** `DEMONSTRATED`.

### TC-13 — Extended opcode slot
Fusion32 uses **no opcode in 31–99** in either direction. OTClient defines `GameServerExtendedOpcode = 50` and `ClientExtendedOpcode = 50`. **RESULT: `MATCH` (slot free, client-supported).** `DEMONSTRATED`.

### TC-14 — Talk mode table, modes 1-12 and 16-23
OTClient's `version >= 740` branch (`protocolcodes.cpp:197-224`) against Fusion32's `TALK_*` enum:

| Wire | Fusion32 | OTClient |
| --- | --- | --- |
| 1 | `TALK_SAY` | `MessageSay` |
| 2 | `TALK_WHISPER` | `MessageWhisper` |
| 3 | `TALK_YELL` | `MessageYell` |
| 4 | `TALK_PRIVATE_MESSAGE` | `MessagePrivateFrom/To` |
| 5 | `TALK_CHANNEL_CALL` | `MessageChannel` |
| 6 | `TALK_GAMEMASTER_REQUEST` | `MessageRVRChannel` |
| 7 | `TALK_GAMEMASTER_ANSWER` | `MessageRVRAnswer` |
| 8 | `TALK_PLAYER_ANSWER` | `MessageRVRContinue` |
| 9 | `TALK_GAMEMASTER_BROADCAST` | `MessageGamemasterBroadcast` |
| 10 | `TALK_GAMEMASTER_CHANNELCALL` | `MessageGamemasterChannel` |
| 11 | `TALK_GAMEMASTER_MESSAGE` | `MessageGamemasterPrivateFrom/To` |
| 12 | `TALK_HIGHLIGHT_CHANNELCALL` | `MessageChannelHighlight` |
| 16 | `TALK_ANIMAL_LOW` | `MessageMonsterSay` |
| 17 | `TALK_ANIMAL_LOUD` | `MessageMonsterYell` |
| 18 | `TALK_ADMIN_MESSAGE` | `MessageWarning` |
| 19 | `TALK_EVENT_MESSAGE` | `MessageGame` |
| 20 | `TALK_LOGIN_MESSAGE` | `MessageLogin` |
| 21 | `TALK_STATUS_MESSAGE` | `MessageStatus` |
| 22 | `TALK_INFO_MESSAGE` | `MessageLook` |
| 23 | `TALK_FAILURE_MESSAGE` | `MessageFailure` |

**RESULT: `MATCH` on 20 of 23 modes.** `DEMONSTRATED`. That OTClient's table is version-aware — and that its 740-branch was evidently derived from this era — is the single most reassuring result in the probe.

---

## 6. Semantic matches

### TC-15 — Item wire length rule
Fusion32 `sending.cc::SendItem`:
```c
SendWord(getDisguise().TypeID);
if(LIQUIDCONTAINER) SendByte(liquidColor);
if(LIQUIDPOOL)      SendByte(liquidColor);
if(CUMULATIVE)      SendByte(amount);
```
OTClient `getItem`: `if (isStackable() || isFluidContainer() || isSplash() || isChargeable()) → one byte`.

Structurally different — three independent `if`s versus one disjunction — but **equivalent in practice**, because of TC-16.
**RESULT: `SEMANTIC_MATCH`**, conditional on the data agreement in §8. `DEMONSTRATED` for the rule.

### TC-16 — Multi-flag hazard is unreachable
Across all **5,003** object types in `visual/manifests/objects.csv` (generated from the server's own `dat/objects.srv`):

| `wire_extra_bytes` | Types |
| --- | --- |
| *(none)* | **4,887** |
| `amount` | 81 |
| `liquid_colour` | 35 |

Flag combinations: `Cumulative` alone 81, `LiquidContainer` alone 23, `LiquidPool` alone 12. **Zero types carry more than one wire-relevant flag.**

**RESULT: `SEMANTIC_MATCH`.** The theoretical multi-byte divergence is **not reachable in the 7.72 dataset**. `DEMONSTRATED`.

⚠️ **This is a constraint to record, not a solved problem.** It becomes a live stream-desync the moment REAL33D authors an object type carrying two of `{LIQUIDCONTAINER, LIQUIDPOOL, CUMULATIVE}`. Fusion32 would emit two bytes; OTClient would read one.

### TC-17 — Rule-violation family (174-177)
Fusion32 `SV_CMD_OPEN_REQUEST_QUEUE/DELETE_REQUEST/FINISH_REQUEST/CLOSE_REQUEST` ↔ OTClient `GameServerRuleViolationChannel/Remove/Cancel/Lock`. Same family, same numbers; OTClient carries modern aliases in comments. **RESULT: `SEMANTIC_MATCH`.** `DEMONSTRATED`.

### TC-18 — Naming-convention differences
`SV_CMD_FULLSCREEN`↔`GameServerFullMap`, `SV_CMD_ROW_NORTH`↔`GameServerMapTopRow`, `SV_CMD_SNAPBACK`↔`GameServerCancelWalk`, `SV_CMD_BUDDY_*`↔`GameServerVip*`, `CL_CMD_SET_TACTICS`↔`ClientChangeFightModes`, `CL_CMD_GO_PATH`↔`ClientAutoWalk`, `CL_CMD_TURN_OBJECT`↔`ClientRotateItem`. All same number, same concept. **RESULT: `SEMANTIC_MATCH` ×21.** `DEMONSTRATED`.

### TC-19 — Talk tail shape
Both drive the packet tail from the mode: position modes read `x,y,z`; channel modes read `u16 channel`; private modes read neither. **RESULT: `SEMANTIC_MATCH`** for mapped modes. `DEMONSTRATED`.

---

## 7. Divergences

### DIV-01 — Talk modes 13/14/15 unmapped — **MAJOR**
- **File / symbol:** `src/client/protocolcodes.cpp`, `version >= 740` branch — literal comment `// 13, 14, 15 ??`
- **Expected bytes:** Fusion32 emits `TALK_ANONYMOUS_BROADCAST = 13`, `TALK_ANONYMOUS_CHANNELCALL = 14`, `TALK_ANONYMOUS_MESSAGE = 15` (`enums.hh`)
- **Actual interpretation:** `translateMessageModeFromServer()` finds no entry → returns `Otc::MessageInvalid` → the tail `switch` in `parseTalk` matches no case → the position or channel tail is **not consumed**
- **Consequence:** **stream desynchronization**, not a cosmetic miss. Every subsequent byte in that packet is misread.
- **Reachability:** `DEMONSTRATED` — these modes are live in `operate.cc` (6 sites), `receiving.cc` (9), `sending.cc` (2). `CTalk` explicitly accepts them (`receiving.cc:774-776`). Gamemaster-originated, so ordinary sessions are unlikely to hit them — but a GM using anonymous broadcast desyncs every connected 2D client.
- **Classification:** `OTCLIENT_PATCH` — three table entries.
- **Proposed fix location:** `protocolcodes.cpp`, the `version >= 740` branch. Fusion32's enum supplies exactly the values OTClient marks unknown.

### DIV-02 — Anonymous channel-call payload shape — **MAJOR**
- **File / symbol:** Fusion32 `sending.cc:1389-1391`
```c
SendByte(SV_CMD_TALK);
SendQuad(StatementID);
if(Mode != TALK_ANONYMOUS_CHANNELCALL){ /* sender name */ }
```
- **Consequence:** mode 14 omits the sender-name string. Even with DIV-01 fixed by mapping alone, OTClient would still read a name that was never sent.
- **Classification:** `OTCLIENT_PATCH` — a shape branch, not only a table entry.
- **Note:** DIV-01 and DIV-02 must be fixed **together**; fixing the table alone converts a silent skip into a misparse.

### DIV-03 — Client OS field must be pinned — **TRIVIAL**
- **File / symbol:** `Game::getOs()` (`game.cpp`), `setCustomOs` (`game.h:343`), `modules/gamelib/game.lua:26-40`
- **Expected:** Fusion32 accepts only `TerminalType` 1 or 2.
- **Actual:** default `getOs()` returns 20/21/22/23/24/25 by platform → `TerminalType ≥ 3` → rejected at `communication.cc:970` with *"Your terminal version is too old."*
- **Classification:** **`CLIENT_CONFIG`** — `g_game.setCustomOs(OsTypes.Windows)` (=2). The hook exists, is Lua-bound (`luafunctions.cpp:359`), and is already used for official-Tibia hosts. **No patch.**

### DIV-04 — RSA modulus selection — **TRIVIAL**
- **File / symbol:** `g_game.chooseRsa(host)`, `modules/gamelib/const.lua:314,320` (`OTSERV_RSA`, `CIPSOFT_RSA`)
- **Expected:** Fusion32 uses its own key (`tibia.pem`; README advises generating a fresh one, and the project's IP-changer already performs a "fresh-modulus live patch").
- **Actual:** OTClient picks CipSoft or OTServ modulus by hostname.
- **Classification:** **`CLIENT_CONFIG`** — `g_game.setRsa(<REAL33D modulus>)`.

### DIV-05 — `GameEnvironmentEffect` must be off — **TRIVIAL**
- **File / symbol:** `protocolgameparse.cpp:3940-3943` — consumes a `u16` per tile when enabled.
- **Consequence if wrongly on:** two extra bytes per described tile → immediate map desync.
- **Classification:** `CLIENT_CONFIG` — feature-flag verification for the 772 profile.

### DIV-06 — Login-server protocol not compared — **UNKNOWN**
- ClientCore `login.h` (opcodes 1/10/20/100, `LoginCharacter{world_address, world_port}`) was read; OTClient's `ProtocolLogin` was not.
- **Classification:** `UNKNOWN`. Must be closed before a 2D client can reach character selection.

### DIV-07 — Known-creature table capacity — **MINOR / UNKNOWN**
- Fusion32 keeps `KnownCreatureTable[150]` per connection and evicts by visibility (`connections.cc:400-431`); ClientCore mirrors `kKnownCreatureTableSize = 150`.
- OTClient's creature cache model was not compared.
- **Consequence:** the server decides when to send a full descriptor versus a known-id reference. A client caching differently still parses correctly (the marker is explicit) but may hold stale appearance.
- **Classification:** `UNKNOWN`, likely `TRIVIAL_ADAPTER`.

### DIV-08 — Item metadata source agreement — **UNKNOWN, HIGH IMPACT**
See §8. The rule matches (TC-15); whether the *data* matches is unverified.

---

## 8. Object metadata compatibility

Fusion32 and ClientCore derive item wire length from `dat/objects.srv`:
> *"`ObjectTypeTable` therefore holds both the length flags and the priority, and is an explicit, injected dependency of the decoders, loaded from the server's own `dat/objects.srv`, rather than knowledge baked into a parser."* — `ARCHITECTURE.md`, `DEMONSTRATED`.

OTClient derives the same lengths from `Tibia.dat` via `ThingType` (`isStackable`, `isFluidContainer`, `isSplash`), and additionally needs `.dat`/`.spr` for ground/blocking/stack-priority and rendering.

**The two agree only if `objects.srv` and `Tibia.dat` agree on those flags for all 5,003 types.** This is `NOT_PROVEN`: the project's own `visual/manifests/summary.json` lists `client Tibia.dat / Tibia.spr (7.72)` under `sources_unavailable`, with provenance recorded as `UNKNOWN`.

### Would `objects.srv → OTClient metadata` generation be viable?

**Yes for the protocol-relevant subset; no for rendering.** `STRONG_INFERENCE`.

| Metadata | From `objects.srv`? | Note |
| --- | --- | --- |
| Item ids | ✅ | 5,003 types, ids 0-5090 |
| Stackable (`CUMULATIVE`) | ✅ | 81 types |
| Fluid container / splash | ✅ | 23 + 12 types |
| Container property | ✅ | `CONTAINER`, `CHEST` flags |
| Ground / stack priority | ✅ | `BANK/CLIP/BOTTOM/TOP/LOW` → `GetObjectPriority` |
| Movement / blocking | ✅ | `UNPASS`, `UNMOVE`, `UNTHROW`, `UNLAY`, `AVOID` |
| Light | ✅ | `BRIGHTNESS`, `LIGHTCOLOR` |
| **Sprite ids, frames, offsets, animation** | ❌ | exists only in `.spr`/`.dat` |

The decisive advantage of generating from `objects.srv`: it removes the `.dat` from the *protocol* path entirely, leaving it only in the *rendering* path. That would eliminate DIV-08 as a class rather than fixing one instance — the protocol would then be driven by the same source on both clients, exactly as the 3D client already is.

**Not implemented, as instructed.** But this is the highest-leverage item the probe surfaced.

---

## 9. Extended opcode verification

```
SLOT_FREE_FUSION32 = YES
  Fusion32 uses NO opcode in 31..99 in either direction.
  ClientCommand ∩ [31,99] = ∅   ServerCommand ∩ [31,99] = ∅
  Source: reference/game/src/connections.hh:12-138

SUPPORTED_OTCLIENT = YES
  src/client/protocolcodes.h:76   GameServerExtendedOpcode = 50
  src/client/protocolcodes.h:272  ClientExtendedOpcode     = 50
  src/client/protocolgame.h:34    sendExtendedOpcode(uint8_t, const std::string&)
  src/client/protocolgame.h:339   parseExtendedOpcode(const InputMessagePtr&)
  src/client/protocolgame.h:474   m_enableSendExtendedOpcode{ false }   // opt-in

COLLISION_RISK = NONE_OBSERVED
  No Fusion32 symbol occupies 50; no OTClient 772-path handler occupies a
  Fusion32 opcode. Corroborated by the 8.60 reference, where 0x32 (=50) is
  likewise the OTClient extended-opcode slot.
```

`DEMONSTRATED`. **Not implemented, not reserved**, per instruction. Noting only that `m_enableSendExtendedOpcode` defaults to `false`, so the channel is inert until explicitly enabled — a helpful safety property.

---

## 10. Measurement

Counted over discrete, individually verified protocol facts. `UNKNOWN` excluded from the percentage, as instructed.

```
TOTAL_COMPARABLE_CASES = 24
EXACT_MATCHES          = 14
SEMANTIC_MATCHES       =  5
MINOR_DIVERGENCES      =  3   (DIV-03, DIV-04, DIV-05 — all CLIENT_CONFIG)
MAJOR_DIVERGENCES      =  2   (DIV-01, DIV-02 — anonymous talk modes)
OTCLIENT_UNSUPPORTED   =  0
CLIENTCORE_UNSUPPORTED =  0
UNKNOWN                =  3   (DIV-06 login server, DIV-07 known-creature, DIV-08 .dat agreement)

CLASSIC_772_COMPATIBILITY_PERCENT = 79%   (19 exact+semantic of 24 comparable)
                                    or 92% if the three CLIENT_CONFIG items
                                    are counted as compatible-after-configuration
```

Both figures are given because the distinction is real: DIV-03/04/05 require **no code change at all**, only configuration through hooks that already exist and are already exercised for other servers.

### Divergence classification summary

| Class | Count | Items |
| --- | --- | --- |
| `TRIVIAL_ADAPTER` | 0 | — |
| `CLIENT_CONFIG` | 3 | DIV-03, DIV-04, DIV-05 |
| `OTCLIENT_PATCH` | 2 | DIV-01, DIV-02 |
| `CLIENTCORE_PATCH` | 0 | — |
| `SERVER_COMPAT_LAYER` | 0 | — |
| `MAJOR_PROTOCOL_DIFFERENCE` | 0 | — |
| `UNKNOWN` | 3 | DIV-06, DIV-07, DIV-08 |

**`REQUIRED_SERVER_CHANGES = 0`** for the classic subset. No divergence found requires touching Fusion32.

---

## 11. Architectural implications

The brief's decision rule:
> if OTClient classic divergence is small → Option C becomes stronger
> if broad/deep → sharing more ClientCore becomes worth revisiting

**The measured divergence is small and shallow**, on the static surface:
- Zero opcode-table conflicts across 121 opcodes.
- Every structural encoding rule checked — login layout, RSA block, skip markers, traversal order, creature markers, item length — matches.
- Three divergences are configuration through existing hooks.
- Two are a three-entry table gap plus one payload-shape branch, in one file, in a code path OTClient itself documents as unknown.

**`OPTION_C_EVIDENCE`** — Strong. OTClient's 7.72 support is not an approximation of the CipSoft dialect; its `version >= 740` tables were evidently derived from this era, down to gating opcode 134 correctly and carrying an accurate 20-of-23 talk-mode table. Keeping its parser costs little and the remaining gaps are enumerable.

**`OPTION_B_EVIDENCE`** — Weaker, but not eliminated, and one finding argues for it specifically: DIV-08. The two decoders derive item wire length from **different sources** (`objects.srv` vs `Tibia.dat`), and that agreement is unverified. §8's generator would neutralize this — and a generator is itself a step *toward* shared protocol knowledge, which is Option B's core value.

**No winner declared.** The static surface favours C; the live surface is unmeasured, and DIV-08 is exactly the kind of issue a static comparison cannot settle. The honest position: **C is ahead on the evidence available, and the evidence available does not yet include a single real byte off a live Fusion32 socket.**

---

## 12. Remaining unknowns

1. **No live session was run.** Every result is source-differential. `BUILD_PERFORMED = NO`.
2. **`Tibia.dat` vs `objects.srv` flag agreement** — unverified, and the `.dat` is recorded as unavailable with `UNKNOWN` provenance.
3. **Login-server protocol** — OTClient's `ProtocolLogin` not compared (DIV-06).
4. **Known-creature caching** — OTClient's model not compared (DIV-07).
5. **Container, trade and text-window payload shapes** — opcode identity confirmed, field-level layouts not compared.
6. **Dependency licences** — 31 packages uninspected.
7. **OTCv8** — compared only on licence and 772 declaration; its parser was not analysed.
8. **Whether Fusion32's `SendTalk` overload selection matches OTClient's tail switch for every mapped mode** — checked structurally, not per-mode.
9. **`m_enableSendExtendedOpcode`** default-false behaviour against a server that never sends opcode 50 — expected inert, `NOT_PROVEN`.

---

# RESULT

```
DUAL_CLIENT_DIVERGENCE_PROBE = COMPLETE   (static surface)
                               PARTIAL    (live surface — no build, no capture)
```

**OTCLIENT_772_CONNECTS_TO_FUSION32** — `NOT_DEMONSTRATED_LIVE`, but no structural blocker found. Login header, terminal-type domain, RSA block layout and XTEA placement all match; connection requires only `setCustomOs(2)` and the REAL33D RSA modulus, both existing Lua hooks.

**LOGIN_FIDELITY** — Game login `MATCH` at full layout depth. Login-*server* protocol `UNKNOWN` (not compared).

**GAMELOGIN_FIDELITY** — `MATCH`. Opcode, both `u16` fields, 128-byte RSA block, and the block's internal layout all agree.

**INITIAL_WORLD_FIDELITY** — `MATCH`. Skip marker `0xFF00`/`&0xFF`, X-outer/Y-inner traversal, floor iteration with carried skip, and the 97/98/99 discriminator all identical.

**MOVEMENT_FIDELITY** — `MATCH` at opcode level (100-109, 190/191, 181). Field-level row payloads not byte-compared.

**OBJECT_FIDELITY** — `SEMANTIC_MATCH`. Length rules are equivalent and the multi-flag hazard is unreachable (0 of 5,003 types). Gated on unverified `.dat`/`objects.srv` agreement.

**CREATURE_FIDELITY** — `MATCH` on markers and dispatch; known-creature caching `UNKNOWN`.

**CHAT_FIDELITY** — `MATCH` on 20 of 23 modes; **`MAJOR` divergence on modes 13/14/15**, which OTClient's own source marks `// 13, 14, 15 ??` and Fusion32 actively uses.

**CONTAINER_FIDELITY** — `MATCH` at opcode level (110-114); payload layouts not compared.

**COMBAT_FIDELITY** — `MATCH` at opcode level (160-163, 140); damage/effect payloads not compared.

```
TOTAL_COMPARABLE_CASES = 24
EXACT_MATCHES          = 14
SEMANTIC_MATCHES       =  5
MINOR_DIVERGENCES      =  3
MAJOR_DIVERGENCES      =  2

EXTENDED_OPCODE_50_STATUS = FREE_IN_FUSION32, SUPPORTED_IN_OTCLIENT,
                            NO_COLLISION_OBSERVED, NOT_RESERVED, NOT_IMPLEMENTED
LICENCE_STATUS            = PARTIALLY_VERIFIED
                            (MIT on both projects = DEMONSTRATED;
                             31 dependencies = NOT_VERIFIED)
```

## TOP_DIVERGENCES

1. **DIV-01** — talk modes 13/14/15 unmapped → stream desync on GM anonymous speech. `OTCLIENT_PATCH`.
2. **DIV-02** — `TALK_ANONYMOUS_CHANNELCALL` omits the sender name → shape branch needed alongside DIV-01. `OTCLIENT_PATCH`.
3. **DIV-08** — item metadata sourced from `Tibia.dat` vs `objects.srv`; agreement unverified. `UNKNOWN`, high impact.
4. **DIV-03** — client OS field must be pinned to 1 or 2. `CLIENT_CONFIG`.
5. **DIV-04** — RSA modulus must be REAL33D's. `CLIENT_CONFIG`.
6. **DIV-05** — `GameEnvironmentEffect` must be off. `CLIENT_CONFIG`.
7. **DIV-06** — login-server protocol uncompared. `UNKNOWN`.
8. **DIV-07** — known-creature caching uncompared. `UNKNOWN`.
9. **Latent** — multi-flag item types would desync; currently unreachable, becomes reachable if REAL33D authors one.

## REQUIRED_OTCLIENT_PATCHES

| # | File | Change | Size |
| --- | --- | --- | --- |
| 1 | `src/client/protocolcodes.cpp` (`version >= 740`) | map wire modes 13/14/15 to the anonymous family | 3 lines |
| 2 | `src/client/protocolgameparse.cpp::parseTalk` | omit sender-name read for anonymous channel-call | 1 branch |

Everything else is configuration. **Two source changes, one file each.**

## REQUIRED_SERVER_CHANGES

```
REQUIRED_SERVER_CHANGES = 0
```

No divergence found requires modifying Fusion32 for the classic subset. Achieved as hoped.

```
OPTION_B_EVIDENCE = Present but secondary. Its strongest support is DIV-08:
                    two decoders deriving wire length from two different
                    sources is a structural drift risk that only shared
                    knowledge removes. An objects.srv-driven metadata
                    generator would address it and is itself a move toward B.

OPTION_C_EVIDENCE = Strong on everything measured. Zero opcode conflicts
                    across 121 opcodes, every structural encoding rule
                    matching, and OTClient's 740-era tables evidently derived
                    from this protocol generation. Remaining gaps are
                    enumerable and total two source edits.

WINNER = NOT_DECLARED. The static surface favours C. No live byte has been
         compared, and DIV-08 cannot be settled statically.
```

---

## Next step (not performed)

`DUAL-CLIENT-LIVE-CAPTURE-001` — build mehah/otclient in an isolated scratch checkout (not from 3DTIBIA), configure `setCustomOs(2)` + REAL33D modulus, connect as terminal type 1 to a local Fusion32, and decode one identical session with both decoders. That closes DIV-06, DIV-07, DIV-08 and converts every "opcode-level `MATCH`" above into a field-level one.

---

```
FILES_MODIFIED_REAL33D = 0
SERVER_MODIFIED        = NO
PROTOCOL_MODIFIED      = NO
UNREAL_MODIFIED        = NO
V08_MODIFIED           = NO
3DTIBIA_MODIFIED       = NO
COMMITS                = 0
PUSHES                 = 0
```

*End of report. `C:\Users\dell\3DTIBIA\cliente` was read strictly as reference; nothing was copied. The concurrent agent's untracked files were not touched. No credentials, keys or session captures appear here.*
