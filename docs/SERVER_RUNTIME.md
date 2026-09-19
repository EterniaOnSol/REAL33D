# Sanitized Fusion32 7.72 Runtime

Status: `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` after independent live repetition. The selected local classic-client artifact set subsequently passed Login, character list, Game entry and a sustained session; historical acquisition provenance is `UNKNOWN` and independent client repetition is `NOT_STARTED`.

## Topology

| Service | Bind / endpoint | Working directory | Persistent local state |
| --- | --- | --- | --- |
| Query Manager | loopback TCP `7173` | `/var/lib/fusion32-server-baseline-772-$UID/querymanager` | `state/tibia.db` |
| Game | TCP `7172` (world row supplies `127.0.0.1`) | `/var/lib/fusion32-server-baseline-772-$UID/game` | `state/map`, `state/usr`, `state/save` |
| Login | TCP `7171` | `/var/lib/fusion32-server-baseline-772-$UID/login` | none beyond generated config/logs |

Each process must run in its own working directory. Query Manager initializes SQLite from `sqlite/schema.sql`, then applies the synthetic seed patch once. Game and Login authenticate to Query Manager with a random generated shared secret. Game reads `.tibia`; Login and Query Manager read `config.cfg`.

## Prerequisites

Validated on WSL2 Ubuntu 26.04 with G++ 15.2.0, GNU Make 4.4.1, Python 3.14.4 and OpenSSL 3.5.5. Game/Login require OpenSSL development headers/libcrypto. Query Manager is built with its bundled SQLite implementation. The historical archive must remain at repository root with the SHA-256 in `SOURCE_MANIFEST.md`.

## Reproducible workflow

From PowerShell, substitute the WSL mount path only if the workspace moves:

```powershell
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/prepare_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/status_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/runtime_smoke_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/stop_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
```

`prepare_wsl.sh` refuses to overwrite an existing runtime. `reset_wsl.sh` validates the exact `/var/lib/fusion32-server-baseline-772-$UID` target, stops services, removes only that generated tree, then prepares it again. Reset deliberately destroys local test passwords, keys and state and creates replacements. The runtime is deliberately WSL-native: DrvFs did not enforce POSIX secret modes.

### Why `/var/lib` and not `/tmp`

The runtime lived under `/tmp` until 2026-09-19. On this distribution `/tmp` is a `tmpfs` mount, so the tree was held in RAM and destroyed every time WSL stopped the distribution — which happens on its own once the last process exits. Discarding a runtime is supposed to be a decision `reset_wsl.sh` makes, not something an idle timeout does.

The loss was silent and it was not free. Every unplanned wipe destroyed the SQLite database, so both characters were recreated at level 1 and `TALK_YELL` went back to being refused; it destroyed the generated credentials, since `prepare_wsl.sh` mints fresh random passwords on every run; and it forced a full recompilation of all three binaries. A level-2 bump applied on 2026-09-18 was found gone on 2026-09-19 for exactly this reason, along with the backup file taken to protect it.

`/var/lib` is on the distribution's ext4 disk and survives shutdown. The runtime stays WSL-native, so the POSIX mode argument above is unaffected, and roughly a gigabyte of reference data stops occupying RAM on an 11.8 GB machine.

Startup order is Query Manager, Game, Login. Game opens its socket before it is ready; the start script therefore waits for the proven map-load marker rather than treating port `7172` as readiness. On error, it attempts a clean rollback. Shutdown uses `SIGTERM` for Login and Query Manager, and `SIGINT` for Game so the disposable smoke environment does not persist map mutations.

## Generated secrets and identities

Preparation creates one fresh 1024-bit PKCS#1 PEM and installs byte-identical private material for Game and Login with effective mode `0600`, below a WSL-native runtime root with mode `0700`. It writes the public-key SHA-256 fingerprint plus hexadecimal and decimal public-modulus forms. The decimal form is bounded and cross-checked for the Fusion32 IP Changer's 312-byte field. The private key, credentials, generated configs, database, binaries and logs live outside the repository and must never be copied into Git.

The seed contains only `Fusion Test`, accounts `772001`/`772002`, and characters `Test Player A`/`Test Player B`. Passwords, Query Manager authentication and salted account `Auth` values are randomly generated locally. Historical accounts are neither extracted nor queried.

## Reset contract

A reset returns database, map and user state to a fresh synthetic baseline by rebuilding the WSL-native runtime. It does not alter `reference/` or the original archive. It was executed successfully: the RSA fingerprint changed while the independently expected data and binary hashes remained stable. The selective historical data provenance and exclusion rules are in `RUNTIME_DATA_PROVENANCE.md`.

## Current gates

- `SERVER-RUNTIME-SMOKE-001`: `CERTIFIED` for sanitized internal startup/liveness/data/shutdown scope.
- `NETWORK-SMOKE-772-001`: `PASS`; connect/close on 7171, 7172 and 7173 did not terminate services.
- `SERVER-SHUTDOWN-001`: `PASS`.
- `CLASSIC-LOGIN-772-001`: `PASS`.
- `CLASSIC-CHARLIST-772-001`: `PASS`.
- `CLASSIC-GAME-ENTRY-772-001`: `PASS`.
- `CLASSIC-SESSION-SUSTAIN-001`: `PASS` for the observed interval greater than 30 minutes.

Runtime certification proves the selected components can initialize together with sanitized data. The later classic-client results prove the bounded Login/Game-entry flow for the exact recorded client hashes; they do not prove complete protocol behavior, gameplay parity or Unreal behavior.
