# HANDOFF - independent UNREAL-CONTAINER-CONTROLS-001 certification

Date/time: 2026-10-22, final validation after 21:13 UTC.
Agent/role: Codex, independent certifier.
Branch: `certification/unreal-container-controls-independent-20261022`.
Worktree: `C:/Users/dell/Desktop/fusion32/build/cert-container-independent-20261022`.
Starting commit: `2136e6f63dd12f73c2575a87d7b44af22aa0c66a`.
Ending commit: the certification commit carrying this handoff; resolve with
`git rev-parse certification/unreal-container-controls-independent-20261022`.
Prior handoff: `archive/20261022-container-controls-implementation-before-independent-cert.md`.

Objective completed: independently certify the requested live controls matrix,
normal milestone push, linear ancestry, fresh worktree, no main merge.

## 2026-10-22 independent UNREAL-CONTAINER-CONTROLS-001 certification

**CERTIFIED** for the requested matrix on implementation
`2136e6f63dd12f73c2575a87d7b44af22aa0c66a`, sole child of main
`af251c9b2de0372c97ecbd128af0ada0c92bc166`. Milestone normally pushed; fresh
worktree `build/cert-container-independent-20261022`; no main merge or production,
test, server/data/inventory edits. This supersedes the historical implementation
limitations below for Up, nested containers and multiple windows.

Live Close 135 held before server response for 12 seconds: window remained until
Close 111; reopen PASS. Minimize/expand local only, same open object independently
observed in server table. Second backpack with nested bag obtained by normal Trade;
Up 136 returned the authoritative parent. Three windows [0,1,2], middle Close left
[0,2]; shifted second window correctly sent Up and Close for ID 2. Trade idle,
invitation, offer, single acceptance, completion/rejection visibility PASS.
Channels beside Local Chat and server-authorized tab PASS. Drag/drop and Ctrl
amount Cancel/confirm/split/merge PASS. Borrowed items and original inventory/order
restored by normal Trade. All 168 valid snapshots: protocol residual/unsupported/
anomalies 0; independent recorded-live audit 27/27.

ClientCore 14/14, native 10/10, Unreal automation 10/10 (errors/warnings 0), build
PASS, final strict harness 11 PASS / 0 FAIL / 0 SKIP over 277 logs, secret_check
6/6. Disk-full failed attempts and four incomplete cleanup JSON files retained;
all requested live checks and inventory restoration predate those incomplete
cleanup captures. A's optional final position restoration is unverified. Remaining
IMPLEMENTED_UNVERIFIED outside scope: invalid IDs, Up-to-body/map Close response,
collapse across reconnect, sixteen simultaneous slots. Existing server services
left running; certification clients closed and debugger detached.

See [independent certification and evidence](evidence/certification/container-controls-20261022/README.md).

## Inspection, changes and evidence

Reviewed selected game revision 386fa9b8 CCloseContainer/CUpContainer and
SendContainer/SendCloseContainer, the matching runtime disassembly, ClientCore
builders, bridge intent queue and HUD routing/collapse/Trade/Channels behavior.
Native foreground input and F9 snapshots captured real windows. Read-only GDB
hardware breakpoints recorded actual decoded opcodes/IDs and server container
table; one response delayed 12 seconds. No server memory/data edits.

Changed PROJECT_STATUS.md, PARITY_MATRIX.md and this handoff; added archived
handoff and evidence/certification/container-controls-20261022. No production or
test changes. README has matrix, exact action labels, reproduction commands,
failure history and limits. provenance.json records remote refs and both DLL
hashes (final harness relinked unchanged source); raw-manifest.json preserves raw
artifact hashes. audit.py passes 27 assertions against the published evidence.

## Critical context and next action

Milestone remote is implementation 2136e6f; main remote remains af251c9.
Certification branch remains local. No further certification work is required for
the requested matrix. Ready for review/integration only when separately authorized;
do not merge or push this certification merely because it is ready.

Both players' inventories and B's backpack content/order are restored. B last at
(32096,32218,7). A's optional return walk stopped at last complete observation
(32098,32216,7), followed by Escape/W during disk-full screenshot failures; exact
final position unverified. Four incomplete JSON and absent B shutdown snapshot
are disclosed, not counted as passes. All requested behavior had already been
captured before cleanup. The first wrong restoration offer (jacket) was rejected
without transfer; the corrected normal rope/backpack trade succeeded.

Pre-existing WSL query 754, game 763 and login 1504 remain alive with executable,
cwd and start identity verified; debugger 8873 detached and both Unreal clients
closed. Original worktree and other agents' work untouched. Generated build files
in this worktree were NTFS-compressed to recover space; final free space about
4 GB. Do not start two memory-heavy clients without checking disk/pagefile space.

Useful review commands:

```powershell
python evidence/certification/container-controls-20261022/audit.py
git diff 2136e6f -- clientcore tests unreal/REAL33D/Source scripts
git status --porcelain
git ls-remote origin refs/heads/main refs/heads/milestone/unreal-container-controls-001
```
