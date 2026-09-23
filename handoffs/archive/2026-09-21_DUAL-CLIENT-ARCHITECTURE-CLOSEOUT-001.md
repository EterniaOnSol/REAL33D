# HANDOFF

Date/time: 2026-09-21, America/Guatemala
Task: DUAL-CLIENT-ARCHITECTURE-CLOSEOUT-001
Branch: main
Status: **CLOSEOUT**. Documentation only. No code, protocol, server, Unreal or V08 change.

This handoff supersedes the UNREAL-WIDE-WORLD-001 handoff. That milestone is
complete and certified; see the attribution note below for its correct commit.

## What is now certified

| Milestone | Status | Commit |
| --- | --- | --- |
| `UNREAL-WIDE-WORLD-001` | `CERTIFIED_PASS` | `3fd5d1d` |
| `DUAL_CLIENT_LIVE_CAPTURE` | `PASS` | `0f9bd505` |

`OPTION_C_VIABLE_FOR_PRODUCTION = YES`

A stock `mehah/otclient` build ran the full ordinary 7.72 flow against the
unmodified Fusion32 server with **configuration only**:

```
STOCK_OTCLIENT_CONNECT     PASS      CONTAINERS           PASS
LOGIN                      PASS      ITEM_MOVE            PASS
CHARACTER_LIST             PASS      ITEMS                PASS
GAMELOGIN                  PASS      INVENTORY            PASS
INITIAL_WORLD              PASS      SAY_CHAT             PASS
CLIENT_INITIATED_MOVEMENT  PASS      LOGOUT_RECONNECT     PASS

INCOMING_PROTOCOL_ERRORS     = 0
UNKNOWN_INCOMING_OPCODES     = 0
UNSUPPORTED_INCOMING_OPCODES = 0

SERVER_CHANGES_REQUIRED              = 0
STOCK_CLIENT_PARSER_PATCHES_REQUIRED = 0
REQUIRED_PRODUCTION_PATCHES          = 1
```

Full evidence: `evidence/clientcore/DUAL_CLIENT_LIVE_CAPTURE.md`, with session
logs `DUAL-CLIENT-LIVE-CAPTURE-001-session1.log` and `-session3.log`.

## Selected production architecture

```
                        Fusion32
                           |
             +-------------+-------------+
             |                           |
        REAL33D 2D                    REAL33D 3D
     OTClient-derived                   Unreal
             |                           |
     own parser/model            ClientCore + WorldState
             |                           |
             +---- same protocol spec ---+
                  shared fixtures
```

- **`REAL33D_2D`** — mehah/OTClient-derived **production** client.
- **`REAL33D_3D`** — Unreal + ClientCore **production** client.
- **`Fusion32`** — sole authoritative game server.
- **Official `Tibia.exe` 7.72** — QA / parity / reference only. **Not a production client.**
- **IP changer** — legacy QA utility only. **Not production infrastructure.**

### The architectural choice, stated plainly

OTClient **keeps its own parser and game model**. ClientCore remains the 3D
implementation and the protocol reference. **We do not replace OTClient's model
with ClientCore's WorldState.**

The measurement behind that decision: OTClient's UI sits on 1,061 Lua binding
registrations and 207 distinct `g_game.*` / `g_map.*` call sites across 76
modules, all backed by its own `Map`, `Creature`, `Item` and `LocalPlayer`.
Substituting WorldState would be a rewrite of the 2D client, not an integration.

What the two clients share instead:

- the protocol specification
- byte-level fixtures
- semantic expectations
- Fusion32 authority

Divergence is therefore caught by a failing fixture rather than by hoping two
implementations stay aligned.

## Known remaining patch

```
TALK_MODE_14 = KNOWN_PRODUCTION_PATCH_REQUIRED
               NOT_LIVE_TESTED
               one protocolcodes.cpp table entry
```

Wire mode 14 (`TALK_ANONYMOUS_CHANNELCALL`) has no entry in OTClient's
`version >= 740` table, so `translateMessageModeFromServer` returns
`MessageInvalid` and `parseTalk` reaches `default: throw` — a loud failure, not
silent corruption. Producing it live needs a gamemaster anonymous channel call,
which this sanitized QA runtime cannot generate. 14 of the 15 emittable modes
already match exactly, tail included.

## DIV-08 standing position

```
CATALOGUE_STATIC_MATCH        = 4990/4990
LIVE_CONFIRMED                = 5/5 observed TypeIds
cumulative/liquid live classes = NOT_LIVE_COVERED
TypeId 5090                   = KNOWN_SERVER_ONLY_OR_UNREACHED_EXCEPTION
```

Every TypeId observed live was a zero-extra-byte item, so the one-byte classes
rest on the catalogue-wide static match rather than on live observation.

## Commit attribution — documentary, no history rewritten

- `3fd5d1d` — `UNREAL-WIDE-WORLD-001` was already **CERTIFIED_PASS** here.
- `c7bcfe7` — its message described the change set as UNREAL-WIDE-WORLD-001. It
  contains principally **`UNREAL-PLAYER-VITALS-001`** and other concurrent work.
  **`UNREAL-PLAYER-VITALS-001` remains `IMPLEMENTED_UNVERIFIED`.**
- `faf8852` — dual-client research and first successful stock OTClient session.
- `0f9bd505` — `DUAL_CLIENT_LIVE_CAPTURE = PASS`.

## Next milestone — REAL33D-2D-BOOTSTRAP-001 (NOT STARTED)

**Objective:** turn the successful isolated OTClient probe into a clean,
reproducible REAL33D-owned 2D client.

**Scope:**

- exact pinned `mehah/otclient` upstream commit
- REAL33D branding
- Fusion32 host/port configuration
- protocol 772 preset
- terminal OS/type configuration
- Fusion32 **public** RSA modulus configuration
- `GameEnvironmentEffect` compatibility
- TALK mode 14 one-entry patch
- remove or disable irrelevant OTClient ecosystem startup modules where justified
- reproducible Windows build
- no embedded account or password
- no private RSA or XTEA material
- no accidental `Tibia.dat` / `Tibia.spr` publication
- login → gameplay → logout/reconnect acceptance run

**Explicitly out of scope:** shops, extended opcode 50, new TerminalTypes, 8.6
protocol, new server features, Unreal changes, V08 changes, visual/art work.

## Scratch tree

`C:\r33dprobe` (~2.2 GB) holds the probe build, its pinned vcpkg, the probe mod
and the session logs. **Keep it untouched until REAL33D-2D-BOOTSTRAP-001
reproduces the live PASS.** Nothing in this repository references it.

## Resume instruction

Begin `REAL33D-2D-BOOTSTRAP-001` from the scope above. Do not re-run the live
probe and do not redo the research; both are certified and published.
