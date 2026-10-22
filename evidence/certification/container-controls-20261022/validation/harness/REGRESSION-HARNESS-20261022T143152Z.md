# REAL33D 3D regression harness

**Overall: FAIL** — 10 PASS, 1 FAIL, 0 SKIP.

A SKIP is not a pass. It records a check this run could not execute, and why.

## Provenance

| | |
|---|---|
| Run (UTC) | `20261022T143152Z` |
| Repository | `/c/Users/dell/Desktop/fusion32/build/cert-container-independent-20261022` |
| Branch | `certification/unreal-container-controls-independent-20261022` |
| HEAD | `2136e6f63dd12f73c2575a87d7b44af22aa0c66a` |
| HEAD subject | feat(unreal): wire the 7.72 container Close and Up controls |
| origin/main | `af251c9b2de0372c97ecbd128af0ada0c92bc166` |
| Merge base | `af251c9b2de0372c97ecbd128af0ada0c92bc166` |
| Ahead / behind origin/main | 1 / 0 |
| Uncommitted paths | 139 |
| Toolchain | WSL Ubuntu-26.04, g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |

Worktrees at run time:

```
C:/Users/dell/Desktop/fusion32                                                          2136e6f [milestone/unreal-container-controls-001]
C:/Users/dell/Desktop/fusion32/build/cert-chat-channels-20261018                        18b587d [certification/unreal-chat-channels-001-20261018]
C:/Users/dell/Desktop/fusion32/build/cert-container-controls-20261022                   2136e6f [certification/unreal-container-controls-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-container-independent-20261022                2136e6f [certification/unreal-container-controls-independent-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-diagonal-input-20261022                       73c01e3 [certification/unreal-diagonal-input-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-editors-ui-20261019                           76a2619 [certification/unreal-editors-ui-001-20261019]
C:/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022                       af251c9 [certification/unreal-login-combined-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-login-handover-20261022                       16cfa3d [certification/unreal-login-handover-001-20261022]
C:/Users/dell/Desktop/fusion32/build/cert-outfit-dialog-20261020                        25c4448 [certification/unreal-outfit-dialog-001-20261020]
C:/Users/dell/Desktop/fusion32/build/cert-skills-panel-20261017                         ca3121c [certification/unreal-skills-panel-001-20261017]
C:/Users/dell/Desktop/fusion32/build/cert-stack-move-controls-20261021                  a868572 [certification/stack-live-sendinput-20261021]
C:/Users/dell/Desktop/fusion32/build/cert-trade-ui-20261020                             2428a14 [certification/unreal-trade-ui-001-20261020]
C:/Users/dell/Desktop/fusion32/build/cert-unreal-world-presentation-polish-001-20261015 bef902c [certification/unreal-world-presentation-polish-001-20261015]
C:/Users/dell/Desktop/fusion32/build/clientcore-channels-772-001                        9b950af [milestone/clientcore-channels-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-editors-772-001                         842fd16 [milestone/clientcore-editors-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-movement-actions-772-001                c666b9b [milestone/clientcore-movement-actions-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-outfit-772-001                          b8e7433 [milestone/clientcore-outfit-772-001]
C:/Users/dell/Desktop/fusion32/build/clientcore-trade-772-001                           fa6c6f6 [milestone/clientcore-trade-772-001]
C:/Users/dell/Desktop/fusion32/build/integration-item-use                               af251c9 [main]
C:/Users/dell/Desktop/fusion32/build/real33d-3d-regression-harness-001                  3443e1b [milestone/real33d-3d-regression-harness-001]
C:/Users/dell/Desktop/fusion32/build/unreal-item-use-interaction-001                    a97cf25 [milestone/unreal-item-use-interaction-001]
C:/Users/dell/Desktop/fusion32/build/unreal-minimap-navigation-001                      ea53ecc [milestone/unreal-minimap-navigation-001]
C:/Users/dell/Desktop/fusion32/build/unreal-skills-panel-001                            63a62dd [milestone/unreal-skills-panel-001]
C:/Users/dell/Desktop/fusion32/build/unreal-world-presentation-polish-001               bc9ce8f [milestone/unreal-world-presentation-polish-001]
```

Uncommitted paths at run time:

```
 M unreal/REAL33D/Content/Visual/obj3501/decoration_bottom_overlay_obj3501_mailbox/Materials/M_3501_metal.uasset
 M unreal/REAL33D/Content/Visual/obj3501/decoration_bottom_overlay_obj3501_mailbox/Materials/M_3501_organic.uasset
 M unreal/REAL33D/Content/Visual/obj3501/decoration_bottom_overlay_obj3501_mailbox/StaticMeshes/decoration_bottom_overlay_obj3501_mailbox.uasset
 M unreal/REAL33D/Content/Visual/obj3501/decoration_bottom_overlay_obj3501_mailbox/Textures/3501.uasset
 M unreal/REAL33D/Resources/UI/ActionBar/actionbar_background-light.png
 M unreal/REAL33D/Resources/UI/ActionBar/actionbarslot.png
 M unreal/REAL33D/Resources/UI/ActionBar/arrow-left.png
 M unreal/REAL33D/Resources/UI/ActionBar/arrow-right.png
 M unreal/REAL33D/Resources/UI/ActionBar/background-dark.png
 M unreal/REAL33D/Resources/UI/ActionBar/border_activespell.png
 M unreal/REAL33D/Resources/UI/ActionBar/double-arrow-left.png
 M unreal/REAL33D/Resources/UI/ActionBar/double-arrow-right.png
 M unreal/REAL33D/Resources/UI/ActionBar/locked.png
 M unreal/REAL33D/Resources/UI/ActionBar/marker-multiactionbutton.png
 M unreal/REAL33D/Resources/UI/ActionBar/splitterActBottom.png
 M unreal/REAL33D/Resources/UI/Automap/automap_buttons.png
 M unreal/REAL33D/Resources/UI/Automap/automap_indicator_maplayers.png
 M unreal/REAL33D/Resources/UI/Automap/automap_indicator_slider_left.png
 M unreal/REAL33D/Resources/UI/Automap/automap_phantom.png
 M unreal/REAL33D/Resources/UI/Automap/automap_rose.png
 M unreal/REAL33D/Resources/UI/Automap/timedisplay_scroll.png
 M unreal/REAL33D/Resources/UI/Battle/icon-battlelist-monster.png
 M unreal/REAL33D/Resources/UI/Battle/icon-battlelist-npc.png
 M unreal/REAL33D/Resources/UI/Battle/icon-battlelist-party.png
 M unreal/REAL33D/Resources/UI/Battle/icon-battlelist-players.png
 M unreal/REAL33D/Resources/UI/Battle/icon-battlelist-skull.png
 M unreal/REAL33D/Resources/UI/Battle/icon-battlelist.png
 M unreal/REAL33D/Resources/UI/Chrome/1pixel_down_frame.png
 M unreal/REAL33D/Resources/UI/Chrome/2pixel_up_frame_borderimage.png
 M unreal/REAL33D/Resources/UI/Chrome/3pixel_frame_borderimage.png
 M unreal/REAL33D/Resources/UI/Chrome/background.png
 M unreal/REAL33D/Resources/UI/Chrome/background_dark.png
 M unreal/REAL33D/Resources/UI/Chrome/button.png
 M unreal/REAL33D/Resources/UI/Chrome/console_button.png
 M unreal/REAL33D/Resources/UI/Chrome/container-browse-left-down.png
 M unreal/REAL33D/Resources/UI/Chrome/container-browse-left-up.png
 M unreal/REAL33D/Resources/UI/Chrome/container-browse-right-down.png
 M unreal/REAL33D/Resources/UI/Chrome/container-browse-right-up.png
 M unreal/REAL33D/Resources/UI/Chrome/containerslot.png
 M unreal/REAL33D/Resources/UI/Chrome/ditherpattern.png
 M unreal/REAL33D/Resources/UI/Chrome/item.png
 M unreal/REAL33D/Resources/UI/Chrome/miniborder.png
 M unreal/REAL33D/Resources/UI/Chrome/miniwindow_body.png
 M unreal/REAL33D/Resources/UI/Chrome/miniwindow_buttons.png
 M unreal/REAL33D/Resources/UI/Chrome/miniwindow_header.png
 M unreal/REAL33D/Resources/UI/Chrome/panel_map.png
 M unreal/REAL33D/Resources/UI/Chrome/topstats_button_panel.png
 M unreal/REAL33D/Resources/UI/Chrome/vertical_line_dark.png
 M unreal/REAL33D/Resources/UI/Cursors/targetcursor.png
 M unreal/REAL33D/Resources/UI/HealthMana/hitpoints_bar_filled.png
 M unreal/REAL33D/Resources/UI/HealthMana/hitpoints_manapoints_bar_border.png
 M unreal/REAL33D/Resources/UI/HealthMana/hitpoints_symbol.png
 M unreal/REAL33D/Resources/UI/HealthMana/mana_bar_filled.png
 M unreal/REAL33D/Resources/UI/HealthMana/mana_symbol.png
 M unreal/REAL33D/Resources/UI/Inventory/button_blessings_grey.png
 M unreal/REAL33D/Resources/UI/Inventory/buttons_general.png
 M unreal/REAL33D/Resources/UI/Inventory/containerslot.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_back.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_feet.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_finger.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_head.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_hip.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_left_hand.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_legs.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_neck.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_right_hand.png
 M unreal/REAL33D/Resources/UI/Inventory/inventory_torso.png
 M unreal/REAL33D/Resources/UI/Inventory/max_button_small.png
 M unreal/REAL33D/Resources/UI/Inventory/min_button_small.png
 M unreal/REAL33D/Resources/UI/Inventory/purse.png
 M unreal/REAL33D/Resources/UI/Options/button_battlelist.png
 M unreal/REAL33D/Resources/UI/Options/button_control.png
 M unreal/REAL33D/Resources/UI/Options/button_empty.png
 M unreal/REAL33D/Resources/UI/Options/button_enlarge.png
 M unreal/REAL33D/Resources/UI/Options/button_logout.png
 M unreal/REAL33D/Resources/UI/Options/button_options.png
 M unreal/REAL33D/Resources/UI/Options/button_skills.png
 M unreal/REAL33D/Resources/UI/Options/button_vip.png
 M unreal/REAL33D/Resources/UI/Options/skills.png
 M unreal/REAL33D/Resources/UI/Sliced/arrow_first.png
 M unreal/REAL33D/Resources/UI/Sliced/arrow_last.png
 M unreal/REAL33D/Resources/UI/Sliced/arrow_next.png
 M unreal/REAL33D/Resources/UI/Sliced/arrow_previous.png
 M unreal/REAL33D/Resources/UI/Sliced/automap_fullmap.png
 M unreal/REAL33D/Resources/UI/Sliced/automap_layers.png
 M unreal/REAL33D/Resources/UI/Sliced/automap_rose.png
 M unreal/REAL33D/Resources/UI/Sliced/automap_zoomin.png
 M unreal/REAL33D/Resources/UI/Sliced/automap_zoomout.png
 M unreal/REAL33D/Resources/UI/Sliced/battle_filter_idle.png
 M unreal/REAL33D/Resources/UI/Sliced/battle_filter_on.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_close.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_close_pressed.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_container_up.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_context_menu.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_filter.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_lock_open.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_lock_shut.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_minimize.png
 M unreal/REAL33D/Resources/UI/Sliced/chrome_restore.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_burning.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_drunk.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_electrified.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_hasted.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_logoutblocked.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_manashield.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_poisoned.png
 M unreal/REAL33D/Resources/UI/Sliced/condition_slowed.png
 M unreal/REAL33D/Resources/UI/Sliced/control_battlelist.png
 M unreal/REAL33D/Resources/UI/Sliced/control_battlelist_on.png
 M unreal/REAL33D/Resources/UI/Sliced/control_control.png
 M unreal/REAL33D/Resources/UI/Sliced/control_control_on.png
 M unreal/REAL33D/Resources/UI/Sliced/control_logout.png
 M unreal/REAL33D/Resources/UI/Sliced/control_logout_on.png
 M unreal/REAL33D/Resources/UI/Sliced/control_options.png
 M unreal/REAL33D/Resources/UI/Sliced/control_options_on.png
 M unreal/REAL33D/Resources/UI/Sliced/control_skills.png
 M unreal/REAL33D/Resources/UI/Sliced/control_skills_on.png
 M unreal/REAL33D/Resources/UI/Sliced/control_vip.png
 M unreal/REAL33D/Resources/UI/Sliced/control_vip_on.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_attack.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_attack_on.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_balanced.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_balanced_on.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_defend.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_defend_on.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_follow.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_follow_on.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_grow.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_purse.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_shrink.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_stand.png
 M unreal/REAL33D/Resources/UI/Sliced/inv_stand_on.png
 M unreal/REAL33D/Resources/UI/Sliced/panel_left_less.png
 M unreal/REAL33D/Resources/UI/Sliced/panel_left_more.png
 M unreal/REAL33D/Resources/UI/Sliced/panel_right_less.png
 M unreal/REAL33D/Resources/UI/Sliced/panel_right_more.png
 M unreal/REAL33D/Resources/UI/Sliced/tab_idle.png
 M unreal/REAL33D/Resources/UI/Sliced/tab_selected.png
 M unreal/REAL33D/Resources/UI/States/player-state-flags.png
```

## Results

| Check | Status | Detail | Log |
|---|---|---|---|
| `provenance` | **PASS** | branch certification/unreal-container-controls-independent-20261022 at 2136e6f6, 1 ahead / 0 behind origin/main, 139 uncommitted path(s) | `logs/provenance-status.txt` |
| `branch-isolation` | **PASS** | on certification/unreal-container-controls-independent-20261022, not on main | — |
| `clientcore-tests` | **PASS** | 14 CTest suite(s) passed via wsl | `logs/clientcore-tests.txt` |
| `real33d-native-tests` | **PASS** | 10 passed, 0 failed via wsl; no --objects-srv, so the runtime MultiUse assertions were not exercised | `logs/real33d-native-tests.txt` |
| `outfit-color-mapping` | **PASS** | 43 byte assertion(s) hold; Colors[0..3] = Head, Body, Legs, Feet | `logs/outfit-color-mapping.txt` |
| `outfit-palette` | **PASS** | 51 assertion(s) hold; 133 colours, 7x19, derived not tabulated | `logs/outfit-palette.txt` |
| `secret-check` | **PASS** | 6 sub-check(s) ok | `logs/secret-check.txt` |
| `build-sanity-msvc` | **PASS** | C++17 suites and the C++20 archive Unreal links both built | `logs/build-sanity-msvc.txt` |
| `unreal-module-build` | **FAIL** | UnrealBuildTool exited 6 | `logs/unreal-module-build.txt` |
| `protocol-log-check` | **PASS** | 7 log(s) scanned, no unsupported opcodes or residual bytes | `logs/protocol-log-check.txt` |
| `output-secret-scan` | **PASS** | logs carry no key material (pattern self-tested) | `logs/output-secret-scan.txt` |

## What each check covers

- `provenance` — branch, HEAD, origin/main, merge base, ahead/behind, dirty paths and the full worktree list, captured before anything else runs.
- `branch-isolation` — refuses to certify a run sitting on `main`.
- `clientcore-tests` — the Protocol772Core CTest suites: transport, crypto, login, gamelogin, initial_world, movement, player_state, worldview, minimap.
- `real33d-native-tests` — the header-only 3D unit tests under `tests/real33d_*.cpp`: mouse gestures, minimap view, item-use/MultiUse classification, stackable sprites, chat views, outfit selection, stack-move controls.
- `secret-check` — `tests/secret_check.sh` over the tracked tree and the whole reachable history.
- `build-sanity-msvc` — `tests/build_clientcore_windows.cmd`: the C++17 suites plus the C++20 archive the Unreal module links.
- `unreal-module-build` — UnrealBuildTool on REAL33DEditor. Opt-in with `--unreal`.
- `protocol-log-check` — `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, plus the bridge diagnostics that say a command was left unwalked.
- `output-secret-scan` — the harness re-reads everything it just wrote and fails rather than leave key material or a credential assignment on disk.

## Scope and limits

- Nothing here proves live behaviour. No Fusion32 server is started, no client is launched, no gameplay and no protocol is modified. Live certification stays a separate, operator-driven step.
- Read-only against the repository. Everything written goes under `build/certification/harness`, inside the gitignored `build/` tree.
- No environment values are printed, and `output-secret-scan` is the last word on what was left on disk.
