# UNREAL-PARITY-AUDIT-001 — full 7.72 gameplay parity audit

2026-10-24, America/Guatemala. Read-only audit branch
`milestone/unreal-parity-audit-001` from `origin/main` (`0b0a6c0`).
**No implementation, no production changes, no merge.** Sources: Fusion32
(`reference/game`), ClientCore (`clientcore/`), Unreal (`unreal/REAL33D`), and
the existing `PARITY_MATRIX.md` / `PROJECT_STATUS.md` / `evidence/` tree.

This is a classification pass, not a new certification. Certified features are
not downgraded without evidence; where a live certification exists it governs
over the older table rows it supersedes. **Visual/asset gaps are separated from
functional gaps**: every mesh in REAL33D is still an engine primitive or an
unapproved V08 placeholder, so no feature claims *visual* parity, and that one
cross-cutting art gap is counted once as `BLOCKED_ASSET` rather than smeared
across every row.

## Classification taxonomy

- **CERTIFIED** — live-observed logical parity for its bounded scope, with
  retained evidence.
- **PARTIALLY_CERTIFIED** — a core path is certified live; named sub-cases
  remain unverified.
- **IMPLEMENTED_UNVERIFIED** — decoded/built and wired, no live certification.
- **NOT_IMPLEMENTED** — no client behaviour (may be decoded server-side only).
- **BLOCKED_ASSET** — functionally reachable but gated on unapproved art.
- **NOT_APPLICABLE** — not a player-facing 7.72 feature.

## Feature classification

### CERTIFIED (18)

| # | Feature | Basis |
|---|---|---|
| 1 | Connection / transport / crypto | CLIENTCORE-TRANSPORT/CRYPTO-772-001; MSVC+WSL builds |
| 2 | Login + character list (session) | combined login/handover cert (PARITY_MATRIX §2026-10-22) |
| 3 | Game login / world entry (INIT_GAME) | CLASSIC-GAME-ENTRY / CLIENTCORE-GAMELOGIN; UNREAL-SLICE |
| 4 | Keepalive / ping | 20s CL_CMD_PING; live 90-round drop+recover (UNREAL-SLICE) |
| 5 | Map / initial world (FULLSCREEN + deltas) | INITIALWORLD/MOVEMENT-772-001; live tile-for-tile |
| 6 | Wide-world static streaming | UNREAL-WIDE-WORLD-001 CERTIFIED_PASS (static reach only) |
| 7 | Creatures (appearance/move/attributes) | 4 suites + UNREAL-SLICE live |
| 8 | Movement — cardinal + stop/turn + snapback | MOVEMENT-772-001; UNREAL-SLICE 18/19 live |
| 9 | Movement — diagonal | UNREAL-DIAGONAL-INPUT-001 CERTIFIED |
| 10 | Combat / follow / tactics | UNREAL-COMBAT-FOLLOW-001 CERTIFIED live |
| 11 | Creature death → corpse → loot | UNREAL-DEATH-LOOT-001 CERTIFIED live |
| 12 | Player death → respawn / relogin | UNREAL-PLAYER-DEATH-RESPAWN-001 CERTIFIED live |
| 13 | Inventory / equipment | UNREAL-INVENTORY-CONTAINERS-001 live vs server save |
| 14 | Containers (open/close/up/nest/drag) | UNREAL-CONTAINER-CONTROLS-001 CERTIFIED (supersedes "not built") |
| 15 | Stack move / split amount | UNREAL-STACK-MOVE-CONTROLS-001 reviewed/live |
| 16 | Turn / rotate object | UNREAL-TURN-OBJECT-001 CERTIFIED live |
| 17 | Outfit chooser (dialog + protocol) | UNREAL-OUTFIT-DIALOG-001 CERTIFIED (bytes/round-trip); see asset note |
| 18 | Text editor — EDIT_TEXT (books/signs) | UNREAL-EDITORS-UI-001 EDIT_TEXT CERTIFIED live |

### PARTIALLY_CERTIFIED (9)

| # | Feature | Certified / Unverified |
|---|---|---|
| 19 | Look / inspect | world Look live PASS / slot-Look variants unverified |
| 20 | Use / use-with | Use + use-with-object live / use-on-creature, use-on-field not live |
| 21 | Player-to-player trade | start/offer/inspect/accept/reject/close/ownership live / accept-time failure, multi-object, offer-time refusals unverified |
| 22 | Chat / channels / private | Say/Whisper/Yell + channel join/leave/private live / request-queue (ch3), own-channel create, display lifetime+colour unverified |
| 23 | Stats / vitals HUD (HP/mana/level/cap/soul) | decoded+HUD live log / full visual inspection pending |
| 24 | Minimap / navigation | minimap decode+view+panel tested / live parity + click-to-walk uncertified |
| 25 | HUD frame / panels | panels present, several certified in context / whole-HUD layout+visual parity not claimed |
| 26 | Floors / vertical movement | floor set/offsets/up-down decoded / no live floor change; 100uu spacing unresolved |
| 27 | Doors / openable fields | reachable via the Use toggle / no door-specific live certification |

### IMPLEMENTED_UNVERIFIED (4)

| # | Feature | State |
|---|---|---|
| 28 | Text editor — EDIT_LIST (house guest/subowner/door lists) | built+decoded; no live run possible on this server |
| 29 | Skills panel (7 fighting skills) | panel added (2026-10-17 review); live/visual pending |
| 30 | Conditions / status icons (PLAYER_STATE) | 8-bit flags decoded, condition strip wired; live mapping unverified |
| 31 | Fields (magic fields presence/decay) | decoded + drawn as tile objects; behaviour not certified |

### NOT_IMPLEMENTED (5)

| # | Feature | Note |
|---|---|---|
| 32 | NPC shops (buy / sell) | explicitly out of scope of the trade milestone; no client path |
| 33 | Party (invite/join/leave/shared XP) | PARTY attribute decoded only; no UI/commands |
| 34 | Buddy / VIP list UI | BUDDY_DATA/ONLINE/OFFLINE decoded; no UI |
| 35 | Effects — graphical / textual / missile (3D render) | decoded as typed events; **not drawn** in Unreal |
| 36 | Projectiles / missiles (3D render) | as above; no 3D presentation |

### BLOCKED_ASSET (1)

| # | Feature | Note |
|---|---|---|
| 37 | Item / creature / outfit visual art (V08 catalog) | 4,913 GLBs imported (VISUAL-FULL-CATALOG-INGEST STANDBY); APPROVED=NO, PRODUCTION_INTEGRATED=NO. All appearance is placeholder; blocks *visual* parity for many functional rows above, counted once here |

### NOT_APPLICABLE (excluded from the counts)

- GM / admin tooling — not player-facing in scope; the one player-visible branch
  (GAMEMASTER_OUTFIT answering with a message) is unreachable without editing
  server data.

## Functional parity estimate

Visual/asset is held out of the functional denominator (feature 37 and the
NOT_APPLICABLE row excluded), leaving **36 functional features**. Weighting
CERTIFIED 1.0, PARTIALLY_CERTIFIED 0.6, IMPLEMENTED_UNVERIFIED 0.4,
NOT_IMPLEMENTED 0:

```
18(1.0) + 9(0.6) + 4(0.4) + 5(0.0) = 18 + 5.4 + 1.6 + 0 = 25.0
25.0 / 36 = 0.694
```

**FUNCTIONAL_PARITY ≈ 69%.** Headline read: the core solo loop — connect, enter,
walk (incl. diagonals), fight/follow, die, loot, respawn, carry/equip, bag,
use/rotate/look, trade with a player, chat — is certified live; the open
functional ground is the economy (NPC shops), grouping (party), effect/projectile
feedback, and full character-sheet/HUD/minimap live certification. Visual parity
is a separate, largely-unstarted track gated on art approval.

## Prioritized backlog

### P0 — gameplay blockers
- **NPC shops (buy/sell).** No economy loop with NPCs; the whole shop protocol
  path is unbuilt.
- **In-client login / relogin UI.** The client `Connect`s once at `BeginPlay`
  and a deliberate close (logout/death) is final, so re-entering requires
  relaunching the app — there is no login screen or in-client "log in again".

### P1 — parity gaps
- Effect + missile/projectile **3D rendering** (combat/magic feedback is invisible).
- **Party** system (invite/join/leave, shared-XP, party UI).
- **Buddy/VIP** list UI.
- **Skills/stats** panel live certification (and full character sheet).
- Chat gaps: request-queue channel (3), own-channel creation, display
  lifetime/colour parity.
- Item-action edge forms live: **use-on-creature**, **use-on-field**, **slot Look**.
- **EDIT_LIST** (house lists) live certification.
- **Floor-change** (vertical) live verification.
- Trade edge cases: accept-time failure, multi-object/container offers,
  offer-time refusals, success-vs-failure signalling.

### P2 — polish / edge cases
- Minimap live parity + click-to-walk navigation certification.
- HUD layout/visual polish and condition status-icon mapping.
- Door-specific behaviour certification (beyond the generic Use toggle).
- Chat message lifetime/colour parity vs the classic client.
- Corpse final decay/disappearance and additional loot variants.
- Long-session keepalive / silent-loss handling (a genuine network loss surfaces
  as read timeouts today and is not retried).

### P3 — asset / visual blockers
- **V08 catalog production approval + import** (director sign-off; APPROVED=NO).
- **Outfit colour rendering**: palette byte→RGB mapping is unverified, so no
  colour is drawn though it round-trips and is stored.
- Item/creature appearance parity (all meshes are engine primitives/proxies).
- Floor spacing (presentation-chosen 100uu) unresolved in data.
- Wall orientation / floor visibility / camera feel final visual review.

## Top 10 next

1. NPC shops (buy/sell economy loop).
2. In-client login / relogin UI (login screen + re-enter after death/logout).
3. Effect + missile/projectile 3D rendering.
4. Party system (grouping + shared XP).
5. Buddy/VIP list UI.
6. Outfit + colour 3D rendering (palette byte→RGB).
7. Skills / stats panel live certification.
8. Chat gaps: request-queue (ch3), own-channel creation, display lifetime.
9. Use-on-creature / use-on-field / slot-Look live certification.
10. V08 visual catalog production approval/import (unblocks the art track).

## Scope and limits

No server was started and no GUI client driven for this audit; it reclassifies
existing evidence and does not add live results. Counts are a defensible
enumeration of player-facing 7.72 features at a consistent granularity, not a
canonical constant — a finer split would move absolute numbers but not the
~69% functional headline or the backlog ordering. `secret_check.sh` 6/6.
