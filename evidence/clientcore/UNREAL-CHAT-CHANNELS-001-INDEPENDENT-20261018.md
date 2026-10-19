# UNREAL-CHAT-CHANNELS-001 independent certification

Status: CERTIFIED for the requested bounded scope. Codex independent reviewer,
2026-10-18 to 2026-10-19,
America/Guatemala. Live evidence was recorded on October 18; final validation
and evidence review continued October 19 after the user's instruction to proceed.
Candidate239f89f07430ecd4d4348d69f183a9935e4bd992; mainca3121c42e8db7902bd155572bcb42c3c852bb8f.
Fresh worktree: build/cert-chat-channels-20261018.
Branch: certification/unreal-chat-channels-001-20261018.

Preservation first: fetched origin; milestone remote absent. Git Bash
secret_check6/6 PASS; normal push of exact239f89f succeeded, including5LFS
objects,14MB. Remote milestone SHA independently queried and matched. Created
the fresh certification worktree only afterward, at exact239f89f. No main merge.
Root's untracked agent/,evidence/agent/,evidence/clientcore/unreal-action-bars/
and tools/ plus the earlier preservation stash remain untouched.

Scope: live channel list, server-confirmed Join, Leave button and tab-close,
separate transcripts, Local Chat, real channel send/receive, private messages
both directions, PanelRegistry and protocol0. Required tests:ClientCore13,
native5,Unreal automation/build,harness selftest10,full corrected harness0FAIL,
Git Bash secret6. Own-channel,guild,GM and request-queue modes remain explicitly
UNVERIFIED and are not blockers for this task.

Inspection: AGENTS,status,architecture,roadmap,parity,handoff,Git provenance,
SOURCE_TRUTH and candidate diff. Fusion32 selected game386fa9b:
receiving.cc::CGetChannels/CJoinChannel/CLeaveChannel/CPrivateChannel/CTalk;
sending.cc::SendChannels/SendOpenChannel/SendPrivateChannel; operate.cc::Talk,
ChannelAvailable/GetChannelName. Those routines have no local TIBIA772 guard;
communication.cc selects772 framing/login. ClientCore channels.cpp is unchanged.
Bridge maps decoded channel events, commits membership only in NoteChannel,
routes per-view transcripts in NoteChat and emits existing command builders.
UI uses actual server names; unknown list is separate from empty response.
Leave has no server acknowledgement. Private sender gets a server message,
not its own private echo; incoming private conversation is UI behavior.

Independent probe: temporary Real33DChannelsCertification.cpp, production files
unchanged. It runs only with -real33d-cert-channels. Primary B uses the actual
in-game HUD; peer A uses a separate real Bridge connection and actual production
chat/channels widgets. SButton::SimulateClick invokes the existing UI delegate;
it does not substitute the handler. The probe sets text fields, selects tabs,
reads received state/actual text, and posts normal requests. No fabricated
server events or gameplay data, no server/configuration edits. Peer A is a
test fixture UI rather than a second full rendered world; native mouse hit
testing is not claimed. Production source will exclude the probe at closeout.

## Independently observed live results

The new session is retained in
`unreal-chat-channels-independent-20261018/live/REAL33D.log`, with nine original
JSON snapshots, seven PNGs and the movement journal. It is not the author's
session. Primary B connected to the existing source-built Fusion32
baseline-772-0; peer A opened its own real connection. No legacy executable
was run and no runtime configuration or character save was edited.

The opt-in driver completed stages 0 through 14 and logged
`CHANNELS_CERT_RESULT=PASS step=15` at 2026-10-18T10:14:51.339Z.
All 58 assertion evaluations passed, none failed. Repeated assertions before
and after a screenshot frame are included in this count.

| Criterion | Result and direct evidence |
| --- | --- |
| Channel list | PASS. Both peers requested and received one list. The four received rows were 4 Game-Chat, 5 Trade, 6 RL-Chat and 7 Help. Driver compared every rendered row with the received entry; initial list was unknown. Log lines 1602-1634 and CertGame PNG. |
| Join | PASS. Both peers clicked Join for 4 and 6. The immediate request created no tab; received OPEN_CHANNEL preceded joined tabs. Four total requests/four opens, using server names. Log 1637-1665. |
| Leave button | PASS. A left Game-Chat through its production button, then B sent a distinct post-leave message. B received it; A's channel-receive counter stayed at 2 and no A transcript contained it after the observation interval. B subsequently used its visible Leave button too. Log 1853-1908. |
| Leave tab | PASS. A used RL-Chat's actual close button; B then sent another distinct message and received the echo while A received nothing. B closed its RL-Chat tab too. Both successful-wire leave counters reached 2, selected view returned to permanent Local Chat. Log 1909-1973; CertLeaves PNG. |
| Separate channel tabs/transcripts | PASS. Game-Chat and RL-Chat each rendered only its received message; actual text traversal and transcript reads excluded cross-channel and Local leakage. CertGame and CertRL PNGs; stages 3-5. |
| Local Chat isolation | PASS. A normal Say appeared in Local Chat and above the authoritative speaker, absent from both channel transcripts. Local retained server messages, including the private-send confirmation. CertLocal and CertLeaves PNGs; stage 6. |
| Channel send/receive | PASS. B sent four real messages, received four server echoes; A received the first two and neither post-leave probe. No optimistic echo was present immediately after send. |
| Private B to A | PASS. B requested the private conversation by A's received name, waited for the private-open response, then sent through the real chat control. A received the unique text in its private conversation with B. Sender's private transcript had no invented self-echo. Log 1779-1824. |
| Private A to B | PASS. A selected its production private tab and replied. B rendered the unique response under Test Player A, absent from Local and channel transcripts. CertPrivate PNG; log 1824-1851. |
| PanelRegistry | PASS. Seven registered real HUD panels closed, opened and toggled with the expected visibility state. The live HUD contained exactly one Channels widget. Log 1585-1600. |
| Protocol | PASS. All nine primary JSON snapshots have residual_bytes/unsupported_opcodes/protocol_anomalies 0. Primary EndPlay: 89 commands, 69 frames. Peer A final driver observation: 42 commands, 31 frames, all three counters 0. Both final private-send/private-receive counters 1/1. |

The five criterion screenshots were opened and visually inspected independently:
Game-Chat, RL-Chat, private reply, Local Chat and post-leave state. Text and tabs
match the log; channel rows return to Join after leaving. Existing skills,
vitals, minimap, battle list, inventory and containers remain laid out around
the chat area at 1600x900. World art and inventory sprite assets were not
mounted for this bounded UI run; their visible placeholders are not a new
visual certification or an implementation regression claim.

`-real33d-minimap-qa` enables existing screenshot support and also drove seven
normal walk requests, all seven accepted, none rejected/unanswered. These are
recorded in the original movement journal; they were not injected world state.

## Authority and implementation review

Selected authority is game revision `386fa9b` as inventoried in SOURCE_TRUTH.
Exact source anchors: `reference/game/src/receiving.cc:750` CTalk, `:911`
CGetChannels, `:920` CJoinChannel, `:949` CLeaveChannel, `:966` CPrivateChannel;
`reference/game/src/sending.cc:1438` SendChannels, `:1497` SendOpenChannel,
`:1523` SendPrivateChannel; `reference/game/src/operate.cc:2164` Talk, `:3390`
ChannelAvailable and `:3428` GetChannelName. These functions have no local
version guard; `communication.cc:27/:920/:949` selects the TIBIA772 path.

Received events and counters support the source behavior: Join gets an open
response; Leave has no acknowledgement; channel broadcasts include the sender;
private speech goes to the recipient while the sender receives a server message.
`channels_closed=0` after two successful leaves is therefore expected.
ClientCore channel builders/decoder are reused unchanged. The bridge alone
routes received views and posts existing commands. Widget callbacks do not
fabricate names, membership or received speech. The private tab opened on
incoming speech is explicitly a REAL33D presentation decision.

## Verification and reproducibility

The temporary driver source and launch script are retained alongside the raw
evidence. They were moved out of Unreal Source before final validation. A
`git diff --exit-code 239f89f -- clientcore unreal tests scripts` then returned
0; no diagnostic file remained in Source. No implementation defect was found.

Initial fresh MSVC C++17 suites/C++20 archive and full Unreal build PASS;
corrected driver incremental build PASS. A restricted-environment incremental
attempt exited 1 before UBT started; its two-line log is retained as
`unreal-build-probe-restricted.txt`. Retrying with required cache access passed.
This was an execution-environment failure, not a product build failure.

Final gates executed independently:

| Gate | Result | Retained artifact under unreal-chat-channels-independent-20261018/ |
| --- | --- | --- |
| ClientCore | PASS 13/13 CTest suites, 0 failures; fresh WSL build | harness/logs/clientcore-tests.txt |
| Native | PASS 5/5, including real runtime MultiUse metadata (204 types) | harness/logs/real33d-native-tests.txt |
| MSVC | PASS C++17 suites and C++20 archive | harness/logs/build-sanity-msvc.txt |
| Unreal build | PASS REAL33DEditor Win64 Development; UBT detected removed source; 3 actions; Result: Succeeded | harness/logs/unreal-module-build.txt |
| Unreal automation | PASS 6/6; 0 errors/warnings, no failed/skipped/unfinished tests | automation/index.json and automation-tests.log |
| Harness selftest | PASS 10/10 | selftest.txt |
| Corrected strict harness | PASS 9, FAIL 0, SKIP 0; 38 logs scanned | harness/REGRESSION-HARNESS-20261019T214118Z.md |
| Git Bash secret_check | PASS 6/6 | harness/logs/secret-check.txt |

Exact-source automation names: CameraAndFloors, ChannelsPanel, ChatTabs,
PanelRegistry, SkillsPanel under REAL33D.Presentation, plus
REAL33D.WideWorld.AuthoritativeWindow. The runner's console names only two
baseline tests; all six actual report entries were independently checked.
`verify-evidence.ps1` validates all six, the live assertions/stages, all nine
snapshot counters, both peers' final counters and the retained gate totals.
SHA256SUMS records the collected raw file bytes before Git text newline
conversion; PNG hashes are unaffected by that conversion.

Final harness invocation (Git Bash under VS2022 vcvars64, using the retained
run-bash.cmd and harness.sh):

```sh
bash tests/regression_harness.sh --out build/certification/harness \
  --objects-srv /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv \
  --unreal --strict --logs evidence/clientcore/unreal-chat-channels-independent-20261018/live
```

Final automation invocation after retiring the driver:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/client/test_unreal_presentation.ps1 -EvidenceDirectory build/certification/automation-exact
```

Preservation rechecked on October 19: remote main remains
ca3121c42e8db7902bd155572bcb42c3c852bb8f; remote milestone is exactly
239f89f07430ecd4d4348d69f183a9935e4bd992. The restricted network check failed;
the permitted retry returned both exact refs. No main merge, implementation
rewrite, rebase, cherry-pick or force push occurred. Certification adds only
evidence/status/handoff material on its separate branch, directly atop the
candidate. Ready for a separately authorized main integration.

To repeat independently, create a fresh branch/worktree at the exact candidate,
run `tests/build_clientcore_windows.cmd` under VS2022 vcvars64, temporarily copy
the retained driver into `unreal/REAL33D/Source/REAL33D/Private/`, and build
REAL33DEditor Win64 Development using UE5.8 Build.bat and that worktree's
uproject. With the sanitized Fusion32 services running and A/B available, run
the retained `launch-live.ps1` from the worktree root. It contains paths and
account labels only, no credentials. The production bridge reads runtime
credentials through its normal existing path. Wait for normal exit, review
assertions plus actual PNGs, then retire the driver and rebuild exact source.

## Explicitly UNVERIFIED

Own-channel creation/invite/exclude; live guild channel 0; GM/anonymous channel
modes; request queue opcodes 174-177 and channel 3; server-forced close 179;
Trade's repeated-send cooldown; overflow with many tabs; classic-client color
and display-lifetime parity. The first four groups are excluded from the user's
certification gate, not failures. Native mouse hit testing, a second fully
rendered peer world, full art coverage and global gameplay parity are not
claimed by this bounded automated UI certification.
