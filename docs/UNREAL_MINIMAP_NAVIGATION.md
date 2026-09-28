# UNREAL-MINIMAP-NAVIGATION-001 — audit and boundary

Base: a97cf25e7449a8a3ef35ef2553181c5032c7a247. Scope: REAL33D 3D,
ClientCore terrain projection/cache, existing Slate HUD and normal movement
intents. Deliverables: explored minimap, floor/player state, local pan/zoom/
recenter, same-floor map-click navigation including unexplored goals, native
tests/build and live evidence. Route planning uses retained observations;
every actual step requires normal live validation and Fusion32 acceptance.
No REAL33D2D runtime coupling, agent system, V08 editing or visual editor.

## Pre-implementation audit

1. `clientcore/.../worldstate.h::WorldState` stores the live window, ordered
   MapTile stacks, decoded floor coordinates and known-creature protocol mirror.
   It has no minimap or explored-terrain cache. `WorldView::Diff` emits live tile
   removals/upserts and creature changes; pruning is required, not discovery loss.
2. `initial_world.cpp::ApplyFullScreen`, `map_scan.cpp`, and
   `movement.cpp::ApplyServerUpdate` already decode/apply fullscreens, rows,
   floor transitions and field changes. No map request is required for discovery.
   Selected Fusion32 source revision 386fa9b8078a1b32187dfcbfc2a0ed7543e16346:
   `sending.cc::SendFullScreen/SendRow/SendFloors` describe an 18x14 rectangle,
   with per-floor X/Y offset PlayerZ-PointZ. These serializers have no local
   TIBIA772 conditional; communication.cc's TIBIA772 gate selects version 772.
   `cract.cc::NotifyGo` advances the viewport anchor for coordinate-less updates.
3. `AReal33DWorld::FindKnownWalkPath/IsKnownWalkTile` implement cardinal BFS over
   current tile Actors. `AReal33DPlayerController::TickClickWalk` revalidates each
   next tile, sends RequestWalk and waits for server acceptance. Refusal,
   unanswered request, external relocation or timeout stops it. No ClientCore
   automap planner exists. `receiving.cc::CGoDirection` is the normal server path;
   CGoPath exists in source but is not needed or newly exposed here.
4. `SReal33DMinimapPanel` already occupies the HUD's right column, uses the local
   UI art and shows actual coordinates. Its surface is empty, buttons inert,
   and it has no marker, floor browsing, pan/zoom or persistence.
5. Read-only conceptual comparison: separate
   `Desktop/REAL33D2D/src/client/minimap.{h,cpp}` separates observed terrain from
   live map and persists OTMM blocks; `modules/game_minimap/minimap.lua` tracks
   current floor/player and recenters; tile.cpp's color routine skips creatures.
   map.cpp has its own pathfinding. None of these implementations or models is
   copied, linked, modified or used by REAL33D 3D.
6. `Real33DCoords.h::ToWorld`: +X east, +Y south, 100 uu/SQM; Unreal height is
   (origin.Z-position.Z)*220 uu. Floor 0 highest, 7 surface, 15 deepest.
   Minimap uses integer Tibia coordinates directly: +X right, +Y down, north up.
   No perspective-floor offsets are applied a second time.
7. REAL33D's existing ignored Saved/WideWorldCache contains static .wws sectors.
   No REAL33D minimap cache/loader exists in tracked source. No .otmm/.otcm was
   found in the separate 2D data directory. 2D personal caches are not input.
8. Existing 3D world-click walking is reusable only within current live tiles.
   Explored-only destinations and other floors must not acquire walkability
   from WideWorld or retained minimap hints.
9. Exact gaps: terrain-only observation projection, independent bounded local
   cache and persistence, semantic bridge access, north-up Slate drawing, player
   marker, selected/current floor distinction, pan/zoom/recenter controls,
   minimap-to-existing-walk input, and reproducible live/UI evidence.

## Three layers

### Unexplored destinations requested by operator

Operator explicitly requested map clicks on unknown coordinates too. Before
implementation, re-audited the known-terrain A* and per-step executor. A request
can retain an unexplored same-floor goal without inventing its terrain: plan a
known reachable segment toward an exploration frontier, walk it through normal
intents, then replan toward the original goal as regular viewport descriptions
arrive. Every actual next tile still needs live validation; unknown terrain is
never inserted into memory, rendered, marked walkable, or claimed reached.
Blocked destinations newly observed as such stop; current blockers can detour,
and refusal/relocation/missing answers stop. No extra map requests or WideWorld
input. Exact gaps: frontier segment planning, preserving the original goal over
segments, continued replanning after segment completion, and live unknown-goal
evidence. Tests must prove memory/observation counts do not grow during planning,
bounded search over existing knowledge, repeated discovery segments and no
unknown tile in the returned segment. Earlier known-only restrictions below
describe the preceding revision, superseded by this operator direction.

### Navigation/UI revision requested after live review (2026-09-28)

Operator accepted the colored minimap, then requested removal of the visible
live-tile footprint/brightness split and the viewport restriction on map clicks.
The minimap now targets a uniform classic-colored terrain-history presentation;
mutable live surroundings remain a separate 3D/WorldState layer, documented and
described as observed/known terrain rather than marked per minimap cell.

Before implementation, re-audited `KnownMinimap` ground/static-obstacle hints,
WorldActor::IsKnownWalkTile/FindKnownWalkPath and
PlayerController::RequestKnownWalk/TickClickWalk. Retained observed Bank ground
and static Unmove+Unpass blockers can estimate a same-floor route beyond the
18x14 window. This estimate is not current walkability. No WideWorld data enters
the planner. Current live blocked tiles override stale hints, and every next
adjacent step must pass the existing live-tile check before normal RequestWalk.
Unknown terrain cannot connect a route. Each step waits for authoritative
acceptance; refusal, relocation or missing answer stops it. A newly observed
blocker can trigger a new route estimate from the real current position.

Exact new gaps: bounded-by-known-data A* (no viewport/distance cutoff), bridge
route access without parser/model leakage, normal executor replanning when
adjacent live assumptions differ, uniform drawing without a cyan viewport or
history dimming, and new long-known-path/live interruption evidence. Tests must
cover routes hundreds of tiles long, unknown/off-floor destinations, static and
current blockers, detours, transactional cache retention and authoritative
next-step validation. Multi-floor route topology remains unsupported because
floor connections are not modeled; browsing floors remains local UI.

WorldState remains the sole observed mutable layer, with the unchanged 18x14
viewport and existing protocol. Minimap memory retains only current-floor
observed ground TypeId, observed static color-source TypeId, and an observed
static obstacle hint (Unpass on Bank ground or Unmove appearances),
never creatures, carried items, mutable stacks or claims about present entities.
Hints can become stale and are used for drawing/local route estimates only.
The minimap uniformly renders known static terrain. At the operator's request,
there is no live-viewport rectangle or history/live brightness distinction.
The tooltip identifies the map as known terrain; it has no live entity layer.
WideWorld continues to render its own static baseline sectors outside live
authority. It never feeds discovery, movement or minimap persistence.

The palette comes from the selected local 7.72 DAT (SHA256
3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd), via
REAL33D's validated `visual/tools/tibia772.py::AppearanceFile` EOF reader.
`extract_minimap_palette.py` reads attribute 0x1C and generates an ignored local
appearance-color table. Colors use the 216-color cube (R major, G middle, B minor;
six levels per channel). The last colored ground/Unmove appearance in the
observed ordered stack supplies the hint. Creatures and movable appearances
cannot supply colors. Unknown remains dark; a missing local palette uses the
explicit gray-ground/ochre-obstacle fallback. The table contains no coordinates
and cannot reveal any map tile. This is not wholesale classic minimap parity:
this milestone separates known static terrain from live mutable authority and
restricts actual navigation to normal movement intents.
Cache v2 persists the static color-source hint; v1 is migrated without discarding
exploration, and current observations refresh its colors. No V08 assets change.
Pan/zoom/floor selection only change the map camera. Recenter restores follow
and actual player floor. A real Z change always switches the selected floor.
Minimap clicks preserve a same-floor destination, including unknown coordinates.
Known-terrain A* finds a complete route when possible; otherwise a reachable
observed segment toward an exploration frontier is returned. Segment completion
replans toward the original destination using newly received normal observations.
No unknown tile is appended or made known by the planner. There is no viewport
or distance cutoff. Current observed blocked tiles override those hints. Each
adjacent step still requires live tile validation and real server acceptance;
changed live assumptions can replan, while refusal/relocation/timeout stops.
History never asserts current walkability or movement success.

## Validation contract

Core tests must exercise retention through viewport pruning, floor round trips,
creature/movable-item exclusion, changed terrain, isolation and transactional
malformed-cache rejection, long historical routes, current/static blockers,
detours, unknown-goal segment continuation without fabricated discovery and
coordinate bounds. Native C++17/C++20 suites and Unreal build must run.
Live movement/floors/rendering/WideWorld/regressions cannot be called PASS from
those tests. Capture session coordinates/deltas/counts/floors, normal request
ledger, screenshots and zero residual/unsupported/anomaly counters; report every
unexercised requirement explicitly. Full procedure/results live in
`evidence/clientcore/UNREAL-MINIMAP-NAVIGATION-001.md`.
