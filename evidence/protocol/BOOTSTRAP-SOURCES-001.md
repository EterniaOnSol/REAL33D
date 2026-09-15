# BOOTSTRAP-SOURCES-001 Evidence

Date: 2026-09-15

## Deterministic PASS results

- `ARCHIVE-INTEGRITY-002`: all eight original archive SHA-256 values matched `PHASE-0-ARCHIVE-HASH-001`; exit code 0.
- `BUNDLE-INTEGRITY-001`: `git bundle verify` reported all five bundles complete, SHA-1 histories and okay; each exposed only `HEAD` and `refs/heads/master`; no tags were found.
- `REFERENCE-BYTE-MATCH-001`: before adding local `_PROVENANCE.md`, every materialized upstream file matched its selected commit blob with `core.autocrlf=false`: Game 56/56, Login 11/11, Query Manager 28/28.
- `SOURCE-SELECTION-STATIC-REVIEW-001`: independent reviewer found no counterexample to the three candidate revisions and confirmed static Query IDs/application IDs/port conventions. Runtime compatibility explicitly excluded.
- `BUILD-GAME-772-001`: selected curated Game compiled in Ubuntu 26.04 WSL2 with explicit `-DTIBIA772=1`; exit 0; binary SHA-256 `E0E96B863FB3FA061501E1EB59C6BAC456733676A15E11F245C6B04505E817C7`.
- `BUILD-LOGIN-772-001`: selected curated Login compiled with explicit `-DTIBIA772=1`; exit 0; binary SHA-256 `23F0E75536A852908DA35E9644785F2E47101948E723FA426BF0FEA453BE788F`.
- `BUILD-QUERYMANAGER-SQLITE-001`: selected curated Query Manager compiled with `DATABASE=sqlite`; exit 0; binary SHA-256 `50474952C55C4DF3EF3B06C08B475550326E5ABA9D8AD8B3502AF401913625E4`.

## Bundle identity

| Component | Bundle SHA-256 | HEAD | Tree | History |
| --- | --- | --- | --- | --- |
| Game | `F085B76733BFFC120FE1A62E3050346E7757DE785C7C7765B89BD5652C893443` | `386fa9b8078a1b32187dfcbfc2a0ed7543e16346` | `54fe26deea146afaa628d89e8e6ce95f3aaca1f7` | 167 linear commits, root `717f5cd...` |
| Login | `9C361232D85B72A81696948DB45823095178EA8C326743D2F1ACDCAB733B57FD` | `f1c839fe7c0334fa036549a641487f21205d0129` | `400ec36b97a4f6aaf8380527b012fb3ab882a2e1` | 13 linear commits, root `baaeac3...` |
| Query Manager | `B798A7FB19E514A87E9A82995E4041060E361DF13653A144CC7F82C92DDB2FDD` | `edea08d11cc306955d8d732164ec383d37ea1f62` | `ebf3a35aa0141e5d723f03e32119f3f263075bdd` | 54 linear commits, root `8dbd1d3...` |
| Web | `4F84BE4445FFF5FADE5721C2A88565A288EEDE0FA1BE53B64C43F86A69513F5F` | `c61e2918e52e929722e5bd97ddaa1747d7ed1744` | not selected | 8 linear commits |
| IP Changer | `2CC5415809D036B7C9191433EA091F84ED242EAF796751B289F3084C8D48E044` | `8215db18abbae05b62bcbd5c4f086856168283a4` | not selected | 3 linear commits |

## Loose snapshot comparison

Comparison enumerated every snapshot path and blob against every plausible Game commit. No filename ID was a Git object. ZIP/TAR pairs normalized to identical 58-file manifests. `game-master`/`game-3fd1...` match 56 candidate blobs; `game-db505...` matches 57. The latter preserves candidate `operate.cc`; the other pair changes trade/container notification view ranges. All differ from candidate `map.hh` only by comment capitalization. Classification: `COMMIT_PLUS_CHANGES`, with no ancestry assertion.

## Reproduction outline

```powershell
Get-FileHash -Algorithm SHA256 *.zip,*.tar.gz
tar -xf .\tibiacacaca.zip -C <temporary-directory>
git -C <empty-repo> bundle verify <bundle>
git clone <bundle> <temporary-clone>
git -C <clone> fsck --full
git -C <clone> for-each-ref
git -C <clone> log --all --graph --format=fuller
git -C <clone> ls-tree -r <revision>
git -c core.autocrlf=false hash-object --no-filters <materialized-file>
```

## Non-PASS conclusions

Static source compatibility is not runtime compatibility. No component was launched, no database/runtime was initialized, and no classic client connected. RSA modulus patch addresses remain unverified against a legitimate 7.72 executable. These remain `REQUIRES_RUNTIME_VERIFICATION` or `UNKNOWN`.

## Build environment and logs

Build host: WSL2 Ubuntu 26.04, Linux x86-64, G++ 15.2.0, GNU Make 4.4.1, OpenSSL 3 libcrypto and headers. Reproduction script: `tests/build_reference_wsl.sh`. Logs:

- `evidence/build/game-772-build.log` (SHA-256 `1750AAA2A310B62AD528C5F83C0573BA381EE764B06D832F7EF22AF914EDA9FE`)
- `evidence/build/login-772-build.log` (SHA-256 `74459AE4F6F2FD7DF3488EBC2D4C78E391FAAF07EAFD9956E014355CDEE20ECA`)
- `evidence/build/querymanager-sqlite-build.log` (SHA-256 `7BE27D5BF1A3DF9C8AA7AD275F8725867515C7FDDC2477D54A2907488C5B24A7`)

An earlier interactive command lost most compiler flags through shell quoting. Its output is discarded and not evidence. The committed script was then executed; log first lines demonstrate full flags, including `TIBIA772=1` for Game and Login.
