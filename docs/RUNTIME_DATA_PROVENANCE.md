# Runtime Data Provenance

Source archive: `tibia-game.tarball.tar.gz`
Revalidated SHA-256: `67B771D1E3B4A6EF48C554B9B8B0DB56DA39CAE6B0DE5444F7BF6E71C0B2DE8E`

The archive remains untouched. Before extraction, `prepare_wsl.sh` requires the exact expected archive hash above. It asks `tar` for only the following bounded paths and materializes them below WSL-native `/tmp/fusion32-server-baseline-772-$UID/game/reference`.

| Source | Files | Destination | Why required | Classification / sanitization |
| --- | ---: | --- | --- | --- |
| `dat/{circles,map,mem,houseareas,objects,conversion,houses,monster,moveuse}.*` exact names in script | 9 | `game/reference/dat` | object/map/config tables read during Game initialization | `REQUIRED_REFERENCE`; `owners.dat` intentionally excluded |
| `origmap/` | 9,873 | `game/reference/origmap` | immutable clean world sectors | `REQUIRED_REFERENCE`; copied to writable `game/state/map` |
| `npc/` | 376 | `game/reference/npc` | NPC definitions loaded by Game | `REQUIRED_REFERENCE`; no histories/accounts |
| `mon/` | 195 | `game/reference/mon` | monster definitions loaded by Game | `REQUIRED_REFERENCE`; no binaries |

The preparation-time manifest contains 10,453 file hashes. Its independently recorded expected SHA-256 is `BB7F3DC393D8686ED89B3949D7FC5F6086D7B49810B54B7F9403E560B2E7ACB5`. Preparation fails if this digest differs; `runtime_smoke_wsl.sh` checks the expected digest again and validates every manifest entry before PASS. The extracted assets remain outside Git.

`game/reference/dat/owners.dat` may be created by Game during clean shutdown. It is generated disposable runtime state, not an extracted historical file, and is absent from the provenance manifest and fresh preparation.

Explicitly excluded: archive `usr/`, accounts/passwords, logs, histories, backups, SSH/dotfiles, legacy configs and credentials, historical PEM/private keys, all archived executables and the mutable historical `map/`. No legacy binary is executed. The writable map starts as a copy of `origmap`; 100 empty user shard directories are created from scratch.
