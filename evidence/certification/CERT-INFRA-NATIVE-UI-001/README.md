# CERT-INFRA-NATIVE-UI-001

Status: PASS for diagnosis and a bounded native mouse UI recovery probe.
Date: 2026-10-21 America/Guatemala (probe 2026-10-22 UTC).
Reviewer: Codex. Starting HEAD: b7d8e29dee7663de9b4360c031eb4255b279ff66.
Branch: certification/cert-infra-native-ui-001-20261021.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-stack-move-controls-20261021.
Production implementation: 1abb013bf711978b9439b6abe836672b5f4148ac, unchanged.
Main remains fbc0de992142e26cab0a80f813d2c09b10dc191a. No merge or push.

## Root cause

The apparent regression compared different mechanisms. The successful prior
certifications did not depend on a working Windows Computer Use native pipe.
They compiled temporary, opt-in certification drivers into Unreal, driving
actual Slate callbacks and input bindings from inside the process. Those
drivers were intentionally retired before the final production builds. The
recent stack-certification attempts instead depended exclusively on
`@oai/sky`, whose native pipe connection returned Windows error 2 before any
Unreal window discovery, focus or input occurred. Treating that unavailable
transport as proof that native UI interaction itself was unavailable was too
broad. This run corrects that conclusion with a real OS mouse probe.

The historical Outfit report explicitly says the Windows helper also failed
then (`failed to write kernel assets`, OS error 3), and explicitly excludes
OS input injection from its successful in-process results. The exact provider
lifecycle reason why this session's Computer Use pipe was absent is not
established. No claim that its service was repaired, or that it had previously
worked for those three certifications, is justified by the evidence.

## Historical comparison

| Aspect | Prior successful certifications | Current diagnosis/probe |
| --- | --- | --- |
| Launcher | Same installed UE_5.8 UnrealEditor.exe, `-game -windowed`, explicit project and runtime paths. Trade: 1800x1050, `-real33d-cert-trade`; Outfit: 1600x1000, `-real33d-cert-outfit`. Both archived launchers used Start-Process with Hidden. | Same engine and unchanged production DLL; 1280x720 visible window, no cert-driver switch. Runtime deliberately omitted for this harmless local panel probe. |
| Driver | Trade calls production slot OnMouseButtonDown/Up and Slate SimulateClick. Outfit drives real controller bindings, traces, menu and Slate callbacks. Editors uses SimulateClick/SetText; its report discloses a RequestUseObject entry call. | External Python/ctypes helper with EnumWindows, GetForegroundWindow, SetCursorPos and SendInput mouse down/up. No callbacks, game API, protocol calls or injected server events. |
| Driver retirement | All reports say the diagnostic sources were removed from production Source and archived as evidence; Editors records UBT source-removal invalidation. | Current Source has no temporary cert driver. Production diff versus 1abb013 is empty; no rebuild or diagnostic instrumentation. |
| Ownership/session | Historic reports/launchers do not retain a full Windows token/session inventory, so exact prior ownership is unknown. | Unreal PID 15684 belongs to DESKTOP-9SKKIUM\\dell, SessionId 1. Probe also session 1, WinSta0 / Default; input desktop Default. |
| Discovery | Prior drivers find widgets/actors inside Unreal rather than requiring an external top-level HWND. | Ready UnrealWindow HWND 1180672, visible, origin (320,216), client 1280x720. Current PID, never a stale hardcoded historical handle. |
| Foreground | Outfit previously had a cursor timing/focus error; driver changed to focus the viewport and trace immediately. | HWND 1180672 verified foreground before both actual mouse clicks and after each capture. |
| Isolation | Outfit retained an AppData Zen sandbox startup failure, then succeeded on an approved normal-desktop launch. This was distinct from its helper failure. | Probe launch/input explicitly ran outside the command sandbox. After startup, discovery succeeded both inside and outside it with identical HWND/session/desktop. No evidence that current window invisibility or desktop mismatch caused the sky pipe error. |
| Worktree/build | Separate cert-trade, cert-outfit and cert-editors worktrees; temporary diagnostic builds then exact production rebuilds. | Existing cert-stack worktree and existing exact-implementation DLL reused. Hash retained in binary-sha256.json. No stack implementation change. |

Primary historical references, all already present before this diagnosis:

- `evidence/clientcore/unreal-trade-ui-independent-20261020/README.md`,
  Independent method and provenance (lines 60-75), and `launch-live.ps1`.
- `evidence/clientcore/unreal-outfit-dialog-independent-20261020/README.md`,
  Authority and method (lines 97-105), retained attempts (113-134),
  diagnostic reproduction (156-164), retirement (173-175).
- `evidence/clientcore/unreal-outfit-dialog-independent-20261020/diagnostic/Real33DOutfitCertification.cpp`:
  SimulateClick lines 48/60; pointer handlers 107; viewport focus 146;
  controller input bindings 154. Its `launch-live.ps1` records the flags.
- `evidence/clientcore/UNREAL-EDITORS-UI-001-INDEPENDENT-20261019.md`,
  method (42-50), malformed UNC launcher correction (116-119), retirement
  and empty production diff (137-140).
- A local older helper also exists at the main workspace's ignored
  `build/unreal-item-use-interaction-001/build/live_ui.ps1`; it uses Win32
  cursor/mouse/key events and a hardcoded PID. It was inspected, not executed
  or reused with that stale PID. The new helper discovers the current window.

## Fix and actual native UI probe

`native_ui_probe.py` is certification tooling outside production Source.
It requires an explicit current PID, enumerates its windows, validates the
client coordinates, activates the single game window, and aborts if the exact
HWND is not foreground. Each click uses real SendInput mouse down/up with
success-count checking. It does not PostMessage to widgets, invoke Slate,
attach to Unreal memory, send packets, or bypass gameplay.

The existing production executable was launched normally with no runtime
directory, intentionally leaving the HUD offline. `application-state.txt`
records the expected missing-runtime message; this is not a live gameplay
test. All server-owned fields show unknown values. Fusion32 was not started
or edited. `SReal33DHUD::HandlePanelToggled` only toggles the local registry,
so Skills is suitable for a harmless input probe with no server side effects.

Observed and inspected screenshots:

1. `before.png`: Skills visible at the top of the sidebar, including Level,
   Experience, Hit Points, Mana and skill rows.
2. Real left mouse click at client (1174,11), on the observed Skills control.
   `skills-closed.png`: Skills disappears; Battle List moves to sidebar top.
   Tooltip independently reads Skills. The corresponding JSON records
   matching foreground HWND and actual click coordinates.
3. Same real click reopens Skills. `skills-reopened.png`: all Skills rows
   reappear above Battle List, restoring the initial panel state.

Both clicks were performed through Windows SendInput, not synthetic protocol
or direct application calls. No stack, inventory, Look, Use or UseWith action
was executed. A normal CloseMainWindow request subsequently closed this
probe's own Unreal process; the resulting EndPlay snapshot is retained.

Early discovery returned no windows while Unreal was starting (log at shader
initialization); it is retained as discovery-sandbox.json. Later ready-state
discovery works both ways. The early empty result is not an isolation failure.

## Reproduction

Use the existing launch.json project/arguments, with a fresh evidence path and
the installed UnrealEditor.exe. Launch on the normal interactive desktop.
Do not copy a diagnostic CPP into Source. Wait for the actual game window.
Use the installed Python 3.11 and Pillow:

```
python native_ui_probe.py --pid <current PID> --activate --capture before.png --record before.json
python native_ui_probe.py --pid <current PID> --activate --click <observed X> <observed Y> --capture closed.png --record closed.json
python native_ui_probe.py --pid <current PID> --activate --click <observed X> <observed Y> --capture reopened.png --record reopened.json
```

Run input in the normal desktop execution context with approved tool access.
Always observe the current geometry; (1174,11) belongs to this 1280x720 run.
The helper fails closed if the target cannot be discovered or focused.

## Disposition

Native UI probe PASS. READY_TO_RESUME_STACK_CERT=YES using the proven external
OS input path. This is readiness, not stack certification. Held modifiers,
drag routing, authoritative move counts, dialog cancellation and every other
stack-certification row remain pending; no deterministic suite was rerun.
The sky native-pipe failure remains unresolved, but it is no longer the only
available route to the application's real UI. No tests weakened, production
files changed, server data changed, or main merge performed.
