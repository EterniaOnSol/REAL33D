# UNREAL-DEATH-LOOT-001 independent certification

2026-10-23, Codex. **CERTIFIED** for the live logical death/open/loot matrix
below. Final disappearance after all decay stages remains **UNVERIFIED**.
No production, protocol, server source or runtime data edits.

## Provenance and method

Main eda5cc236596911ba6020b57df3f448ff454a379 is the sole parent of audit
a76edadc01c579dde7e6eb726ad228be663130e6. The audit branch
milestone/unreal-death-loot-001 was pushed normally and independently checked
on origin. Main and origin/main stayed at eda5cc2. Fresh branch:
certification/unreal-death-loot-independent-20261023, worktree
C:/Users/dell/Desktop/fusion32/build/cert-death-loot-20261023.
Resolve that branch for the evidence commit; the live executable was built from
the unchanged audit head. DLL SHA-256:
e4622904c1daa1f0932cae43d400794c156766ee16bea5743c08a0a8d1e4901a.

The existing game process was gone after a normal save/shutdown. Its old log was
preserved; the already-built game service was recovered as PID 20261. Existing
querymanager 754 and login 1504 were preserved. No creatures were spawned,
teleported, damaged or looted through debug calls, packet injection or data edits.

native_ui_driver.py guards the exact Unreal PID/foreground ownership and uses
real Win32 keyboard/mouse input. probe.py records native gestures, screenshots,
F9 before/after snapshots and file-timestamp freshness. All 29 successful probes
have fresh snapshots. Probe 011 was disconnected and makes no gameplay claim.
Screenshots show the current placeholder presentation, not creature/corpse meshes.

Three hardware breakpoints read plaintext outgoing payloads before encryption
and allowlisted incoming gameplay commands. Login packets/credentials/keys are
not captured. No inferior calls or memory writes. The standalone Death symbol
was not hit in the optimized binary; no direct Death-call claim is made.
Health-zero and field commands prove the death sequence. Full disassembly,
observer trace and exact packet bytes are retained.

replay.cpp links unchanged ClientCore decoders and reconstructs each connection
from INIT_GAME/FULLSCREEN onward, respecting command lengths, tile priorities,
stack positions and container indices. verify.py checks the recorded evidence:
**136 PASS / 0 FAIL**, **1,011 server commands**, zero decode/state anomalies,
and **28 connected client snapshots** whose inventory/container state matches
the independently observed server bytes. This is wire-to-client agreement; it
does not claim a second independent implementation of the decoder.

## Live observations

| Check | Result and evidence |
|---|---|
| Creature death/combat | PASS. A normal battle-list attack with chase killed two real rabbits: 1073743323 at 22:42:57 UTC, 1073743321 at 23:04:22 UTC. Each has server CREATURE_HEALTH 140 reaching 0 and target-clear 163. Two attack commands total, one per combat. Repeated health notifications in a server payload are server emissions, not duplicate client attacks. |
| Removal | PASS. DELETE_FIELD 108 removes each exact creature from its tile stack. Subsequent client snapshots and battle list omit the dead creature. Existing CreatureVanished handler destroys its actor. |
| Corpse tile/stack | PASS. ADD_FIELD 106 adds type 4173 at (32112,32248,7) and (32105,32249,7), the dying creatures' tiles. Packet replay retains full before/after stacks, including the intervening blood pool; removal changes the corpse's final stack index normally. Reconnected first-corpse Use addresses stack 2 after blood expired. |
| Right-click Use | PASS. Probes A2/012 and 019 emit Use 130 and receive Container 110 for dead rabbit, capacity 5, meat 3577 x2. Both corpses open through the existing generic path. |
| Contents/counts | PASS. Exact Container 110 payloads and fresh snapshots agree on meat x2. This is the server's transmitted container contents; no invented expected RNG loot. |
| Corpse to inventory | PASS. A2/020 Shift-drag sends one Move 120, count 1, source container 1 slot 0 to body slot 10. Server SET_INVENTORY and CHANGE_IN_CONTAINER yield inventory meat x1, corpse meat x1. |
| Corpse to container | PASS. A2/021 ordinary drag sends remaining meat from corpse container 1 to backpack container 0. Server CREATE_IN_CONTAINER and DELETE_IN_CONTAINER yield backpack meat x1 and empty corpse. |
| Updates/empty corpse | PASS. Corpse progresses 2 -> 1 -> empty from server commands. A2/024 reopens it and receives a zero-item Container 110. |
| Decay | PASS for first natural stage only: at 23:02:57 UTC the first corpse closes via 111 and changes 4173 -> 4174 via 107, exactly 1,200 seconds after creation. Final 4174 -> 4175 -> disappearance was not watched. |
| Authoritative state | PASS for observed flow. All compared client inventory/container states match wire-derived state, map changes follow field commands, and no special corpse/loot state was added. Code trace: WorldState -> WorldView -> existing Unreal handlers. |
| Containers | PASS. Backpack and corpse coexist, update independently, corpse closes/reopens, and loot crosses between them. |
| Stack/amount | PASS. Shift splits one from two; A2/025 merges into two; Ctrl-drag 026 opens amount dialog with no Move; 027 selects one with no Move; 028 confirms exactly one Move and server returns one in inventory/one in backpack. |
| Look/Use/drag | PASS. Meat Look returns server description; correct Shift-left-click A2/023 returns "You see a dead rabbit (Vol:5)." Use/reopen and both drag destinations are exercised above. |
| Protocol | PASS. Every retained live snapshot reports unsupported_opcodes=0, residual_bytes=0, protocol_anomalies=0. Replay also has zero errors/anomalies. See final harness for the log scan. |

## Interruptions and limits

The first session hit the documented 15-minute idle warning and normal disconnect
during a pause. A2 is a fresh connection, not fabricated continuity. The first
corpse was opened just before natural decay; by A2/013 the container had closed.
That stale screen coordinate hit ground: Look described shallow water and the
server refused the ground Move. It is retained and explicitly **not** counted as a loot transfer.
The second kill produced the successful loot run.

Shift-right-click A2/022 invokes Use, not Look, and closed the open corpse. The
driver was extended only in certification tooling to support Shift-left-click;
A2/023 is the valid corpse Look observation. No production fix was warranted.
Early startup probes found no window and sent no gameplay input.

The first final-harness invocation supplied a Windows runtime-metadata path to
WSL; item-use-policy aborted at file.good() before testing metadata. Its FAIL is
retained under validation/final-harness. The corrected invocation uses /c/...
conversion; validation/final-harness-corrected is the final acceptance run.
The harness rebuilds/relinks the native archive; final DLL hashes can differ
from the live DLL without a source change. launch.json identifies the exact
live build, and the final build result is recorded separately.

Final disappearance after the remaining 20-minute decay stages is UNVERIFIED.
Only rabbit corpses/meat stacks on floor 7 were exercised; other creature loot,
floors and stack variants are not certified. Visual corpse meshes/animations
are not certified. Player death is outside this task.

## Validation and closure

Fresh ClientCore 14/14, native 10/10, Unreal automation 10/10
(0 failed, 0 warnings), MSVC archives and REAL33DEditor Win64 Development build
passed. Initial strict harness: 11 PASS / 0 FAIL / 0 SKIP. Final strict harness
also includes copied read-only runtime object metadata for MultiUse assertions;
final acceptance is 11 PASS / 0 FAIL / 0 SKIP, 298 scanned files with zero
anomalies, and secret_check 6/6. Exact logs are under validation/.

Both certification clients were closed normally. The observer detached cleanly;
all three server processes remain identity/port verified. QA A ends at
(32104,32250,7), health 141/155, with legitimately looted meat x1 in inventory
slot 10 and x1 in its backpack. The second corpse is empty. No unrelated client,
branch, worktree or evidence was removed. No merge or push of the certification
branch was performed.

## Reproduction

Retained artifacts are hashed in manifest.json. Run python verify.py from any
directory to recheck the retained evidence. Rebuild the offline replay with the
retained validation/compile-replay.sh, then run it against the matching
read-only runtime objects.srv and live/packets.tsv. Live reproduction requires
the existing authorized local runtime and QA account; launch/driver scripts
do not embed credentials. See input JSON files for exact observed gestures.
