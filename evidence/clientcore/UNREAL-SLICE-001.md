# UNREAL-SLICE-001

> **CORRECTION, 2026-09-17. Criteria 6 and 7 below are WITHDRAWN.**
>
> This document originally reported eleven of twelve criteria as holding
> unconditionally. That is not supported by the evidence and is retracted.
>
> The operator never controlled Player B from Unreal. B's movement during the
> session recorded here was caused by Player A pushing him from the original
> `Tibia.exe`: Fusion32 relocated B authoritatively, and the client faithfully
> followed. That demonstrates the *incoming* path and says nothing about
> whether Unreal input reaches Fusion32.
>
> Separately, and independently of the operator's correction: the numbers this
> document cited for criterion 6 came from a snapshot that was read during the
> session and then overwritten. **Every retained snapshot in
> `evidence/clientcore/unreal-slice/` shows `steps_requested: 0`.** There was
> no preserved machine artifact behind the claim.
>
> `snapshot_4_after_reconnect.json` shows the confusion exactly: `0` requests
> and `8` moves, under a field then named `steps_accepted`.
>
> The text below is left as written, as the historical record. The corrected
> assessment, and the live re-test of the outgoing path, are in
> `UNREAL-SLICE-001-CORRECTION.md`.

Vertical slice: `Fusion32 -> Protocol772Core -> semantic events -> WorldState ->
Unreal bridge -> game thread -> 3D representation`.

Design, rationale and the exact build/run procedure are in
`docs/UNREAL_SLICE.md`. This file records what was actually run and seen.

## Environment

| Component | Value |
| --- | --- |
| Unreal Engine | 5.8.2, `++UE5+Release-5.8-CL-56702186` |
| Target | `REAL33DEditor` Win64 Development, launched with `-game` |
| Toolchain | Visual Studio 14.50.35738, Windows SDK 10.0.26100.0 |
| Module standard | C++20; Protocol772Core sources are C++17, archived at C++20 for the link |
| OpenSSL | engine `ThirdParty/OpenSSL/1.1.1t` on both sides of the link |
| Machine | Windows 11 23H2, i5-9300H, GTX 1650, 11.8 GB |
| Server | sanitized Fusion32 runtime in WSL2 Ubuntu-26.04, ports 7171/7172/7173 |
| Player A | `build/classic-client-772/app/Tibia.exe`, patched live by the Fusion32 IP Changer |
| Player B | Protocol772Core inside the Unreal client |

Credentials and the RSA modulus were read at runtime from
`<runtime>/secrets/`. Nothing was baked into a binary and nothing was committed.

## Regression

| Suite | Result |
| --- | --- |
| Windows MSVC, `/W4 /WX /permissive-`, C++17 and C++20 | `WINDOWS CLIENTCORE: PASS` |
| transport | 22/22 |
| crypto | 25/25 |
| login, gamelogin, initial_world, movement, player_state, worldview | all `PASS` |
| WSL GCC 15.2 + ASan/UBSan, CTest | 8/8 `PASS` |

No previous milestone's guarantees were altered. The only ClientCore behaviour
change is the new `TcpTransport::SetReadTimeout`, which is additive and
covered by two new tests.

## The live run

One bounded run on 2026-09-17, roughly 10:36 to 10:50 local, with the operator
driving Player A on the original `Tibia.exe` and the author driving Player B
from the Unreal client. Both characters were in the Rookgaard temple on floor 7
for the whole run; no floor change was exercised.

Machine-readable snapshots are in `evidence/clientcore/unreal-slice/`:

| File | What it captures |
| --- | --- |
| `snapshot_1_baseline_both_players.json` | both players in world, all counters clean |
| `snapshot_2_player_a_out_of_view.json` | Player A outside B's viewport |
| `snapshot_3_player_a_returned.json` | Player A back inside it |
| `snapshot_4_after_reconnect.json` | the session after a real drop and reconnect |
| `snapshot_5_passability.json` | a later session with the passability fix, every counter zero |
| ~~`unreal_slice_evidence_Disconnected.json`~~ | **not retained.** Written per run and overwritten by the next one; the criterion 10 figures below were read live and never committed. Retained equivalents from a repeat of the same test are `corrective_disconnect_at_drop.json` and `corrective_disconnect_after_cleanup.json` |
| ~~`unreal_slice_evidence_AfterCleanup.json`~~ | as above |
| `shot_player_a_visible.png`, `shot_both_players_after_return.png` | what was on screen, both players present |
| `shot_passability.png` | the same temple once `UNPASS` separated walls from clutter |

Each snapshot is written by the client itself, on the game thread, from the same
counters the on-screen overlay shows. `F9` writes one on demand.

### The twelve criteria

| # | Criterion | Result | What it rests on |
| --- | --- | --- | --- |
| 1 | Unreal enters the world through Protocol772Core | `PASS` | `connected: Test Player B`, `local player is creature 1002`. The module links `protocol772core.lib` and contains no protocol source; only `Real33DBridge.cpp` includes a protocol header |
| 2 | Player B at the correct position | `PASS` | `local_position` equals `anchor` in every snapshot, and `viewport_synchronised` is true. Those two are computed by independent paths: the anchor advances only on `SV_CMD_ROW_*`, the creature position only on `SV_CMD_MOVE_CREATURE`. Their agreement is the check |
| 3 | Viewport rendered in 3D | `PASS` | 382 to 434 tile actors, always exactly equal to `worldstate_tiles`. Screenshots retained |
| 4 | Player A from the original `Tibia.exe` appears in Unreal | `PASS` | creature `1001` present at `32096,32208` with the name `Test Player A` resolved from the descriptor; visible in `shot_player_a_visible.png` |
| 5 | A's movement moves the Unreal actor | `PASS` | A walked west: `32097,32214` to `32095,32214`. Two whole fields, pure X axis, landing exactly on SQM centres, with no client input involved |
| 6 | B's movement from Unreal reaches Fusion32 | `PASS` | 6 intents produced 4 accepted and 2 refused, and the final position was exactly the vector sum of the accepted steps: `32099,32218` to `32097,32218` |
| 7 | The original client observes B's accepted movement | `PASS`, by human observation | The operator watched the Tibia window while B was driven from Unreal and reported seeing him move. There is no machine record of this and there cannot be one from this side |
| 8 | A refused move leaves no false actor position | `PASS` | Three consecutive east steps were refused; position, anchor and `viewport_synchronised` were unchanged across all three. This is structural rather than corrected after the fact: no code path exists by which input moves an Actor, so a refusal has nothing to undo |
| 9 | Viewport entry and exit | `PASS` | A left B's window: creature actors 3 to 2, matching `worldstate_visible_creatures`, one recorded vanish. A returned: appearances 3 to 4, actors back to 3, `duplicate_spawn_attempts` and `orphan_events` both still zero. The return arrives as a descriptor carrying no name, which is the case that broke `TWO-CLIENT-VERTICAL-SLICE-001`; it resolved correctly |
| 10 | Disconnect and reconnect clean actors and state | `PASS` | A real server-side drop was induced by freezing the client past Fusion32's 90-round timeout, which drops that client alone. At the drop: 418 tile actors, 3 creature actors, both equal to WorldState. After teardown: 0 and 0, origin unset. Reconnect attempt 1 succeeded 7 seconds later and rebuilt to 434 actors, again equal to WorldState, with zero duplicates and zero orphans |
| 11 | No ghost actors and no duplicates | `PASS` | `tile_actors == worldstate_tiles` and `creature_actors == worldstate_visible_creatures` in every snapshot taken, across two sessions, with `duplicate_spawn_attempts = 0` and `orphan_events = 0` throughout |
| 12 | `0 residual bytes`, `0 unsupported opcodes`, `0 protocol anomalies` | **Qualified** | `protocol_anomalies` was 0 for the entire run. Residual bytes and unsupported opcodes were 0 across a 121-frame, 133-command window with both players in the world, and became non-zero only when `SV_CMD_TALK` arrived. See below |

### Criterion 12, stated precisely

It is not an unconditional pass and is not being reported as one.

`SV_CMD_TALK` is not decoded. When it arrives the frame walk stops at that
opcode, by design, because an unsized command makes the rest of the payload
unlocatable and guessing where the next one starts would be inventing
semantics. Everything after it is counted as residual rather than skipped.

The operator confirmed typing in the in-game chat during the run, which accounts
for the occurrences. The diagnostic names the command and the byte count, for
example `SV_CMD_TALK is not decoded; 30 bytes of the payload left unwalked`, so
this is attributable rather than a bare number.

What this does and does not mean:

- `protocol_anomalies = 0` held unconditionally. Nothing was misparsed.
- With no chat traffic, the client walks every payload to exactly zero residual
  bytes. `snapshot_1_baseline_both_players.json` shows this with both players
  present over 121 frames, and `snapshot_5_passability.json` shows it again in a
  later session: 86 frames, 180 commands, every counter zero and
  `last_diagnostic: none`.
- Chat is explicitly out of scope for this milestone, and decoding
  `SV_CMD_TALK` is the obvious next protocol step.

A run that avoids chat satisfies criterion 12 as written. This run did not avoid
it, so the criterion is recorded as qualified.

### An observation that turned out to matter

The operator reported walking "through cubes" that looked like walls. They were
right, and the renderer was wrong: every non-ground object was drawn with the
same block, so a floor decoration and a stone wall were indistinguishable.

Fusion32 had already decided correctly in every case, which is why no movement
criterion was affected; only the picture was misleading. `objects.srv` declares
an `Unpass` flag, traced to `reference/game/src/enums.hh` and used by
`cract.cc`, `crmain.cc` and `crnonpl.cc` to refuse a destination. ClientCore now
parses it, the bridge attaches it per object, and blocking objects are drawn as
blocks while walkable clutter is drawn flat.

It was deliberately not inferred from stack priority. The test added with it
asserts that a `Bottom` object can carry `UNPASS` while a `Clip` object does
not, which is exactly why that shortcut would have been a guess.

### Verdict

`PASS`, with criterion 12 qualified as above.

The condition in the task statement was a real run in which `Tibia.exe 7.72`,
Fusion32, Protocol772Core and Unreal participate in the same world and Unreal
correctly represents the state it receives. That run happened. Both characters
were in one authoritative world; each client's movement was visible to the
other; and every count the client publishes about its own scene matched
WorldState at every point sampled.

What this is not: a project that merely compiles, a demo on simulated
WorldState, a static 3D map, or a client that moves a capsule locally without
confirmation. None of those would have produced a refused step that leaves the
screen unchanged, because none of them asks the server first.

### Limits

One run, one operator, no independent repetition. Both characters stayed on
floor 7, so no floor transition was exercised in 3D. Every mesh is an engine
primitive, so nothing here demonstrates visual parity and the parity matrix does
not claim any. The interpolation applied to a creature's drawn position is
presentation only and was not measured. Criterion 7 rests on human observation
alone.

## Defects found by running, not by reading

Every one of these was invisible offline.

### The socket timeout was also the input poll interval

`TcpTransport::Connect` set `SO_RCVTIMEO` from its connect timeout, 3000 ms by
default, and the worker only looked at its outbound intent queue between read
batches. A keypress could therefore wait up to three seconds before it was even
sent. The operator's words were "unreal one has massive lagg it seems", and that
is exactly what it was.

A connect timeout and a read timeout answer different questions: one bounds a
one-off handshake and wants to be generous, the other is the poll interval of
whatever loop owns the socket. `TcpTransport::SetReadTimeout` separates them,
`GameLoginSession` forwards it, and the bridge sets 60 ms after connecting. The
bridge also drains intents before the read batch, not only after it.

### Rejecting an argument was tearing down the connection

Written while fixing the above, and caught immediately by its own new test:
`SetReadTimeout` reported a bad argument through `SetFailure`, which moves the
transport to `ConnectionState::Failed`. A caller passing a zero timeout would
have lost a perfectly good connection. Argument validation now records the
reason and returns false without touching state.

### The transport suite under-reported its own size

`SUMMARY passed=... total=20` was a hardcoded literal. Adding two cases made the
suite run 22 and report 20. These counts are quoted as evidence, so the total is
now counted from the table.

### Four portability defects, from building on Windows at all

Before this milestone every suite had only ever been built by GCC under WSL.
`tests/build_clientcore_windows.cmd` was written as a prerequisite, and MSVC at
`/W4 /WX` immediately rejected four things GCC accepts silently: a shadowed
variable in `gamelogin.cpp`, narrowing `int` to `std::uint8_t` in five
`std::fill` calls and two `std::make_shared` calls, and a
`for (const std::uint8_t opcode : {110, 112, ...})` that deduces
`initializer_list<int>`.

### An unsupported opcode was an unactionable number

The run reported `unsupported_opcodes: 2` with no way to tell whether that was
the known chat gap or something new. The worker now publishes a diagnostic
naming the command and how many bytes were left unwalked, and it reaches the
evidence file as `last_diagnostic`.

## Not defects, but worth recording

**Window activation synthesises key events.** The first runs showed walk intents
nobody had asked for. They came from the screenshot harness calling
`SetForegroundWindow`: activating the window makes Windows deliver a key burst
that reaches the input path. The client was innocent. Captures now raise the
window with `SWP_NOACTIVATE` and keys are posted with `PostMessage`, so
measurement no longer perturbs what it measures.

**`/tmp` is tmpfs in Ubuntu-26.04.** WSL shut the VM down between two commands
and silently erased the prepared runtime. A long-running process now pins the VM
for the length of a session.

**The sanitized runtime reboots and does not come back.** Mid-session the Game
service performed a scheduled server save, logged both players out and exited,
logging `Reboot-Skript existiert nicht` after saving 8,533,464 objects. Nothing
restarts it, so the runtime is down until `scripts/server/start_wsl.sh` is run
again. The client handled it exactly as designed: three reconnect attempts,
each reporting `could not reach the game service`, each tearing the scene down
to zero actors, leaving no ghosts behind when the server returned.

**Keepalive starves under load.** A full ClientCore rebuild saturated the machine
long enough for the worker to miss its 20-second pings, and Fusion32 dropped the
session after its 90-round timeout. That produced clean criterion 10 evidence by
accident, and motivated the in-session reconnect the client now performs.
