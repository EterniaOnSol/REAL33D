# UNREAL-STACKABLE-SPRITE-PATTERNS-001 independent certification

Status: **CERTIFIED**. All requested checks passed independently.
Ready for a separately authorized merge; no merge or push performed in this run.

Candidate: `68f2ffe488627d5da52981bd83a2e95a5846ecc3`.
Base/origin/main at start: `1a15ddbd79a23a2841d882584afaa82fabd9f74d`.
Branch: `certification/unreal-stackable-sprite-patterns-001-20261015`.
Isolated checkout: `C:/Users/dell/Desktop/fusion32/build/cert-stackable-20261015`.
Reviewer: Codex, independent certification, 2026-10-15 America/Guatemala.
Ending revision: the evidence commit containing this report. No main merge or push.

The implementing checkout was read for its contract/report only. Its compiled
client, libraries, generated item pictures, live captures and test results were
not used as independent proof. This checkout extracted its own PNGs, compiled
its own ClientCore library and REAL33DEditor, and started a new live connection.
No implementation, server rule, saved character data, or production art was edited.

Evidence directory: `unreal-stackable-sprite-independent-20261015/` beside this report.

## Independent data and source checks

`extractor_probe.py` compares the fresh extractor output against a separate
decode through `visual/tools/tibia772.py::AppearanceFile/SpriteFile` and actual
RGBA pixels, using the existing read-only classic data in
`build/classic-client-772/app` of the root checkout. Source SHA256 values are in
`extractor-result.json`; neither a legacy executable nor an alternate server ran.

- PASS: 81 cumulative types, 432 decoded frames checked exactly.
- PASS: 4,832 non-cumulative types emitted one image each, including 1,433 types
  with multiple source sprites. No amount suffix was emitted for those objects.
- The author's "49 types / 351 extra images" counts types that gained images;
  there are also 32 cumulative types with one source picture. Spears 3277 are
  in that latter group, so the author's inclusion of spears among gained frames
  is imprecise. Worms 3492 have 8 amount patterns and 2 animation phases (16
  extracted images); runtime correctly addresses the first 8 amount patterns.
  The independent probe initially assumed one phase; that probe assumption was
  corrected to check every phase, with no candidate change.

Generic rule PASS: `Real33DStackSprite.h::StackSpriteFrame` selects by amount and
available frame count, with no item IDs. `Real33DUIStyle.cpp::ItemFrameCount`
counts contiguous files, bounded by the eight-tier table. `ItemBrush` caches by
type AND selected frame. Tiers are 1,2,3,4,5,10,25,50; x1 selects frame 0 and
x12 selects frame 5. Native tests execute all boundaries, malformed counts,
short ranges, high amounts and monotonic/in-range properties.

The candidate explicitly leaves 10/25/50 unverified against the original 7.72
executable. This review proves the declared rule and its actual displayed
result, not a disassembly of that legacy binary. Local REAL33D2D
`src/client/item.cpp::updatePatterns` corroborates the rule but is not substituted
for Fusion32 authority or claimed as original-client proof.

`Real33DPanelChrome.cpp` changed only the brush request, using the same count
already passed by InventoryPanel and ContainersPanel. The numeric count remains
`Printf("%u", InArgs._Count)`; the bridge copies received `bHasAmount/Amount`.
There is no new locally computed gameplay quantity or protocol change.

## New live evidence

Computer input automation failed before sending input (kernel assets path
error). The operator performed each requested action; the reviewer inspected
new machine-written snapshots, logs and screenshots. This is independent
operator-assisted QA, not a reuse of the implementing session or an inference
from the operator's answers alone.

Live directory: `build/unreal-world-presentation-polish-001/live-20261015T204404`
in this checkout. Client process 3600, freshly built candidate. Server remains
the existing Fusion32 runtime in Ubuntu-26.04,
`/var/lib/fusion32-server-baseline-772-0` (querymanager 601, game 622, login 931).
No reset, spawn or save edit. Existing local art junctions provide V08 from the
root checkout and BrotherCreatures from `build/unreal-minimap-certification-20260928`;
neither is the implementing checkout and neither was edited.

| UTC capture / action | Received state and visible result | Evidence |
|---|---|---|
| 02:47:57.663 | Fresh session baseline, stationary at 32092,32204,7 | 01-baseline.json |
| 02:49:26.957 | Open backpack 0 contains gold 3031 x4, shovel 3457, rope 3003; four-coin image and text 4 | 02-container-four.json/png |
| 02:50:47.036 | Request 5 picked up one ground coin into body slot 10; authoritative x1, single-coin image and text 1; backpack stays x4 | 03-inventory-one.json/png |
| 03:02:06.921 | Request 6 combined inventory x1 with backpack x4, requests 7..13 collected seven real ground coins; backpack x12, mound image and text 12 | 04-container-twelve.json/png |
| 03:03:26.025 | Full x12 stack moved to body slot 10; backpack no longer contains gold; same mound and exact text 12 in inventory | 05-inventory-twelve.json/png |
| 03:12:15.425 | Normal Obi trade delivered worms 3492 x6 to backpack; six-worm amount image, text 6, and authoritative gold 10 to 5 | 06-worms-six.json/png |

LIVE_X1, LIVE_X12, INVENTORY and CONTAINER PASS for these named cases. Shovel,
rope and equipment retained single pictures and no invented count text.
`live_pixels.py` independently compares opaque sprite pixels, omitting the
counter overlay, against every extracted gold frame in small slot regions.
Frame 0 is the best match for x1; frame 5 is best for x12 in both panels;
worms x6 selects frame 4, also the best match across its eight amount pictures
(`live-pixels.json`). This is a nearest-image corroboration, not pixel-identical
screen compositing; Slate color blending changes displayed pixel values.

SECOND_STACKABLE PASS: the character obtained six worms through an unmodified
NPC sale, not a fixture. The operator first bought a torch for two gold, which
explains 12 -> 10 gold; the worm sale then made 10 -> 5. At 03:12:09.046 the server
received `1 worms`, Obi offered six for five at 03:12:10.033, the operator sent
`yes` at 03:12:14.504, and the server delivered the item and acknowledgement.
Earlier incomplete/typo phrases produced no sale and were not counted.
Source: read-only runtime `game/reference/npc/obi.npc`,
`%1,"worms" -> Type=3492, Amount=%1*6, Price=5*%1`, normal Topic=1 trade.

Protocol PASS: all 51 new snapshots (retained under `live-state/`) have
unsupported_opcodes=0, protocol_anomalies=0, residual_bytes=0 and diagnostic
`none`. EndPlay at 03:14:38.099 after 1772.3 seconds has 1170 decoded commands,
33/33 walks accepted, 0 rejected/unanswered, 13 move requests, and a synchronized
viewport. `07-endplay.json` and `live-events.log` retain the final state and
selected LogReal33D events (only repetitive floor/cache lines omitted).

## Tests and setup findings

- Fresh ClientCore CTest: 13/13 PASS (`clientcore-tests.txt`).
- Fresh MSVC C++17 tests / C++20 core archive: PASS (`build-sanity-msvc.txt`).
- Fresh REAL33DEditor Win64 Development: Succeeded (`unreal-module-build.txt`).
- Direct strict native run: four suites PASS, including runtime MultiUse
  metadata (204 types) and stackable sprites. Invocation:
  `wsl.exe -d Ubuntu-26.04 -u root -- bash .../build/certification/native-direct.sh`.
  The reproducible script is retained in evidence; it uses `set -euo pipefail`.
- First harness invocation had toolchain discovery skips. A later full harness
  built successfully but used the wrong objects.srv path (`game/dat` instead of
  `game/reference/dat`). Its native raw log contains a `file.good()` assertion
  even though the wrapper reported 4 PASS: its nested shell exit handling masks
  that failure. That native summary is NOT accepted as proof. Correct-path
  strict native execution passed independently. The final corrected harness also
  passed, and its raw native output explicitly reports all four successful suites.
- A failed UBA PCH write during the fresh build was automatically retried without
  UBA and the build succeeded. Disk space was recovered only by NTFS compression
  of this checkout's own generated PCH (2.49 GB to 0.90 GB); no other work deleted.
- Four mailbox assets were LFS pointers at first startup, yielding package-tag
  warnings unrelated to item sprites. After normal client close all four were
  hydrated and SHA256-checked against their HEAD LFS pointers; index refresh
  produced no asset diff. These were setup warnings, not a sprite defect.
- First Slate diagnostic invocation reused UBT's cached source list and ran
  zero matching tests despite exit code 0. It is NOT counted as a pass. The
  corrected runner forces `-NoUBTMakefiles` and requires the exact named test
  in the exported report with Success/zero errors. No reliance on exit code alone.

FALLBACK PASS: `REAL33D.Certification.StackableSpriteFiles` executed at
03:19:28 UTC, Success, 0 errors, 0 warnings, 0 failed/notRun/inProcess. It called
the actual candidate UIStyle and StackSpriteFrame implementations, checking gold
x1/base and x12/frame5, worms x12/frame5, separate brush cache keys, non-stackable
invariance, gapped and short suffix ranges, missing base/null numeric-ID fallback,
reserved empty ID, selected file missing after count probe, and malformed counts.
Fixtures were six owned PNG copies at IDs64000..64003; they never entered
WorldState or Fusion32 and were removed by the test. Source and exported result
are `StackSpriteCertification.cpp` and `slate-automation-result.json` in evidence.
These are file-lookup/headless Slate checks; live panel appearance is proved by
the separate actual gameplay captures above.

The temporary diagnostic source was removed from Private, then the exact
candidate source was rebuilt. UBT initially retained that removed file in its
source index; only this checkout's generated SourceFileCache.bin and Makefile.bin
were cleared to regenerate the list. `candidate-restored-build.txt` records
Succeeded with no diagnostic source included. `git diff 68f2ffe -- clientcore
unreal scripts tests` is empty. No implementation repair was needed.

Final corrected harness: `harness/REGRESSION-HARNESS-20261016T032947Z.md`,
**9 PASS / 0 FAIL / 0 SKIP** under `--strict --unreal`, correct read-only
objects.srv path, and this session's `--logs` directory. ClientCore13/13,
native4/4 (including stackable and runtimeMultiUse204), MSVC PASS,
REAL33DEditor Win64 Development PASS, secret_check SIX sub-checks PASS,
175 machine logs scanned with0 anomalies, and output-secret-scan PASS.
The raw check logs are retained. Initial rejected setup runs are retained as
`setup-*.txt`; they are not folded into successful execution evidence.

## Reproduction and closeout

Create an isolated checkout at the candidate, hydrate required LFS art, and
regenerate UI item pictures from the read-only classic DAT/SPR using the normal
launcher. Build ClientCore and REAL33DEditor locally. Repeat the named moves and
normal Obi trade above, saving F9 snapshots; compare received amounts and icons.
Run `extractor_probe.py` and `live_pixels.py` from the checkout root (adjust only
the read-only source/checkout paths if relocating). For the optional diagnostic,
copy `StackSpriteCertification.cpp` and `run-slate-test.ps1` from this evidence
directory to `build/certification/`, close the client and execute the runner.
It adds a temporary test source and requires the exact successful exported
test result. Remove only that diagnostic source after testing, clear the two
own generated UBT index files if necessary, and rebuild the candidate.
`run-harness.cmd/.sh` retain the actual final harness invocation; adapt only
machine-specific compiler paths and the correct read-only runtime fixture path.

Main and milestone remote heads were rechecked unchanged: main1a15ddbd,
milestone68f2ffe. Project status, parity note and CURRENT were updated, prior
substantive handoff archived unchanged. Only evidence and documentation are
included in the certification commit. Repository source is the exact candidate.

## Presentation boundary

World 3D objects still use one mesh per type, independent of amount. This is the
explicitly accepted 3D presentation boundary and is not a certification failure.
Source pictures and inventory/container counts are the scope of this review.
Art alignment, outfit completeness, camera behavior and prior milestone gaps
are not recertified or reclassified by this result.

Malformed frame-count and missing/gapped-file behavior are the fallback scope;
arbitrary PNG byte corruption or hot replacement of an already cached brush is
not claimed as tested or supported.
