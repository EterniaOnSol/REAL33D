# Operator-assisted local QA, 2026-10-14

Task UNREAL-WORLD-PRESENTATION-POLISH-001 remains IMPLEMENTED_UNVERIFIED.
No code, art, server configuration, certification or main merge changed.
Starting/local/preserved remote HEAD: 54f6abc6b323fee014c79d46b929b621a19ae7c5.
Remote 6ac8a8e was unchanged and verified an ancestor; ordinary push succeeded.
Git for Windows bash secret_check executed successfully before push. The initial
WSL bash invocation had invalid Windows worktree gitdir resolution and is NOT
credited, despite its misleading PASS output.

## Preconditions and observed evidence

Existing runtime was stopped. LockGame in reference/game/src/main.cc refuses
any nonzero SAVEPATH/game.pid. Stale lock 845 was checked against absent process
and preserved as game.pid.stale-polish-20261014. Existing startup completed:
querymanager 618, game 633, login 846; identity/cwd/start/ports/readiness verified.
Hidden bounded WSL keeper host PID 15936 prevents premature distro shutdown.
Normal launcher Account B, unchanged 54f6abc build, fresh evidence directories:
build/unreal-world-presentation-polish-001/live-20261014T020859 (PID 16704),
and live-20261014T021415 (PID 12280). Both clients closed normally by operator.
Agent used no desktop input automation. Operator provided all gameplay input.
Agent read JSON/logs and personally inspected generated PNGs.
Compact artifacts: unreal-world-presentation-polish-operator-qa-20261014/.

## Results and limits

- CAMERA_COLLISION FAILED acceptance: operator explicitly rejects camera
  approaching player and requests removal. Actual live EndPlay records requested
  distance 1293.50, actual 482.99, obstructed=true. No code changed; removal is
  future implementation work requiring a changed task scope.
- WALL_OCCLUSION PASS bounded smoke: first session 08:12:11 cutaway_meshes=1,
  player/label readable; cave floor-8 captures hide upper floors. Second session
  bookcase capture shows readable player under roof with upper floors hidden.
  Full art approval is not claimed.
- WORLD_SPEECH PASS: automatic Speech 08:15:34.736 PNG shows hola above local
  player; later Manual 08:16:05.861 shows retained Test Player B: hola transcript.
  Actual received talk=1, speech_on_creature=1, says_requested=1.
- USE_FEEDBACK PASS: Manual 08:16:05.861 shows Use requested, server-backed
  bookcase panel; log use 53 object2435 map32101,32194,7 stack1. Earlier ordinary
  coin Use received You cannot use this object, not manufactured success.
- CONTAINER_SMOKE PASS: same PNG/JSON contains bookcase number0/type2435,
  capacity6/objects0. Panel opened following ordinary world Use.
- FOLLOW_SMOKE PASS: Manual 08:16:17.220 shows blue Follow Seymour, target
  1073742035; log command54 and following combat state. Prior certification retained.
- STOP_FOLLOW_SMOKE PASS: Manual 08:16:19.950 shows normal Seymour label;
  command55 general cancel followed by target0/action none and server clear.
- USE_WITH_FEEDBACK IMPLEMENTED_UNVERIFIED: no targeting/cancel/target attempt
  captured; operator reports no weapon. Weapon is not a necessary condition for
  cursor QA, but no invented substitute or unperformed PASS is recorded.
- MINIMAP_COMPATIBILITY IMPLEMENTED_UNVERIFIED: synchronized coordinates/floor,
  cache growth, sector loads/unloads and ordinary 21-step route arrival proven.
  Log lacks evidence attributing route specifically to a minimap click. Prior
  minimap certification retained; complete requested new smoke not claimed.
- NPC_SPEECH IMPLEMENTED_UNVERIFIED: Cipfried and Seymour naturally visible,
  no received NPC response. Only player hola was sent.
- GAMEPLAY_REGRESSION IMPLEMENTED_UNVERIFIED: walking/refusal/floor round-trip,
  sector streaming, Use/container and Follow/Stop exercised, but full required
  scope remains incomplete and camera acceptance failed. Session2 reports 71
  requested,63 accepted,9 rejected,0 unanswered; do not assume these counts
  partition requests (Follow-induced movement participated).
- PROTOCOL_ERRORS=0 in both EndPlay snapshots: residual bytes, unsupported
  opcodes and protocol anomalies each0. Viewport synchronized, actor counts
  match state, duplicate/orphan0. First session85 accepted/8 rejected/0 unanswered.

Unreal startup emitted engine Ensure IsInGameThread in AppTime.cpp. Client
continued; no source correction or clean-engine-log claim made.
Not PASS LOCAL; independent certification not started and readiness NO.
Final commit/push conditional on all required gaps passing is not authorized by
the unmet condition. Documentation/evidence left for review, no code edits.
