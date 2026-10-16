# UNREAL-HUD-PANEL-REGISTRY-001

Status: `PASS`. Architectural only. No protocol, gameplay, server or visual
change: the HUD draws exactly what it drew before, from the same panels, in
the same places.

Base `origin/main` at `1a15ddbd79a23a2841d882584afaa82fabd9f74d`, developed on
`milestone/unreal-hud-panel-registry-001`. Not merged.

## Parallel safety

Checked before any edit, and again at the end because `origin/main` moved
mid-milestone (the stackable milestone was certified and merged as
`ab54527`). The two change sets are disjoint both times:

| Stackable milestone touched | This milestone touches |
|---|---|
| `Real33DUIStyle.h/.cpp` | `Real33DHUD.h/.cpp` |
| `Real33DPanelChrome.cpp` | `Real33DPresentationTests.cpp` |
| `Real33DBridge.h/.cpp` | `Real33DPanelRegistry.h/.cpp` (new) |
| `Real33DStackSprite.h`, `extract_item_sprites.py`, `tests/regression_harness.sh`, `tests/real33d_stackable_sprite_tests.cpp` | — |

No file appears on both sides. `tests/regression_harness.sh` in particular was
left alone, which is why the new test is an Unreal automation suite rather
than another `tests/real33d_*.cpp` stem needing registration there.

## Architecture before

Adding a panel meant editing `SReal33DHUD` in four places:

1. a forward declaration and a `TSharedPtr` member for the panel;
2. for a toggleable panel, a second `TSharedPtr<SWidget>` member for its
   window (`SkillsWindow`, `BattleWindow`);
3. a twelve-to-fourteen-line block in `Construct` repeating the slot, the
   mini-window chrome and the content;
4. a branch in `HandlePanelToggled`, a hand-written chain mapping a name to
   one of those members:

```cpp
const TSharedPtr<SWidget> Target = Panel == TEXT("Skills")
    ? SkillsWindow : (Panel == TEXT("Battle") ? BattleWindow : nullptr);
```

Every UI milestone had to pass through this one file, and the chain grew a
ternary per toggleable panel. That is the bottleneck this removes.

## Architecture after

`FReal33DPanelRegistry` (new, `Public/Real33DPanelRegistry.h`) holds the
panels and is the only thing that opens or closes them. `SReal33DHUD` owns one
as a member and exposes it through `GetPanels()`.

`SReal33DHUD::AddPanel` builds the standard mini window, adds it to a column
and registers it, so the four steps above collapse into one call plus, where
the HUD needs to push values into the panel each frame, the existing panel
pointer.

Two design choices worth stating, because both could have gone the other way:

- **Open state is not cached.** `IsOpen` reads the widget's own visibility.
  A bool in the registry would be a second source of truth for something
  Slate already knows, and the two would eventually disagree. The suite
  asserts this by changing a widget's visibility behind the registry's back
  and checking the registry reports the change.
- **The registry does not own the widgets** in the Slate sense; the widget
  tree does, through the slots they were added to. It keeps a shared pointer
  so lookups stay valid, and lives as a HUD member so it cannot outlive the
  tree it points into. `Reset()` forgets panels and deliberately leaves the
  widgets alone; the suite asserts that too.

A duplicate id is refused rather than silently replacing the first, and an
unknown id is inert — `Toggle` returns false and changes nothing, exactly as
the old if-chain's `nullptr` branch did, so a control naming a panel the HUD
does not carry still does nothing rather than failing.

## Registration API

```cpp
// in Construct, once per panel:
AddPanel(Inner, Real33D::PanelIds::Skills,
    FText::FromString(TEXT("Skills")), "Real33D.Control.SkillsIcon",
    SReal33DSkillsPanel::DesiredContentHeight(),
    SAssignNew(Skills, SReal33DSkillsPanel));

// from anywhere holding the HUD:
Hud->GetPanels().Toggle(Real33D::PanelIds::Skills);
Hud->GetPanels().Open(Id);   Hud->GetPanels().Close(Id);
Hud->GetPanels().IsOpen(Id); Hud->GetPanels().Find(Id);
Hud->GetPanels().GetIds(OutIds);
```

Ids live in `Real33D::PanelIds` so the control strip, the registry and the HUD
cannot drift apart over a typo. `Skills` and `Battle` keep the exact spellings
`SReal33DControlPanel` already emits.

**A future panel needs: its own panel file, one `PanelIds` entry, and one
`AddPanel` call.** No new HUD member unless the HUD must push values into it
each frame, and no toggle edit at all.

## What changed, concretely

All six existing mini windows were moved onto `AddPanel` and are now
registered: Minimap, Health, Inventory, Skills, Battle, Containers. The two
bespoke `SkillsWindow` / `BattleWindow` members are gone, replaced by the
registry. `HandlePanelToggled` is one line.

| | before | after |
|---|---|---|
| `Real33DHUD.cpp` | 784 lines | 759 lines |
| `Construct()` | 230 lines | 189 lines |
| `HandlePanelToggled()` body | 11 lines | 1 line |
| toggleable-panel members in the HUD | 2 | 0 |

`Real33DHUD.h` grew by 28 lines, almost all of it documentation for the new
API. The honest summary is that the `.cpp` shrank only 25 net lines because
`AddPanel`'s own body lives in that same file; the reduction that matters is
structural — the per-panel cost and the toggle chain — not the raw count.

## Behaviour preservation

The refactor is intended to be a no-op on screen, and the mechanical risks
were checked rather than assumed:

- `SReal33DMiniWindow`'s `_IconBrush` default is `NAME_None`, so passing
  `NAME_None` for the five panels that never set an icon is exactly what they
  had before.
- Every panel keeps its original title, content height, icon and content
  widget, and is added to the same column in the same order.
- `Register` does not touch visibility, so each panel starts exactly as it was
  built. Nothing applies a default state at construction.
- `Toggle` flips Collapsed to Visible and back, the same two values and the
  same direction the old chain used.
- Inventory, containers, chat, vitals, minimap and battle all keep their
  existing panel pointers and their `Refresh` path, which was not touched.

## Tests

`REAL33D.Presentation.PanelRegistry`, added to the existing automation suite
so it runs under the gate that already exists:

empty-registry behaviour and four inert operations on an unknown id;
registration and lookup; duplicate id refused without replacing or growing the
list; empty id refused; open/close/toggle round trips; closed state asserted
to be `Collapsed` specifically, so the column reclaims the space; visibility
changed behind the registry's back and still reported correctly; registration
order preserved in `GetIds`; panels independent of one another;
unregister, double-unregister, and `Reset` leaving the widget untouched.

The suite earned its keep twice while being written. It first failed because
the test used `SNullWidget::NullWidget` as a stand-in panel — Slate ensures on
any attempt to mutate that shared singleton — and then failed again because it
asserted a freshly built `SBox` reports exactly `EVisibility::Visible`, which
is the widget's business and not the registry's. Both were defects in the
test, not the registry, and both are fixed.

## Results

| Gate | Result |
|---|---|
| Unreal UI/presentation suites | **4/4 Success**, 0 errors: `PanelRegistry`, `CameraAndFloors`, `WideWorld.AuthoritativeWindow`, `AssetRegistry.ExperimentalV08` |
| `REAL33DEditor Win64 Development` | **`Result: Succeeded`** |
| ClientCore CTest | **13/13 PASS**, unchanged |
| `real33d-native-tests` | 3 passed, 0 failed |
| MSVC | `WINDOWS CLIENTCORE: PASS` |
| Regression harness | **`PASS_WITH_SKIPS`, 0 FAIL**; only `protocol-log-check` SKIP |
| `secret_check` | PASS, 6 sub-checks, from Git Bash |

## Found but not fixed here: a second harness false PASS

`tests/regression_harness.sh` can report `clientcore-tests` **PASS** when the
suites did not run. Observed directly during this milestone: a run whose repo
root resolved to a path WSL could not reach recorded

```
| `clientcore-tests` | **PASS** | 0 CTest suite(s) passed via wsl |
```

while its own log held

```
CMake Error: The source directory "...": does not exist.
```

Same class as the `cmd.exe` defect fixed in
`REAL33D-3D-REGRESSION-HARNESS-001`: the check trusts an exit status that did
not survive the hop into WSL, and "0 suites passed" is reported as success
rather than treated as the contradiction it is. The fix belongs on the harness
branch, not here — this milestone is forbidden from touching that file, and
did not. A `clientcore-tests` PASS should require a suite count above zero.

The correct run for this milestone, with a resolvable repo root, reports 13
CTest suites and 3 native tests and is the one quoted in the table above.
