# UNREAL-PARITY-AUDIT-001 — full 7.72 gameplay parity audit (reconciled)

2026-10-24, America/Guatemala. Read-only audit branch
`milestone/unreal-parity-audit-001` from `origin/main` (`0b0a6c0`).
**No implementation, no production changes, no merge.** Sources: Fusion32
(`reference/game`), ClientCore, Unreal (`unreal/REAL33D`), and the existing
`PARITY_MATRIX.md` / `PROJECT_STATUS.md` / `evidence/` tree.

**This revision repairs the first pass (`72ee5dd`), which downgraded or omitted
previously CERTIFIED scope.** The governing rule here: *a certified milestone is
never downgraded because broader edge cases remain* — the certified core keeps
its status and each open edge is split into its own sub-feature row. Visual/asset
gaps are held separate from functional gaps and counted once as `BLOCKED_ASSET`.

## Corrections applied vs 72ee5dd (reconciled against evidence)

| Feature | Was (72ee5dd) | Now | Evidence |
|---|---|---|---|
| NPC economy (buy/sell via TALK) | NOT_IMPLEMENTED | **CERTIFIED** + split | NPC conversation sale observed live — Dixi sold a scroll (2815) for 5 gold "by ordinary play" (UNREAL-EDITORS-UI-001); operator confirms Al Dee buy/sell PASS. The classic shop **window** is extended opcode `GAME_SHOP_CODE 201` (OTClient ecosystem, `m_enableSendExtendedOpcode=false`), NOT base 7.72 — split out as its own NOT_IMPLEMENTED row, not a blocker |
| Skills panel | IMPLEMENTED_UNVERIFIED | **CERTIFIED** | UNREAL-SKILLS-PANEL-001 CERTIFIED: 7 skills, live Sword 67→83%, 9 percent bars, 6 registered HUD panels, retained layout |
| Minimap / navigation | PARTIALLY | **CERTIFIED** | UNREAL-WORLD-PRESENTATION-POLISH-001 (independent): "minimap click with exact authoritative arrival" reproduced live; visual/cross-floor edges split to P2/P3 |
| Chat / channels | PARTIALLY | **CERTIFIED (core)** + split | CHAT-772-001 + live Say/Whisper + join/leave/private. Own-channel-create and request-queue (ch3) split to NOT_IMPLEMENTED; lifetime/colour to polish |
| Look / inspect | PARTIALLY | **CERTIFIED (world)** + split | Look reproduced live (polish cert). Slot-Look split to IMPLEMENTED_UNVERIFIED |
| Use / use-with | PARTIALLY | **CERTIFIED (use + use-with obj/field)** + split | Use and Use With reproduced live (polish cert). Use-on-creature split to IMPLEMENTED_UNVERIFIED |
| Player trade | PARTIALLY | **CERTIFIED (bounded)** | UNREAL-TRADE-UI-001 CERTIFIED live; accept-fail/multi-object/offer-refusal are edge sub-cases, moved to P1, not a downgrade |
| HUD panel registry / framework | PARTIALLY | **CERTIFIED** | Registry-managed panels certified across milestones; "six real registered HUD panels and retained HUD layout" (skills cert). Final HUD *visual* parity split to P2/P3 |
| Stats / vitals HUD | PARTIALLY | **CERTIFIED** | Vitals HUD live (UNREAL-PLAYER-VITALS-001; skills cert "received vitals/skills") |
| In-client login / char-select / relogin UI | (backlog only) | **NOT_IMPLEMENTED (explicit row)** | Login/session *protocol* stays CERTIFIED; the missing *UI* is now its own feature and the single real P0 |
| Floors / vertical movement | PARTIALLY | **IMPLEMENTED_UNVERIFIED** | Decoded both directions; no live floor change ever observed — honest move, not a downgrade of a cert |

## Classification

### CERTIFIED (29) — live-certified for the stated (bounded) scope
Connection/transport/crypto · Login+character-list protocol/session · Game
login / world entry (INIT_GAME) · Keepalive/ping · Map / initial world
(FULLSCREEN+deltas) · Items on map (presence/passability/stack order) ·
Wide-world static streaming · Creatures (appearance/move/attributes) · Movement —
cardinal · Movement — diagonal · Look / inspect (world) · Use (object) · Use-with
(object/field) · Turn / rotate object · Inventory / equipment · Containers
(open/close/up/nest/drag) · Stack move / split · Combat / follow / tactics ·
Creature death → corpse → loot · Player death → respawn / relogin · **NPC economy
via TALK (buy/sell)** · Player-to-player trade (bounded) · **Chat + channels core
(local/say/whisper/yell/join/leave/private)** · **Stats / vitals HUD** · **Skills
panel (7 skills)** · Outfit chooser (dialog + protocol) · Text editor EDIT_TEXT ·
**Minimap + click-to-walk navigation** · **HUD panel registry / framework**

### PARTIALLY_CERTIFIED (1)
Doors / openable environment objects — the Use mechanism that opens them is
certified; door-specific behaviour (locked/level/quest doors) is not.

### IMPLEMENTED_UNVERIFIED (6)
Slot Look (inventory/container point) · Use-with on creature · Text editor
EDIT_LIST (house guest/subowner/door lists) · Conditions / status icons
(PLAYER_STATE) · Magic fields (presence/decay behaviour) · Floors / vertical
movement (decoded; no live floor change).

### NOT_IMPLEMENTED (8)
Classic shop UI window (extended opcode 201; non-base-7.72) · Party (invite/join/
leave/shared-XP) · Buddy / VIP list UI · Effects — graphical/textual/missile (3D
render) · Projectiles / missiles (3D render) · Own-channel creation
(CL_CMD_OPEN_CHANNEL, no builder) · Request-queue channel (ch3 /
SV_CMD_OPEN_REQUEST_QUEUE 174 undecoded) · In-client login / character-select /
relogin UI.

### BLOCKED_ASSET (1)
Item/creature/outfit visual art (V08 catalog): 4,913 GLBs imported, APPROVED=NO,
PRODUCTION_INTEGRATED=NO. One cross-cutting layer; it also carries outfit
colour rendering (palette byte→RGB). Gates *visual* parity, not function.

### NOT_APPLICABLE (excluded from counts)
GM / admin tooling (not player-facing; the GAMEMASTER_OUTFIT message branch is
unreachable without editing server data).

## Functional parity estimate

Visual/asset (BLOCKED_ASSET) and NOT_APPLICABLE held out → **44 functional
features**. Weights CERTIFIED 1.0, PARTIALLY 0.6, IMPLEMENTED_UNVERIFIED 0.4,
NOT_IMPLEMENTED 0:

```
29(1.0) + 1(0.6) + 6(0.4) + 8(0) = 29 + 0.6 + 2.4 = 32.0
32.0 / 44 = 0.727
```

**FUNCTIONAL_PARITY ≈ 73%** (was mis-stated ~69% when certified scope was
under-credited). The full solo + economy + social-lite loop is certified:
connect, enter, walk (incl. diagonals), look/use/use-with, rotate, fight/follow,
die, loot, respawn, carry/equip, bag, buy from NPCs, trade with players, chat in
channels, read skills/vitals, navigate by minimap click. Open functional ground:
in-client login UI, grouping (party), buddy UI, effect/projectile rendering, and
a handful of unverified interaction/edit/field edges.

## Prioritized backlog

### P0 — gameplay blockers
- **In-client login / character-select / relogin UI.** The session *protocol* is
  certified, but the client `Connect`s once at `BeginPlay` with runtime-read
  credentials and a deliberate close (logout/death) is final, so there is no way
  to log in, pick a character, or re-enter after death without relaunching. This
  is the one true blocker. (NPC shops are **not** a blocker: the economy works
  via TALK.)

### P1 — parity gaps
- Effects + missile/projectile **3D rendering** (combat/magic feedback invisible).
- **Party** system (invite/join/leave, shared-XP, party UI).
- **Buddy/VIP** list UI.
- **Floors / vertical movement** live certification.
- **Use-on-creature** and **slot-Look** live certification.
- **EDIT_LIST** (house lists) and **conditions/status-icon** live certification.
- **Channel completeness**: own-channel creation + request-queue (ch3).
- Trade edge cases: accept-time failure, multi-object/container offers,
  offer-time refusals, success-vs-failure signalling.

### P2 — polish / edge cases
- **Classic shop UI window** (convenience; economy already works via TALK;
  extended opcode, non-base-7.72 — low priority).
- Door-specific behaviour (locked/level/quest doors).
- Magic-field behaviour certification.
- Minimap visual rendering + cross-floor navigation edges.
- Chat message lifetime/colour parity.
- Corpse final decay/disappearance + loot variants.
- Long-session silent network-loss handling.

### P3 — asset / visual blockers
- V08 catalog production approval + import (director sign-off; APPROVED=NO).
- Outfit colour rendering (palette byte→RGB; colour stored/round-tripped, not drawn).
- Item/creature appearance parity (meshes are primitives/proxies).
- Floor spacing (presentation-chosen 100uu) unresolved; wall orientation / floor
  visibility / camera-feel final review.

## Top 10 next

1. In-client login / character-select / relogin UI (only P0).
2. Effect + missile/projectile 3D rendering.
3. Party system (grouping + shared XP).
4. Buddy/VIP list UI.
5. Floors / vertical movement live certification.
6. Use-on-creature + slot-Look live certification.
7. EDIT_LIST + conditions/status live certification.
8. Channel completeness (own-channel create + request-queue ch3).
9. V08 visual catalog production approval/import (unblocks art track).
10. Outfit colour rendering (palette byte→RGB).

## Scope and limits

No server was started and no GUI client driven; this reclassifies existing
evidence. Counts are a defensible enumeration at a consistent granularity, finer
than the first pass because certified cores were split from their open edges per
the no-downgrade rule. `secret_check.sh` 6/6.
