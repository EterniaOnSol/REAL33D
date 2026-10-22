# UNREAL-TURN-OBJECT-001

2026-10-22, America/Guatemala. Work branch `milestone/unreal-turn-object-001-rebased`
from current `origin/main` (`119dc2e`), which already contains the previous
milestone (`2136e6f`). No merge to main. No push.
**No protocol and no server change.**

STATUS = **implementation and offline verification PASS.** The end-to-end live
rotation QA is **UNVERIFIED LIVE** in this environment for one concrete reason
given below, not for want of trying: no Fusion32 session was driven and no GUI
client was launched here, so nothing in this document claims a live rotation
was observed.

## What existed before

`CL_CMD_TURN_OBJECT` (133) was already built and tested in ClientCore
(`BuildTurnObjectCommand`, `movement_actions_tests.cpp`) and **used nowhere** in
Unreal. There was no Rotate entry on any object, and the bridge had no entry
point to send one.

## Implemented

- `UReal33DBridge::RequestTurnObject(const FMoveSlot&, uint16 TypeId,
  uint8 StackIndex)`. It queues an `FUse` of the new kind `Turn`, which the
  worker drains in `DrainUses` straight into the existing ClientCore builder
  `BuildTurnObjectCommand`. One new counter, `TurnsRequested`, incremented only
  when the command actually reaches the wire, so a live run can count what went
  out — the same shape as `LooksRequested`/`UsesRequested`.
- **Rotate on the slot context menu** (`ShowSlotContextMenu`), the existing
  ctrl+right-click object menu for inventory and container objects, beside Look
  and Trade.
- **A world object context menu** (`ShowWorldObjectContextMenu`) offering Look,
  Use and Rotate for the top object on a map field, opened by **ctrl+right-click
  on the world** — the map counterpart of the slot ctrl+right-click menu, since
  most rotatable 7.72 objects (lamps, torches, some decorations) sit on the map
  rather than in a bag.

### Routing is by the object reference the player pointed at

Rotate carries the same five-field reference every other object command carries:
x/y/z, type id, stack index, encoded by the one `ToEndpoint` path ClientCore
owns. The field and stack the player right-clicked are exactly what the server
resolves. Unlike move and trade, `CTurnObject` does **not** overwrite the stack
index for a map object, so the index sent is the index that resolves; this is
asserted byte-for-byte in `movement_actions_tests.cpp`
(`85 617d db7d 07 260b 02`, and the stack-index-preserved case).

### Server-authoritative; nothing turns locally

Rotate is an intent, exactly like use and look. Nothing is rotated or mutated on
screen. The object's new orientation arrives as the ordinary field or container
update Fusion32 sends afterwards, and a refused rotate produces none — which is
correct, because nothing changed.

### "Only when applicable", without predicting the server

The entry is offered whenever there is an actual object to act on (a real slot
item, or a tile with a top object) and a live bridge to ask. Whether the object
is *rotatable* is **not** pre-filtered: `CTurnObject` itself carries the open
question `// TODO(fusion): Check Type.getFlag(ROTATE)?`, so a non-rotatable
object is Fusion32's to refuse, and this client follows the same no-local-
prediction rule it applies to every other object action (look, use, trade). A
map container is likewise the server's to ignore. Guessing either here is the
invention this project's architecture forbids.

### No regression to Look / Use / UseWith / drag-drop / stacks

Every change is additive. The slot menu gains one entry; the world gains a new
ctrl+right-click menu while a **plain** right-click still uses the object
directly, so the Use gesture is untouched. `Real33DMouseGesture` and its
header-only test are unchanged — Ctrl is read ad hoc at interaction time exactly
as Shift/Alt already are, so the gesture contract and its 10/10 native suite
still hold. A pending use-with still consumes the world click as its target
rather than opening a menu (`!IsTargeting()` guard). The drag/drop and
stack-move routes were not touched.

## Offline verification (this environment)

| Check | Result |
|---|---|
| ClientCore CTest (WSL Ubuntu-26.04, g++) | **14/14** — transport, crypto, login, gamelogin, initial_world, movement, player_state, worldview, movement_actions, trade, channels, editors, outfit, minimap |
| ClientCore native MSVC (`build_clientcore_windows.cmd`, VS2022 BuildTools, engine OpenSSL) | **WINDOWS CLIENTCORE: PASS** — C++17 suites + C++20 archive Unreal links; 14/14 suites, `movement_actions` golden bytes incl. TURN_OBJECT |
| `regression_harness.sh` | **0 FAIL** (PASS_WITH_SKIPS): clientcore-tests 14, real33d-native-tests 10/0, secret-check 6, outfit-color 43, outfit-palette 51, protocol-log-check 91 logs/0 anomalies, output-secret-scan clean; build-sanity-msvc and unreal-module-build SKIP on the harness's own PATH (run separately below) |
| `secret_check.sh` | **6/6 ok** |
| Unreal module build (`Build.bat REAL33DEditor Win64 Development`, VS2022 BuildTools, UE 5.8) | **Result: Succeeded** — 25/25 actions, `UnrealEditor-REAL33D.dll` linked, 0 warnings / 0 errors. `Real33DBridge.cpp`, `Real33DHUD.cpp` and `Real33DPlayerController.cpp` all compiled individually (non-unity) |

## Remaining UNVERIFIED LIVE

No Fusion32 server was started and no REAL33D GUI client was launched or driven
in this environment, so the following — each of which needs the operator-driven
live step this repo documents (`scripts/server/start_wsl.sh` + a human or input
automation at the GUI) — were **not** demonstrated here and must not be read as
observed:

- Rotating at least two different legitimate rotatable objects.
- The authoritative server update actually changing the object's orientation.
- The correct tile/stack object being targeted end to end on the wire.
- Blocked / non-rotatable behaviour (the server's refusal path).
- Adjacent vs non-adjacent (range) behaviour where the server permits.
- Exactly one send per activation, and zero protocol errors / anomalies /
  unsupported opcodes on a **new** live session.

What *is* proven offline is the wire format and the routing: the byte layout and
stack-index handling of `BuildTurnObjectCommand` are golden-asserted, the bridge
path from `RequestTurnObject` to that builder is in place and compiles, and the
UI offers Rotate only on a real object through a live bridge. What is missing is
a live case to point it at, which is the same class of gap the previous
milestone recorded and left for the operator-driven certification step.
