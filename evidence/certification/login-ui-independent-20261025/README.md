# UNREAL-LOGIN-CHARACTER-SELECT-UI-001 independent certification

2026-10-25 America/Guatemala (2026-10-26 UTC). **CERTIFICATION = FAILED**
against the complete requested acceptance criteria. The observed gameplay/UI
matrix passes; the literal requirement that credentials be absent from logs
fails because account identifiers are logged. No production fix is included.

Implementation `bf54da4c576859fdd7ac7188d7a9f925e966d421` has exactly one parent,
main `1ac7245da68d602697c59f3241277ed43577737b`. Normal implementation push was
executed and returned Everything up-to-date; both remote hashes matched.
Fresh worktree `build/cert-login-ui-independent-20261025`, branch
`certification/login-ui-independent-20261025`. No merge of main, force operation,
protocol/server source change, fabricated character or character-data edit.

## Observed results

| Criterion | Result and scope |
|---|---|
| Invalid login | PASS. Real server refusal `Accountnumber or password is not correct.`; form remained usable and the next login succeeded. |
| Character list/select | PASS. Server offered one Test Player B row; selection and entry used the existing Slate driver. No live multi-character claim. |
| Entry/logout/relogin | PASS. Five entries and five logouts in PID 2364, no application restart. Logout returned to the retained list. |
| Teardown/rebuild | PASS. All five driver logout observations report zero tile actors and no local player. Retained cleanup snapshots additionally show zero creature actors. Re-entry tile counts match WorldState. |
| INIT_GAME | PASS. Exactly one per observed successful session, including post-death entry and takeover. Counts reset per worker. Refused entries are not successful sessions. |
| Movement | PASS. Five QA sessions each accepted a walk. Driver calls RequestWalk; physical keyboard input is separately evidenced by operator movement after death. |
| Literal death | PASS, operator-assisted. PID 2512 received `You are dead.` at 04:13:56.957Z and HP 0/150. At 04:14:26.913Z it returned to character select. Screenshot and immutable cleanup copy retained. |
| Death cleanup | PASS. Zero tile/creature actors and local ID, inventory/containers cleared, no retry. No local fabricated respawn. |
| Relogin after death | PASS. Same PID 2512, same process start. Explicit operator entry at 04:15:24, InGame 04:15:26.069Z. One INIT_GAME, temple origin (32097,32219,7), HP 150/150. First north step accepted at 04:15:29.552Z; east step also accepted. |
| World after death | PASS. 395 WorldState tiles = 395 tile actors, two visible creatures = two actors. Final position (32098,32218,7), adjacent to Cipfried's temple. |
| Displacement/takeover | PASS for observed ownership behavior. New PID 17140 entered at 03:29:02 and displaced PID 2364. Operator explicitly re-entered the old client at 03:29:21, displacing 17140. Neither log contains an automatic reconnect attempt; the displaced new client remained at selection until normal closure. The explicit operator reclaim is not an automatic ping-pong. |
| Credentials | FAILED for account identifiers: four occurrences in raw QA log/transcript. Passwords: zero occurrences across the retained raw live text files. Published evidence copies redact IDs, and preserve raw-file hashes. Sanitization does not turn the product result into PASS. |
| Protocol | PASS. All retained snapshots have unsupported_opcodes, residual_bytes and protocol_anomalies = 0. |

The five-cycle QA uses the implementation's existing opt-in driver, independently
launched and checked. Death/relogin and later gameplay were operator-assisted;
this report does not claim an unattended death test or a new server packet trace.
`verify.py` independently checks retained observations: **37 PASS / 0 FAILED**.
One passing assertion reproduces the credential finding; this is evidence
integrity, not a claim that all acceptance criteria passed.

## Tests and build

- ClientCore CTest 14/14; native 11/11, including real runtime MultiUse metadata.
- Unreal automation 14/14; zero errors/warnings within those 14 test results.
  Engine startup printed Condition failed messages before the suite; full log
  retained, and these are not described as a globally error-free engine log.
- Fresh REAL33DEditor build completed 39 actions. Final unchanged-source relink
  passed after rebuilding the MSVC C++17 suites/C++20 library.
- Final strict harness: **11 PASS, 0 FAIL, 0 SKIP**, 42 logs scanned. It predates
  the operator's later death; `verify.py` checks the later death/relogin snapshots.
- secret_check 6/6. This check does not ban numeric account identifiers in logs.
  Separate literal-value scan found zero passwords and four account-ID occurrences.
- Published text evidence scan: zero runtime password/account-ID occurrences.

## Interruptions and limitations

Initial harness failures are preserved: incorrect objects.srv path and absent
ignored classic binary for static byte checks. Correct inputs passed; the classic
binary was never executed. The initial Unreal build was interrupted during
preparation and the harness misclassified its exit as no cmd.exe. Recovery and
final builds succeeded. Despite a copied local configuration, recovery output
shows UBA and compiler 14.50; no claim is made that this configuration disabled UBA.

The first sandboxed graphical process never connected and was later terminated;
desktop launches used separately named runs. Existing runtime startup required
archiving a verified stale PID lock. No server character/map changes were made.
The current log includes an earlier idle session ending at 03:56:06 and explicit
operator re-entry at 04:11:24. These precede the literal death and are preserved
as separate sessions. An initial verifier wrongly expected only two InGame entries;
it was corrected to account for all three, without removing observations.

The operator requested existing world and creature art. Two local content junctions
reuse existing imported assets; final screenshot shows the temple/player art.
Cipfried's unsupported outfit still uses a placeholder. No art certification is
claimed. The art launch is a separate earlier restart, not part of the demonstrated
death/relogin interval. User was told closure was safe after evidence preservation.

UNVERIFIED: switching between distinct characters, empty/multi-world account lists,
game-service login-error/waiting-list paths, lost-link reconnect, nonzero premium
days, additional death/penalty variants and comprehensive visual coverage.

## Reproduce and next action

Run `python verify.py` from this directory. `reproduce/` retains launch/build/scan
commands; runtime credentials stay outside Git. `SOURCE_NOTES.md` identifies
authoritative functions and version guard. `manifest.json` hashes evidence and
records original hashes for sanitized copies.

Before accepting the complete task, remove/redact numeric account logging in
`Real33DBridge.cpp::BeginAccountLogin` and `Real33DLoginQA.cpp::Start`, then repeat
the credential scan and affected login tests on that new revision. Do not certify
the original implementation by silently applying a fix in the certification branch.
