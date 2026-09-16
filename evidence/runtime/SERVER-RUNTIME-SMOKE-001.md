# SERVER-RUNTIME-SMOKE-001 Evidence

Date: 2026-09-15
Environment: WSL2 Ubuntu 26.04, Linux x86-64
Status: `CERTIFIED` after corrected local run plus independent live repetition

## Preconditions

- Immutable selected sources: Game `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, Login `f1c839fe7c0334fa036549a641487f21205d0129`, Query Manager `edea08d11cc306955d8d732164ec383d37ea1f62`.
- Game and Login compiled with explicit `-DTIBIA772=1`; Query Manager compiled with `DATABASE=sqlite`.
- Historical runtime archive hash rechecked as `67B771D1E3B4A6EF48C554B9B8B0DB56DA39CAE6B0DE5444F7BF6E71C0B2DE8E`.
- Selective reference data only; new SQLite state, credentials, accounts, writable map and RSA key.

## Commands

```powershell
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/prepare_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/runtime_smoke_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/stop_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
```

Three full service starts and three clean shutdowns were observed while hardening the orchestration. The final deterministic run produced:

```text
TCP_CONNECT_CLOSE port=7171 PASS
TCP_CONNECT_CLOSE port=7172 PASS
TCP_CONNECT_CLOSE port=7173 PASS
querymanager ALIVE exe=VERIFIED cwd=VERIFIED start=VERIFIED port=7173
game ALIVE exe=VERIFIED cwd=VERIFIED start=VERIFIED port=7172
login ALIVE exe=VERIFIED cwd=VERIFIED start=VERIFIED port=7171
IPCHANGER_RSA_DECIMAL chars=309 max_with_nul=312 PASS
SANITIZED_DB worlds=1 accounts=2 characters=2 PASS
GAME_WORLD sectors=9873 objects=8533464 PASS
RSA_SHARED_PRIVATE_MATERIAL_MATCH PASS
SECRET_ACCESS runtime=700 sensitive_files=600 PASS
SERVER-RUNTIME-SMOKE-001 PASS
All services stopped cleanly.
```

## Additional verified facts

- Query Manager applied the synthetic seed and logged authorized Game and Login connections.
- Login advertised server/client version `7.72`, world `Fusion Test`, and remained alive after an empty TCP connect/close.
- Game completed world load before Login startup. Opening its port was found to precede readiness, so the final script waits for the map-load marker.
- Preparation verifies the archive hash before extraction and requires independently recorded expected hashes for the data manifest and all three binaries. Runtime smoke repeats those comparisons.
- Runtime preparation now derives a decimal public modulus for the Fusion32 IP Changer; smoke proves equivalence with the hexadecimal modulus and enforces the 312-byte source limit without recording the value.
- Binary hashes matched prior build evidence: Game `E0E96B...17C7`, Login `23F0E7...788F`, Query Manager `504749...25E4`.
- The post-reset generated public-key fingerprint was `B99D7D00EB269DDFCF519375DC8652558C997661AEBBF550E1CB3AB7A83B68B7`; reset intentionally changed it from the preceding run.
- Recorded PIDs are correlated with Linux process start token, exact executable, exact cwd and owned listening socket.
- No PEM, credential, account auth blob, DB, generated config or full runtime log is retained as tracked evidence.

## Independent review history

The first implementation was `REJECT` because DrvFs yielded effective mode `0777`, archive/data/binary checks were self-referential, PID/listener identity was weak, and a secret-exposure claim was unsupported. That generated runtime was stopped and deleted. The corrected implementation uses WSL-native private storage, expected pre-recorded hashes, effective permission assertions, process identity/start/socket ownership checks and precise RSA-match wording.

`SERVER-RUNTIME-REVIEW-002 = ACCEPT`: an independent agent created a fresh runtime in its own Ubuntu-26.04 instance, then executed prepare -> start -> smoke -> stop. It observed all fixed hash gates, three verified process/socket identities, exact DB/world assertions, effective 700/600 modes, clean shutdown, no remaining 7171/7172/7173 listeners and no changes under `reference/`. Certification is limited to this evidence file's runtime scope.

## Scope boundary

This PASS covers service initialization, internal authorization, data/world loading, simultaneous liveness, bounded network connect/close, sanitized DB assertions and clean shutdown. It is not a classic-client login, character-list, world-entry, gameplay, protocol-parity or Unreal PASS.
