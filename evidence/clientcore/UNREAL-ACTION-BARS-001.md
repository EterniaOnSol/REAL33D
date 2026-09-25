# UNREAL-ACTION-BARS-001

Base: `bd15cc0a49d8182dc1cc3732b8487859f1662044`  
Date: 2026-09-23, America/Guatemala  
State: `IN_PROGRESS`

## Source inspection

- REAL33D2D `modules/game_actionbar/game_actionbar.lua::setupActionBar` creates
  34x34 slots and takes an item drop; `logics/ActionButtonLogic.lua` assigns
  item TypeIds and implements Use, SelectUseTarget and chatText. Its spell list,
  cooldown and count facilities are outside the Fusion32 7.72 state available
  to this client and were not ported.
- `reference/game/src/receiving.cc::CUseObject`, `CUseTwoObjects`,
  `CUseOnCreature`, `CTalk` define the existing 7.72 command shapes.
- `reference/game/src/info.cc::GetObject` uses the body slot or the open
  container number and `RNum`; body-slot `RNum` is ignored, container `RNum`
  selects the item. `clientcore::ResolveCarriedItem` follows those positions.
- REAL33D `SReal33DSlot` already starts a drag carrying a server slot; HUD
  already owns temporary use-with and Escape cancellation; bridge already
  builds use and TALK via ClientCore. The action bars now use those same paths.

## Implementation contract

The saved JSON contains 28 local preferences (bottom 12, left 8, right 8):
empty, item use, item use-with, or Say text. Item bindings retain only TypeId.
The game-thread view shades a bound item when neither worn nor in an open
container. The worker independently resolves the type in its current
WorldState just before sending, preferring body slots then open containers in
number/index order. An absent item emits a local notice and sends no command.
An item moved between slots still resolves; a closed container does not.

## Deterministic checks

`tests/build_clientcore_windows.cmd` in the VS 2022 x64 environment: C++17 and
C++20 library builds PASS, all eight suites PASS. The new resolver test covers
missing type, closed container, open-container stack index, body-slot
precedence, relocation back into the container and disappearance.

`Build.bat REAL33DEditor Win64 Development ... -WaitMutex`: `Result: Succeeded`
after the initial implementation; a final incremental build remains before
certification.

## Live run under review

Fusion32 querymanager/game/login were verified alive on 7173/7172/7171.
REAL33D account B connected as creature 1002. The first run log is
`unreal/REAL33D/Saved/Logs/REAL33D.log` (local only). Observed so far:

```
02:26:41 action slot 22 bound item 3270 mode use
02:26:49 action slot 22 use item 3270 request 2
02:26:53 action slot 22 item 3270 unavailable; nothing sent
02:26:57 use 6: object 2854 at body slot 3, open as container 0
02:27:15 action slot 25 bound text
02:27:15 action slot 25 talk request 8: exura
02:27:15 server: You must learn this spell first.
```

The text test proves TALK delivery and honest server rejection, not spell
success. The normal-use `3270` test was refused by Fusion32, so a valid
normal-use item still needs live coverage from an action slot. The local
`Saved/ActionBars.json` contains the assigned slot values; restart loading
still needs to be observed. Use-with, all three bars, clear/replace, and final
protocol counters remain under review. No commit or push until these pass.
