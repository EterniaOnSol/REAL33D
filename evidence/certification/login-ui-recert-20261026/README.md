# UNREAL-LOGIN-CHARACTER-SELECT-UI-001 recertification

2026-10-26, Codex independent recertification, with operator-assisted death.
Status: CERTIFIED for the bounded acceptance matrix below.
Tested production revision: `444a89e268289cad31fd64260b04762bef73dfdc`.
Fresh worktree: `build/cert-login-ui-recert-20261026`; branch:
`certification/login-ui-recert-20261026`. Evidence/documentation changes only.

## Ancestry and publication

Actual sole-parent chain:
`1ac7245 -> bf54da4 -> 3562dd0 -> 316d888 -> 444a89e`.
The failed certification `3b82a9a8c87a5fbadcd17e9f50e0647f47fb384f` is a
separate child of `bf54da4`, not an ancestor of the fix or this certification.
The original implementation is present. The fixes remove account values from
existing evidence, replace numeric logging with process-local tags, and persist
a private minimap scope salt. ClientCore, protocol and server sources are
unchanged relative to `bf54da4`.

Normal push advanced `origin/milestone/unreal-login-character-select-ui-001`
from `bf54da4` to `444a89e`; remote hashes were independently checked afterward.
No force push, rewrite, failed-head merge, main merge, or certification push.
Main remains `1ac7245da68d602697c59f3241277ed43577737b`.
`ancestry.json` records complete parent IDs.

## Observed acceptance matrix

| Criterion | Result and evidence |
| --- | --- |
| Invalid login | PASS. Two independent process runs receive Fusion32's exact refusal, zero characters, and a usable account form. The next correct login succeeds. |
| Login, selection, entry | PASS. Real Slate QA controls in `live/cycles/login_qa.jsonl`: one server-provided Test Player B row; password box cleared; five successful game sessions in PID 11184. |
| Logout to selection | PASS. Five exits return to the retained list, zero tile actors, no local player, no retry. |
| Same-process relogin | PASS. Five sessions in PID 11184 and death/reentry in PID 11680, whose original process start time is preserved in `process-after-relogin.json`. |
| Exactly one INIT_GAME | PASS. Every successful observed session reports one. Too-fast entries refused by Fusion32 are separate unsuccessful attempts, not duplicate sessions. |
| World teardown | PASS. Cleanup snapshots show zero tile/creature actors and local ID, cleared inventory and containers. |
| World rebuild | PASS. Entry snapshots/transcripts have positive actor counts matching authoritative WorldState. Postdeath relogin rebuilds the temple scene. |
| Literal death | PASS, operator-assisted. Rats kill A on floor 8; server EventMessage `You are dead.` at 21:26:24.546Z, HP 0/150 at 21:26:24.595Z. Close and selection at 21:26:54Z; immutable copies in `live/death/death-observed/`. |
| Postdeath reentry | PASS. Explicit Enter Game at 21:27:30Z in the same process. Server temple origin (32097,32219,7), HP 150/150, one INIT_GAME. First north step accepted to (32097,32218,7). |
| Displacement | PASS. New PID 17936 displaced PID 2880 at 21:15:28Z. The operator explicitly reclaimed in 2880 at 21:15:34Z, displacing 17936. Its 21:16:57 observation remains at selection, over 80 seconds after close. Both logs contain zero automatic retry attempts. |
| Protocol errors | PASS. Unsupported opcodes, residual bytes and protocol anomalies are zero in all retained snapshots; harness log sweep also passes. |
| Raw account/password exposure | PASS for current evidence tree and new runtime output: zero each, independently scanned. No runtime output was redacted to obtain this result. |
| Screenshots | PASS. Eight new images individually inspected; no entered account number or password appears. The two historical login screenshots show an empty form or character selection. The number above the character list is the server MOTD, not the account field. |
| Scanner negative tests | PASS. Each of both real IDs and both real passwords planted separately yields scanner exit 1 and the expected credential kind. Clean control exits 0. Temporary planted files deleted. No values printed or published. |
| Minimap restart | PASS. Fresh installation cache created by PID 11184, then reused by PID 2880: same filename and private salt, `minimap cache load: accepted cells=192`. Cache names/content contain zero account/password values. Salt and private comparison baseline are excluded from evidence. |

The first restart process was manually re-entered by the operator at 21:14:32Z.
The auditor's later intended Enter Game click occurred after that entry and
produced an ordinary world walk. It is not claimed as the cause of that entry.
The operator explicitly confirmed the later ownership reclaim; this is not
automatic reconnect ping-pong. The untouched reconnect policy still supports
lost-link retries; none was deliberately induced here.

## Tests and reproducibility

Initial strict harness: ClientCore 14/14, native 12/12 including runtime object
metadata, secret_check 6/6, MSVC C++17 suites and C++20 archive PASS,
fresh REAL33DEditor build PASS (39 actions, 631.22 seconds), 12 PASS / 0 FAIL /
0 SKIP. Unreal automation independently passes 15/15, including CredentialLog.
Final strict harness: **12 PASS / 0 FAIL / 0 SKIP**, 107 logs checked,
118 produced files scanned for credentials; all suites/builds passed again.
Results are retained under `final-harness/`. Independent evidence verification:
**109 PASS / 0 FAILED**.

Run `python verify.py` from this evidence directory. It checks observed session
counts, invalid login, teardown/rebuild, literal death, process identity,
postdeath movement, ownership hold, protocol counters, minimap continuity,
negative scanner results and credential-scan counts. `verification.json` records
each assertion. `manifest.json` records original byte hashes for retained files;
all copied runtime artifacts have `redacted: false`.

`reproduce/` contains the executed launch, test, input and observation scripts.
They were run from `<fresh-worktree>/build/recert/`; copy them there to repeat.
Use the same configured sanitized runtime or adjust its path, never embed its
credentials. `security_audit.py` reads needles privately and independently scans
tracked evidence plus generated text, including UTF-16 output. Its planted-value
probe invokes the production `tests/scan_credential_values.py` unchanged.
`run-harness.cmd` enables `--strict --unreal --runtime` and uses the copied,
read-only objects.srv and classic executable for static byte checks. The classic
executable was never run. No test or production source was modified.

## Source authority and inspection

Startup contract and six required documents inspected; Git branch, HEAD, status,
remotes and real parent chain checked. Source selection is recorded in
`docs/protocol772/SOURCE_TRUTH.md`: game `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`,
derived/manual decompilation. `communication.cc` selects terminal 772 under
`TIBIA772`; the following lifecycle functions have no separate version branch.

- `reference/game/src/connections.cc::JoinGame`: takeover clears old identity,
  calls Logout, then TakeOver. `Die` changes connection state to DEAD.
- `crplayer.cc::TPlayer::Death`: sends player data and the literal death message,
  then calls connection Die. `crmain.cc::TCreature::~TCreature` calls Logout(30,true).
- `Real33DBridge.cpp::BeginAccountLogin`, `AccountTagFor`, `MinimapScopeSaltValue`,
  `StartWorker`, `EnterGame`, `TickSession`, `DrainEvents`, `NoteSession`:
  logging boundaries, cache addressing, one-worker guard and ownership behavior.
- `Real33DLoginQA.cpp::Start/Record/BuildScript`: real Slate controls; movement
  in this QA driver uses RequestWalk. Independent later walking uses SendInput.
- `Real33DLoginScreen.cpp::HandleLogin/HandleEnterGame/Refresh`: password clearing,
  character selection and phase visibility. `Real33DWorldActor.cpp::ClearWorld`
  and `HandleEvent`: teardown and before/after snapshots.

## Environment and limits

The three existing QA services were initially stopped; the normal startup script
started their existing sanitized binaries successfully. No character/map edits,
debugger mutation or fabricated death. The legitimate death penalty remains.
All launched QA clients were closed normally; final service status is retained.

The first isolated automation process failed before any test at Unreal's DDC
access. Its log is retained separately under `automation/`; the desktop repeat
under `automation-desktop/` passes. A duplicate coordinator's output-file conflict
was resolved before live testing. Two focus attempts were refused without input;
subsequent target-window activation succeeded. The optional computer-use package's
node_repl was unavailable, so existing repository Win32 certification input code
was reused, with exact-window focus recovery and normal WM_CLOSE added locally.

Account/password absence applies to the current files and new runtime outputs.
Old commits still retain their historical account-ID exposures; history was not
rewritten, as requested. Existing account IDs in the runtime generator and its
operator documentation are outside the evidence-output scan. Passwords and the
minimap salt remain outside Git.

UNVERIFIED outside this bounded matrix: multiple characters on one account,
empty/multi-world lists, waiting-list/game-login-error variants, deliberate
lost-link retry, nonzero premium days, repeated death variants and visual-art
parity. No broader gameplay or cryptographic anonymity claim is made.
