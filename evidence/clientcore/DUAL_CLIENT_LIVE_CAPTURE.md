# DUAL-CLIENT-LIVE-CAPTURE-001 — stock OTClient live session against Fusion32

Status: **PASS**
Date: 2026-09-21
Supersedes: the `BUILD IN_PROGRESS / NOT_REACHED` state published in `faf8852`.

A stock `mehah/otclient` build, configured but **not patched**, completed a full
7.72 session against the unmodified Fusion32 server: login, character selection,
game login, initial world, client-initiated movement, chat, container open/close,
an authoritative item move, safe logout and reconnect.

```
server changes        = 0
protocol changes      = 0
client parser patches = 0
configuration only
```

---

## 1. Build

| Item | Value |
| --- | --- |
| Source | `mehah/otclient`, fresh clone, HEAD `d7b821f` |
| Licence | MIT (project). 31 vcpkg dependency licences remain `NOT_VERIFIED` |
| Toolchain | isolated vcpkg pinned to the manifest baseline `9e593bb18`, release-only triplet |
| Result | `APP BUILD EXIT 0`, `otclient.exe` 8,779,264 bytes |
| Client assets | `Tibia.dat` sha256 `3c5e857f…77aedd`, `Tibia.spr` sha256 `86abbf5f…e0c64a` |

Four environmental blockers were cleared (stale vcpkg baseline, outdated ports
tree, Windows `MAX_PATH`, `VCPKG_INSTALLED_DIR` pinned inside the configure tree).
None implicated Fusion32 or the 7.72 dialect. For a future checkout they reduce to:
**build from a short path with its own pinned vcpkg.**

## 2. Configuration applied — the entire client-side delta

`mods/real33d_probe/` in the scratch checkout. No OTClient source file was edited.

| Setting | Value | Reason |
| --- | --- | --- |
| `g_game.setRsa(<Fusion32 modulus>)` | custom | Fusion32 uses its own key. A custom modulus also makes `chooseRsa()` return early, so it cannot later reset the RSA or the OS. |
| `g_game.setCustomOs(2)` | 2 | `connections.cc:214` accepts TerminalType 1 or 2 only; `OsTypes.Windows == 2`. |
| `setClientVersion/ProtocolVersion` | 772 | declared in `getSupportedClients()` |
| `disableFeature(GameEnvironmentEffect)` | off | would consume 2 extra bytes per described tile |

Credentials were read from the WSL `0600` file into process environment at launch.
No credential, private key or XTEA material was written to disk or to this report.

**Observation harness.** `protocolgameparse.cpp:65-70` calls Lua `onOpcode` for every
opcode *before* the C++ parser and restores the read position when it returns false.
The mod wraps it and delegates, giving a complete opcode trace with **no parser patch**.

## 3. Phase results

Session 3 (`DUAL-CLIENT-LIVE-CAPTURE-001-session3.log`), character **Test Player A**.

| Phase | Result | Evidence |
| --- | --- | --- |
| `OTCLIENT_BUILD` | **PASS** | exit 0, 0 compile errors |
| `STOCK_OTCLIENT_CONNECT` | **PASS** | TCP to `127.0.0.1:7171` |
| `LOGIN` | **PASS** | server: `19:14:00 Spieler Test Player A loggt ein an Socket 16` |
| `CHARACTER_LIST` | **PASS** | character resolved and entered without manual selection |
| `GAMELOGIN` | **PASS** | `op=10 (0x0A)` INIT_GAME, `unread=2271`; `EVENT onGameStart` |
| `INITIAL_WORLD` | **PASS** | `op=100 (0x64)` FULLSCREEN; `pos=(32098,32205,7) hp=144/150` |
| `CLIENT_INITIATED_MOVEMENT` | **PASS** | four steps, exact round trip (below) |
| `SAY_CHAT` | **PASS** | outgoing + incoming echo (below) |
| `ITEMS` | **PASS** | tile stack `n=2`, item `870` = *cobbled pavement* |
| `INVENTORY` | **PASS** | 4 slots `[3:2853, 4:3561, 6:3270, 10:2920]` |
| `CONTAINERS` | **PASS** | open `op=110`, close `op=111` |
| `ITEM_MOVE` | **PASS** | authoritative confirmation (below) |
| `LOGOUT_RECONNECT` | **PASS** | both confirmed server-side |

### 3.1 Client-initiated movement — exact round trip

```
BASELINE                      pos=(32098,32205,7)
MOVE requesting North  ->     pos=(32098,32204,7)   y-1
MOVE requesting South  ->     pos=(32098,32205,7)   y+1
MOVE requesting East   ->     pos=(32099,32205,7)   x+1
MOVE requesting West   ->     pos=(32098,32205,7)   x-1
net displacement: start == now
```

Each request produced `op=109 MOVE_CREATURE` plus the matching row update
(`op=101/102/103/104`). Session 2 additionally captured `op=181 (0xB5) SNAPBACK`
— Fusion32 refusing a step — decoded correctly by the stock client.

### 3.2 Chat — outgoing and incoming

```
19:14:19.313  CHAT sending say "REAL33D probe alpha"
19:14:19.333  op=170 (0xAA) unread=46
19:14:19.333  TALK from=Test Player A mode=1 ch=0 text=REAL33D probe alpha
```

20 ms request→echo. `mode=1` is `TALK_SAY`, matching the emitter in
`sending.cc::SendTalk(..., int x, int y, int z, ...)`.

### 3.3 Item move — authoritative, joined by request

```
19:14:26.814  ITEM_MOVE moving inv slot=4 id=3561 -> container cid=0
19:14:26.882  op=121 (0x79) DELETE_INVENTORY     unread=47
19:14:26.883  op=112 (0x70) CREATE_IN_CONTAINER  unread=45
19:14:26.884  op=160 (0xA0) PLAYER_DATA x2       (capacity update)
```

68 ms request→result. The server removed the item from the body slot and created
it inside the container; the client did not move it locally.

**Persistence cross-check:** before the move the login burst carried **four**
`op=120 SET_INVENTORY`; after reconnect it carried **three**, because item `3561`
now lives in the bag. The move survived logout.

### 3.4 Logout and reconnect

```
19:14:31.814  LOGOUT requesting safeLogout
19:14:32.850  EVENT onGameEnd
19:14:34.314  LOGOUT confirmed offline
19:15:23.084  op=10 (0x0A) INIT_GAME unread=2271   <- reconnect
19:15:23.116  EVENT onGameStart
```

Server-side:
```
19:14:00  loggt ein an Socket 16
19:14:32  loggt aus
19:15:23  loggt ein an Socket 17
19:17:03  loggt aus
```

The in-client verification step reported `online=false` because its 8-second
window closed before the login completed; the reconnect itself succeeded, as both
`onGameStart` and the second server-side socket record show.

## 4. Incoming protocol health

Measured over session 3: 103 traced packets, 20 distinct incoming opcodes.

```
incoming protocol decode errors = 0
unknown incoming opcodes        = 0
unsupported incoming opcodes    = 0
unknown message modes           = 0
```

Opcodes exercised: `10, 30, 100, 101, 102, 103, 104, 109, 110, 111, 112, 120,
121, 130, 131, 141, 160, 161, 170, 180` — plus `181` in session 2.

### 4.1 The two `[error]` lines are outbound module noise, not protocol failures

Four `[error]` lines appear, two per login, and **all four are the same pair**:

| Line | Inner id | Sent by | Nature |
| --- | --- | --- | --- |
| `Unable to send extended opcode 1` | `ExtendedIds.Locale = 1` (`gamelib/const.lua:361`) | `modules/client_locales/locales.lua:11` | client announcing its locale to an OTClient-aware server |
| `Unable to send extended opcode 201` | `GAME_SHOP_CODE = 201` (`game_shop.lua:2`) | `modules/game_shop/game_shop.lua:98` | OTClient shop module fetching its catalogue |

Both are **OTClient ecosystem features, not Tibia 7.72 protocol**. Both are inner
ids carried inside extended opcode 50, and both were **blocked client-side** by
`m_enableSendExtendedOpcode = false` (`protocolgame.h:474`, default). Nothing was
transmitted to Fusion32 and the server never saw them.

```
CLASSIFICATION = LOCAL_MODULE_NOISE, NO_EFFECT_ON_FUSION32
extended opcode 50 = UNTOUCHED (not enabled, not reserved, not implemented)
```

## 5. DIV-08 — live confirmation

Static position (unchanged): the wire-length rule derived from `Tibia.dat`
(`isStackable || isFluidContainer || isSplash`) and from `objects.srv`
(`CUMULATIVE || LIQUIDCONTAINER || LIQUIDPOOL`) agree for **4990/4990**
comparable ids.

Live confirmation for the TypeIds actually observed:

| TypeId | Role | dat extra | objects.srv extra | Name | Match |
| ---: | --- | ---: | --- | --- | --- |
| 870 | tile item | 0 | (none) | cobbled pavement | OK |
| 2853 | container | 0 | (none) | a bag | OK |
| 3561 | inventory | 0 | (none) | a jacket | OK |
| 3270 | inventory | 0 | (none) | a club | OK |
| 2920 | inventory | 0 | (none) | a torch | OK |

**Honest limit:** every TypeId that appeared live is a zero-extra-byte item. No
cumulative or liquid item entered the session, so the live evidence confirms only
the zero-extra-byte class. The one-byte classes rest on the catalogue-wide static
match, not on live observation.

```
DIV08 = CATALOGUE_STATIC_MATCH 4990/4990
        LIVE_CONFIRMED 5/5 observed TypeIds (all zero-extra-byte)
        LIVE_UNCOVERED: cumulative and liquid classes
        GLOBAL_CLIENT_RANGE_EXCEPTION TypeId 5090
```

TypeId 5090 (*a treasure map*) is declared in `objects.srv` but absent from the
`.dat` item range and referenced by no map, loot table, NPC, data file, player
file or source path. It carries no wire payload.
`KNOWN_SERVER_ONLY_OR_UNREACHED_EXCEPTION`. No workaround was created.

## 6. TALK mode 14

```
TALK_MODE_14 = KNOWN_PRODUCTION_PATCH_REQUIRED, NOT_LIVE_TESTED
```

Producing it requires a gamemaster account issuing an anonymous channel call,
which this sanitized QA runtime does not provide. Source evidence is sufficient
to specify the fix precisely and it was **not applied**:

- Emittable modes are `{1,2,3,4,5,6,7,8,9,10,11,12,14,16,17}`. Modes 13 and 15
  are accepted by `CTalk` client→server but emitted by **no** `SendTalk` overload.
- Mode 14 sends `SendString(Connection, "")` — the sender field is **present and
  empty**, never absent (`sending.cc:1389-1393`, `docs/protocol772/TALK.md:62`).
- OTClient's `version >= 740` table has no entry for 14, so
  `translateMessageModeFromServer` returns `MessageInvalid` and `parseTalk` hits
  `default: throw` — a **loud** failure, not silent corruption.
- Fix: one table entry in `src/client/protocolcodes.cpp`.

14 of the 15 emittable modes already match exactly, tail included.

## 7. Result

```
DUAL_CLIENT_LIVE_CAPTURE = PASS

OTCLIENT_BUILD            = PASS
STOCK_OTCLIENT_CONNECT    = PASS
LOGIN                     = PASS
CHARACTER_LIST            = PASS
GAMELOGIN                 = PASS
INITIAL_WORLD             = PASS
CLIENT_INITIATED_MOVEMENT = PASS
SAY_CHAT                  = PASS
ITEMS                     = PASS
INVENTORY                 = PASS
CONTAINERS                = PASS
ITEM_MOVE                 = PASS
LOGOUT_RECONNECT          = PASS

INCOMING_PROTOCOL_ERRORS    = 0
UNKNOWN_INCOMING_OPCODES    = 0
UNSUPPORTED_INCOMING_OPCODES = 0

DIV08        = CATALOGUE_STATIC_MATCH 4990/4990; LIVE_CONFIRMED 5/5 observed
TALK_MODE_14 = KNOWN_PRODUCTION_PATCH_REQUIRED, NOT_LIVE_TESTED

SERVER_CHANGES_REQUIRED             = 0
STOCK_CLIENT_PARSER_PATCHES_REQUIRED = 0
REQUIRED_PRODUCTION_PATCHES          = 1  (TALK wire mode 14, one table entry)

OPTION_C_VIABLE_FOR_PRODUCTION = YES
```

**Why YES.** The complete ordinary 7.72 flow — login through reconnect, including
authoritative item movement — ran on a stock client with configuration only, zero
server changes and zero parser patches, with no incoming decode error, unknown
opcode or unknown message mode. That is the base Option C required. The single
known production patch is one table entry for a gamemaster-only mode that this
runtime cannot generate, and it is tracked separately rather than folded into the
viability claim.

**Not established by this milestone:** combat and follow (not exercised),
cumulative and liquid item classes live, dependency licence clearance, and
TALK mode 14 live behaviour.

## 8. Commit attribution correction

Documentary only; no history was rewritten.

- `3fd5d1d` — `UNREAL-WIDE-WORLD-001` was already **CERTIFIED_PASS** at this commit.
- `faf8852` — dual-client research plus the first successful stock OTClient live
  session.
- `c7bcfe7` — its message described the change set as UNREAL-WIDE-WORLD-001. It
  contains principally **`UNREAL-PLAYER-VITALS-001` and other concurrent work**.
  `UNREAL-PLAYER-VITALS-001` remains **IMPLEMENTED_UNVERIFIED**.

## 9. Reproduction

Scratch tree `C:\r33dprobe` retained pending review: `otclient/` (build + probe mod),
`vcpkg/` (pinned), `evidence/` (session logs), `launch_probe.sh`, `parse_dat.py`.
Nothing in it is referenced by this repository.

Session logs: `DUAL-CLIENT-LIVE-CAPTURE-001-session1.log` (first contact),
`-session3.log` (full phase sequence).
