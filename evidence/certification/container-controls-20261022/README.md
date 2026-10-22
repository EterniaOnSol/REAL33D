# UNREAL-CONTAINER-CONTROLS-001 independent certification

**CERTIFIED for the requested live matrix.** Tested implementation
`2136e6f63dd12f73c2575a87d7b44af22aa0c66a`, whose sole parent is main
`af251c9b2de0372c97ecbd128af0ada0c92bc166`. The milestone was normally pushed
before testing (`Everything up-to-date`); remote hashes were checked again after
testing. Fresh worktree: `build/cert-container-independent-20261022`, branch
`certification/unreal-container-controls-independent-20261022`. No merge into
main, production/test changes, server code changes, or server/data/inventory
editing. This commit adds certification tools, evidence and documentation only.

## Method and authority

Two actual Unreal windows connected to the existing local 7.72 server with QA
accounts A and B. `native_ui_driver.py` sends foreground-guarded native keyboard
and mouse input, captures the real window, and presses the existing F9 evidence
key. It never invokes game handlers or synthesizes protocol/state. `probe.py`
records before/after snapshots and gestures; `launch.json` records each process,
arguments, source revision and DLL hash. GUI screenshots were reviewed visually.

Authority: selected Fusion32 game revision
`386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, `receiving.cc::CCloseContainer` /
`CUpContainer`, `sending.cc::SendContainer` / `SendCloseContainer`, and the
7.72 command definitions. These container commands have no distinct TIBIA772
layout branch; that build flag selects the 772 login/version handling.
`SendContainer` reports a parent only for a container inside a non-body container.
The existing ClientCore builders and `Real33DBridge.cpp::DrainContainers` send
the server number; HUD handlers do not remove or navigate containers themselves.

`server_observer.py` attached GDB hardware breakpoints to the running game,
reading decoded IDs and the sixteen-slot open-container table. It neither calls
inferior functions nor writes process memory. Offsets and register use are
documented in the script and retained disassembly. One Close handler was held
for 12 seconds **before mutation/response**, then continued, to make the wait
observable. SIGUSR1/SIGUSR2 were passed through. Two earlier observers detached
on SIGUSR1; those failed observation attempts are retained, not counted as passes.
The successful observer was detached at completion. Pre-existing query/game/login
services remain running with their original PIDs; certification clients are closed.

## Live results

Paths below are relative to `live/`; each numbered action has before/after JSON,
an input record, and selected actions also have an original PNG.

| Requirement | Result and independently recorded evidence |
| --- | --- |
| Real container opens | PASS: `B-round1/01-open`, backpack 2854, ID 0, capacity 20, actual coins/rope/shovel. |
| Close sends 135; wait for server | PASS: `B-round1/02-close-held` has close counter 1 and ID 0 still open at 15:46:29.039Z. Server held 15:46:28.267Z through 15:46:40.274Z; emitted Close 111 at 15:46:40.281Z. `03-close-response` has no container. |
| Reopen | PASS: `B-round1/04-reopen` and `07-reopen-expanded`; same contents return, expanded. |
| Minimize local; server remains open | PASS: `B-round1/05-minimize` changes no request counter or container state; subsequent `06-close-minimized` finds the same object 19280191 in server slot 0 before Close. Clean visual repeat `B/07-minimize`, `08-expand`. |
| Drag/drop | PASS: `B-round1/08-drag` sends one move and receives rope-first content order. `B/14-stack-restore` and `24-order-restore` also receive actual server updates. |
| Ctrl amount | PASS: `B-round1/09-ctrl-amount` opens 2 of 2; `10-cancel` sends nothing. `B/11-ctrl-amount`, `12-amount-one`, `13-amount-confirm` send exactly one move only after OK, and server returns 1+1 coins. `14-stack-restore` reunites them. |
| Trade visibility | PASS: idle hidden; first prose invitation still hidden (`B-round1/11-trade-invited`, `A/00-return-invited`); own/reciprocal offers show Trade; one acceptance keeps it open with inventory unchanged; completion hides it. Reject also hides it (`A/04-reject-wrong-slot`, `B/18-invited-return`). |
| Channels beside Local Chat | PASS: `B/09-channels` shows server channel list immediately beside chat; `10-channel-join` opens Game-Chat alongside Local Chat. `25-leave-channel` leaves normally. |
| Legitimate nested container | PASS: A walked to B and traded its backpack **with existing bag inside** for B's rope. Both players accepted normally. `B-round1/17-nested-open` has parent true. No object was created or edited. |
| Up 136 and authoritative parent | PASS: `B-round1/18-up-parent` server receives Up ID 1 and returns parent under ID 1, equal to root contents and parent false. `B/05-up-id2` server receives Up ID 2 and returns backpack parent for the bag under unchanged ID 2. |
| Multiple/middle close | PASS: `B/03-third` contains IDs [0,1,2] with parent flags [false,true,true]; `04-close-middle` server receives Close ID 1 and leaves [0,2] with other contents unchanged. |
| ID routing after reflow | PASS: second visible window is now ID 2. `05-up-id2` sends 136/2; `06-close-id2` sends 135/2, leaves ID 0 intact. Both confirmed by server trace. |
| Authoritative state | PASS: delayed Close, server parent responses, exact surviving IDs, authoritative moves and two-party Trade snapshots agree. No UI-side speculative container removal. |
| Protocol diagnostics | PASS: all **168 valid snapshots** have residual_bytes=0, unsupported_opcodes=0, protocol_anomalies=0. Final strict harness scans 277 logs with no protocol anomaly. |

`audit.py` repeats 27 assertions against the published snapshots and server trace:

```powershell
python evidence/certification/container-controls-20261022/audit.py
```

## Restoration and interruptions

The reciprocal normal Trade restored A's complete original inventory, including
its backpack and bag, and B's original inventory and backpack contents/order
(`A/09-restored`, `B/24-order-restore`). During restoration the operator selected
A's jacket slot once; the displayed item ID exposed the mistake. Reject cancelled
that offer without any transfer; the next offer used rope 3003 correctly. This
operator mistake is retained and is not an implementation failure.

The first window was closed before active testing. Round-one windows closed during
the long pause; the resumed PID guard aborted before sending input. A new session
reopened the legitimately acquired items. Server trace activity after the last
round-one scripted action is retained but not used to certify multiple-container
routing; that test was independently repeated in the resumed session.

Disk exhaustion affected the initial build and final cleanup capture. Generated
files in this worktree were NTFS-compressed without deleting unrelated files.
Closing both clients recovered about 4 GB. Four incomplete JSON artifacts are
preserved and explicitly listed by `audit.json`; no success is inferred from
them. Final shutdown snapshots are unavailable/incomplete. This happened after
all matrix checks and inventory restoration had complete evidence. A's optional
walk back to its initial position was unfinished: last complete observation
(32098,32216,7), then Escape/W were sent while screenshot writes failed; exact
final position is unverified. B remained at (32096,32218,7). The walk helper's
old message incorrectly said "refused" when a focused UI consumed a key; counters
show no request was sent, not a server rejection. Its diagnostic now distinguishes
that case. No movement certification is claimed here.

An incidental notification in the first minimize screenshot is excluded from
publication; the JSON remains and clean minimize/expand screenshots were repeated.
All selected published screenshots were visually inspected. Raw artifact hashes
are in `raw-manifest.json`; source logs and incomplete JSON are retained unedited.

## Validation

| Check | Result |
| --- | --- |
| ClientCore | 14/14 PASS |
| Native Unreal-independent suites | 10/10 PASS |
| Unreal automation | 10/10 PASS, 0 errors, 0 warnings; `validation/automation-report.json` |
| Unreal build | PASS, REAL33DEditor Win64 Development, UE 5.8 / MSVC 14.44.35207 |
| MSVC integration | C++17 suites and C++20 archive PASS |
| Final strict harness | 11 PASS / 0 FAIL / 0 SKIP, `validation/harness-final/REGRESSION-HARNESS-20261022T211355Z.md` |
| secret_check | 6/6 PASS in final harness; repeated on staged certification files |
| Recorded-live audit | 27/27 PASS, 168 valid protocol snapshots |

Initial strict harness had 10 PASS / 1 FAIL, solely the Unreal build: disk-full PCH
with MSVC 14.50. Subsequent attempts recorded a sandbox UBA failure and corrupt
partial PCH. The successful full build used explicit compiler 14.44, two parallel
actions, and no UBA detouring. Logs and excerpts are retained under `validation/`.
The final harness rebuilt the MSVC archive and relinked the unchanged module;
live/automation used the earlier successful DLL. Both DLL hashes are recorded in
`provenance.json`; source is identical. No test or production fix was introduced
to make certification pass. Native optional runtime MultiUse assertions were not
enabled; this is outside the requested container-controls matrix.

Reproduce the harness using the command in `validation/run-harness-final.cmd` in
a fresh worktree with the documented runtime assets and toolchain. For live
repetition use normal gameplay to obtain containers and inspect each screenshot
before issuing the next gesture. PIDs, coordinates, GDB offsets and object handles
must be revalidated for the new process; do not replay these blindly.

## Remaining IMPLEMENTED_UNVERIFIED outside the requested matrix

Invalid/stale container-ID requests; Up returning Close for a body/map parent;
collapsed presentation across reconnect; all sixteen simultaneous slots. The
requested Up/nested/multiple cases are **CERTIFIED**, superseding the implementation
handoff's unavailable-second-container limitation. Final cleanup position and
shutdown-snapshot limits are disclosed above and do not extend the certification.

Ready for main integration review; **main was not merged**. Certification commit
is local; only the explicitly authorized milestone push was performed.
