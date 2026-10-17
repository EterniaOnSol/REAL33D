# UNREAL-SKILLS-PANEL-001

Status: `PASS`. **The panel this milestone asked for already existed.** The
work delivered here is the test coverage it was missing and the live evidence
that it updates from authoritative Fusion32 values. No protocol, server or
gameplay change; no HUD architecture change.

Base `origin/main` at `370b5676e2a186d44e11aeffe1fb4766593f8d9e`, on
`milestone/unreal-skills-panel-001`. Not merged.

## Audit first: the panel is already there

Stated plainly because it changes what this milestone is. `SReal33DSkillsPanel`
on main already:

- draws all seven fighting skills — Fist, Club, Sword, Axe, Distance,
  Shielding, Fishing — from `FReal33DPlayerSkills`, plus seven vitals rows
  (level, experience, hit points, mana, soul, capacity, magic level);
- gives magic level and all seven skills a percent bar (`HasBar(Row)` is
  `Row == RowLevel || Row >= RowMagic`);
- shows `--`, never a zero, for anything the server has not sent;
- is registered through the panel registry at `Real33DHUD.cpp:235` as
  `Real33D::PanelIds::Skills`, which UNREAL-HUD-PANEL-REGISTRY-001 did;
- is fed every frame by `Skills->SetValues(VitalsNow, SkillsNow)` at
  `Real33DHUD.cpp:708`.

Re-implementing any of that would have been duplicated work. What was genuinely
missing was a test for the panel and live proof that a skill moves.

## Change

Two rules were lifted out of a lambda in `SetValues` into public statics the
panel itself now calls, so the behaviour under test is the behaviour drawn
rather than a copy that could drift:

| | |
|---|---|
| `RowText(Value, bKnown)` | the number, or `--` while the server has not spoken |
| `BarFill(Percent, bKnown)` | clamped fill, empty while unknown |

`SetValues` now passes raw values and the lambda calls both. The dead `Number()`
helper went with it. No row, format, colour, height or value changed.

## Tests

`REAL33D.Presentation.SkillsPanel`, in the existing automation suite:

- **the rule that matters** — for values 0, 1, 10, 4200 and -1, an unsent value
  always renders `--`, because a zero drawn there would be a number the client
  invented;
- a server-sent **zero is shown as `0`**, not hidden behind the same dashes;
  magic level 0 is the ordinary Rookgaard case and is a fact, not an absence;
- skill-sized, experience-sized and >32-bit values all format;
- bar fill: unknown empty, 0/50/100 exact, clamped above 100 and below 0 (the
  wire carries a byte this client does not get to trust blindly);
- monotonic and in range across the whole 0..255 byte;
- `DesiredContentHeight()` positive, since a non-positive height would collapse
  the panel the registry shows.

## Results

| Gate | Result |
|---|---|
| Unreal UI suites | **5/5 Success**, 0 errors: `SkillsPanel` (new), `PanelRegistry`, `CameraAndFloors`, `WideWorld.AuthoritativeWindow`, `AssetRegistry.ExperimentalV08` |
| `REAL33DEditor Win64 Development` | `Result: Succeeded` |
| ClientCore CTest | **13/13 PASS** |
| `real33d-native-tests` | 4 passed, 0 failed |
| MSVC | `WINDOWS CLIENTCORE: PASS` |
| Regression harness | `PASS_WITH_SKIPS`, **0 FAIL**; only `protocol-log-check` SKIP |
| `secret_check` | PASS, 6 sub-checks |

## Live QA

`live-20261017T044646`, account B against the running Fusion32. 0 client
errors; `residual_bytes`, `unsupported_opcodes`, `protocol_anomalies` all 0 and
`last_diagnostic "none"` across all 13 snapshots of the session.

**Authoritative values.** `01-skills-before.png` shows all fourteen rows live:
Level 4, Experience 441, Hit Points 65, Mana 15, Soul Points 100, Capacity 292,
Magic Level 0, then Fist 11, Club 13, Sword 11, Axe 10, Distance 10,
Shielding 10, Fishing 10, with percent bars on magic level and the seven
skills. Magic Level renders `0` — a real server-sent zero, exactly the case the
test pins.

**Nothing invented.** The log holds the transition from no data to data:

```
HUD vitals: HP --/--   Mana --/--   Level --
HUD vitals: HP 65/165  Mana 15/15   Level 4
```

Before `SV_CMD_PLAYER_DATA` arrived the panel showed dashes, not zeroes.

**Live update.** After killing a rat, `02-skills-after-combat.png`:

| row | before | after |
|---|---|---|
| Experience | 441 | **471** |
| Hit Points | 65 | **64** |
| Capacity | 292 | **285** |
| Sword Fighting bar | ~35% | **~65%** |

The Sword Fighting *percent bar* advancing is the important one: that is
`SV_CMD_PLAYER_SKILLS` progress redrawing from server state, not just a vitals
tick. Skill *levels* did not change, which is correct — a level takes far more
than one rat, and none was claimed.

## Limits

- Only the seven skills Fusion32 actually sends are shown. The 2D client can
  draw criticals, leech and momentum; no 7.72 command carries them, so a row
  for one could only show an invented number. That boundary is the panel's own
  and is unchanged.
- Skill *level* increments were not observed live, only percent progress. One
  rat is not enough, and nothing was edited to force one.
