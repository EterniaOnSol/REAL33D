# Current handoff

2026-10-15 16:10 America/Guatemala. Independent reviewer, certification.
UNREAL-WORLD-PRESENTATION-POLISH-001 = **CERTIFIED**.
`main` was NOT merged and was NOT modified. Merging is a separate authorised step.

Branch `certification/unreal-world-presentation-polish-001-20261015`.
Starting and candidate commit `bc9ce8fd00102c4d35f9cdfd60ddbf6e5b1f36ce`
(= `origin/milestone/unreal-world-presentation-polish-001`, untouched).
Base/main `fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf`, rechecked unchanged at close;
merge base is that commit, 10 ahead / 0 behind, no merge and no rebase.
Ending commit is the certification evidence commit containing this handoff.
Certification worktree `build/cert-unreal-world-presentation-polish-001-20261015`,
created with `git worktree add -b … bc9ce8fd…`.

**The implementing agent's worktree `build/unreal-world-presentation-polish-001`
was not used** — not built in, not run from, not read as input, not written to.
The certification checkout compiled its own `build/clientcore-windows/protocol772core.lib`
and linked `REAL33DEditor Win64 Development` against it.

Two gitignored local-only art trees were supplied as junctions so the checkout
rendered the art the milestone is judged on: `Content/Experimental/V08` from the
root worktree, `Content/Experimental/BrotherCreatures` from
`build/unreal-minimap-certification-20260928`. Neither is the implementing
worktree; both are SHA-pinned by committed manifests. The minimap cache was left
EMPTY on purpose and discovered 1819 cells during play.

Closed this run — all seventeen named criteria reproduced live by the reviewer in
ONE new 2811 s connection, none carried over from a prior session:
- CAMERA PASS `obstructed=true` pitch −25.00 yaw 265.15. Detect-only by design
  (`Real33DWorldActor.cpp:755-758`): zoom is kept, `cutaway_meshes` always 0.
- OCCLUSION PASS `hide_upper_floors=true`, `covered=true hidden upper tiles=143`,
  outdoor roofs retained.
- READABILITY PASS, bounded: labels/health bars/battle list plus correct
  authoritative positions; outfits 21 and 128 loaded delivered models, outfits
  136 and 57 stayed documented placeholders.
- WORLD_SPEECH / NPC_SPEECH PASS, `speech_shown_on_creature=15`, `chat_only=0`,
  `talk_messages_decoded=15`; Cipfried heal and a completed Tom trade.
- LOOK PASS 7 requests, 7 server descriptions.
- USE PASS req 7; CONTAINER PASS `open_containers=1` capacity 20, 3 objects.
- USE_WITH PASS req 359 rope 3003 on 435; same creature id moved z=8 → z=7 in 191 ms.
- WORLD_DRAG PASS reqs 28/49/50; INVENTORY_GROUND_DRAG PASS reqs 20/25/32/40/47/48/51;
  GROUND_INVENTORY_PICKUP PASS reqs 29/30 plus eight coin pickups.
- PUSH_CREATURE PASS **only** on req 447: snapshot 1.64 s later holds creature
  1073743167 on the exact requested tile 32093,32204,7. Earlier attempts were
  authoritative refusals ("There is not enough room.", "Sorry, not possible.")
  and are retained as refusals, not rounded up.
- FOLLOW/STOP PASS reqs 208/211 with a server-driven relocation while following.
- MINIMAP PASS destination 32096,32202,7 → 12 accepted steps → exact authoritative
  arrival; two further click-walks also arrived exactly.
- WIDEWORLD PASS `loads=187 unloads=125 sectors=62`, 17097 tiles spawned /
  16735 removed, `orphan_events=0`, `duplicate_spawn_attempts=0`.
- LIGHT PASS lamp 2109 → 2108 at 32094,32214,7 with the untouched twin at
  32100,32214,7 still 2109 as a control.

PROTOCOL_ERRORS = 0, UNSUPPORTED_OPCODES = 0, PROTOCOL_ANOMALIES = 0 across all 30
snapshots; `last_diagnostic "none"`, `viewport_synchronised true`, 357 walks
requested / 311 accepted / 46 refused / 0 unanswered. The 46 refusals are blocked
tiles — normal gameplay, preserved verbatim.

Tests: nine ClientCore CTest suites PASS; `build_clientcore_windows.cmd` PASS
(transport 22/22, crypto 25/25, C++17 suites + C++20 archive); three REAL33D native
tests PASS with `--objects-srv` so the MultiUse assertions ran; three Unreal suites
Success 0 errors / 0 failed / 0 notRun with only the expected TEST_IMPORTED warning;
`REAL33DEditor Win64 Development` Succeeded; `secret_check` PASS with six sub-checks.
Regression harness run **from its preserved worktree**
`build/real33d-3d-regression-harness-001` (`377b47be`) with `--repo-root` pointed at
this checkout, `--unreal`, `--objects-srv` and `--logs <live dir>`:
**OVERALL PASS, 9 PASS / 0 FAIL / 0 SKIP**, 293 logs scanned, 0 anomalies.

**Finding, recorded and NOT patched:** `tests/secret_check.sh` returns a vacuous
PASS when run through WSL inside a *linked worktree*. The worktree's `.git` is a
file holding a Windows `gitdir:` path that WSL git cannot resolve, every `git` call
fails, and because each check is `hits="$(git … || true)"` an empty result reads as
clean. The tell is sub-check 6, which only prints when `git ls-files` returns rows:
a real run shows six `ok` lines, a vacuous one shows five. Run it from Git Bash and
require six. Both runs recorded here produced six. Hardening it (one up-front
`git rev-parse` guard) is a scoped task, not a certification change.

Deliberately NOT re-examined, because they sit outside the seventeen criteria and
the candidate already records them as open — none was reclassified or waived: the
new grass-edge and house-roof alignment offsets, the underground
`StaticSectorActor::DesiredSectors` Z=0..7 coverage limit, and the requested
right-click cancellation of Use With.

Fusion32 was left running on the same saved runtime: `querymanager` 601, `game` 622,
`login` 931 in `Ubuntu-26.04`, `/var/lib/fusion32-server-baseline-772-0`. No prepare,
reset, save edit, equipment preparation or rule change. The character closed with
backpack 3 / jacket 4 / sword 5 and shovel, rope, 12 gold.

Evidence: `evidence/clientcore/UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-20261015.md`
and `evidence/clientcore/unreal-world-presentation-polish-independent-20261015/`
(11 received-state snapshots incl. EndPlay, 4 geometry dumps, 3 screenshots,
movement journal, 2266-line selected event log, harness report and check logs,
test and build output). All earlier reports and handoffs are preserved unchanged.
Previous handoff archived as
`archive/UNREAL-WORLD-PRESENTATION-POLISH-001-FINAL-QA-20261015T1452.md`.

Next work only as explicitly requested. The merge of this milestone into `main` is
authorised separately and was deliberately not performed here.
