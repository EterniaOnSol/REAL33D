# Historical Runtime Classification

Artifact: `tibia-game.tarball.tar.gz`, SHA-256 recorded in `SOURCE_MANIFEST.md`. It is never to be extracted wholesale into Git and no bundled executable may be run.

| Area | Classification | Intended handling |
| --- | --- | --- |
| `dat/objects.srv`, `conversion.lst`, `map.dat`, `mem.dat`, `moveuse.dat`, `monster.db`, circles/houses data | `REQUIRED_REFERENCE` candidate | Extract only in isolation; verify source compatibility before sanitized use |
| `origmap/` | `REQUIRED_REFERENCE` candidate | Prefer as pristine baseline after format/compatibility checks |
| `map/` | `POTENTIALLY_REQUIRED` historical mutable state | Do not use as clean baseline before comparison with `origmap/` |
| `npc/`, `mon/` | `REQUIRED_REFERENCE` candidate | Preserve after format and sensitivity review |
| save scaffolding and house ownership state | `POTENTIALLY_REQUIRED` | Recreate clean structure; do not reuse historical ownership/PID state |
| `map.bak/`, old data copies, documentation logs | `TEST_DATA` / `HISTORICAL_DATA` | Keep only in the original archive unless bounded research requires them |
| `usr/`, `usr.bak/`, logs, histories, SSH/dotfiles, historical network/config values | `SENSITIVE` | Never publish or use for tests; exclude from extraction and Git |
| all `bin/` content | `LEGACY_BINARY_DO_NOT_EXECUTE` | Static inspection only under a future bounded task |
| untraced miscellaneous files | `UNKNOWN` | Quarantine until classified; never assume safe |

The sanitized runtime now does exactly this through `scripts/server/prepare_wsl.sh`: it selectively extracts the required reference candidates, creates new writable map/user/save areas, initializes a new SQLite database with two synthetic identities, and generates a fresh key shared by Login/Game. It never extracts historical accounts, logs, credentials, private keys or binaries. Exact materialized provenance is in `RUNTIME_DATA_PROVENANCE.md`.

`owners.dat` is deliberately not extracted. Game can create a new disposable `owners.dat` in its generated runtime data directory during shutdown; this generated file is not historical provenance.
