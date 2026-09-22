# DUAL_CLIENT_LIVE_CAPTURE

Milestone: `DUAL-CLIENT-LIVE-CAPTURE-001`
Date: 2026-09-21
Status: **scratch, uncommitted, outside every project tree.**

Contains: **Part A** — correction to `DUAL_CLIENT_DIVERGENCE_PROBE` on TALK modes, with recalculated matrix. **Part B** — live capture execution status.

---

# PART A — TALK correction

The project owner's correction is **confirmed in full** against all three named sources. My previous DIV-01 and DIV-02 were wrong.

## A.1 What the sources say

**`reference/game/src/sending.cc`** — three `SendTalk` overloads, each opening with an explicit mode whitelist:

| Overload | Accepted modes | Tail after mode byte |
| --- | --- | --- |
| `SendTalk(…, const char *Text, int Data)` @ `:1328` | 4, 6, 7, 8, 9, 11 | `quad Data` **only** when mode 6; else nothing |
| `SendTalk(…, int Channel, const char *Text)` @ `:1365` | 5, 10, 12, **14** | `word Channel` |
| `SendTalk(…, int x, int y, int z, const char *Text)` @ `:1402` | 1, 2, 3, 16, 17 | `x, y, z` |

Server→client emittable set = **{1,2,3,4,5,6,7,8,9,10,11,12,14,16,17}** — fifteen modes. **13 and 15 appear in no whitelist.**

**Mode 14 does not omit the sender field** (`sending.cc:1389-1393`):
```c
SendByte(Connection, SV_CMD_TALK);
SendQuad(Connection, StatementID);
if(Mode != TALK_ANONYMOUS_CHANNELCALL){
    SendString(Connection, Sender);
}else{
    SendString(Connection, "");        // ← present, u16 length = 0
}
```
`SendString` is `SendWord(length)` then bytes (`sending.cc:149`), so an empty sender is a two-byte `0x0000`. The wire shape is **identical across every mode in that overload**.

**`docs/protocol772/TALK.md`** states both facts independently:
- `:62` — *"`TALK_ANONYMOUS_CHANNELCALL` sends an **empty** sender rather than omitting the field… A decoder that treated anonymity as an absent field would lose two bytes and desynchronise. The field is always present; it is the contents that are blanked."*
- `:96` — *"`TALK_ANONYMOUS_BROADCAST` 13 and `TALK_ANONYMOUS_MESSAGE` 15 are declared and accepted by **no** overload… these never reach the wire under this opcode."*

**`clientcore/src/talk_command.cpp:45-47`** — *"TALK_ANONYMOUS_BROADCAST 13 and TALK_ANONYMOUS_MESSAGE 15 are in CTalk's [accept set]"* — i.e. client→server only.

`DEMONSTRATED` on all three.

## A.2 What I got wrong

| Claim | Verdict |
| --- | --- |
| **DIV-01** — modes 13/14/15 unmapped → incoming stream desync | **Partly wrong.** 13 and 15 are outbound-only from the client; the server never emits them, so they cannot desynchronise an incoming stream. Only **mode 14** is affected. |
| **DIV-02** — mode 14 omits the sender name, needing a shape branch | **Wrong, withdrawn entirely.** The field is always present; mode 14 blanks its contents. No shape branch exists or is needed. |

I had read the `if/else` at `sending.cc:1391` and stopped at the `if`, without reading the `else` two lines below that writes the empty string. The project's own TALK.md called out exactly this misreading as a hazard, and I made it.

## A.3 The one real divergence, restated

Mapping every **emittable** mode through OTClient's `version >= 740` table and then through `parseTalk`'s tail switch:

| Wire | Fusion32 | Fusion32 tail | OTClient mode | OTClient tail | Result |
| ---: | --- | --- | --- | --- | --- |
| 1 | `TALK_SAY` | x,y,z | `MessageSay` | `getPosition` | ✅ |
| 2 | `TALK_WHISPER` | x,y,z | `MessageWhisper` | `getPosition` | ✅ |
| 3 | `TALK_YELL` | x,y,z | `MessageYell` | `getPosition` | ✅ |
| 4 | `TALK_PRIVATE_MESSAGE` | — | `MessagePrivateFrom` | — | ✅ |
| 5 | `TALK_CHANNEL_CALL` | `word` | `MessageChannel` | `getU16` | ✅ |
| 6 | `TALK_GAMEMASTER_REQUEST` | `quad` | `MessageRVRChannel` | `getU32` | ✅ |
| 7 | `TALK_GAMEMASTER_ANSWER` | — | `MessageRVRAnswer` | — | ✅ |
| 8 | `TALK_PLAYER_ANSWER` | — | `MessageRVRContinue` | — | ✅ |
| 9 | `TALK_GAMEMASTER_BROADCAST` | — | `MessageGamemasterBroadcast` | — | ✅ |
| 10 | `TALK_GAMEMASTER_CHANNELCALL` | `word` | `MessageGamemasterChannel` | `getU16` | ✅ |
| 11 | `TALK_GAMEMASTER_MESSAGE` | — | `MessageGamemasterPrivateFrom` | — | ✅ |
| 12 | `TALK_HIGHLIGHT_CHANNELCALL` | `word` | `MessageChannelHighlight` | `getU16` | ✅ |
| **14** | **`TALK_ANONYMOUS_CHANNELCALL`** | **`word`** | **unmapped → `MessageInvalid`** | **`default: throw`** | ❌ |
| 16 | `TALK_ANIMAL_LOW` | x,y,z | `MessageMonsterSay` | `getPosition` | ✅ |
| 17 | `TALK_ANIMAL_LOUD` | x,y,z | `MessageMonsterYell` | `getPosition` | ✅ |

**14 of 15 emittable modes match exactly, tail included.** `DEMONSTRATED`.

**Revised DIV-01** — mode 14 only:
- **Raw opcode:** `SV_CMD_TALK` = 170 (`0xAA`), mode byte = 14
- **Packet shape:** `byte 170 | quad StatementID | string "" | byte 14 | word Channel | string Text`
- **Fusion32 emitter:** `sending.cc:1365-1398`, overload (C)
- **OTClient parser:** `protocolcodes.cpp` `version >= 740` table has no entry for 14 (its own comment reads `// 13, 14, 15 ??`) → `translateMessageModeFromServer(14)` returns `Otc::MessageInvalid` → `parseTalk`'s switch hits `default: throw Exception("ProtocolGame::parseTalk: unknown message mode {}")`
- **Protocol772Core parser:** maps 14 to `TalkMode::AnonymousChannelCall`, reads the empty sender and the channel word correctly
- **Exact consequence:** **an exception, not a silent desync.** The empty sender parses fine (OTClient's `getString()` reads `u16 = 0` and returns `""`); the failure is the unmapped mode byte, and the `word Channel` is never consumed. OTClient's protocol layer surfaces this as an error rather than corrupting state — a **fail-loud** outcome.
- **Classification:** `OTCLIENT_PATCH` — **one** table entry.
- **Reachability:** only a gamemaster anonymous channel call.

Also verified while checking this: wire modes 4 and 11 are each mapped from **two** Otc enum values (`PrivateFrom`/`PrivateTo`, `GamemasterPrivateFrom`/`To`). `translateMessageModeFromServer` reverse-looks-up by iteration order, returning the lower enum value — `MessagePrivateFrom` (4) and `MessageGamemasterPrivateFrom` (14). Both appear in `parseTalk`'s no-tail case list, so the ambiguity resolves safely. `DEMONSTRATED`. Counted as a new exact match.

## A.4 Recalculated matrix

```
TOTAL_COMPARABLE_CASES = 27      (was 24; +TC-20 talk tails, +TC-21 reverse lookup,
                                  +TC-22 mode-6 quad Data)
EXACT_MATCHES          = 17      (was 14)
SEMANTIC_MATCHES       =  5
MINOR_DIVERGENCES      =  3      (DIV-03/04/05, all CLIENT_CONFIG)
MAJOR_DIVERGENCES      =  1      (was 2 — DIV-02 withdrawn; DIV-01 narrowed to mode 14)
OTCLIENT_UNSUPPORTED   =  0
CLIENTCORE_UNSUPPORTED =  0
UNKNOWN                =  3      (DIV-06 login server, DIV-07 known-creature, DIV-08 .dat)

CLASSIC_772_COMPATIBILITY_PERCENT = 92%   (22 of 24 comparable, UNKNOWN excluded)
                                    96%   counting the three CLIENT_CONFIG items
                                          as compatible-after-configuration
```

Up from 79%/92%. **`REQUIRED_OTCLIENT_PATCHES` drops from 2 to 1** — a single table entry in `protocolcodes.cpp`. `REQUIRED_SERVER_CHANGES` remains **0**.

---

# PART B — Live capture execution

```
DUAL_CLIENT_LIVE_CAPTURE = PARTIAL
```

Real work was performed and is still running. No result is claimed that was not produced.

## B.1 Environment established

| Item | Status | Evidence |
| --- | --- | --- |
| Isolated checkout | ✅ | `git clone --depth 1 https://github.com/mehah/otclient.git`, HEAD `d7b821f`, 95 MB. **Nothing copied from 3DTIBIA.** |
| 772 declared in fresh clone | ✅ | `modules/gamelib/game.lua:55` — `740, 741, 750, 755, 760, 770, 772, 780, …` |
| **Fusion32 running** | ✅ | WSL `Ubuntu-26.04`, PID **2905**, `/var/lib/fusion32-server-baseline-772-0/game/bin/game` |
| Network | ✅ | `git ls-remote` against GitHub succeeds |
| CMake | ✅ | 4.4, VS 18 2026 generator |
| Dependency build | 🔄 **running** | detached; `abseil` installing, 2 of ~31 packages staged |

## B.2 Four build blockers found, each with its fix

These were not knowable before attempting and are the concrete deliverable of this phase.

**BLOCK-1 — user's vcpkg lacks the manifest baseline.**
`vcpkg.json:70` pins `builtin-baseline: 9e593bb18ea69cc5095e012465dcd675a822ed0d`; `C:\Users\dell\vcpkg` (HEAD `40f3c709`) did not contain it.
*Action taken:* `git fetch origin --filter=blob:none` in the user's vcpkg — **non-destructive**, adds objects only. HEAD verified unchanged at `40f3c709` before and after. Baseline commit confirmed present afterwards.

**BLOCK-2 — user's vcpkg ports tree is too old regardless.**
Even with the baseline fetched, resolution failed on `cpp-httplib@0.51.0` (not in that registry's version database). Its installed set is a **TFS server** set — boost, cryptopp, libmariadb, luajit, mpir, pugixml — with almost none of OTClient's 31 dependencies.
*Action taken:* **did not** check out a newer commit in the user's vcpkg, because that would risk their TFS server builds. Cloned an **isolated** vcpkg into scratch and checked out the exact baseline `9e593bb18`, then bootstrapped it (`vcpkg 2026-07-27`). 114 MB.

**BLOCK-3 — Windows `MAX_PATH` defeats a scratchpad-rooted build.**
The session scratchpad path is ~120 characters; vcpkg's `vcpkg-buildtrees/versioning_/versions/<port>/<sha>_<n>.tmp/…` adds ~150 more. Git failed with `unable to create file …: Filename too long` across `abseil`, `angle`, `libvorbis`, `luajit`, `vcpkg-cmake-config`.

**BLOCK-4 — installed dir cannot be redirected out of the tree.**
The obvious workaround (`-DVCPKG_INSTALLED_DIR=C:/short/inst`) is rejected by mehah's own build system:
```
CMake Error at cmake/SharedBuildCache.cmake:1118 (message):
  Opt-out VCPKG_INSTALLED_DIR must remain inside this configure tree
```
*Action taken for 3 and 4:* relocated the whole scratch checkout to a short root, `C:\r33dprobe\{otclient,vcpkg}`. Configure then proceeded past path resolution into genuine dependency acquisition.

**All four are environmental, not protocol findings.** None implicates Fusion32, ClientCore or the 7.72 dialect. For a future REAL33D 2D checkout they reduce to one rule: **build from a short path with its own pinned vcpkg.**

## B.3 Current state

The dependency build is running detached under `C:\r33dprobe`, log at `C:\r33dprobe\configure5.log`. Remaining ports include heavy ones — `angle`, `protobuf`, `openal-soft`, `freetype`, `luajit` — on an 11.8 GB laptop. Completion is hours away, and any individual port may still fail.

## B.4 Phases — all NOT_REACHED

No phase past build was attempted, so every one is reported unreached rather than estimated.

| Phase | Result |
| --- | --- |
| `OTCLIENT_BUILD` | `IN_PROGRESS` — dependencies building, application not yet compiled |
| `LOGIN` | `NOT_REACHED` |
| `CHARACTER_LIST` | `NOT_REACHED` |
| `GAMELOGIN` | `NOT_REACHED` |
| `INITIAL_WORLD` | `NOT_REACHED` |
| `MOVEMENT` | `NOT_REACHED` |
| `CHAT` | `NOT_REACHED` |
| `ITEMS` | `NOT_REACHED` |
| `INVENTORY` | `NOT_REACHED` |
| `CONTAINERS` | `NOT_REACHED` |
| `COMBAT` | `NOT_REACHED` |
| `LOGOUT_RECONNECT` | `NOT_REACHED` |

**DIV-08 (mandatory)** — `NOT_LIVE_TESTED`. Requires a running client. The static position is unchanged and deliberately not strengthened: the length *rule* matches and no 7.72 type carries more than one wire-relevant flag (0 of 5,003), but **no claim is made that `.dat` and `objects.srv` are globally equivalent.**

**Chat mode 14** — `NOT_LIVE_TESTED`. It also needs a gamemaster account and an anonymous channel call, which this sanitized runtime may not permit; that is a second gate beyond the build.

## B.5 Configuration prepared (not yet applied)

For the moment the binary exists, derived from the static probe:

| Setting | Value | Why |
| --- | --- | --- |
| `g_game.setCustomOs(OsTypes.Windows)` | `2` | Fusion32 accepts `TerminalType` 1 or 2 only (`connections.cc:214`); OTClient's default `getOs()` returns 20-25 → rejected |
| `g_game.setRsa(<REAL33D modulus>)` | project key | Fusion32 uses its own key, not `CIPSOFT_RSA`/`OTSERV_RSA` |
| protocol version | `772` | declared supported |
| `GameEnvironmentEffect` | **off** | would consume 2 extra bytes per tile → map desync |
| login endpoint | local Fusion32 login port | game endpoint arrives per character from the DB |

No experimental OTClient patches were applied — stock + configuration first, as instructed.

---

# RESULT

```
DUAL_CLIENT_LIVE_CAPTURE = PARTIAL

OTCLIENT_BUILD     = IN_PROGRESS  (deps building, detached; 4 blockers found and cleared)
LOGIN              = NOT_REACHED
CHARACTER_LIST     = NOT_REACHED
GAMELOGIN          = NOT_REACHED
INITIAL_WORLD      = NOT_REACHED
MOVEMENT           = NOT_REACHED
CHAT               = NOT_REACHED
ITEMS              = NOT_REACHED
INVENTORY          = NOT_REACHED
CONTAINERS         = NOT_REACHED
COMBAT             = NOT_REACHED
LOGOUT_RECONNECT   = NOT_REACHED

SERVER_CHANGES_REQUIRED    = 0        (static evidence; unchanged by live work)
OTCLIENT_PATCHES_REQUIRED  = 1        (was 2 — DIV-02 withdrawn, DIV-01 narrowed
                                       to a single table entry for wire mode 14)

STATIC_COMPARABLE_CASES = 27
LIVE_COMPARABLE_CASES   = 0
LIVE_EXACT_MATCHES      = 0
LIVE_SEMANTIC_MATCHES   = 0
LIVE_MINOR_DIVERGENCES  = 0
LIVE_MAJOR_DIVERGENCES  = 0

OPTION_C_VIABLE_FOR_PRODUCTION = NOT_PROVEN
```

**On `OPTION_C_VIABLE_FOR_PRODUCTION`.** The static evidence improved materially this round — 92% comparable-case agreement, 14 of 15 emittable talk modes exact including tails, one remaining patch, zero server changes. That is a stronger position than the previous report described. But the brief permits a viability call **only** if live evidence supports it, and `LIVE_COMPARABLE_CASES = 0`. Not a single byte has crossed a real socket into a real OTClient build. `NOT_PROVEN` is the only honest answer, and the static improvement does not substitute for it.

## Next step

Resume when the dependency build finishes: compile the application, apply the §B.5 configuration, connect to the running Fusion32 (PID 2905) as terminal type 2, and execute the phase list with paired decoding. The environment is prepared and the four blockers are cleared, so the remaining work is the build completing plus the session itself.

---

```
FILES_MODIFIED_REAL33D = 0
SERVER_MODIFIED        = NO
PROTOCOL_MODIFIED      = NO
UNREAL_MODIFIED        = NO
V08_MODIFIED           = NO
3DTIBIA_MODIFIED       = NO
COMMITS_REAL33D        = 0
PUSHES_REAL33D         = 0
```

**Outside those trees, for disclosure:** `git fetch` in `C:\Users\dell\vcpkg` (non-destructive, HEAD unchanged at `40f3c709`); scratch build tree created at `C:\r33dprobe` (isolated, deletable). No credentials, account secrets, RSA private material or XTEA keys were captured or recorded.

*Nothing was copied from 3DTIBIA into REAL33D or into the probe checkout.*

---

# PART C — Resumed run (second session)

```
DUAL_CLIENT_LIVE_CAPTURE = PARTIAL   (pre-flight complete; build still compiling)
```

## C.1 Build optimisation applied

The first run was compiling **both debug and release** despite `-DVCPKG_BUILD_TYPE=release` — that variable only takes effect inside a triplet, not on the CMake command line. Appended to the **scratch** vcpkg triplet (`C:\r33dprobe\vcpkg\triplets\x64-windows.cmake`), documented here as the only change to it:

```cmake
set(VCPKG_BUILD_TYPE release)
```

Rebuilt from clean; the log now shows only `Building x64-windows-rel`. Roughly halves remaining dependency work.

The user's `C:\Users\dell\vcpkg` was **not** touched this session. Its HEAD remains `40f3c709`; no global upgrade, no other project's manifest altered.

## C.2 Pre-flight — everything except the binary is now cleared

| Item | Status | Evidence |
| --- | --- | --- |
| Fusion32 game | ✅ running | PID 2905, listening `0.0.0.0:7172` |
| Fusion32 login | ✅ running | PID 3078, listening `0.0.0.0:7171` |
| QueryManager | ✅ running | PID 2890, `127.0.0.1:7173` (loopback-only, as designed) |
| MariaDB | ✅ running | `127.0.0.1:3306` |
| **Windows → WSL reachability** | ✅ | `Test-NetConnection` `TcpTestSucceeded=True` on both 7171 and 7172 |
| World config | ✅ | `Worlds` row: `'Fusion Test'`, Host `127.0.0.1`, Port `7172` — reachable from Windows via WSL2 localhost forwarding |
| **RSA public modulus** | ✅ extracted | 1024-bit, 309 decimal digits, via `openssl rsa -noout -modulus`. **Public component only; the private key was never read, copied or recorded.** |
| QA accounts | ✅ identified | `AccountID` 772001 / 772002; characters "Test Player A" / "Test Player B" |
| QA passwords | ✅ located, **not extracted** | `Auth` column is a salted SHA-256 (`prepare_wsl.sh:113-121`) — not recoverable. Plaintext lives only in `<runtime>/secrets/credentials.env`, mode `0600`, inside WSL |
| ClientCore reference decoder | ✅ built | `protocol772core.lib` + all 8 test suites present under `build/clientcore-windows/` |
| Probe config mod | ✅ written | `C:\r33dprobe\otclient\mods\real33d_probe\` |

## C.3 Probe configuration (scratch only, stock client otherwise)

`mods/real33d_probe/real33d_probe.lua` — the only client-side change, and it contains **no patch to any parser**. It sets exactly the four CLIENT_CONFIG items from the static probe:

| Setting | Value | Reason |
| --- | --- | --- |
| `g_game.setRsa(<Fusion32 modulus>)` | custom | DIV-04. A custom modulus also makes `g_game.chooseRsa()` return early (`if currentRsa ~= CIPSOFT_RSA and ~= OTSERV_RSA then return end`), so it cannot later overwrite the RSA **or** reset the OS below. |
| `g_game.setCustomOs(2)` | 2 | DIV-03. `OsTypes.Windows == 2`; Fusion32 accepts TerminalType 1 or 2 only. |
| `setClientVersion/ProtocolVersion(772)` | 772 | declared supported |
| `disableFeature(GameEnvironmentEffect)` | off | DIV-05. Would consume 2 extra bytes per described tile. |

Credentials are read from **environment variables at launch** (`R33D_ACC`, `R33D_PW`) rather than written into the mod, so no secret is persisted to disk or to this report.

**The TALK mode 14 patch was deliberately NOT applied**, per instruction, so that `STOCK_OTCLIENT_RESULT` can be recorded before any modification.

## C.4 Build status at end of session

| Metric | Value |
| --- | --- |
| Dependencies staged | **17 of ~31** |
| `angle` | ✅ **completed** (was the long pole: 668 MB buildtree, ~32 obj/min measured) |
| Current port | `fmt`, fetching `freetype` |
| Remaining | `freetype`, `openal-soft`, `protobuf`, `luajit`, `physfs`, `libarchive`, … — all materially smaller than angle |
| Then | the application itself (~700 sources) |

`angle` is an unconditional `"platform": "windows"` dependency in `vcpkg.json` with no CMake option to skip it, so it cannot be bypassed.

**Noted for the application build step:** `OPTIONS_ENABLE_IPO` defaults to `ON` (LTO). Passing `-DOPTIONS_ENABLE_IPO=OFF` on the final configure will materially shorten the link.

## C.4b DIV-08 — resolved catalogue-wide against the real 7.72 `.dat`

The 7.72 client assets exist in the project at `build/classic-client-772/app/`. They were **copied into the scratch checkout** (read-only from `fusion32`; nothing there modified):

```
data/things/772/Tibia.dat   sha256 3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd
data/things/772/Tibia.spr   sha256 86abbf5fadcf84313a03615a55a8bd626baab98671f0a78c9b36cb61cce0c64a
```

That made it possible to settle DIV-08 **across the whole catalogue instead of three samples**, without the client running. A `.dat` parser was written to OTClient's own 7.55-7.72 rules (`thingtype.cpp`: attributes read natively except `23 → FloorChange`; parameterised attributes `Ground/Writable/WritableOnce/Elevation/MinimapColor/LensHelp` = `u16`, `Light`/`Displacement` = `2×u16`), then the 772 wire rule from `getItem()` — one extra byte iff `isStackable() || isFluidContainer() || isSplash()` — was compared against `objects.srv`'s `wire_extra_bytes`.

```
Tibia.dat header: signature=0x439D5A33 items=5089 outfits=254 effects=25 missiles=15
parsed item types: 4990   (ids 100..5089)

ids comparable : 4990
agree          : 4990
DISAGREE       :    0
```

**Every comparable object type derives the identical wire length from both sources.** `DEMONSTRATED`.

This is stronger than the requested three samples for the *rule*, but it is **static, not live** — it proves the two flag sets agree, not that a running client parses a running server's stream. The live check for actually-observed TypeIds remains outstanding.

**One narrow asymmetry found.** 13 ids exist in `objects.srv` but not in the `.dat` item range:
- `0-10` and `99` — server-internal ids below the client's item range (which starts at 100); `99` is also the creature-descriptor marker. All carry zero wire-extra bytes.
- **`5090` — "a treasure map"**. The `.dat` declares `items = 5089`, so this type has no client entry. ClientCore handles it (its `ObjectTypeTable` comes from `objects.srv`); OTClient's `getItem()` would throw on `Item::create` for an id it cannot resolve. The classic `Tibia.exe` shares this `.dat` and would have the same gap, which suggests either the type is never emitted or this `.dat` is one entry behind this server build. **Unresolved** — `UNKNOWN`, narrow, and worth a live check.

## C.4c TypeId 5090 — reachability determination

Per instruction: determine only whether it can ever reach a client. **No workaround, alias or remap was created.**

`objects.srv` declares it last, and the declaration itself is unremarkable:

```
TypeID      = 5090
Name        = "a treasure map"
Flags       = {Text,Take}
Attributes  = {FontSize=1,Weight=830}
```

`Flags` carries neither `Cumulative` nor a liquid flag, so it is a **zero-extra-byte** item — it could not cause a length divergence even if emitted. `DEMONSTRATED`.

Exhaustive search for any path that places, creates, gives or transforms into it:

| Source | Path | Hits |
| --- | --- | --- |
| Clean map | `reference/origmap/*.sec` | **0** |
| Live map | `state/map/*.sec` | **0** |
| Monster loot | `reference/mon/*.mon` | **0** |
| NPC trade/give | `reference/npc/*.npc` | **0** |
| Item behaviour & all data | `reference/dat/*` except `objects.srv` — `moveuse.dat`, `map.dat`, `houses.dat`, `houseareas.dat`, `circles.dat`, `conversion.lst`, `mem.dat`, `monster.db` | **0** |
| Player inventories | `state/usr/**.usr` | **0** |
| Swap/save state | `state/save/` | **0** |
| Server source | `reference/game/src`, `login/src`, `querymanager/src` | **0** |
| ClientCore | `clientcore/` | **0** |

`DEMONSTRATED`.

```
5090_IS_EVER_EMITTED_TO_CLIENT = NO
```

…for every content-driven path in this dataset. The one residual is that a gamemaster object-creation command taking an arbitrary TypeID was not ruled out, so absolute impossibility is not claimed.

```
CLASSIFICATION = KNOWN_SERVER_ONLY_OR_UNREACHED_EXCEPTION
```

`objects.srv` declares 5,003 types; the `.dat` declares 5,089 items. 5090 is the single declaration beyond the client's range, is referenced by nothing, and carries no wire payload. No action taken.

## C.5 Resume command

The build is detached and survives; log at `C:\r33dprobe\cfg6.log`.

```bash
# 1. wait for dependency resolution to finish
until grep -qE "Configuring done|Generating done|vcpkg install failed" /c/r33dprobe/cfg6.log; do sleep 30; done

# 2. build the application (LTO off for speed)
cd /c/r33dprobe/otclient
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/r33dprobe/vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release -DOPTIONS_ENABLE_IPO=OFF
cmake --build build --config Release -j4

# 3. launch with credentials in env only (never on disk)
#    R33D_ACC / R33D_PW sourced from the WSL 0600 credentials file at launch time
```

Then execute phases 3-10 against PID 2905.

---

# RESULT — resumed session

```
DUAL_CLIENT_LIVE_CAPTURE = PARTIAL

OTCLIENT_BUILD          = IN_PROGRESS   (7/~31 deps; angle compiling; release-only)
STOCK_OTCLIENT_CONNECT  = NOT_REACHED
LOGIN                   = NOT_REACHED
CHARACTER_LIST          = NOT_REACHED
GAMELOGIN               = NOT_REACHED
INITIAL_WORLD           = NOT_REACHED
MOVEMENT                = NOT_REACHED
CHAT                    = NOT_REACHED
ITEMS                   = NOT_REACHED
INVENTORY               = NOT_REACHED
CONTAINERS              = NOT_REACHED
LOGOUT_RECONNECT        = NOT_REACHED

DIV08          = NOT_LIVE_TESTED
TALK_MODE_14   = NOT_LIVE_TESTED   (patch deliberately withheld; stock result wanted first)

SERVER_CHANGES_REQUIRED       = 0    (static; unchanged — no server change was needed
                                      for any pre-flight step either)
STOCK_CLIENT_PATCHES_REQUIRED = 0 so far   (config-only; no parser patch applied)
REQUIRED_PRODUCTION_PATCHES   = 1 known    (TALK wire mode 14 table entry, from source
                                            evidence — not yet live-confirmed)

LIVE_COMPARABLE_CASES      = 0
LIVE_EXACT_MATCHES         = 0
LIVE_SEMANTIC_MATCHES      = 0
LIVE_CONFIG_ONLY           = 0
LIVE_CLIENT_PATCH_REQUIRED = 0
LIVE_MAJOR_DIVERGENCES     = 0
LIVE_UNKNOWN               = 0

OPTION_C_VIABLE_FOR_PRODUCTION = NOT_PROVEN
```

Static evidence stands at 27 comparable cases / 92% agreement / 1 known patch / 0 server changes. No live byte has yet crossed a socket into an OTClient build, so the viability call remains unproven by the rule set for this milestone.

```
SCRATCH_DISK_USAGE    = 2.22 GB
SCRATCH_PATH          = C:\r33dprobe   (build tree, isolated vcpkg, probe mod)
                        plus session scratchpad for reports
SCRATCH_SAFE_TO_DELETE = YES — nothing in it is referenced by REAL33D, Fusion32,
                         ClientCore, Unreal, V08 or 3DTIBIA. Deleting only discards
                         build progress.
```
