# UNREAL-WORLD-PRESENTATION-POLISH-001 - remaining live gaps

Date: 2026-10-09 America/Guatemala. State: IMPLEMENTED_UNVERIFIED.
Scope: only camera collision, wall occlusion, world/NPC speech, Use/Use With,
minimap coexistence and the requested regression smoke. Retain all prior PASS
results. No V08 production art, protocol, gameplay, map or frozen-agent changes.

## Repository recovery and push

Clean milestone branch at 6ac8a8e858a9e356c6c9e1b4f467abf4e7dabcb1 verified.
Live remote was 4305d23738454c2530007b16e759cbcaeddbca55. Both this remote and
certified main fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf are ancestors of HEAD.
User explicitly authorized pushing the existing commits first. Ordinary push
completed (17 LFS objects); remote was verified equal to local 6ac8a8e afterward.
No force push, reset, merge, rebase, certification or branch recreation.

## Live execution

Existing querymanager 830/game 845/login 1067 passed identity/cwd/start/port
checks. Launched unchanged normal QA client, PID 3728, session
build/unreal-world-presentation-polish-001/live-20261009T194912.
The settled image shows connected normal gameplay at 32097,32206,7.
No completed PASS cases are rerun merely for startup.

Native input tooling is exposed in this turn but failed before initialization:
"failed to write kernel assets ... path not found (os error 3)". A kernel reset
and one initialization retry failed identically. No Windows inputs were issued.
Operator input requested for the real QA session; no synthetic server scenarios
or hidden map state are used. Tool availability is not proof of working control.

## Remaining procedure and acceptance

- Say a short local phrase. Actual received speech generates Speech JSON/PNG;
  require readable world text plus retained chat line. NPC speech is optional
  if no reasonably available encounter permits a natural response.
- Walk/orbit near a wall and return to clear space. F9 at obstruction and after
  clearance; require no player occlusion, expected cutaway, camera retraction
  and recovery toward the requested distance without clipping.
- Follow a visible creature using Battle List Follow; F9. Press Stop; F9.
  Require client following state and target feedback to clear appropriately.
- Open an available container with normal Use. Require an actual server-backed
  container panel, not just a request counter. Shift+left click performs Look.
- Use With: Shift+right click an available source; capture targeting hint/cursor,
  Escape cancellation, and a naturally meaningful target attempt where available.
  Require request feedback to distinguish a sent request from server success.
- Click a nearby reachable minimap tile on the current floor and wait. Require
  minimap target log plus authoritative arrival at that same coordinate, matching
  player/minimap position. Exercise normal sector movement in the same smoke.
- Require zero residual bytes, unsupported opcodes and protocol anomalies;
  document rejected/unanswered steps, duplicate/orphan events and target loss.

Unperformed cases stay IMPLEMENTED_UNVERIFIED. No art-completeness requirement
is added. The existing source wall-family/outfit/lamp/mountain issues remain
separate in the October9 continuation report. No source code changed in this run.

Checkpoint verification: staged secret_check PASS; git diff --cached --check PASS.
Main worktree remains clean. No new Manual/Speech captures at checkpoint; client
left open for operator input. This evidence/handoff checkpoint is local only.
