# UNREAL-WORLD-PRESENTATION-POLISH-001 independent certification

**Verdict: CERTIFIED.** Every named runtime criterion was reproduced live, by this
reviewer, against an isolated checkout of the certified candidate. No source,
asset, protocol, server-rule or character-data change was made. `main` was not
merged and was not modified.

2026-10-15, America/Guatemala. Independent reviewer, operator-assisted live QA.
This report supersedes nothing: the implementation QA in
`UNREAL-WORLD-PRESENTATION-POLISH-001-FINAL-QA-20261015.md` remains as written,
and none of its historical verdicts were rewritten. Where that report rested a
`PASS` on retained prior-session evidence, this run re-obtained the result
first-hand in a single new connection.

## Provenance

| | |
|---|---|
| Certified candidate | `bc9ce8fd00102c4d35f9cdfd60ddbf6e5b1f36ce` |
| Certification branch | `certification/unreal-world-presentation-polish-001-20261015` |
| Certification worktree | `build/cert-unreal-world-presentation-polish-001-20261015` |
| Created by | `git worktree add -b <branch> <dir> bc9ce8fd…` — a fresh checkout of exactly that commit |
| `origin/main` | `fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, unchanged at start and at close |
| Merge base with `origin/main` | `fa3adcfd…` — 10 ahead, 0 behind; no merge, no rebase |
| Candidate branch on origin | `milestone/unreal-world-presentation-polish-001` = `bc9ce8fd…`, untouched |
| Harness worktree (preserved) | `build/real33d-3d-regression-harness-001` at `377b47be22089d81034c46c0f684ec4ad40b8572` |

**The implementing agent's worktree `build/unreal-world-presentation-polish-001`
was not used.** Nothing was built, run, read as input or written there. The
certification checkout was compiled from scratch: `tests\build_clientcore_windows.cmd`
produced its own `build/clientcore-windows/protocol772core.lib`, and
`REAL33DEditor Win64 Development` linked against that archive in this worktree
(31 actions, `Result: Succeeded`).

### Fusion32 runtime

Pre-existing and untouched: WSL `Ubuntu-26.04`, runtime
`/var/lib/fusion32-server-baseline-772-0`, `querymanager` pid 601 / `game` pid 622 /
`login` pid 931, ports 7171/7172/7173. No `prepare`, no `reset`, no save edit, no
equipment preparation, no creature or rule change. The character closed with the
same backpack/jacket/sword it opened with.

### Local-only assets, disclosed

Two asset trees are gitignored machine-local data, not part of the commit, and a
fresh worktree therefore has neither. They were supplied as directory junctions
so the certification checkout rendered the same art the milestone was judged on:

- `Content/Experimental/V08` → the repository root worktree's copy (15 466 files).
- `Content/Experimental/BrotherCreatures` → `build/unreal-minimap-certification-20260928`'s
  copy (442 files), a previous certification's tree.

Neither source is the implementing agent's worktree. Both trees are produced by
SHA-pinned manifests (`visual/qa/full_catalog_v08/experimental_catalog_runtime.json`,
`visual/qa/brother_creatures/runtime.json`), so their content is determined by the
commit, not by the worktree they happen to sit in. The junctions are inside
gitignored paths and left the worktree clean (`git status --porcelain` = 0 lines
before and after the whole run).

The **minimap cache was deliberately left empty.** The candidate's session loaded
5 499 previously discovered cells; this one started from nothing and discovered
1 819 cells during play. That makes the minimap result stronger, not weaker.

## Live session

`build/unreal-world-presentation-polish-001/live-20261015T152052` inside the
certification worktree. Launched with the committed
`scripts\client\run_unreal_presentation_qa.ps1`, account B, 127.0.0.1:7171.
Session 2811.0 s, `connected_at_close: true`, normal close with an `EndPlay` record.
30 received-state snapshots, 3 retained screenshots, 4 retained geometry dumps.

The operator drove every click and keystroke; this reviewer issued one action at a
time and read the result from the log and the received-state snapshots rather than
from the operator's description. The operator also performed additional ordinary
gameplay between prompts (extra walking, extra Looks, container reshuffling, combat,
an NPC trade). All of it is in the retained log and is timestamped. **No verdict
below rests on an action that is not in the log**, and two operator reports were
checked against the log and corrected rather than accepted:

- a reported creature push that sent no request at all (see PUSH_CREATURE);
- a creature that "moved" after a drag, which the log showed was ordinary rat
  walking, not a push result.

## Results

Every row was obtained in this one connection. `req N` is the client request id in
`selected-live-events.txt`; times are UTC.

| Criterion | Result | Live proof |
|---|---|---|
| Camera collision | PASS | `04-camera-obstructed.json` 21:34:55.601Z: `obstructed=true` at pitch −25.00 / yaw 265.15 with a wall on the camera ray |
| Wall/roof/floor readability | PASS | `03-indoor-roof-occlusion.json` 21:31:11.861Z: `hide_upper_floors=true`; log `floor visibility: player floor 7 covered=true hidden upper tiles=143`; outdoor roofs retained in `04-…png` |
| Player/creature/NPC readability | PASS (bounded) | Name labels, health bars, battle-list entries and correct authoritative positions throughout; `creature 1073743164 outfit 21: brother visual loaded parts=10 height=80.3` (rat) and `creature 1073742035 outfit 128: … parts=8 height=104.0` (Seymour) are delivered models, not placeholders |
| World speech | PASS | 21:37:31.622Z `talk [Say] at 32097,32214,7, Test Player B: "hi" speaker Resolved creature 1002` |
| NPC speech | PASS | 21:37:32.641Z Cipfried `"Hello, Test Player B! …"`; later `"heal"` → `"You are looking really bad. Let me heal your wounds."` with HP 65/160; Tom the Tanner completed a corpse-for-gold trade. `speech_shown_on_creature=15`, `chat_area_only=0`, `talk_messages_decoded=15` |
| Look | PASS | req 6 → `You see a dead rat.` (58 ms); also req 21 `short sword (Atk:11 Def:11)`, req 27 `a shovel`, req 31 `grass`, reqs 33/41/44 gold-coin counts. 7 looks, 7 server descriptions |
| Use | PASS | req 7 `use 7: object 2854 at body 0 slot 3` → server opened the container |
| Use With | PASS | req 359 `use-with 359: object 3003 on object 435 at 32097,32205,7`; 191 ms later `creature 1073765520 "rat" appeared at 32097,32206,7`. **Same creature id**, last observed at 32090,32213,**8** at 22:03:40.533Z — the rope moved that creature from z=8 to z=7. Also req 180 on the same rope spot earlier, which lifted creature 1073743164 from 32097,32206,**8** to 32097,32206,**7** in 187 ms |
| World→world drag | PASS | req 28 `type=3294 32097,32213,7 → 32098,32213,7`, proven by req 29 reading the sword *from* 32098,32213,7; chained reqs 49, 50 |
| Inventory→ground drag | PASS | req 20 `slot-to-world drag: object=3294 from=body 0 slot=5 to=32097,32213,7`; `06-inventory-to-ground.json` shows slot 5 gone and `06-…geometry.json` shows type 3294 on 32097,32213,7. Also reqs 25, 32, 40, 47, 48, 51 |
| Ground→inventory pickup | PASS | req 29 (sword) and req 30 (shovel) `world-to-slot drag … to=container 0 slot=9`, corroborated by req 47 later dragging the sword back *out of* container slot 2; plus eight coin pickups |
| Push creature | PASS | req 447 22:07:15.114Z `world drag 447: type=99 creature=1073743167 from=32094,32205,7 stack=1 to=32093,32204,7`; `10-push-result.json` at 22:07:16.753Z, no failure message in between, has `id=1073743167 at 32093,32204,7` — the exact requested tile — while the player stood at 32095,32206,7 |
| Follow / Stop Follow | PASS | req 208 `from battle list: follow creature 1073741889` → `action=follow`, then `Fusion32 relocated us with no request outstanding, 32097,32216,7 -> 32097,32217,7`; req 211 `general cancel` → `target=0 action=none` 133 ms later |
| Container open | PASS | `02-container-open.json`: `open_containers=1`, `number 0, type 2854 "backpack", capacity 20, object_count 3` → 3031×4, 3457, 3003 |
| Minimap click + authoritative arrival | PASS | 21:46:45.990Z `minimap destination: 32096,32202,7` → `12 cardinal steps` → requests 17–28 all accepted → 21:46:51.623Z `left-click walk arrived: authoritative player=32096,32202,7`. Two further click-walks arrived exactly at 32091,32194,7 and 32091,32188,7 |
| WideWorld | PASS | `11-EndPlay.json`: `loads=187 unloads=125 sectors=62` — streaming proven in both directions with a bounded resident set; 17 097 tiles spawned vs 16 735 removed, `orphan_events=0`, `duplicate_spawn_attempts=0` |
| Light toggle | PASS | `use 119 from world: object 2109 at 32094,32214,7 stack 3` 21:51:29.596Z. `08-light-before.geometry.json` has **2109** at 32094,32214,7; `09-light-after.geometry.json` has **2108** there and the untouched twin at 32100,32214,7 **still 2109**. The control makes it the server transforming exactly the used object |

### Protocol state

All 30 received-state snapshots of this session, checked individually:

```
30 "unsupported_opcodes":0
30 "protocol_anomalies":0
30 "residual_bytes":0
30 "last_diagnostic": "none"
```

No bridge diagnostic (`is not decoded`, `could not be decoded`, `left unwalked`,
`desynchronis…`) appears anywhere in the session. `EndPlay`: 2602 frames,
3762 commands, `viewport_synchronised: true`, 357 walks requested / 311 accepted /
46 refused / **0 unanswered**, 25 external relocations.

```
PROTOCOL_ERRORS      = 0
UNSUPPORTED_OPCODES  = 0
PROTOCOL_ANOMALIES   = 0
```

The 46 walk refusals and the `Sorry, not possible.` / `There is not enough room.`
messages are authoritative gameplay refusals of blocked tiles and illegal pushes.
They are preserved verbatim and are **not** normalised away: they are the evidence
that the server really adjudicates these commands, which is what makes the one
confirmed push meaningful.

## Tests, build and secret check

Run against the certification checkout, not against the candidate's worktree.

- **ClientCore suites** — 9 CTest suites PASS (`clientcore-tests.txt`). Natively on
  Windows with MSVC, `tests\build_clientcore_windows.cmd` also PASS: transport 22/22,
  crypto 25/25, login, gamelogin, initial_world, movement, player_state, worldview,
  minimap; both the C++17 suite build and the C++20 archive Unreal links.
- **REAL33D native tests** — 3 passed, 0 failed, with `--objects-srv` supplied so the
  runtime MultiUse assertions were exercised (`real33d-native-tests.txt`).
- **Unreal automation** — `REAL33D.Presentation.CameraAndFloors`,
  `REAL33D.WideWorld.AuthoritativeWindow`, `REAL33D.AssetRegistry.ExperimentalV08`
  all `Success`, 0 errors, 0 failed, 0 notRun, 0 inProcess. The single warning is the
  expected `TEST_IMPORTED is not APPROVED/READY/INTEGRATED`, which is not an art
  approval (`unreal-automation.json`). Two reading notes on that file, neither a
  defect: Unreal writes it UTF-8 **with a BOM** (the candidate's committed copy is
  the same), so it needs `utf-8-sig`, and its top-level `succeeded` field reads `2`
  while all three tests are individually `Success`. The per-test `state`/`errors`
  fields are the authoritative ones, and `test_unreal_presentation.ps1` gates on
  those plus `failed`/`notRun`/`inProcess`, which is why it reported PASS.
- **Unreal build** — `REAL33DEditor Win64 Development`, `Result: Succeeded`
  (`unreal-build.txt`).
- **secret_check** — PASS, **six** sub-checks (`secret-check.txt`).

### Finding: `secret_check.sh` reports a vacuous PASS when run under WSL in a linked worktree

Not a defect introduced by `bc9ce8fd` — `tests/secret_check.sh` is unchanged by it —
but it changes how the check must be run, so it is recorded here rather than left
for the next reader to rediscover.

A linked worktree's `.git` is a file containing `gitdir: C:/Users/dell/…`. WSL's git
cannot resolve that Windows path, so every `git` call in the script fails with
`fatal: not a git repository`. Because each check is written as
`hits="$(git … || true)"` and empty output means "clean", **all five scanning
sub-checks then pass without examining anything**, and the script still prints
`secret_check: PASS`. The tell is sub-check 6, which only prints when `git ls-files`
actually returns rows:

```
# WSL, linked worktree — vacuous
ok   : no secret-bearing file extensions tracked        (5 × "fatal: not a git repository" to stderr)
secret_check: PASS                                      ← only 5 "ok" lines

# Git Bash, same worktree — real
ok   : archived upstream config templates present and accepted (see evidence)
secret_check: PASS                                      ← 6 "ok" lines
```

**Both the manual run and the harness run recorded here produced six sub-checks**,
so the `SECRET_CHECK = PASS` above is non-vacuous. Nothing was patched; the
hardening (checking `git rev-parse` once, up front) belongs to a scoped task, not to
a certification.

## Regression harness

`tests/regression_harness.sh` executed **from the preserved harness worktree**
`build/real33d-3d-regression-harness-001` (`377b47be`), pointed at this
certification checkout with `--repo-root`, and additionally given this session's
live directory with `--logs`:

```
bash build/real33d-3d-regression-harness-001/tests/regression_harness.sh \
  --repo-root build/cert-unreal-world-presentation-polish-001-20261015 \
  --out    …/build/certification-20261015/regression-harness \
  --unreal \
  --objects-srv /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv \
  --logs   …/build/unreal-world-presentation-polish-001/live-20261015T152052
```

| Check | Status | Detail |
|---|---|---|
| `provenance` | PASS | branch `certification/…` at `bc9ce8fd`, 10 ahead / 0 behind `origin/main`, 0 uncommitted paths |
| `branch-isolation` | PASS | not on `main` |
| `clientcore-tests` | PASS | 9 CTest suites via WSL |
| `real33d-native-tests` | PASS | 3 passed, 0 failed, MultiUse assertions exercised |
| `secret-check` | PASS | 6 sub-checks ok |
| `build-sanity-msvc` | PASS | C++17 suites and the C++20 archive both built |
| `unreal-module-build` | PASS | `REAL33DEditor Win64 Development` built |
| `protocol-log-check` | PASS | 293 logs scanned, 0 carried anomalies |
| `output-secret-scan` | PASS | no key material, pattern self-tested |

**`OVERALL PASS` — 9 PASS, 0 FAIL, 0 SKIP**, exit 0. The harness reserves `PASS` for
a run in which every check actually executed; a single unexecuted check would have
been `PASS_WITH_SKIPS`. Full report in `regression-harness-report.md`.

## Scope and limits

What a reader must not read into this verdict:

- **Art completeness is not certified.** Creatures using outfits 136 (Dixi, and the
  player's own) and 57 (Cipfried) rendered as the documented explicit placeholder
  the whole session — only 16 outfits have delivered models. Readability above is
  carried by placeholder body plus name label, health bar, battle-list entry and
  correct authoritative position, which is what the criterion claims. Outfits 21 and
  128 did load real delivered models.
- **Camera collision is detection, by design.** `Real33DWorldActor.cpp:755-758`
  measures obstruction and then deliberately keeps the operator's zoom and cuts
  nothing, so `actual_distance == requested_distance` and `cutaway_meshes == 0` in
  every snapshot. `obstructed=true` is the whole signal. At a near-horizontal angle
  the body is genuinely hidden behind the wall and only the label remains; that is
  the documented behaviour, not a regression.
- **Not re-examined**, because they are outside the seventeen named criteria and the
  candidate already records them as open: the newly reported grass-edge and
  house-roof alignment offsets, the underground `DesiredSectors` Z=0..7 coverage
  limit, and the requested right-click cancellation of Use With. None of them was
  silently reclassified; all remain exactly as the candidate left them.
- Nothing here certifies performance, long-session stability, or any path not listed
  in the results table.

## Repository state at close

Certification evidence committed to `certification/unreal-world-presentation-polish-001-20261015`,
which contains `bc9ce8fd` plus this report and its artifacts, and nothing else.
`origin/main` is `fa3adcfd…`, unchanged. **`main` was not merged and must not be
merged as part of this certification** — that is a separate, explicitly authorised
step.
