# PHASE-0-ARCHIVE-HASH-001

Date: 2026-09-15
Status: `PASS` only for hashing all eight top-level archives; no source, runtime, protocol, or parity claim.

## SHA-256

```text
A9CBEBB1FD70DA9A942F75298FC7BA709229DB8ADDB6DBE55AC6C215FAE00F5B  game-3fd1bdbaaffc6582943e3c9b3c28c20976fcb2ad.tar.gz
53CF616DFF7170AF97CBCF9E1870A72D096D00548B2F4139B75B405876E174D9  game-3fd1bdbaaffc6582943e3c9b3c28c20976fcb2ad.zip
C7A643C013DDD0B8F3F28846AF73AD300EEA5E2E13BA72028F8243E1960BEE4C  game-db5050d5dbc0525d05837f728bea52f7954286cc.tar.gz
65621C1D6FAA93DFD3E95B231F2AE8BC1F76CD1998A194048E9DCC380BDD57FF  game-db5050d5dbc0525d05837f728bea52f7954286cc.zip
014370B68F99CBB0D73D6186C5143DF8E714A44FE67D6A6C79A1D5DFCF3793BE  game-master.tar.gz
64E7A41CD1EF5F843C62D5149DB943198FC1798610834F97C612CC9FCB7BAE90  game-master.zip
67B771D1E3B4A6EF48C554B9B8B0DB56DA39CAE6B0DE5444F7BF6E71C0B2DE8E  tibia-game.tarball.tar.gz
347DB521AC1A87B5F54F77F13C06C251983CD93C01696F0C7B1B729C30DD6E50  tibiacacaca.zip
```

## Reproduction commands

```powershell
Get-ChildItem -Force
Get-FileHash -Algorithm SHA256 *
tar -tf <archive>
git bundle list-heads <extracted-bundle>
```

Bundle extraction/cloning and loose source extraction were performed only under `%TEMP%/fusion32-audit-20260915`. No archived executable was run. No server build or network test was performed.

## Reproduced observations (not PASS)

Top-level archive sizes in bytes:

```text
293696     game-3fd1bdbaaffc6582943e3c9b3c28c20976fcb2ad.tar.gz
321291     game-3fd1bdbaaffc6582943e3c9b3c28c20976fcb2ad.zip
293449     game-db5050d5dbc0525d05837f728bea52f7954286cc.tar.gz
321013     game-db5050d5dbc0525d05837f728bea52f7954286cc.zip
293160     game-master.tar.gz
317143     game-master.zip
304721917  tibia-game.tarball.tar.gz
4181058    tibiacacaca.zip
```

Each loose game source archive listed 61 entries. Temporary `git bundle list-heads` output:

```text
386fa9b8078a1b32187dfcbfc2a0ed7543e16346 HEAD / refs/heads/master  game.bundle
f1c839fe7c0334fa036549a641487f21205d0129 HEAD / refs/heads/master  login.bundle
edea08d11cc306955d8d732164ec383d37ea1f62 HEAD / refs/heads/master  querymanager.bundle
c61e2918e52e929722e5bd97ddaa1747d7ed1744 HEAD / refs/heads/master  web.bundle
8215db18abbae05b62bcbd5c4f086856168283a4 HEAD / refs/heads/master  ipchanger.bundle
```

These observations remain `UNVERIFIED` as canonical source selection. `BOOTSTRAP-SOURCES-001` must run `git bundle verify`, persist a normalized manifest/diff, and establish provenance.
