# REAL33D — ChatGPT Project Context

Updated: 2026-09-21

Purpose: durable recovery context for continuing REAL33D if conversational context is lost. Repository source truth and newer evidence always override this file.

## Project

REAL33D is a **dual-client** project on the Fusion32/Tibia 7.72 world: a 2D
client and a 3D client sharing one authoritative server.

Selected production architecture (`DUAL-CLIENT-ARCHITECTURE-CLOSEOUT-001`):

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

- `REAL33D_2D` = mehah/OTClient-derived production client.
- `REAL33D_3D` = Unreal + ClientCore production client:
  `Fusion32 7.72 → Protocol772Core → semantic events → WorldState → Unreal Bridge → Unreal Game Thread → 3D presentation`
- `Fusion32` = sole authoritative game server.
- Official `Tibia.exe` 7.72 = QA / parity / reference only, **not** a production client.
- IP changer = legacy QA utility only, **not** production infrastructure.

OTClient keeps its own parser and game model. ClientCore remains the 3D
implementation and the protocol reference. We do **not** replace OTClient's model
with ClientCore's WorldState. The clients share the protocol specification,
byte-level fixtures, semantic expectations and Fusion32 authority.

Fusion32 remains authoritative for both. Neither client may become gameplay
authority. Unreal must not independently parse Protocol 772. Initial convention:
`1 SQM ≈ 100 Unreal Units`.

Future visual resolution boundary:

`Fusion32 visual identity → REAL33D Asset Registry → Unreal asset`

## Completed core milestones

- BOOTSTRAP-SOURCES-001 — PASS
- CLIENTCORE-TRANSPORT-772-001 — PASS
- CLIENTCORE-CRYPTO-772-001 — PASS
- CLIENTCORE-LOGIN-772-001 — PASS + LIVE
- CLIENTCORE-GAMELOGIN-772-001 — PASS + LIVE
- INITIALWORLD-772-001 — PASS + LIVE
- MOVEMENT-772-001 — PASS + LIVE
- PLAYERSTATE-772-001 — PASS + LIVE + published
- UNREAL-WIDE-WORLD-001 — CERTIFIED_PASS (`3fd5d1d`)
- DUAL_CLIENT_LIVE_CAPTURE — PASS (`0f9bd505`); OPTION_C_VIABLE_FOR_PRODUCTION = YES
- DUAL-CLIENT-ARCHITECTURE-CLOSEOUT-001 — architecture selected

## TWO-CLIENT-VERTICAL-SLICE-001

PASS, reported published HEAD `31d4be5`.

Original Tibia.exe 7.72 and Protocol772Core sustained two distinct players in the same Fusion32 world for about 19 minutes. Appearance, 11 remote steps, viewport boundary behavior, accepted B movement, shared player collision/blocking, disconnect/reconnect and keepalive were exercised.

Reported protocol health: 689 commands, 0 residual bytes, 0 anomalies, 0 unsupported opcodes.

Important live discoveries:

- `DELETE_FIELD` does not necessarily mean forget creature identity; a creature can leave view and return while the server assumes it remains known.
- Marker 97 was initially misinterpreted as evicting an identity; player CreatureID can persist/reuse its slot.
- The harness originally did not print Snapback/Message, making legitimate rejected walks appear silent.
- A passive client needs keepalive; B pinged roughly every 20 seconds.

Limits: one run/operator, floor 7 only, chat intentionally excluded, some B→A evidence human-observed, no 3D rendering proven.

## Visual production

The project director's brother owns visual/art production: environment/world, items, creatures/NPCs/player/outfits, modeling, textures/materials, rigging, animation, VFX and applicable visual UI/art.

Boundary: the artist decides how something is represented visually; Fusion32/REAL33D decides what it is and how it behaves.

There is NO 50/50 fidelity/realism rule for REAL33D. The original data establishes identity/context; the artist proposes; the project director gives final APPROVED/REJECTED decision.

Workflow:

`TODO → MOCKUP → REVIEW → APPROVED/REJECTED → MODELING → TEXTURING → RIGGING → ANIMATION → READY → INTEGRATED`

Rejected concepts/versions are retained.

## VISUAL-ASSET-MASTER-INVENTORY-001

PASS. Reported implementation commit `4260d98`; final published HEAD `dde7cd6`.

Reported inventory:

- 5,690 visual identities
- 5,003 object types
- 159 monster races
- 337 NPCs
- 152 outfit identities
- 26 graphical effects
- 13 projectiles
- 4,991/5,003 objects named; 12 unnamed
- initial grouping: 5,003 object IDs → about 1,351 inferred visual assets
- 4,141 IDs in 489 multi-member reuse groups
- Rookgaard P0: 456 types from 48 sectors around NewbieStart

Sources included `objects.srv`, `map.dat`, `origmap`, `monster.db`, `mon/`, `npc/`, `enums.hh`.

Tracker regeneration preserves artist state; disappearing identities become ORPHANED. Git LFS rules were prepared for future art binaries while manifests/tracker remain diffable Git text.

## VISUAL-REFERENCE-PACK-001

PASS, reported published HEAD `d3f0e5d`.

Exact visual inputs were already in the authorized local workspace; no substitutes were downloaded. Client provenance remains UNKNOWN, so original client files and generated derivative previews are not committed.

Reported:

- 5,284 appearances decoded
- 10,926 sprites decoded
- 5,284 previews rendered
- 456 Rookgaard P0 entries queued
- 4,587 demonstrated shared sprite sets covering 835 IDs
- 100 unreferenced client outfits
- server object 5090 (`a treasure map`) outside client object range
- 36 empty sprite slots

`visual/reference_pack/` is gitignored; roughly 99 MB of generated previews are local. Setup verifies the exact visual inputs by SHA-256 and refuses mismatched versions.

Artist flow:

`clone → provide exact authorized 7.72 visual inputs → run documented setup → open generated HTML catalog → browse references/Rookgaard P0 → MOCKUP → director REVIEW → APPROVED/REJECTED → production`

Reference sheets are nearest-neighbor/pixel-exact. Artistic interpretation is separate.

At this context checkpoint, the brother is cloning/setting up his workstation. A later fresh-workstation validation is useful but does not block Unreal engineering.

## CURRENT STATE — dual-client closeout (2026-09-21)

- `UNREAL-WIDE-WORLD-001` = `CERTIFIED_PASS` at `3fd5d1d`.
- `DUAL_CLIENT_LIVE_CAPTURE` = `PASS` at `0f9bd505`.
  Stock mehah/otclient completed connect, login, character list, game login,
  initial world, client-initiated movement, say chat, items, inventory,
  containers, item move and logout/reconnect against the unmodified server with
  configuration only. Incoming decode errors, unknown opcodes and unsupported
  opcodes were 0 each. `OPTION_C_VIABLE_FOR_PRODUCTION = YES`.
- Remaining patch: `TALK_MODE_14`, one `protocolcodes.cpp` table entry,
  `NOT_LIVE_TESTED` (needs a gamemaster anonymous channel call).
- DIV-08: static match 4990/4990; 5/5 observed TypeIds live-confirmed;
  cumulative/liquid classes `NOT_LIVE_COVERED`; TypeId 5090 is
  `KNOWN_SERVER_ONLY_OR_UNREACHED_EXCEPTION`.
- Attribution, documentary only: `c7bcfe7` is principally
  `UNREAL-PLAYER-VITALS-001` and other concurrent work, which remains
  `IMPLEMENTED_UNVERIFIED`.
- **Next milestone: `REAL33D-2D-BOOTSTRAP-001` (NOT_STARTED)** — turn the
  isolated OTClient probe into a clean, reproducible REAL33D-owned 2D client.
  Out of scope: shops, extended opcode 50, new TerminalTypes, 8.6 protocol, new
  server features, Unreal changes, V08 changes, visual/art work.
- Probe scratch tree `C:
33dprobe` must stay untouched until the bootstrap
  reproduces the live PASS.

## EARLIER MILESTONE — UNREAL-SLICE-001

**IN PROGRESS. DO NOT RESTART FROM ZERO.**

Codex was paused because of token limits, not because a technical blocker had been established.

Visible state at pause:

- 7 tasks total: 1 done, 1 in progress, 5 open
- Unreal 5.8 project / ClientCore module work had begun
- approximately 125 lines had been written to `unreal/REAL33D/Source/REAL33D/Public/Real33DBridge.h`
- pending work included the thread-safe semantic bridge, coordinate transform/Asset Registry, presentation for tiles/creatures/viewport/movement, live acceptance and closeout

Before continuing, inspect actual workspace/diff/TODO state. Do not discard valid partial work and do not assume partially written files are correct.

Objective:

`Fusion32 → Protocol772Core → semantic events/WorldState → Unreal Bridge → Game Thread → 3D presentation`

Minimum presentation: local player, other players/creatures, visible tiles, appearance/disappearance, cardinal movement and viewport updates. Use planes/cubes/capsules as placeholders; do not wait for final art.

Remote movement:

`Fusion32 → ClientCore → WorldState → Bridge → Actor`

Local movement:

`Unreal input → semantic ClientCore action → Protocol772Core → Fusion32 → confirmed WorldState → presentation`

Unreal Actor movement is not authoritative. Rejected movement must not leave visual desync.

PASS requires a real chain involving `Tibia.exe 7.72 + Fusion32 + Protocol772Core + Unreal`. Compilation, a static map or simulated WorldState alone is insufficient.

Live acceptance should demonstrate:

1. Unreal enters the real world through ClientCore.
2. Local player appears correctly.
3. Viewport is represented.
4. Original Tibia.exe player appears in Unreal.
5. Remote movement updates Unreal.
6. Unreal-side movement reaches Fusion32.
7. Tibia.exe observes accepted Unreal-side movement.
8. Rejected movement leaves no false Unreal position.
9. Viewport enter/leave works.
10. Disconnect/reconnect cleans presentation.
11. No ghost/duplicate actors.
12. Protocol health remains clean unless new real evidence reveals an issue.

Scope exclusions unless strictly required: final art, chat, inventory/containers, combat UI, spells/runes, final VFX, full UI, minimap, audio, mobile/tablet and monolithic offline map conversion.

## Resume instruction

Start `REAL33D-2D-BOOTSTRAP-001`. Do **not** re-run the live probe and do **not**
redo the dual-client research; both are certified and published at `0f9bd505`.

Objective: turn the successful isolated OTClient probe into a clean, reproducible
REAL33D-owned 2D client.

Scope: pinned `mehah/otclient` upstream commit, REAL33D branding, Fusion32
host/port configuration, protocol 772 preset, terminal OS/type configuration,
Fusion32 **public** RSA modulus configuration, `GameEnvironmentEffect`
compatibility, the TALK mode 14 one-entry patch, justified removal or disabling
of irrelevant OTClient ecosystem startup modules, a reproducible Windows build,
no embedded account or password, no private RSA or XTEA material, no accidental
`Tibia.dat`/`Tibia.spr` publication, and a login to gameplay to logout/reconnect
acceptance run.

Out of scope: shops, extended opcode 50, new TerminalTypes, 8.6 protocol, new
server features, Unreal changes, V08 changes, visual/art work.

Keep the architecture intact: OTClient retains its own parser and game model;
ClientCore stays the 3D implementation and the protocol reference; the two
clients share the protocol specification, byte-level fixtures, semantic
expectations and Fusion32 authority. Neither client is gameplay authority.

Keep `C:33dprobe` untouched until the bootstrap reproduces the live PASS.

At close verify `HEAD == origin/main` and a clean worktree. Do not start the
following milestone automatically.

## Parallel workstreams

Engineering:

`TWO-CLIENT PASS → UNREAL-SLICE IN PROGRESS → real Unreal/Fusion32 slice`

Visual:

`MASTER INVENTORY PASS → REFERENCE PACK PASS → brother workstation setup → Rookgaard P0 mockups → director review → approved production → READY`

Integration:

`Unreal placeholders + approved visual production → Asset Registry → REAL33D`

## Recovery in a new chat

Say:

> Estamos continuando REAL33D. Lee `docs/CHATGPT_PROJECT_CONTEXT.md` y después `AGENTS.md`, `ARCHITECTURE.md`, `PROJECT_STATUS.md`, `PARITY_MATRIX.md`, el último handoff y la evidencia del repo. El estado más nuevo del repo manda sobre el snapshot. Continúa desde el milestone actual.

Working preference: concise Spanish, action-oriented guidance, professional copy/paste Codex prompts with minimal redundancy. Codex executes implementation; ChatGPT helps direct/review architecture, milestones and reports.

Do not confuse REAL33D with older unrelated Tibia projects/repositories.

Standing closeout: tests + relevant sanitizers + evidence + docs + secret check + commit + handoff + push; verify `HEAD == origin/main` and clean worktree; no secrets; no history rewrite.
