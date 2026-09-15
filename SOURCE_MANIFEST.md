# Fusion32 Source Manifest

TARGET: Tibia 7.72 / `TIBIA772`

Selection status: `PASS` under independent static review for immutable candidate choice. Operational compatibility remains `REQUIRES_RUNTIME_VERIFICATION`; this is not protocol or gameplay certification.

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
- materialized: `reference/querymanager`; 28 tracked files copied byte-for-byte from the commit
- explicit omission: none among source/build/schema/template files; Git metadata and repository `.gitignore` are intentionally not vendored
- note: `config.cfg.dist` is retained as an authoritative template; generated runtime configuration must replace its sample credential

## WEB

- artifact: `web.bundle`; SHA-256 `4F84BE4445FFF5FADE5721C2A88565A288EEDE0FA1BE53B64C43F86A69513F5F`
- observed revision: `c61e2918e52e929722e5bd97ddaa1747d7ed1744`
- status: `SUPPORTING_REFERENCE`; optional for the vertical slice; not materialized

## IPCHANGER

- artifact: `ipchanger.bundle`; SHA-256 `2CC5415809D036B7C9191433EA091F84ED242EAF796751B289F3084C8D48E044`
- observed revision: `8215db18abbae05b62bcbd5c4f086856168283a4`
- status: `SUPPORTING_REFERENCE`; contains explicit 7.72 address table and modulus patching; not materialized or live-validated

## LOOSE GAME SNAPSHOTS

- status: `SUPPORTING_REFERENCE`, classified `COMMIT_PLUS_CHANGES` relative to Game candidate by exhaustive path/blob comparison
- `game-master` and `game-3fd1...` are mutually identical: 56/58 blobs match candidate, with `map.hh` comment-only and `operate.cc` behavioral differences
- `game-db505...`: 57/58 blobs match candidate; only a comment capitalization in `map.hh` differs
- filename-like IDs `3fd1...` and `db505...` are not objects in the bundled Git history; no ancestry is claimed

## RUNTIME

- artifact: `tibia-game.tarball.tar.gz`
- SHA-256: `67B771D1E3B4A6EF48C554B9B8B0DB56DA39CAE6B0DE5444F7BF6E71C0B2DE8E`
- classification: `HISTORICAL_DATA` containing candidate required references, `SENSITIVE` data, and `LEGACY_BINARY_DO_NOT_EXECUTE`
- status: not materialized; a later task must produce a minimal sanitized runtime without historical accounts, logs, dotfiles, backups, credentials, or binaries
