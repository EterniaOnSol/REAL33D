# Fusion32 Source Manifest

TARGET: Tibia 7.72 / `TIBIA772`

Selection status: `PASS` under independent static review for immutable candidate choice. Sanitized three-service startup compatibility is `CERTIFIED` under independently repeated `SERVER-RUNTIME-SMOKE-001`. Bounded classic Login, character-list and Game-entry compatibility is `PASS` for the selected local client hashes; full protocol/gameplay compatibility remains unverified.

## GAME

- artifact: `tibiacacaca.zip` -> `game.bundle`
- artifact SHA-256: `F085B76733BFFC120FE1A62E3050346E7757DE785C7C7765B89BD5652C893443`
- revision: `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`
- Git tree: `54fe26deea146afaa628d89e8e6ce95f3aaca1f7`
- status: `CANONICAL_CANDIDATE`, `AUTHORITATIVE_REFERENCE`
- materialized: `reference/game`; 56 tracked files copied byte-for-byte from the commit
- explicit omission: tracked `tibia.pem` (compromised private key material)

## LOGIN

- artifact: `tibiacacaca.zip` -> `login.bundle`
- artifact SHA-256: `9C361232D85B72A81696948DB45823095178EA8C326743D2F1ACDCAB733B57FD`
- revision: `f1c839fe7c0334fa036549a641487f21205d0129`
- Git tree: `400ec36b97a4f6aaf8380527b012fb3ab882a2e1`
- status: `CANONICAL_CANDIDATE`, `AUTHORITATIVE_REFERENCE`
- materialized: `reference/login`; 11 tracked files copied byte-for-byte from the commit
- explicit omission: tracked `tibia.pem` (compromised private key material)
- note: `config.cfg.dist` is retained as an authoritative template; its sample shared credential is not suitable for runtime use

## QUERYMANAGER

- artifact: `tibiacacaca.zip` -> `querymanager.bundle`
- artifact SHA-256: `B798A7FB19E514A87E9A82995E4041060E361DF13653A144CC7F82C92DDB2FDD`
- revision: `edea08d11cc306955d8d732164ec383d37ea1f62`
- Git tree: `ebf3a35aa0141e5d723f03e32119f3f263075bdd`
- status: `CANONICAL_CANDIDATE`, `AUTHORITATIVE_REFERENCE`
- materialized: `reference/querymanager`; 29 tracked files copied byte-for-byte from the commit
- explicit omission: none among source/build/schema/template files; Git metadata and repository `.gitignore` are intentionally not vendored
- note: `config.cfg.dist` is retained as an authoritative template; generated runtime configuration must replace its sample credential

## WEB

- artifact: `web.bundle`; SHA-256 `4F84BE4445FFF5FADE5721C2A88565A288EEDE0FA1BE53B64C43F86A69513F5F`
- observed revision: `c61e2918e52e929722e5bd97ddaa1747d7ed1744`
- status: `SUPPORTING_REFERENCE`; optional for the vertical slice; not materialized

## IPCHANGER

- artifact: `ipchanger.bundle`; SHA-256 `2CC5415809D036B7C9191433EA091F84ED242EAF796751B289F3084C8D48E044`
- revision: `8215db18abbae05b62bcbd5c4f086856168283a4`; tree `aa78645284e1da20384dd9789840415cfa296234`
- status: `CANONICAL_CANDIDATE`, `AUTHORITATIVE_REFERENCE` for classic-client host/port/RSA patching; this is the Fusion32-authored tool named by the Game README
- materialized: `reference/ipchanger`; all six upstream blobs match the selected commit
- build: Windows x86 `BUILD PASS` from source; no archived binary was used or executed
- review: `IPCHANGER-772-REVIEW-001 = ACCEPT` after independent bundle, blob, address-table, build-output and configuration verification
- live status: `IPCHANGER-772-LIVE-001 = PASS` for the selected local Tibia 7.72 executable hash and fresh generated modulus; independent repetition remains pending

## CLASSIC CLIENT (LOCAL TEST ARTIFACT, NOT SOURCE AUTHORITY)

- location: ignored `build/classic-client-772/app/`; never commit the client binaries/data by default
- identity: exact EXE/DAT/SPR/PIC hashes are recorded in `evidence/client/CLASSIC-CLIENT-772-001.md`
- provenance: `USER-SUPPLIED_LOCAL_COPY`; pre-existing on the operator's computer; original URL/archive/date/chain of custody `UNKNOWN`
- metadata: PE resources report `Tibia Player` version `7.72`, company `CipSoft GmbH`; no `Zone.Identifier` was present
- status: static address-table validation, Login, character list, Game entry and sustained session `PASS`; provenance certification and independent functional repetition pending

## LOOSE GAME SNAPSHOTS

- status: `SUPPORTING_REFERENCE`, classified `COMMIT_PLUS_CHANGES` relative to Game candidate by exhaustive path/blob comparison
- `game-master` and `game-3fd1...` are mutually identical: 56/58 blobs match candidate, with `map.hh` comment-only and `operate.cc` behavioral differences
- `game-db505...`: 57/58 blobs match candidate; only a comment capitalization in `map.hh` differs
- filename-like IDs `3fd1...` and `db505...` are not objects in the bundled Git history; no ancestry is claimed

## RUNTIME

- artifact: `tibia-game.tarball.tar.gz`
- SHA-256: `67B771D1E3B4A6EF48C554B9B8B0DB56DA39CAE6B0DE5444F7BF6E71C0B2DE8E`
- classification: `HISTORICAL_DATA` containing candidate required references, `SENSITIVE` data, and `LEGACY_BINARY_DO_NOT_EXECUTE`
- status: bounded reference data is materialized only into WSL-native `/var/lib/fusion32-server-baseline-772-$UID` by `scripts/server/prepare_wsl.sh`, discarded by `reset_wsl.sh` and by nothing else; historical accounts, logs, dotfiles, backups, credentials, private keys and binaries are excluded
- sanitized state: new SQLite database; synthetic accounts/characters; fresh shared 1024-bit RSA key; writable map copied from `origmap`; generated configs and credentials
- evidence: `docs/RUNTIME_DATA_PROVENANCE.md` and `evidence/runtime/SERVER-RUNTIME-SMOKE-001.md`
- repository status: `HISTORICAL_DATA` archive remains untracked; generated runtime and all secrets remain outside the repository
