# Independent HUD live observations

Reviewer: Codex; operator provided normal mouse/keyboard input. Session started
2026-10-16 at22:09:16 UTC and closed normally at22:27:33 UTC. Review completed
2026-10-17 America/Guatemala. No implementation or server-rule changes.

Binary was built in the isolated certification checkout from merge55d1d466;
HEAD304fa11d adds only preserved certification documentation/evidence. No source
changes were made after the exact-merge build.

## Fresh captures

- `unreal_slice_evidence_Manual_20261016T221130_849.png` and same-name JSON:
  all six panels visible (Skills, Battle List, Containers, Minimap, Health,
  Inventory), plus separate chat. Gold3031 amount5 in inventory slot10 is drawn
  as the cumulative stack image with count5. Minimap marker32107,32204,7 matches
  the authoritative player; health65/166, mana15/15 and level4 are visible.
  Battle List shows the received Obi/Dixi entries. Containers correctly starts
  empty. Chat displays the received login message.
- Operator opened the equipped backpack with normal right-click and confirmed
  its contents appeared. `unreal_slice_evidence_Manual_20261016T221216_123.png`
  and JSON show one authoritative container, capacity20, three objects: worms3492
  amount6, shovel3457, rope3003. The worm stack image and count6 are visible;
  inventory gold5 and the other HUD panels remain present. Both stackables select
  frame4 through the unchanged generic frame selector; source/extractor/native
  tests are complementary evidence, not a replacement for these live captures.
- The operator explicitly confirmed Skills appears/disappears when its existing
  control-strip button is clicked (reply received2026-10-17). This is operator
  evidence; no separate hidden-Skills screenshot was captured. All six registry
  open/close/toggle operations were independently executed against the actual
  `SReal33DHUD` by CertificationHUDSixPanels (see exported automation results).
  No live Battle-button toggle is claimed. Its callback and all panel content
  implementations retain their reviewed routing/unchanged main behavior.

## Session and protocol

All five non-geometry snapshots are retained verbatim with hashes in
`protocol-summary.json`. Every snapshot reports residual_bytes0,
unsupported_opcodes0 and protocol_anomalies0. EndPlay records581frames,
1137commands,27walks requested/26accepted/1authoritative rejection/0unanswered,
one Use, one open backpack, zero duplicate spawns and zero orphan events.
Viewport synchronization remains true. WideWorld loaded56sectors. This is HUD
compatibility evidence, not a new full gameplay certification. No item movement,
combat, Follow, Look or Say is claimed for this session. Chat received login and
idle-warning messages; the latter is retained in the final transcript.

`real33d-events.log` preserves all LogReal33D lines plus exit markers from the
raw session log; its source path and SHA256 are in protocol-summary.json. Raw
geometry and engine asset-build noise remain in the ignored session directory.

## Startup recovery, separate from gameplay results

Cold shader/model generation delayed the initial launches. Once shaders were
ready, PID28912 could not connect because the existing Fusion32 game service
had exited. Its previous log ends with map/player saves and normal connection
shutdown, not an observed client protocol failure. The prior game log was saved
to build/certification/game-before-restart.log; only the selected existing game
service was restarted, with no data/rule/config edits. Its identity/port and
querymanager/login identities were checked by status_wsl.sh. The failed client
had exhausted three reconnect attempts without entering gameplay and was stopped.

The final PID10080/live-20261016T160844-two-workers reused the exact built binary
and per-process two-worker shader settings, connected to authoritative Fusion32,
then generated the captures above. Both early failed startup records remain in
the ignored build evidence. No startup failure is counted as a live PASS.
