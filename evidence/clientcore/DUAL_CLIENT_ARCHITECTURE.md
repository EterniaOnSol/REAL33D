# DUAL_CLIENT_ARCHITECTURE

Milestone: `DUAL-CLIENT-ARCHITECTURE-001`
Mode: **RESEARCH ONLY / READ ONLY**
Subject: `C:\Users\dell\Desktop\fusion32` (Fusion32 + Protocol772Core + REAL33D)
Date: 2026-09-21
Status: **scratch, uncommitted, outside every project tree.**

Concurrency note: another agent completed `UNREAL-WIDE-WORLD-001 = PASS` during this session. Its untracked files (`Real33DStaticSectorActor.*`, `build_wide_world_cache.py`) were **not opened, modified, or depended upon**. Its own verification records "no change under `clientcore/` or `reference/`", which this report relies on only as published evidence.

Evidence labels: `DEMONSTRATED` · `STRONG_INFERENCE` · `HYPOTHESIS` · `NOT_PROVEN`.

---

## 1. Audit — what is already dual-client ready

The encouraging result: **the existing stack was built with a capability boundary already in place**, in three separate layers, none of which needs inventing.

| Asset | State | Evidence |
| --- | --- | --- |
| **Fusion32 `TerminalType`** | Client-declared, server-validated, session-persistent, **already branched on** | `communication.cc:28` `TERMINALVERSION[] = {772,772,772}`; `:970` validation; `connections.cc:207-222` persist + `JoinGame` accepts only 1 and 2 |
| **Fusion32 unknown-opcode policy** | Inbound unknown opcodes **ignored, not fatal** | `receiving.cc:1786` `default: print(3, ...); break;` |
| **Fusion32 free opcode space** | ~130 client + ~110 server ids unused | `connections.hh:12-138` |
| **ClientCore `LoginRequestOptions`** | **Already parameterizes `terminal_type` and `terminal_version`** | `clientcore/include/fusion32/protocol772/login.h:41-42` |
| **ClientCore `TcpTransport`** | `Connect(host, port, timeout)` — endpoint is a plain argument | `tcp_transport.h:66-69` |
| **ClientCore build** | Plain C++17, CMake 3.20, OpenSSL only, no engine dependency, granular static libs, `/W4 /WX /permissive-` | `clientcore/CMakeLists.txt` |
| **ClientCore test fixtures** | Byte-level vectors checked in | `clientcore/tests/fixtures/{crypto_772_vectors.h, fullscreen_772_vectors.h}` |
| **WorldState** | **Fully presentation-agnostic** — no rendering concept anywhere | `worldstate.h` — tiles, stacks, creatures, stats, skills, state flags, ambient light |
| **WorldView** | Diffs successive WorldStates into engine-agnostic `WorldEvent`s | `worldview.h` |
| **WorldState anomaly reporting** | Desync is a *reported condition*, not silent drift | `worldstate.h::WorldStateAnomalyKind` |
| **Login endpoint flow** | Login server returns **per-character world address + port from the database** | `reference/login/src/connections.cc:576-579` `Write32BE(WorldAddress); Write16(WorldPort)`; mirrored in `login.h::LoginCharacter{world_address, world_port}` |
| **Unreal boundary guards** | Linked-not-copied lib; exactly one file may include a protocol header; diff lives in ClientCore | `ARCHITECTURE.md` |

`DEMONSTRATED` throughout.

**Reusable by both clients today, unchanged:** the whole of ClientCore below the presentation line — transport, framing, crypto, login, game login, initial world, movement, player state, chat, `ObjectTypeTable`, WorldState, WorldView. None of it references Unreal. The three Unreal boundary guards mean a second consumer is an *additional* caller, not a refactor.

**Not yet dual-client ready:** `TERMINALVERSION[]` has exactly three entries and `JoinGame` hard-rejects anything but types 1 and 2 — so a third and fourth client identity do not yet exist. That is the one deliberate change the whole architecture hinges on, and it is four lines.

---

## 2. 2D client base — candidate comparison

### 2.1 Method and its limits

Only candidates **physically present on this machine** were evaluated. Claims about forks not on disk would be recall, not evidence, so they are marked `NOT_PROVEN` and excluded from ranking.

| Candidate | On disk? | Location |
| --- | --- | --- |
| **OTCv8** (`OTCv8/otcv8-dev`) | ✅ | `Downloads/otcv8-dev-master.zip`; second copy in `distribuciones/otcv8-The-Josh-Wife-Edition-main.zip` |
| **mehah/otclient** | ✅ | full source tree at `C:\Users\dell\3DTIBIA\cliente` |
| OTClient original (`edubart/otclient`) | ❌ | only as mehah's stated ancestor (rev 2.760) |
| OTClient Redemption | ❌ | not found |
| The Forgotten Client | ✅ but n/a | `distribuciones/` — 197 `.h` / 105 `.cpp`, a separate C++ client, not an OTClient fork |
| RonClient | ✅ but n/a | 58 `.h` / 57 `.cpp`, too small to be a protocol-complete base |

**Project-separation note.** The mehah tree lives inside `3DTIBIA`, a different and independent project. It was read **only to verify facts about the upstream fork**. Nothing should be copied from it into REAL33D; a REAL33D 2D client would start from its own upstream clone.

### 2.2 The decisive criterion: does it speak 7.72?

Both do, declared explicitly rather than inferred:

```lua
-- mehah/otclient · modules/gamelib/game.lua:54-55
local supportedClients = {
    740, 741, 750, 755, 760, 770, 772, 780, ... 1525 }

-- OTCv8 · modules/gamelib/game.lua
function g_game.getSupportedClients()
  return { 740, 741, 750, 760, 770, 772, 780, ... }
```
`DEMONSTRATED`.

### 2.3 Comparison

| Criterion | OTCv8 | mehah/otclient |
| --- | --- | --- |
| Protocol 7.72 | ✅ declared | ✅ declared |
| Upper protocol range | ~11.x | **1525 (Tibia 15.x)** |
| Identity | `OTCv8/otcv8-dev` | `mehah/otclient`, from `edubart/otclient` rev 2.760, CI at `opentibiabr/otclient` |
| Build system | CMake | CMake ≥ 3.16 + **`vcpkg.json` manifest** |
| Unit tests | not as a tree | **`tests/` with CMakeLists — `map`, `otml`, `stdext`** |
| CI | GitHub Actions | GitHub Actions (opentibiabr) |
| Cross-platform | Windows/Linux/Android/web layouts | **`Dockerfile`, `Dockerfile.android`, `Dockerfile.browser`** (emscripten) + `android/`, `browser/` trees |
| Maintenance signal | active fork, mobile focus | changelog entry dated **2026-02-04** |
| Extension channel | `ClientExtendedOpcode = 50` | present (same lineage) |
| Feature gating | `g_game.enableFeature`, PRs required to be optional | 127 feature flags, 224 gates in the parser |
| Configurable endpoint | Lua `entergame` module | Lua `entergame` module |
| Licence | GPL-ish OTClient lineage — **`NOT_PROVEN`, verify before adoption** | same caveat |

### 2.4 Assessment

**mehah/otclient is the cleaner base for REAL33D 2D**, on evidence rather than preference: it is the only candidate with a dependency manifest, a unit-test tree, and first-class cross-platform build targets including browser and Android — which matters because §9's launcher and any future reach beyond Windows ride on it. Its 740→1525 range also means the *same* base could later speak an extended or post-7.72 dialect without changing client families.

**OTCv8 remains a credible second choice**, particularly if mobile is prioritized early or if its extension ecosystem proves valuable.

**Unresolved before any adoption decision:** licence terms for both (`NOT_PROVEN` — neither was read), and §2.5's divergence risk, which is larger than the choice between the two.

### 2.5 The risk that outranks the choice

OTClient's 7.72 support was written against **OTServ-lineage servers**. Fusion32 is a **CipSoft decompilation**. These are not guaranteed to agree byte-for-byte within "7.72".

The project already knows this class of hazard: `ARCHITECTURE.md` records that `sending.cc::SendItem` *"decides an item's on-wire length from server object type flags the protocol never carries"*, which is why ClientCore injects `ObjectTypeTable` from the server's own `dat/objects.srv`. OTClient derives the same lengths from its `Tibia.dat`. **The two agree only if the `.dat` and `objects.srv` agree.**

`STRONG_INFERENCE` that divergences exist somewhere; **`NOT_PROVEN`** where. This must be *measured*, not assumed — see §12's first vertical slice.

---

## 3. No IP changer — endpoint configuration

The IP changer exists for exactly one reason: `Tibia.exe` has its login host compiled into the binary. A client whose source we control has no such constraint. `DEMONSTRATED` by the flow:

```
REAL33D client ── configured login host:port ──▶ Login server
                                                    │
                    character list, each carrying   │
                    WorldAddress (u32 BE) + WorldPort (u16)
                                                    ▼
REAL33D client ──────── connects to that endpoint ──▶ Game server
```
`reference/login/src/connections.cc:576-579`; the address itself originates in the database via `loadWorldConfig`.

Only **one** value must be configured client-side — the login endpoint. Everything downstream is server-supplied, which means environment changes are a database edit, not a client redeploy.

| Mechanism | Assessment |
| --- | --- |
| **Compiled default** | Good as a *fallback only*. Rebuilding to change environment is the trap the IP changer existed to work around. |
| **Config file** | Natural for OTClient (Lua `entergame` module already owns this) and correct for development. Weak for production: user-editable, so it cannot carry trust. |
| **Launcher-provided** | **Recommended primary.** Launcher owns environment selection and hands host/port/capability to the client at start. Single source of truth, no rebuild, no user editing of the live path. |
| **Remote signed environment config** | Correct *eventual* answer for production: launcher fetches a signed environment descriptor, verifies, then launches. Adds key management — premature now, but the launcher should be shaped so this drops in later without redesign. |

**Layering that satisfies both today and later:** compiled default → config file (dev only) → launcher-provided (production) → signed remote (future), each overriding the previous. The client should treat the launcher-provided value as authoritative when present.

---

## 4. Terminal capabilities

### 4.1 What exists

```c
communication.cc:28   static const int TERMINALVERSION[] = {772, 772, 772};
communication.cc:970  if(TerminalType < 0 || TerminalType >= NARRAY(TERMINALVERSION)
                          || TERMINALVERSION[TerminalType] != TerminalVersion) → reject
connections.cc:207    this->TerminalType = (int)Buffer->readWord();
connections.cc:214    if(this->TerminalType != 1 && this->TerminalType != 2) → reject
connections.cc:219-222  TerminalOffsetX=8; TerminalOffsetY=6; TerminalWidth=18; TerminalHeight=14;
```
`DEMONSTRATED`. The bounds check is `NARRAY`-driven, so extending the table extends the accepted set automatically.

### 4.2 Target capability identities

| Identity | Terminal type | Role |
| --- | --- | --- |
| `CLASSIC_772_REFERENCE` | 1 (and 2 = gamemaster) | **QA/parity only**, never distributed |
| `REAL33D_2D` | proposed new | production |
| `REAL33D_3D` | proposed new | production |

Keeping 1 and 2 untouched is what preserves QA. `DEMONSTRATED` that this is sufficient: classic clients keep sending 1, take unchanged code paths, and never see a byte they did not before.

### 4.3 Change surface per layer

| Layer | Required change | Size |
| --- | --- | --- |
| **Login (Fusion32)** | extend `TERMINALVERSION[]`; decide whether REAL33D types carry version `772` or a distinct number | ~2 lines |
| **Game login** | widen `JoinGame`'s accept set beyond `1, 2` | ~1 line |
| **Session** | none — `TerminalType` is already persisted on `TConnection` | **0** |
| **Sending** | branch on `Connection->TerminalType` inside `Send*` before emitting any extended opcode | per feature |
| **Receiving** | gate extended inbound opcodes in `CommandAllowed`; unknown ones are already safely ignored | per feature |
| **ClientCore** | none for the mechanism — `LoginRequestOptions.terminal_type` is already a parameter | **0** |
| **OTClient (2D)** | set the terminal type it sends; `entergame` already owns login composition | small |
| **Unreal (3D)** | pass its own terminal type through the existing `LoginRequestOptions` | small |

**The capability mechanism itself costs roughly three lines of server change.** Everything expensive lives in the per-feature gating, not in the negotiation.

### 4.4 Two design cautions

1. **Capability must never become entitlement.** `TerminalType` selects *representation*, never gameplay advantage. A 2D and a 3D player must be subject to identical authority. Viewport is the trap here: the earlier milestone established that `TerminalWidth` is per-connection and *interest-management radii are hardcoded constants* (`operate.cc:100/218/243` at `(16,14)`), so a per-client viewport change silently produces a stale world unless the radii move too. **Both production clients should keep 18×14 live.** The wide world is static and client-side — which `UNREAL-WIDE-WORLD-001` has now demonstrated in practice.
2. **Version numbering.** If REAL33D types declare `TerminalVersion = 772`, a stock OTClient could impersonate REAL33D. A distinct version number makes the capability claim explicit. Neither is a security boundary — that requires §13-style hardening — but the distinct number makes logs and QA honest.

---

## 5. Common vs client-specific classification

| System | Class | Note |
| --- | --- | --- |
| Movement | `SHARED_AUTHORITATIVE` | server resolves; both clients send intent. `MovementLedger` already distinguishes requested from external motion |
| Map (live 18×14) | `SHARED_AUTHORITATIVE` | WorldState owns the rectangle |
| Creatures | `SHARED_AUTHORITATIVE` | |
| Items | `SHARED_AUTHORITATIVE` | identity shared; *appearance* is per-client (§8) |
| Containers | `SHARED_AUTHORITATIVE` | `OpenContainer[16]` is server state |
| Inventory | `SHARED_AUTHORITATIVE` | |
| Combat | `SHARED_AUTHORITATIVE` | |
| Spells | `SHARED_AUTHORITATIVE` | hardcoded server-side |
| NPCs | `SHARED_AUTHORITATIVE` | dialogue is server-driven |
| **Shops** | `SHARED_AUTHORITATIVE` + `SHARED_SEMANTIC` | validation server-side; §6 |
| Quests | `SHARED_AUTHORITATIVE` | `QuestValues[500]` |
| Houses | `SHARED_AUTHORITATIVE` | |
| Chat | `SHARED_AUTHORITATIVE` | `CHAT-772-001` decoded; speaker resolution already in ClientCore |
| Economy | `SHARED_AUTHORITATIVE` | money is items |
| Outfits | `SHARED_SEMANTIC` | id + 4 colours shared; rendering differs |
| Effects | `SHARED_SEMANTIC` | byte id shared; sprite vs particle |
| Projectiles | `SHARED_SEMANTIC` | byte id shared |
| Lighting / ambience | `SHARED_SEMANTIC` | `SV_CMD_AMBIENTE` shared; interpretation differs |
| Player stats / skills | `SHARED_SEMANTIC` | same values, different HUDs |
| **Wide world** | `CLIENT_PRESENTATION_ONLY` | static `.sec` baseline outside the live window; **zero server involvement** — now demonstrated |
| Minimap | `CLIENT_PRESENTATION_ONLY` | derived from map data |
| World map | `CLIENT_PRESENTATION_ONLY` | from files |
| UI | `CLIENT_PRESENTATION_ONLY` | |
| Camera / 3D transform | `CLIENT_PRESENTATION_ONLY` | `1 SQM = 100 UU` is Unreal-only |
| Weather | `CLIENT_SPECIFIC_EXTENSION` | no 7.72 carrier; needs gated opcode |
| Shop **UI** | `CLIENT_SPECIFIC_EXTENSION` | same semantics, different presentation |
| Chat presentation | `CLIENT_SPECIFIC_EXTENSION` | 3D draws above creature; 2D uses a console |
| Cosmetics beyond 7.72 outfits | `CLIENT_SPECIFIC_EXTENSION` | |

**The line that keeps this honest:** anything a player could gain advantage from must be `SHARED_AUTHORITATIVE`. Everything in `CLIENT_PRESENTATION_ONLY` must be *incapable* of affecting the server — which the wide-world milestone enforced structurally by making static content carry no runtime state at all.

---

## 6. NPC shops — the worked example

7.72 has **no shop protocol**. Verified: no shop/purchase/sale opcode exists anywhere in `connections.hh`, and 7.72 NPCs traded through conversation. The 8.60 reference uses `0x79-0x7C` inbound and `0x7A/0x7B` outbound. `DEMONSTRATED` (prior milestone). The earlier content study also measured that **128 of 228 NPCs (56%)** in the era-accurate 8.6 reference are shop-driven — so this is not a marginal feature.

### 6.1 Authority placement

```
                    Fusion32
              authoritative NpcShop
        (inventory, prices, stock, capacity,
         money, distance, ownership checks)
                        │
        ┌───────────────┴───────────────┐
        ▼                               ▼
   REAL33D 2D                      REAL33D 3D
   shop panel (OTUI)               shop UI (Unreal)
```

Every validation — money, capacity, weight, stock, range, PZ rules — resides in Fusion32. Neither client may compute a price or decide a trade is legal. A client-side check is a **display convenience only** and must never gate the request.

### 6.2 Semantic messages (conceptual — no opcodes fixed)

| Message | Direction | Carries | Notes |
| --- | --- | --- | --- |
| `ShopOpen` | S→C | shop id, NPC creature id, entry count | opens the session |
| `ShopEntry` | S→C | item type id, buy price, sell price, stock/unlimited, max amount | one per tradable line; batched |
| `ShopPlayerFunds` | S→C | money, free capacity | needed to render affordability truthfully |
| `BuyRequest` | C→S | item type id, amount, ignore-capacity flag | intent only |
| `SellRequest` | C→S | item type id, amount | intent only |
| `ShopTransactionResult` | S→C | accepted/rejected + reason, resulting funds | **authoritative causality anchor** |
| `ShopClose` | C→S and S→C | — | either side may end it |

Design notes:
- `ShopTransactionResult` must exist even on success. Without it, the same-world test (§10) can only observe outcomes, and the brief explicitly requires **causality proven by request/result**, not by looking at the screen. The project has already been burned here: the `UNREAL-SLICE-001` correction withdrew two criteria credited to observation when the retained snapshots showed `steps_requested: 0`.
- Requests carry **no prices**. A client that can name a price can propose one.
- Inventory changes still flow through existing container/inventory opcodes. Shop messages describe the *transaction*, not its effects.
- Both clients consume the identical semantic events; only the panel differs.

### 6.3 Classic degradation

`CLASSIC_772_NEEDS_MAPPING` — gated off entirely by `TerminalType`. `Tibia.exe` continues to trade by conversation, which is what 7.72 does natively. No degradation logic is required because the classic path is the original path.

---

## 7. ClientCore for the 2D client — Option A vs B

### 7.1 The measurement that decides it

OTClient is not merely a renderer with a parser attached. Measured on the mehah tree:

| Measure | Value |
| --- | --- |
| Lua binding registrations in `src/client/luafunctions.cpp` | **1,061** |
| Distinct `g_game.*` / `g_map.*` call sites across modules | **207** |
| UI modules | **76** |
| Feature flags | **127** |
| Version/feature gates in `protocolgameparse.cpp` | **224** |

`DEMONSTRATED`.

OTClient's entire UI — every window, every panel, every module — is built on `g_game` and `g_map`, which are backed by **OTClient's own** `Map`, `Creature`, `Item`, `Container`, `LocalPlayer`. Replacing its protocol and state with ClientCore/WorldState means reimplementing that 1,061-function binding surface on top of a different model.

**Option A as stated — one shared semantic layer feeding both clients — is therefore not an integration. It is a rewrite of the 2D client.** `STRONG_INFERENCE`, resting on the measurements above.

### 7.2 Comparison

| Criterion | **A: ClientCore for both** | **B: OTClient keeps its parser** |
| --- | --- | --- |
| Duplicated protocol knowledge | none | **two implementations of 7.72** |
| Migration cost | **very high** — reimplement 1,061 bindings | **low** — 772 already supported |
| Maintenance | one parser, two renderers | two parsers, divergence risk |
| Testability | ClientCore suites cover both | ClientCore suites cover only 3D |
| Protocol extensions | implement once | implement twice |
| Performance | ClientCore is C++17, no engine dep | OTClient's parser is already tuned for its own renderer |
| Future evolution | single point of change | drift compounds |
| Risk profile | one big risk up front | many small risks forever |

### 7.3 Assessment — a third option the brief did not list

Neither A nor B as stated is right, and the evidence supports saying so rather than forcing a pick.

**Option C — shared specification and fixtures, separate implementations.**

- OTClient keeps its own parser for the **classic 7.72 subset**. It already works, 772 is declared supported, and reusing it costs nothing.
- ClientCore remains the implementation for **3D**, and simultaneously the **reference specification** for the protocol, because it is the one traced line-by-line to Fusion32 source.
- **REAL33D extensions are implemented twice but specified once**, with the specification expressed as *byte-level fixtures* rather than prose. ClientCore already ships exactly this: `clientcore/tests/fixtures/{crypto_772_vectors.h, fullscreen_772_vectors.h}`. The same vectors can validate the OTClient implementation.
- Divergence stops being a hope and becomes a **test failure**.

This gets Option B's low migration cost and most of Option A's anti-drift property. What it does not get is single-implementation economy — extensions cost two implementations. That is the honest price.

**Insufficient evidence to go further.** Whether OTClient's 7.72 actually matches Fusion32's is `NOT_PROVEN` (§2.5), and that answer could change the recommendation: if divergences are numerous, more of ClientCore becomes attractive; if near-zero, Option C is clearly right. **Measure before deciding** — §12's slice is designed to produce exactly that number.

---

## 8. Post-7.72 content under two clients

With `Tibia.exe` demoted to QA, players are no longer bound by its `.dat/.spr`. The constraint that dominated the previous milestone — ~63% of era-accurate 8.6 items falling outside Fusion32's `0–5090` TypeID space — changes character: it stops being a *rendering* ceiling and becomes a *server identity* ceiling plus an *art pipeline* cost.

```
            canonical game identity
        (Fusion32 object type / creature race)
                       │
        ┌──────────────┴──────────────┐
        ▼                             ▼
   2D visual identity           3D visual identity
   sprite set (.dat/.spr        mesh + material
   or a REAL33D-owned           (V08 catalogue)
   sprite catalogue)
                       │
                       ▼
          CLASSIC_772_REFERENCE
     renders only the classic subset;
     later content may be absent — acceptable
```

Consequences:

1. **The server identity space is the real ceiling**, not either client. Fusion32 declares 5,003 object types with ids `0–5090` (`visual/manifests/objects.csv`), and `ResizeHashTable` aborts rather than growing. Expanding *identity* is a server change; expanding *appearance* is an art change.
2. **Both production clients need a resolution step from canonical id → visual asset.** Unreal already has one (`UReal33DAssetRegistry`), and the wide-world milestone demonstrated the pattern under pressure: resolve to a V08 mesh, else a tagged `CLASSIC_SPRITE_FALLBACK_<TypeId>`, else count `MISSING_PHYSICAL_ASSET` and continue. The 2D client needs the same three-way contract, and OTClient's `.dat` is only one possible backing store for it.
3. **Missing assets must never be silent.** That the wide-world work reports `MISSING_PHYSICAL_ASSET` as a counted category rather than an invisible hole is the right precedent and should be a requirement for the 2D catalogue too.
4. **The classic client falling behind is now expected, not a defect.** Its QA value is parity on the classic subset; content outside that subset simply is not exercised there.

---

## 9. Launcher architecture

```
                  REAL33D Launcher
                         │
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
    Play 2D          Play 3D        (dev) QA tools
   OTClient-based     Unreal         classic client
        │                │
        └── receives ────┴── environment descriptor ──┘
```

Descriptor contents (identical for both clients):

| Field | Purpose |
| --- | --- |
| `environment` | dev / qa / production — must be visible in-client to prevent wrong-world testing |
| `login_host`, `login_port` | the **only** endpoint needed; game endpoint arrives per character |
| `client_version` / `patch` | asset and binary version |
| `terminal_type` | the capability identity this build claims |
| `asset_manifest` | catalogue version + hashes |
| `signature` | future: signed descriptor |

Design points:
- **One descriptor format for both clients.** Two formats guarantee two behaviours.
- **Launcher owns environment; client owns nothing durable about endpoints.** This is what structurally prevents the IP-changer pattern from returning.
- **Assets version with the binary.** The wide-world milestone showed the failure mode: a frozen catalogue missing one id (469) turned into a rendering decision. Catalogue version must be explicit and checkable.
- **Shape for signing now, implement later.** If the descriptor is a signed blob from day one — even self-signed in dev — adding real verification is a key-management task, not a redesign.
- The launcher must be able to launch the **classic client** for QA against a chosen environment, without that path ever being reachable in a production build.

---

## 10. Same-world vertical test

Mandatory future test. `Player A → REAL33D 2D`, `Player B → REAL33D 3D`, one Fusion32 world, concurrently.

| # | Criterion | Causality requirement |
| --- | --- | --- |
| 1 | Both log in to the same server | distinct character ids; identical world endpoint logged |
| 2 | Each sees the other | creature id matches on both sides; name and position agree |
| 3 | Movement A→B | A's step carries an `input_id`; B observes the position change; joined by id |
| 4 | Movement B→A | same, reversed |
| 5 | Refused movement | a blocked step produces a refusal and **no** visual movement on either client |
| 6 | Chat A→B and B→A | statement id matches; speaker resolved to the same creature id |
| 7 | Combat | attack request → authoritative health change observed by both |
| 8 | Item movement | move request → identical resulting stack on both clients |
| 9 | Container state | open/close/add/remove consistent; container id agrees |
| 10 | NPC interaction | same dialogue state from both clients |
| 11 | **Shop transaction** | `BuyRequest` → `ShopTransactionResult` → inventory + funds change, all joined by request id |
| 12 | Reconnect | one client drops and rebuilds; other unaffected; no orphan creature |
| 13 | **No duplicate state** | creature/tile counts equal WorldState on the 3D side and `g_map` on the 2D side at every sample |
| 14 | Zero protocol anomalies | zero unsupported opcodes, zero residual bytes, zero `WorldStateAnomalyKind` reports, both clients |

**Causality is the point.** Every criterion must be provable from a retained artifact joining a request to an authoritative result. Visual observation alone is explicitly insufficient — the `UNREAL-SLICE-001` correction withdrew two criteria for exactly this reason, and the `MovementLedger` exists because of it. The shop criterion should reuse that ledger pattern rather than invent one.

A useful precursor: **A → REAL33D 2D, B → classic `Tibia.exe`**, which isolates §2.5's divergence question before the 3D client is in the picture.

---

## 11. Production vs QA — explicit statuses

| Client | Status | Distribution | Purpose |
| --- | --- | --- | --- |
| **`Tibia.exe` 7.72** | `QA_REFERENCE_ONLY` | **never shipped to players** | parity oracle for the classic subset during development |
| **REAL33D 2D** | `FUTURE_PRODUCTION_CLIENT` | launcher | sprite presentation |
| **REAL33D 3D** | `FUTURE_PRODUCTION_CLIENT` | launcher | mesh presentation |

Consequences worth stating:
- `TerminalType 1` and `2` must keep working for as long as QA needs them, but carry **no** production obligation.
- Tooling that exists only to make the classic client reachable — the IP changer — is **development-only** and must not appear in any production path.
- The classic client's provenance is recorded as `UNKNOWN` in the project's own docs; that is tolerable for a QA oracle and would not be for a shipped artifact. Demoting it resolves that concern rather than deferring it.

---

## 12. Results

# DUAL_CLIENT_ARCHITECTURE_RESEARCH = COMPLETE

**2D_CLIENT_BASE_CANDIDATES** — Two verified on disk, both declaring 772 explicitly in `getSupportedClients()`: **mehah/otclient** (from edubart rev 2.760, CI at opentibiabr, 740→1525, CMake+vcpkg manifest, `tests/` tree, Docker for Linux/Android/browser, changelog dated 2026-02-04) and **OTCv8** (740→11.x, GitHub Actions CI, mobile-oriented). mehah is the cleaner base on maintainability evidence; OTCv8 is a credible second. Forks not present on this machine were not ranked. Licences for both are `NOT_PROVEN` and must be read before adoption.

**SHARED_PROTOCOL_FEASIBILITY** — High for the classic subset, because both clients already speak 772 and the server already tolerates unknown inbound opcodes. The unresolved question is not capability but *fidelity*: OTClient's 7.72 was written against OTServ while Fusion32 is a CipSoft decompilation, and where they diverge is unmeasured.

**SHARED_WORLDSTATE_FEASIBILITY** — Technically excellent, practically constrained. WorldState and WorldView are already fully presentation-agnostic and engine-free, so a second consumer costs nothing structurally. But OTClient's 76 UI modules sit on 1,061 Lua bindings over its *own* state model, so sharing WorldState with the 2D client means rewriting the 2D client.

**TERMINALTYPE_EXTENSION_FEASIBILITY** — The strongest result in this study. The mechanism exists end to end: client-declared, server-validated against a `NARRAY`-bounded table, persisted on `TConnection`, already branched on to distinguish gamemaster clients — and `LoginRequestOptions` already carries `terminal_type` as a parameter. Reaching three production identities costs roughly three lines of server change; all real cost is in per-feature gating.

**NPC_SHOP_DUAL_CLIENT_FEASIBILITY** — Architecturally clean, genuinely new work. Authority sits naturally in Fusion32; seven semantic messages cover it; both clients render the same events differently; the classic client is simply gated off and keeps trading by conversation. The cost is that 7.72 has no shop protocol at all, so this is net-new server plus net-new protocol plus two UIs.

**POST_772_CONTENT_FEASIBILITY** — Improved by this decision. Demoting the classic client removes the `.dat/.spr` rendering ceiling; the binding constraint becomes Fusion32's `0–5090` object identity space plus art pipeline throughput. Both production clients need a canonical-id → visual-asset resolver with an explicit missing-asset category, which the wide-world milestone has now demonstrated for the 3D side.

**LAUNCHER_ARCHITECTURE** — One launcher, one environment descriptor, two production targets plus a dev-only QA path. Only the login endpoint needs configuring because the login server returns each character's world address and port from the database. Shape the descriptor for signing immediately; implement verification later.

**MAJOR_RISKS** — In order: (1) unmeasured OTClient-vs-Fusion32 7.72 divergence; (2) protocol knowledge duplicated across two client implementations, drifting silently; (3) capability leaking into entitlement, with per-client viewport the specific trap given hardcoded announce radii; (4) two UIs doubling every feature's presentation cost; (5) asset catalogue drift between sprite and mesh identity; (6) licence terms unread for both candidate bases; (7) no server-side test suite to catch regressions from gating changes.

---

## PROPOSED_ARCHITECTURE

```
                         Fusion32  (authoritative)
                    TerminalType capability gate
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
      type 1/2 (QA)        type = 2D           type = 3D
      Tibia.exe 7.72       REAL33D 2D          REAL33D 3D
      classic subset       OTClient-based       Unreal
      NEVER SHIPPED             │                  │
                           own parser         Protocol772Core
                          (classic 7.72)            │
                                │              WorldState
                                │              WorldView
                                │                  │
                          OTClient state       Unreal adapter
                                │                  │
                            sprite UI          mesh scene
                                │                  │
                                └── shared byte fixtures ──┘
                                    (one specification,
                                     two implementations,
                                     drift = test failure)

              static wide world: client-side, both clients,
              from .sec + objects.srv — zero server involvement
```

## CLIENT_RESPONSIBILITY_MATRIX

| Responsibility | 2D | 3D | Classic (QA) |
| --- | --- | --- | --- |
| Input capture | ✅ | ✅ | ✅ |
| Intent submission | ✅ | ✅ | ✅ |
| Protocol decode (classic) | own parser | Protocol772Core | built-in |
| Protocol decode (extensions) | own impl, shared fixtures | Protocol772Core | **never receives** |
| Logical client state | OTClient model | WorldState | built-in |
| Visual identity resolution | sprite catalogue | `UReal33DAssetRegistry` | `.dat`/`.spr` |
| Static wide world | eligible | ✅ demonstrated | ❌ |
| UI / HUD | OTUI | Unreal widgets | built-in |
| Minimap / world map | ✅ | ✅ | ✅ |
| **Any authority** | ❌ | ❌ | ❌ |

## SERVER_RESPONSIBILITY_MATRIX

| Responsibility | Fusion32 |
| --- | --- |
| Accounts, characters, sessions | ✅ sole |
| World, creatures, items, containers, inventory | ✅ sole |
| Movement validation, combat, spells | ✅ sole |
| NPCs, dialogue, **shop validation** | ✅ sole |
| Quests, houses, economy | ✅ sole |
| Chat routing | ✅ sole |
| Capability gating by `TerminalType` | ✅ sole |
| Presentation | ❌ never |
| Asset knowledge | ❌ never |

## CAPABILITY_MODEL

| | `CLASSIC_772_REFERENCE` | `REAL33D_2D` | `REAL33D_3D` |
| --- | --- | --- | --- |
| Terminal type | 1 / 2 | new | new |
| Status | QA only | production | production |
| Classic 7.72 opcodes | ✅ | ✅ | ✅ |
| Extended opcodes | ❌ gated off | ✅ | ✅ |
| Live viewport | 18×14 | 18×14 | 18×14 |
| Static wide world | ❌ | client-side | client-side |
| Post-7.72 visual content | ❌ expected gap | sprite catalogue | mesh catalogue |
| Authority | none | none | none |

## MIGRATION_STAGES

| Stage | Scope | Exit criterion (demonstrated, not observed) |
| --- | --- | --- |
| **0** | Today | 3D client live; wide world PASS; classic client is QA |
| **1** | **Measure divergence** | An OTClient build connects to Fusion32 as terminal type 1 and completes login → world → walk → chat; every divergence from Fusion32's dialect is enumerated with a byte-level artifact. **No code committed.** |
| **2** | Capability identities | `TERMINALVERSION[]` extended; `JoinGame` accepts the new types; a REAL33D-typed session and a classic session run concurrently with zero anomalies on both |
| **3** | 2D client baseline | REAL33D 2D reaches feature parity with the classic client on the classic subset, from its own configured endpoint, **no IP changer anywhere in the path** |
| **4** | Launcher | One descriptor launches both production clients into a chosen environment; endpoint never user-edited |
| **5** | First extension (shops) | Shop semantics gated by `TerminalType`; both production clients transact; classic client unaffected and still trading by conversation |
| **6** | Same-world vertical | §10's fourteen criteria, all causality-proven |

Stage 1 gates everything. Its output — a divergence count — is what finally decides §7 between Option B and Option C.

## FIRST_VERTICAL_SLICE

**`DUAL-CLIENT-DIVERGENCE-PROBE-001`** — the smallest work that produces a decision-grade number.

- **Goal:** enumerate, with evidence, where an unmodified OTClient's 7.72 disagrees with Fusion32's 7.72.
- **Setup:** an OTClient build (own clone, not copied from another project) connecting as terminal type **1** — no server change at all, so the probe cannot destabilize anything.
- **Capture:** every server→client packet, decoded twice — once by OTClient, once by Protocol772Core — over an identical session.
- **Compare:** opcode coverage, payload lengths, stack ordering, item length derivation (`.dat` flags vs `objects.srv` flags), outfit encoding, `SV_CMD_ROW_*` anchor handling.
- **Output:** a divergence table; zero-divergence areas marked reusable, each divergence marked as a client fix, a fixture, or a genuine incompatibility.
- **Why first:** it needs no server change, no protocol change, no new opcode, no art, and no launcher — yet it is the input to the single largest open architectural decision in this report. Everything downstream is cheaper once that number exists.

---

## Questions still unanswered

1. Licence terms of OTCv8 and mehah/otclient. `NOT_PROVEN` — neither read.
2. Where OTClient's 7.72 diverges from Fusion32's. `NOT_PROVEN` — the point of Stage 1.
3. Whether OTClient's `.dat`-derived item lengths match `objects.srv` flag-derived lengths in all cases. `NOT_PROVEN`.
4. What `Tibia.exe` 7.72 does on an unknown server opcode. Still assumed fatal. `NOT_PROVEN`.
5. Whether a REAL33D-owned 2D sprite catalogue should replace `.dat`/`.spr` or extend it. `HYPOTHESIS` either way.
6. Cost of implementing each extension twice, in practice. `NOT_MEASURED` — one feature would calibrate it.
7. Whether opcode 50 (`0x32`), which OTClient already implements as a generic extended channel and Fusion32 leaves free, is the right transport for REAL33D extensions or whether dedicated opcodes are cleaner. `HYPOTHESIS` — worth deciding explicitly, since the client-side plumbing already exists.

---

*End of report. No project file was created, modified, or committed. The concurrent agent's untracked files were not touched. `C:\Users\dell\3DTIBIA\cliente` was read strictly as a reference for upstream facts; nothing was taken from it.*
