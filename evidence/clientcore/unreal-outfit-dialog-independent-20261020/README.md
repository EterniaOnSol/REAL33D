# UNREAL-OUTFIT-DIALOG-001 independent certification

Review date: 2026-10-20. Reviewer: Codex, independent execution in a fresh
worktree. Original candidate: `56bf508537ba9123edf09dcf81653b5218f8ee6e`.
UI correction: `61ceaa955933672c526864b5ed24dca0e4c8de84`.
Base/main: `b8e7433baeddc2875943f519d33a5cb9380f3eaa`.
Branch: `certification/unreal-outfit-dialog-001-20261020`.
Worktree: `C:/Users/dell/Desktop/fusion32/build/cert-outfit-dialog-20261020`.

The implementation branch was absent remotely and was pushed normally to
`origin/milestone/unreal-outfit-dialog-001` before certification. The worktree
was created at the full resolved candidate hash. No merge, rebase, cherry-pick
or force operation. One real UI defect was found and corrected on the isolated
certification branch (details below). The published milestone remains the exact
original candidate. Temporary diagnostics are archived separately and retired
before the final production-source build.

## Defect found in the original candidate

`live-original/` passed all 68 executed functional assertions: GET 4, server
chooser 4, SET 2, authoritative update 2, protocol counters zero. Its screenshots
nevertheless show **all ten stepper arrows blank** in opened, selected and
reopened states. Functional activation by Slate does not establish usability.

The cause is deterministic: `Real33DOutfitPanel.cpp` fixes each stepper at
32 Slate units. UE 5.8 `SButton` adds default horizontal content padding 4+4
to Starship's normal/pressed style padding 12+12 (`CoreStyle.cpp::ButtonMargins`,
`FStarshipCoreStyle::SetupButtonStyles`, `SButton::GetCombinedPadding`). The
sum is 32, leaving zero width for the label. This is an implementation defect,
not the missing RGB renderer or a protocol defect.

Commit `61ceaa9` overrides only normal/pressed stepper style padding to 2 units
on each side, retaining vertical normal/pressed padding. Together with existing
content padding, labels have 20 units within the same 32-unit buttons. Nine
added lines in one production file; no commands, bridge, server or semantics
changed. The original candidate is **not approved for merge as-is**. The
corrected revision is assessed separately. The live diagnostic now also checks
each of the ten actual label geometries against its glyph's desired size;
screenshots, not that geometry assertion alone, establish visible arrows.

## Independent live results

Corrected run: `live-fixed/REAL33D.log`, 2026-10-21 04:15-04:16 UTC
(2026-10-20 locally), real baseline Fusion32, account B/creature 1002,
1600x1000 window. **90 executed assertions PASS**, independently audited by
`diagnostic/audit_live.py`; see `live-fixed/audit.json`. No failed assertion.
Original run: `live-original/`, 68 functional assertions PASS with the visual
defect above. The correction's screenshots were independently viewed: the
numeric chooser is readable and all ten `<` / `>` labels are visible.

| Check | Observed result |
|---|---|
| Entry | Real cursor collision identifies own creature; actual right-button press/release bindings open self menu; the real Set outfit menu action requests the chooser |
| Server-only open | Initially collapsed; request leaves it closed with unchanged revision; received 200 opens it |
| GET_OUTFIT 210 | Four actual pre-transport command buffers, each `d2`; four server choosers |
| Server band/preview | Exact server First=136, Last=139; six forward steps and one backward step, including wrap, stay in band; no speculative actor change |
| Four colours | Each named arrow changes only its indexed draft byte; all four differ on the outgoing command |
| SET_OUTFIT 211 | `d3 88 00 45 3c 32 43`: id 136, Head 69, Body 60, Legs 50, Feet 67 |
| Authority/reopen | Received creature-outfit update yields that exact actor tuple; next server chooser and draft reproduce all five values |
| Cancel | Changes an unsent draft, cancels without another SET, leaves actor unchanged; next reopen discards the cancelled draft |
| Restoration | `d3 8b 00 44 3b 31 42`: original id 139, colours 68/59/49/66; authoritative update and final server reopen confirm it |
| Registry/overlay | Ten registered surfaces; exact outfit widget under Outfit; open/closed registry state; SelfHitTestInvisible overlay; Slate hit-test reaches Channel List outside the centred panel |
| Bounded UI regression | Real channel list request/response while chooser remains open; backpack slot right-click opens server container; slot Look runs; existing HUD remains visible |
| Arrow correction | Each of ten labels allocated 20.00 x 15.12, desired 6.48 x 15.12; visible in opened/selected/reopened screenshots |
| Protocol | GET=4, received=4, SET=2, updates=2, refused=0; commands=87; residual=0, unsupported=0, anomalies=0 |

The wire trace records only outfit command bytes immediately before the
ordinary send path (not encrypted network frames). Independent server replies
and authoritative actor/reopen state corroborate transport and persistence.
Zero refusals means none occurred; it does not certify refusal handling.
The panel is a centred dialog with background hit testing preserved, not a
global input-blocking modal. No claim that it freezes world input.

## Authority and method

Fusion32 game revision `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, selected in
`docs/protocol772/SOURCE_TRUTH.md`, is the authority:

- `reference/game/src/receiving.cc::CGetOutfit` (1390): GET has no payload;
  the gamemaster restriction sends a message and no chooser.
- `CSetOutfit` (1410): word id, then four bytes unconditionally; sex and
  premium determine the band; every colour must be <=132. No acknowledgement.
  `OrgOutfit` is stored; the appearance broadcast is conditional on the old
  `Outfit == OrgOutfit`.
- `reference/game/src/sending.cc::SendOutfit` (1661): server command 200,
  current original outfit, First, Last. Shared outfit encoding (162) branches
  between object type and four colours; `SendCreatureOutfit` (1011) uses it.
- These handlers have no local version conditional. The 7.72 connection
  version/handshake guard is `communication.cc`, `#if TIBIA772`.

The existing closed verifier reads the hash-locked classic executable and
asserts 43 byte patterns across label selection, slot storage, palette store,
server load, OK and ascending wire send. It was not edited. No legacy binary
was executed. Its result is **Colors[0..3] = Head/Body/Legs/Feet**.
Executable SHA256:
`1a82fe97a39e536c29327d6d5dde316775f537dcc01506831429b1a0433b76f8`.

The Windows computer-use helper failed to initialize (`failed to write kernel
assets`, OS error 3). Live diagnostics therefore operate inside Unreal:
real collision traces, the production controller's right-button bindings,
the actual HUD menu and Slate buttons, a real Fusion32 connection and
received authoritative actor state. No fabricated server events or direct
state injection. This proves those application paths, **not OS input
injection**. The driver, launch/build scripts, narrow outgoing outfit-byte
logging patch and audit script are archived under `diagnostic/`.

The temporary bridge test calls the actual static `ToUnrealOutfit` converter
with ordinary, object-disguise and invisible descriptors. This is fixture
transport evidence, not a live object-disguise or rendering test.

## Retained unsuccessful attempts

`automated/unreal-build-probe.txt` records a compile failure in the temporary
diagnostic: iteration over `FArrangedChildren`. Indexed iteration corrected
the diagnostic; subsequent builds passed. No implementation correction.

`live-failed-cursor/` reached gameplay but failed its cursor-target assertion
before GET or SET. All protocol counters were zero. The diagnostic had put
the pointer at a fixed projected height one frame earlier. The revised
driver finds a real collision ray, focuses the viewport and traces immediately
after setting the pointer, with projected/actual coordinates logged. Startup
was slow, not blocked by a dialog; the operator confirmed the client loaded.

`live-startup-blocked/` records a subsequent launch that could not access the
AppData Zen cache in the sandbox and never reached gameplay. That isolated
process was stopped; the launch was repeated with explicit approval outside
the sandbox. It is not a dialog test result.

`live-failed-menu/` passed the real cursor trace and the production right-click
bindings, opening the actual self menu. The diagnostic then timed out before
any GET (GET/SET both zero); its generic button search chose an enclosing
button before the menu-entry button. The diagnostic was revised to prioritize
`SMenuEntryButton` and to log the selected widget type. The real menu's action
and the bridge were unchanged. This attempt is retained as FAILED, not counted
as live outfit success.

## Rendering boundaries

**COLOR_VISUAL_RENDERING = NOT_IMPLEMENTED.** The dialog previews numeric
indices; it has no colour swatches or rendered outfit preview. The four bytes
are bridged and stored by `AReal33DCreature::ApplyOutfit`, but `ResolveOutfit`
selects a model by outfit id alone. No byte-to-RGB rendering PASS is claimed.

**Object disguise rendering = NOT_IMPLEMENTED.** `ToUnrealOutfit` preserves
the disguise flag and object type; the actor stores them but explicitly keeps
a placeholder body. Transport and rendering are distinct results.

## Repeatability and limits

Use a detached/new worktree at `61ceaa9` to reproduce the corrected result,
or at `56bf5085` to reproduce the original defect. Run Git Bash
`tests/regression_harness.sh`, with the local hash-locked classic artifact at
`build/classic-client-772/app/Tibia.exe`, MSVC initialized and WSL available.
The harness's optional Unreal build is separate here. Build with the archived
UE 5.8 / MSVC 14.44 wrapper (adapt its absolute worktree path). For the
diagnostic run, copy the archived `Real33DOutfitCertification.cpp` driver
and converter fixture into the module's Private directory and apply only the
archived bridge patch. Launch the local 7.72 baseline with test account B via
the runtime credential loader; no credentials are recorded here. The original
candidate uses `original-live-driver.cpp`, renamed to
`Real33DOutfitCertification.cpp`, without the new glyph-size assertion. Run the
live audit against the new evidence directory. Retire diagnostic sources and
restore the candidate bridge, then rebuild and rerun original tests.

The live band available to this account is the only band claimed live. Other
sex/premium bands, refusal paths, the no-dialog gamemaster path, live disguise,
and RGB rendering require separate evidence. Native tests without
`--objects-srv` do not exercise the optional runtime MultiUse catalogue.
The bounded UI regression does not certify all game panels or full gameplay.

## Final automated checks and disposition

The temporary driver/converter include and wire logging were removed. The
module's production source is byte-for-byte Git-equivalent to `61ceaa9`.
`automated/final/` records the final checks:

- ClientCore CTest **14/14**, native **6/6** (outfit selection includes 20 cases).
- Closed colour verifier **43/43**; unchanged verifier and locked executable.
- MSVC C++17 suites and C++20 archive **PASS**.
- Production REAL33DEditor Win64 Development build **PASS**, UE 5.8, MSVC
  14.44.35207 selected (compiler reports 14.44.35228).
- Production Unreal automation **9/9**, zero errors/warnings: eight
  Presentation tests and WideWorld.AuthoritativeWindow. The earlier separate
  bridge fixture also passed, giving 10/10 in the instrumented automation run.
- Final harness **9 PASS / 0 FAIL / 1 SKIP**. The skip is only its optional
  Unreal build; the production build above was executed separately.
- Git Bash secret_check **6/6**. Final tracked/history check also repeated
  after the certification evidence commit.

The first final-build attempt raced the editor's shutdown and was refused
while Live Coding was still active. The process then exited and the same
production build succeeded; both logs are retained. The harness's 91 dirty
paths are staged certification evidence, not production changes.
PowerShell UTF-16 console captures are normalized to UTF-8 in this evidence
copy for review and secret scanning; original captures remain under build/.
Raw Unreal logs and the reproducible patch retain their original whitespace.

**DIALOG_CERTIFICATION = CERTIFIED at corrected production revision
61ceaa955933672c526864b5ed24dca0e4c8de84**, bounded to dialog/bytes and the named
UI checks. **Original 56bf5085 is not merge-ready without the arrow fix.**
The correction and evidence are local on the certification branch; the
implementation milestone remains published at the requested original hash.
Main and origin/main remain `b8e7433baeddc2875943f519d33a5cb9380f3eaa`.
No main merge. Full outfit rendering is not certified.

The earlier ClientCore integration status is preserved:
`CLIENTCORE_OUTFIT_772 = PASS_LAYER`, `LIVE_CERTIFICATION = NOT_DONE` for that
integration. This independent dialog certification does not claim complete
outfit presentation or change those original scope markers.
