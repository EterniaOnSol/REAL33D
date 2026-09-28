# UNREAL-ITEM-USE-INTERACTION-001

Date: 2026-09-27, America/Guatemala. Branch:
`milestone/unreal-item-use-interaction-001`, based on
`bd15cc0a49d8182dc1cc3732b8487859f1662044` (`origin/main`).
State: `PASS` for the named live paths below; independent repetition for
`CERTIFIED` remains open. No main merge. The milestone branch is published
as part of the 2026-09-27 closeout.

## Audit before editing

- `reference/game/src/connections.hh::ClientCommand`: Use Object 130, Use
  Two Objects 131, Use On Creature 132 and Look At Point 140.
- `receiving.cc::CUseObject/CUseTwoObjects/CUseOnCreature`: complete normal
  request paths; `CUseObject` rejects a MultiUse source and uses its last
  byte as the destination open-container number.
- `receiving.cc::CLookAtPoint`: reads only two little-endian words and a
  byte (x/y/z), checks special coordinates and map visibility, then calls
  `GetObject(Player->ID, x, y, z, x==0xFFFF ? z : -1, 0)`.
  `info.cc::GetObject` resolves body slots directly, container items by z,
  and the server's top object on a map point. `operate.cc::Look` creates the
  description and calls `SendMessage(..., TALK_INFO_MESSAGE, ...)`.
- Existing ClientCore already built all three Use packets. REAL33D already
  routed world right-click, body/container right-click, Shift+right-click
  Use With, mouse target selection, container opening and `SV_CMD_MESSAGE`
  presentation. `UNREAL-INVENTORY-CONTAINERS-001` certified live body/nested
  bag opening and flour-on-bucket Use With; `UNREAL-COMBAT-FOLLOW-001`
  retained a live world right-click that opened a corpse container.

## Change

`BuildLookAtPointCommand` adds only the source-derived six-byte request.
Alt+right-click on the world or a creature position posts a map-point Look.
Alt+right-click an occupied body or open-container slot posts its current
server address. The worker sends through `GameLoginSession::SendCommand`;
WorldState and Actors are never changed by the request. Any description must
arrive from Fusion32's existing message path. A pending Use With keeps
priority over a world click. The post-change F9 schema counts
`looks_requested` separately from `uses_requested`. No
reference/server/runtime or 2D agent source was changed.

The V08 QA launcher now requires a nonempty WideWorld cache and passes the
cache, preview directory and visual radius explicitly. The default radius is
64. It reports a missing cache or imported mesh directory rather than
silently showing the short view. V08 remains QA art, not approved final art.

## Verification

Precondition: local UE 5.8.2, VS 2022 BuildTools, selected Fusion32 source
and sanitized local runtime. `tests/build_clientcore_windows.cmd` was run
under `vcvars64.bat`: C++17 and C++20 archive builds PASS, all eight suites
PASS. The new player-state assertions match the exact Look bytes for a map
point `140 61 7D DB 7D 07`, body slot 3
`140 FF FF 03 00 00` and container 0 item 2
`140 FF FF 40 00 02`. `Build.bat REAL33DEditor Win64 Development
-Project=<worktree>/unreal/REAL33D/REAL33D.uproject -WaitMutex`:
`Result: Succeeded`.

Live precondition: Fusion32's Query Manager, Game and Login were identity
verified on 7173/7172/7171. The first start failed on a stale
`game/state/save/game.pid`; PID 557 was checked and belonged to
`SessionLeader`, so that single stale lock was removed and start repeated
successfully. Account B then reached Game through the normal client login.

The first REAL33D launch had no local V08 assets or WideWorld cache in the
isolated worktree and rendered placeholders. For the corrected run the
ignored project `Content/Experimental/V08` was linked locally to the
existing ignored V08 import, and the command line named the existing local
catalog, sector cache and preview directory explicitly. The runtime log
recorded `experimental V08 QA catalog ENABLED: 4913 entries`,
`wide world enabled: radius=64`, and 69 sector-load records. The
locally retained screen capture is
`build/v08-wideworld-3d.png` in this worktree; it visibly shows the
extended street/buildings beyond the live 18x14 window with V08 meshes.
It is kept local because imported art and generated previews are not
published by this repository.

The F9 snapshot at 2026-09-27T23:37:59.201Z recorded 532 decoded commands,
zero residual bytes, zero unsupported opcodes, zero protocol anomalies,
410 WorldState tiles and 410 tile actors, three WorldState creatures and
three creature actors, and `viewport_synchronised=true`. It also records
eight client walks accepted by the server, six use requests and nine server
messages. The snapshot is ignored locally at
`evidence/clientcore/unreal-item-use-interaction/unreal_slice_evidence_Manual.json`.
The six uses were not a retained Look request/reply pair. A subsequent live
run retained three world Look request/reply pairs in
`unreal/REAL33D/Saved/Logs/REAL33D.log` (local, ignored):

```text
2026-09-27 23:59:47.177 UTC look 1 at world point 32093,32207,7
2026-09-27 23:59:47.308 UTC server message [InfoMessage]: You see a mountain.
2026-09-28 00:01:21.985 UTC look 42 at world point 32100,32199,7
2026-09-28 00:01:22.027 UTC server message [InfoMessage]: You see a framework wall.
2026-09-28 00:02:06.278 UTC look 55 at world point 32094,32203,7
2026-09-28 00:02:06.354 UTC server message [InfoMessage]: You see grass.
```

These replies were decoded from Fusion32; the client has no local description
table. A local screenshot at `build/look-qa3.png` shows the running V08 and
WideWorld presentation with HUD `unsupported 0 anomalies 0` after Look.
The later process closed cleanly before another F9 export; the earlier F9
snapshot is not represented as post-Look protocol evidence.

## Verdict and repeat

World Use, inventory Use, container Use and one valid Use With have prior
live `PASS` evidence in the two named certified milestones above. The
current live log contains world Use requests and server refusal text for
unusable objects; that is protocol delivery, not successful gameplay for
those objects. World Look is live `PASS`: three normal Alt+right-click
requests received prompt Fusion32-authored descriptions, with zero unsupported
opcodes and anomalies on the live HUD. Body/container Look addresses have
exact-byte tests and compiled UI paths but no separately retained live click;
they remain `IMPLEMENTED_UNVERIFIED`. Use On Creature and Use With on a
world object likewise remain implemented paths without a separately retained
valid live result. The overall item-use milestone is `PASS` for its named
live world/body/container Use, valid object-object Use With, and world Look;
it is not `CERTIFIED` pending independent repetition.

To repeat from a checkout with local V08 import, catalog and sector cache:

```text
tests/build_clientcore_windows.cmd
Build.bat REAL33DEditor Win64 Development -Project=<repo>/unreal/REAL33D/REAL33D.uproject -WaitMutex
scripts/client/run_unreal_v08_experimental.cmd B <runtime> <cache> <catalog> <previews> 64
```

In the REAL33D window, Alt+right-click a visible world tile and an occupied
body/container slot. Retain the `look <id>` line, the subsequent server
`InfoMessage: You see ...` message, and an F9 snapshot after those requests.
Repeat ordinary right-click on a supported world object, backpack and
nested bag; use Shift+right-click on a MultiUse source and click a valid
target. Confirm server-authored container/message/item changes and zero
protocol counters. The world Look path and overall milestone have local
`PASS`; the slot Look variants and independent repetition remain for
`CERTIFIED`.

The next separate 3D milestone requested by the operator is
`UNREAL-LIVE-VISUAL-EDITOR-001`: select a visible world instance while
navigating, preview mesh replacement, rotation and grass presentation,
save reversible local visual overrides, and keep Fusion32 gameplay/state
untouched. The operator explicitly chose local presentation-only edits.
The current V08 inspector's `Girar 90` button only writes a `ROTATE_90`
verdict to a TSV note; it does not rotate a mesh. The next milestone should
reuse its selection data while adding an actual reversible visual override.

## Closeout checks - 2026-09-27 18:14 America/Guatemala

On the clean isolated worktree at `d351e9a9139fde0168d65f68d2731ceb4f05bfb1`,
`tests/build_clientcore_windows.cmd` was rerun under VS 2022 BuildTools x64:
C++17 and C++20 archives built, and all eight suites passed, including
`protocol772_player_state_tests`. Rebuilding `REAL33DEditor Win64
Development` after the refreshed ClientCore archive reported
`Result: Succeeded` (two incremental actions).

The local ignored Unreal log was checked against the three Look request and
server `InfoMessage` pairs above. The retained earlier F9 JSON was checked
separately and is still explicitly pre-Look; it records zero residual bytes,
unsupported opcodes and anomalies, without proving post-Look counters.
The live HUD screenshot after Look shows unsupported 0 and anomalies 0.
Prior certified evidence was rechecked for world corpse Use, body backpack
Use, nested-bag Use and flour-on-bucket Use With. There is still no retained
live slot Look, Use On Creature or Use With field result, so none is promoted.

`tests/secret_check.sh` passed all checks when invoked from WSL with explicit
`GIT_DIR` and `GIT_WORK_TREE` for this Windows-created worktree. Invoking it
without those variables printed a misleading PASS after Git repository
errors and was discarded. This closeout adds no gameplay code and makes no
new live gameplay certification claim.
