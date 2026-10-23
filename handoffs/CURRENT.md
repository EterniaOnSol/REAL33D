# HANDOFF - independent UNREAL-DEATH-LOOT-001 certification

Date: 2026-10-23. Agent: Codex.
Branch: certification/unreal-death-loot-independent-20261023.
Worktree: C:/Users/dell/Desktop/fusion32/build/cert-death-loot-20261023.
Starting audit: a76edadc01c579dde7e6eb726ad228be663130e6.
Ending commit: resolve the certification branch; evidence/docs only.
Main/origin/main: eda5cc236596911ba6020b57df3f448ff454a379, unchanged.
Audit branch normally pushed. Prior handoff archived alongside this file.

## 2026-10-23 independent UNREAL-DEATH-LOOT-001 certification

**CERTIFIED** for observed logical creature death, corpse Use and loot.
Audit a76edad (sole parent main eda5cc2) pushed normally; fresh branch
certification/unreal-death-loot-independent-20261023. No production or server
data changes, certification push, or merge to main.

Two real rabbits killed through the Unreal battle list; health 0, target clear,
creature removal and corpse 4173 on each correct floor-7 tile/stack observed in
server bytes. Right-click Use opens meat x2. One item moved corpse -> inventory,
the other corpse -> backpack; server updates yield 2 -> 1 -> empty, and reopen
confirms empty. Look, Use, drag, containers and Shift/Ctrl amount controls PASS.
First natural decay 4173 -> 4174 plus server container close PASS. Final decay
disappearance, other creatures/floors/loot variants and visual meshes UNVERIFIED.

Read-only packet observation plus unchanged ClientCore replay: 1,011 commands,
zero errors/anomalies; 28 connected client snapshots agree with server inventory
and containers. Evidence assertions 136/136. ClientCore 14/14, native 10/10 and
Unreal automation 10/10; fresh Unreal build and initial strict harness PASS.
Final corrected strict harness: 11 PASS / 0 FAIL / 0 SKIP; 298 logs, zero
anomalies; Git Bash secret_check 6/6. Native runtime metadata assertions PASS.
All live protocol diagnostics zero. No product defect found.

Initial game service recovery, normal idle disconnect/reconnect, stale-coordinate
refusal after natural decay, and a corrected harness metadata-path failure are
documented without promoting them into product PASS claims. Clients closed;
debugger detached; existing three server services healthy. QA A retains two
legitimately looted meat units, one in slot 10 and one in backpack.

See [independent certification](evidence/certification/death-loot-20261023/README.md).

Next: review the evidence-only certification commit for the requested scope.
Do not merge main or push certification without authorization. If expanding
decay coverage, watch all three natural 1,200-second stages without runtime edits.
Reproduction: evidence/certification/death-loot-20261023/verify.py and README.md.
Raw live data, observer disassembly, replay and validation logs are retained there;
ignored build/certification retains original working artifacts.
Both Unreal sessions are closed; read-only gdb detached; game PID 20261,
querymanager 754, login 1504 remain running and verified. No fake corpse logic,
protocol changes, memory edits or fixture creature spawns were introduced.
