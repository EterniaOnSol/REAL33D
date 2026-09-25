# REAL33D-AGENT-MEMORY-001 progress

Historical state: `IMPLEMENTED_UNVERIFIED` (2026-09-24 20:36 -06:00). Branch:
`milestone/real33d-agent-memory-001`. The prior `REAL33D-AGENT-MVP-001` and
`REAL33D-AGENT-BRIDGE-001` live PASS results are separate.

The later two-session certification supersedes this progress snapshot. See
`REAL33D-AGENT-MEMORY-001-certification.md` and the retained artifacts under
`evidence/agent/memory/`.

## Scope and mechanism

The opt-in `R33D_AGENT_MEMORY=1` path stores only facts derived from accepted
`AgentObservation` values. Each record has an identity, category, source fixed
to `direct_observation`, observation ids and times. The Brain can read the store
to prefer a remembered hunting area; actions still pass the current observation
schema, state validator and rate budget. The normal 2D parser/model and
`g_game` dispatch path remain in place. No Fusion32 gameplay or protocol was
changed. With the memory option off, the prior bridge path has no memory IO.

## Tests

`C:\Users\dell\Desktop\REAL33D2D\build\vcpkg_installed\x64-windows\tools\luajit\luajit.exe tests/real33d_agent_test.lua`
printed MVP, BRIDGE and MEMORY PASS. The memory tests exercise JSON round trip,
source/identity isolation, provenance, category/field rejection, stale records,
route choices, current-visibility action refusal and the mock Brain's use of
the same validator. `git diff --check` exited zero (line-ending warnings only).

During review, malformed persisted numeric fields were found to load and later
reach arithmetic. Record validation now checks category field types; places
require integer x/y/z; failed updates use a copy and retain the previous
record. Tests cover malformed values and the unchanged previous record. A
rejected file no longer gets overwritten by a later session save. Another
regression test covers the observed floor loop: a remembered hunting area on
another floor is ineligible unless the agent has a route it actually traversed.
Identity keys and file names now hex-encode each identity byte, so names that
differ only by a space versus underscore cannot collide. The previous file-name
scheme is not migrated automatically; these fixes have not had a live rerun.

## Live observation and limit

The local REAL33D2D text log at
`C:\Users\dell\Desktop\REAL33D2D\real33d2d.log` was read without changing the
running client. At 20:26:28 a memory-enabled mock session entered on floor 8
with `memory=loaded`; it spoke, opened inventory, observed a Cave Rat lose HP,
then fought and looted rats while its own HP fell. At 20:29:39 the session ended
with `memory_saved=true records=56`. At 20:29:42 a later session started with
`memory=loaded` on floor 7 at full HP, proving that the client did load memory
again. Its movement then cycled through four nearby temple tiles while the
remembered hunting ground was on floor 8. The floor-selection guard above was
added after that run and has only a deterministic test, not a new live run.

The live session did not demonstrate prolonged survival: HP reached 2/160,
followed by a new session at full HP. The text log alone is insufficient to
certify specific remembered records, their use in a decision, or an
authoritative result linked to a memory-guided intent. Keep the memory
milestone `IMPLEMENTED_UNVERIFIED` until a correlated two-session JSONL trace
and on-disk memory file are retained and independently checked. The live
client used a separate REAL33D2D checkout; these final validation and floor
fixes have not been deployed to or retested in that checkout.
