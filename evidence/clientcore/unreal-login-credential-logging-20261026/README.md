# UNREAL-LOGIN-CREDENTIAL-LOGGING-001 — the account number leaves the logs

2026-10-26. Repairs the one criterion the independent certification of
UNREAL-LOGIN-CHARACTER-SELECT-UI-001 failed
(`certification/login-ui-independent-20261025`, `3b82a9a`): **raw account
numbers appeared in runtime logs and in packaged evidence.** Passwords were
already absent, and still are.

No protocol change, no server change, no wire change. Login and character
select behave exactly as certified; the five-session live run below is the same
driver, through the same controls, against the same running Fusion32.

---

## 1. Root cause

The milestone classified only the **password** as a credential. The account
number was treated as ordinary diagnostic context and logged on purpose — the
code said so in as many words:

```
// The account number is logged; the password is not, and no call anywhere
// in this class puts it in a format string.
UE_LOG(LogReal33D, Log, TEXT("account login: account %u at %s:%d"),
    AccountNumber, *Config.Host, Config.LoginPort);
```

Two format strings took it, and the second one wrote to two places at once,
which is why the number reached evidence and not only a log:

| Site | Reached |
|---|---|
| `Real33DBridge.cpp::BeginAccountLogin` | `REAL33D.log` — once per login attempt |
| `Real33DLoginQA.cpp::Start` → `Record` | `REAL33D.log` **and** `login_qa.jsonl`, a packaged artifact |

The reason it survived review is in this directory's predecessor:
`evidence/clientcore/unreal-login-character-select-ui-001/credential-scan.txt`
searched for `ACCOUNT_A_PASSWORD` and `ACCOUNT_B_PASSWORD` and nothing else, so
it returned **PASS** over files that carried the account id. `secret_check.sh`
could not have caught it either — it bans tracked secret files and
`KEY=VALUE` assignments, not a bare number inside a log line.

An account number is half of the credential Fusion32 answers
`Accountnumber or password is not correct.` about. It is typed into the same
form, and it is the half that cannot be changed. It is now treated exactly as
the password is.

## 2. What the log says instead

`Real33DCredentialLog.h` is the only thing in this client that may put an
account in writing, and it has no parameter that could take a number or a
password. It produces a source and a tag:

```
account login: attempt 1, typed account (tag tmdt) at 127.0.0.1:7171
account login: attempt 2, typed account (tag tmdt) at 127.0.0.1:7171
login QA: start -- runtime-file account (tag tmdt) at 127.0.0.1:7171, 4 return cycle(s)
```

Every diagnostic the old line carried is still there and two are new:

* **which attempt** it was — what the number was really being read for, and a
  fact about the session rather than about the player;
* **where the account came from** — the player's form or the sanitized runtime
  account file. The old line did not distinguish these at all;
* **whether two logins used the same account** — the tag above is the same
  `tmdt` in all three lines, so a reader can see the mistyped password was
  retried on the *same* account. A different run tags the same account
  differently (`rhcs`, `cvqk` in an earlier run of this same fix), so nothing
  correlates one player across two runs' evidence;
* where the request went, unchanged.

**The tag cannot contain an account number, by construction.** Its alphabet is
sixteen consonants — `bcdfghjkmnpqrstv` — so no string this header produces
contains a decimal digit at all, whatever anything hashes to. A hex tag could
legitimately have come out as `7720`, which would have read as an opaque label
while being a leak. The salt behind the tag is drawn once per process from
`FGuid::NewGuid`, shared by the bridge and the QA driver so one account reads
alike in one log, and never written to a log, a file or an event.

### The one other place the number was derived into something persistent

`StartWorker` scoped the minimap cache by SHA1 over host, port, protocol
version, runtime path **and the raw account number**. Not a log — but every
other field of that preimage is printed in the log's own command line, leaving
a six-digit space anybody could enumerate, so the file name was recoverable
credential data. It is now `MakeAccountStorageScope`: the same per-account
separation, salted from `Saved/Minimap/scope.salt`, which persists so a cache
stays addressable across restarts and is never logged or published. Verified
live below.

## 3. Account-number exposures: 7 → 0

Counted by `tests/scan_credential_values.py` against the runtime's own
`secrets/credentials.env`, over `evidence/` of each **pushed** tree. Account ids
match on digit boundaries; passwords match as plain substrings.

| Tree | Files | Account ids | Passwords |
|---|---|---|---|
| `bf54da4` implementation, as pushed → `credential-scan-before-implementation.json` | 2165 | **7** | 0 |
| `3b82a9a` certification, as pushed → `credential-scan-before-certification.json` | 2225 | **7** | 0 |
| this tree, plus the harness output of this run → `credential-scan-after.json` | 2300 | **0** | **0** |

The certification branch's **own** published artifacts under
`evidence/certification/login-ui-independent-20261025/` were already redacted by
the auditor and scan clean. The seven are inherited from the implementation
branch, in artifacts of this milestone and of DUAL-CLIENT-LIVE-CAPTURE-001:

| File | Count |
|---|---|
| `evidence/clientcore/unreal-login-character-select-ui-001/screens/character-list.log` | 3 |
| `evidence/clientcore/unreal-login-character-select-ui-001/login_qa.jsonl` | 1 |
| `evidence/clientcore/unreal-login-character-select-ui-001/screens/login_qa.jsonl` | 1 |
| `evidence/clientcore/DUAL-CLIENT-LIVE-CAPTURE-001-session1.log` | 1 |
| `evidence/clientcore/DUAL-CLIENT-LIVE-CAPTURE-001-session3.log` | 1 |

All seven are sanitized **in a normal commit** that replaces the value with
`[REDACTED_ACCOUNT_?_ID]` — the marker the auditor used, so a sanitized line
from either source reads the same — and changes nothing else on the line. One
line per occurrence in the diff. **No history was rewritten and nothing was
force-pushed.** The last two come from a 2026-09-21 OTClient-based capture whose
harness no longer exists in the tree, so there is no code to fix for them; the
artifact is the whole of it.

### Deliberately NOT redacted

`scripts/server/prepare_wsl.sh` and `docs/SERVER_RUNTIME.md` carry the two
`ACCOUNT_?_ID` values as literals. They are not runtime output:
`prepare_wsl.sh` is the script that *assigns* those two fixed ids on every
preparation, and the doc documents it. Redacting the generator
while it goes on generating them would be theatre, and would delete working
operator documentation. The defect was a credential reaching a **log or an
evidence artifact**, and `tests/scan_credential_values.py` is scoped to
produced output for that reason.

## 4. Password exposures: 0 → 0

Zero before, zero after, in every scan above. Nothing about the password
handling changed; `FReal33DCredentials::Clear` still overwrites, the box is
still `IsPassword`, no format string takes it, and the QA driver still
overwrites its own copy when a run ends. `DescribeAccountForLog` has no
parameter a password could be passed through —
`tests/real33d_credential_log_tests.cpp::TheDescriptorIsAClosedForm` asserts the
output is fully determined by the source enum and the tag.

## 5. Regression tests

**`tests/real33d_credential_log_tests.cpp`** — 12th native suite, no engine, no
server. 9 cases. It proves the absolute property, which implies the one asked
for: *no string this client produces about an account contains a decimal digit*,
so no account number of any length can appear in one. It also sweeps the known
runtime pair explicitly over 512 salts, checks the tag alphabet is digit-free
and closed, checks the tag is stable within a run and uncorrelated across runs,
checks it still tells two accounts apart, and checks the storage scope is wide,
salted and never the identity. The digit predicate is **self-tested first** —
a check that cannot see a digit is indistinguishable from a clean result. No
swept value is ever printed.

**`REAL33D.Presentation.CredentialLog`** — 15th Unreal automation test. The same
rule inside the engine, through `FString::Printf` with `%hs` exactly as both
call sites use it, so what is asserted is the text that reaches the log and the
transcript.

**`credential-value-scan`** — new harness check, `tests/scan_credential_values.py`.
This is the end-to-end half: the runtime's **real** ids and passwords, searched
for literally in the logs and evidence a run actually produced. Needles come
from the gitignored secrets file, never from anything tracked, and only counts
and key names are printed, so its own output is publishable. Patterns are
self-tested against a planted copy before the targets are read, and the check
is FAIL — never PASS — when it cannot be made trustworthy. SKIP without
`--runtime`, because a scan with no needle is vacuous.

Both scanner modes were checked against planted real values before being
trusted: exit 1 naming `ACCOUNT_B_ID` and `ACCOUNT_B_PASSWORD`, and `--redact`
re-scanning the same files clean at exit 0. See `reproduce/commands.md`.

## 6. Live behaviour, unchanged

`live/`, PID 15832, one process, five sessions, no restart.

| Observed | Result |
|---|---|
| Invalid login | Fusion32's own refusal, verbatim: `Accountnumber or password is not correct.` 0 characters offered; still on the account form. |
| Character list | `1 offered [Test Player B@Fusion Test:7172]`; 1 row drawn; **password box cleared yes**. |
| Entry / logout / relogin | 5 of 5 sessions entered and logged out; back on the retained list each time with 1 row still offered. |
| INIT_GAME | Exactly **1** per session, all five. |
| Teardown | `tile_actors 0`, `creature_actors 0`, `local_creature_id 0` after cleanup; `562`/`566` tiles rebuilt per entry, matching WorldState. |
| Movement | A step accepted in every one of the five sessions. |
| Reconnect | `reconnect_attempts 0` across the whole run — a deliberate close is still never retried. |
| Protocol | `unsupported_opcodes`, `protocol_anomalies`, `residual_bytes` all **0** in every snapshot. |
| Credentials | `credential-value-scan` over the run: 0 ids, 0 passwords, 7 files. |
| Minimap cache | `minimap cache load: accepted cells=192` — the salted scope from the *previous* process resolved to the same file, so cache addressing survives a restart. |

The QA transcript's `result` is `PASS` and its summary reads
`sessions entered 5 of 5 expected; init_games in the last session 1; entries
Fusion32 refused in the last cycle 0; reconnect attempts across the whole run 0`.

## 7. Tests, build, harness

| | |
|---|---|
| ClientCore CTest | **14 / 14** |
| Native suites | **12 / 12** (11 existing + the new credential-log suite) |
| Unreal automation | **15 / 15**, 0 failed, 0 notRun, 0 inProcess, and `errors`/`warnings` 0 on every test |
| `REAL33DEditor Win64 Development` | `Result: Succeeded`, 27 actions, zero warnings |
| `build_clientcore_windows.cmd` | `WINDOWS CLIENTCORE: PASS` — C++17 suites and the C++20 archive Unreal links |
| Strict harness | **12 PASS, 0 FAIL, 0 SKIP**, 124 logs scanned |
| `secret_check.sh` | **6 / 6** |

`tests/REGRESSION-HARNESS-20261026T095548Z.md` and `tests/logs/` are that run.
`tests/automation-report-index.json` is the automation report.

## 8. Limits

* The live run is the five-cycle QA driver. Death, displacement and
  lost-link reconnect were certified separately by `3b82a9a` and were not
  re-run here; nothing in this change touches those paths.
* The tag is a 16-bit label. Two accounts can collide onto one in a log, which
  a label can afford — the native suite holds it to >3800 distinct over 4096
  consecutive accounts. The minimap **storage** scope is full 64-bit width for
  the opposite reason, so two accounts cannot share one cache.
* An account number typed into the account box is visible on screen, as it is
  in the classic 7.72 client. That is not changed, and it means a screenshot of
  a filled account form would show it. No screenshot in this directory does.
* `credential-value-scan` can only search for the values of the runtime it is
  pointed at. It is not a general secret scanner; `secret_check.sh` and
  `output-secret-scan` keep their own jobs.
