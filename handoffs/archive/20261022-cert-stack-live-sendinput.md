# Independent UNREAL-STACK-MOVE-CONTROLS-001 live resume

Date: 2026-10-21 America/Guatemala; evidence 2026-10-22 UTC. Reviewer Codex.
Branch: certification/stack-live-sendinput-20261021.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-stack-move-controls-20261021.
Starting HEAD b8b7f3252afaacc193948c686d85ac5f3f582da7. Ending HEAD is this
certification-only commit. Implementation remains 1abb013bf711978b9439b6abe836672b5f4148ac.
Main remains fbc0de992142e26cab0a80f813d2c09b10dc191a; not merged.

CERTIFIED for the exercised live scope. READY_TO_MERGE_MAIN=YES with the report's
explicit limits. Real OS mouse/keyboard through certification-only SendInput
helper, no game API/Slate callback/protocol injection or production changes.
Verified whole stack; held Shift one; held Ctrl dialog; clamp; Cancel/Escape;
inventory/world/container routes; container slots in the same bag; non-stackables;
server-received counts; unique moves; tier art 1/2; Look/Use/UseWith targeting and
server refusal regression. Controlled run2 has 34 F9 snapshots, 19 unique moves,
2 uses and 5 looks. EndPlay 355 frames/664 commands, residual/unsupported/anomalies
zero and last_diagnostic none. Inventory and bag equal run2 baseline exactly.
Unreal closed normally; services started by certification stopped cleanly.

The first session also contains intervening actions without matching agent input
records; these are excluded from independent certification. A tooling timing
failure at run2 step15 released Ctrl adjacent to mouse-up and moved the full pile.
Keeping Ctrl held for 300 ms past mouse-up fixes the external driver; step17 opens
the correct dialog and sends nothing. Both attempts are retained and disclosed.
No production defect was established or patched.

UNVERIFIED: two distinct containers, concurrent shrink with amount dialog open,
world 3D amount appearance, classic 10/25/50 thresholds, live tiers above 2,
second observer, successful environmental UseWith, ground->ground route.
Only one bag was naturally available. No fixtures/server data were fabricated.

Prior deterministic results retained, not repeated: ClientCore14/14, native7/7,
Unreal10/10/build PASS, Git Bash secret_check6/6, harness0FAIL. Production DLL
SHA256 62FD3AA4AE610C35AD8D3FFEC7141B105375882E1EB3E30E020EC47F9F6E6076 unchanged.
Read-only evidence audit PASS: 34 expected move totals, 19 unique requests, protocol
zero, inventory/container restoration. Changed files: certification helper/audit,
snapshots/screenshots/logs/report, PROJECT_STATUS/PARITY_MATRIX and handoff only.

Evidence: evidence/certification/stack-live-sendinput-20261021/README.md, audit.json,
run1 and run2. Reproduce only through a fresh PID and observed window coordinates;
the driver takes --pid, --activate, --drag/--modifier, --click/--right-click,
--key, --capture, --record. The live processes have exited; do not reuse their PIDs.
No further task authorized beyond reporting. Do not merge main automatically.
Prior infra handoff archived at handoffs/archive/20261021-cert-infra-native-ui-001.md.
