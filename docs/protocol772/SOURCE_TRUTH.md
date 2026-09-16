# Protocol 7.72 Source Truth - Initial Inventory

Status: source baseline selected under independent static review. Sanitized internal runtime compatibility is `CERTIFIED` under independently repeated `SERVER-RUNTIME-SMOKE-001`. `CLIENTCORE-TRANSPORT-772-001 = PASS` and `CLIENTCORE-CRYPTO-772-001 = PASS` for their bounded layers; application Login/Game protocol remains `IN_PROGRESS` and is not certified.

The immutable candidate revisions and artifact hashes are in `SOURCE_MANIFEST.md`. Curated reference trees are under `reference/`; tracked private PEM files were explicitly omitted.

## Artifacts and provenance

- Six loose `game-*` archives: three source snapshots in both ZIP and TAR.GZ form. `game-master` and `game-3fd1...` are content-identical; `game-db505...` differs only in `src/operate.cc`. They describe themselves as a manual decompilation of a leaked Tibia 7.7 server with changes and possible translation errors: `REFERENCE / DERIVED`.
- `tibiacacaca.zip`: container with Git bundles for game, login, querymanager, web, and ipchanger. Selected heads: game `386fa9b8078a1b32187dfcbfc2a0ed7543e16346`, login `f1c839fe7c0334fa036549a641487f21205d0129`, querymanager `edea08d11cc306955d8d732164ec383d37ea1f62`, and Fusion32 IP Changer `8215db18abbae05b62bcbd5c4f086856168283a4`; web `c61e2918e52e929722e5bd97ddaa1747d7ed1744` remains supporting reference only. The four selected revisions have independent source-selection review; client-memory compatibility of the IP Changer still requires the exact client executable.
- `tibia-game.tarball.tar.gz`: large legacy runtime with binaries, config, map/origmap/map backups, objects and conversion data, monsters, NPCs, logs, users and backups. Treat as sensitive/untrusted reference; do not execute or import wholesale.
- The initial source/runtime inventory contained no classic client and no Unreal project. A later operator-supplied local Tibia 7.72 EXE/DAT/SPR/PIC set is recorded by exact hash in `evidence/client/CLASSIC-CLIENT-772-001.md`; it is functional test input, not protocol source authority, and its historical provenance is `UNKNOWN`. No Unreal project has been found.

Archive SHA-256 values are recorded in `evidence/protocol/PHASE-0-AUDIT.md`.

## Version evidence

Loose game `src/communication.cc`: `TIBIA772` selects terminal versions `{772,772,772}` instead of `{770,770,770}` and changes placement of terminal type/version around the RSA block. The README documents `-DTIBIA772=1`. The default Makefile does not define it; archived runtime binary version is `UNKNOWN`.

## Complete TIBIA772 matrix at selected revisions

Six textual occurrences exist across Game, Login, and Query Manager candidate trees: two README descriptions and four preprocessor sites. Query Manager has none.

| Component | File / symbol | 7.70 behavior | `TIBIA772` behavior | Protocol impact | Evidence |
| --- | --- | --- | --- | --- | --- |
| Game | `communication.cc`, `TERMINALVERSION` | accepts version 770 | accepts version 772 | game endpoint version gate | lines 27-31; introduced by `11bd6ba...` |
| Game | `communication.cc::HandleLogin` before RSA block | does not read terminal fields here | reads terminal type and version as two LE words before 128-byte RSA ciphertext | changes game-login wire layout | lines 920-927 |
| Game | `communication.cc::HandleLogin` after XTEA key | reads terminal type/version from decrypted RSA plaintext | omits those reads | completes relocation of the same fields | lines 949-952 |
| Login | `connections.cc`, `TERMINALVERSION` | accepts version 770 | accepts version 772 | character-list endpoint routes only target version; request layout itself is unchanged | lines 10-14; introduced by `a8a6a140...` |
| Query Manager | entire selected tree | no Tibia client-version conditional | same | none; internal service protocol only | zero `TIBIA772` matches |

The RSA plaintext-zero validations added in the version-support commits are unconditional and are not flag-selected behavior. Neither Makefile enables the flag by default.

## Login / character list source trace

Selected Login `connections.cc` reads an outer LE `uint16` size and requires a 145-byte request payload: command byte `1`, terminal type/version LE words, three LE signature quads, and 128-byte RSA ciphertext. RSA plaintext is zero byte, four LE XTEA quads, account ID quad, and length-prefixed password. The encrypted response contains optional MOTD, character-list opcode `100`, a byte count, per-character name/world strings, IPv4 in network order, LE port, then LE premium days. Source-traced, not packet-fixture PASS.

Login sends account/password/client-IP to Query Manager query `11`; no login-issued token was found. The classic client opens a separate Game connection using the returned endpoint.

## Cross-component compatibility

Static inspection and independent review found aligned application IDs, query IDs, LE framing, shared-auth convention, world endpoint fields, and default port 7173 among the selected Game/Login/Query Manager revisions. Query Manager contains ancestor `8c2a846...`, required for Login's world-status query authorization and robust header input. Both Game and Login fail startup without Query Manager.

Runtime verification proves a clean SQLite schema/seed, internal Game and Login authorization, world resolution, Game world load, simultaneous liveness on Query Manager `7173`, Game `7172`, Login `7171`, bounded TCP connect/close resilience, and clean shutdown. That runtime test alone does not prove external protocol behavior. Subsequent bounded classic-client evidence proves Login, character list, Game entry and a sustained connection for the exact recorded local client hashes; it does not provide golden packet fixtures or full gameplay parity.

Query Manager applies `sqlite/schema.sql` to a fresh DB and then patch files alphabetically. The synthetic seed contains one world plus two independent accounts/characters. Account `Auth` is 64 bytes: `SHA256(SHA256(password) XOR random_32_byte_salt) || salt`. Passwords/auth blobs remain generated ignored state.

## RSA and classic client

Selected Game and Login commits track byte-identical 1024-bit reference private keys, which are compromised and excluded from materialized sources. Their public modulus does not match the default modulus embedded in the inspected IP Changer source example. The project-selected Fusion32 IP Changer revision `8215db...` explicitly supports client 7.72 and patches five login endpoints plus the decimal RSA modulus in process memory.

A sanitized environment now generates one shared fresh 1024-bit PKCS#1 private key for Login and Game. `SERVER-RUNTIME-SMOKE-001` verifies their installed files match without exposing them and verifies that hexadecimal/decimal public modulus forms are equivalent and fit the IP Changer limit. The selected IP Changer has Windows x86 `BUILD PASS`. `IPCHANGER-772-LIVE-001 = PASS` validates its source-defined 7.72 addresses and fresh-modulus patch for the exact client executable hash recorded in `evidence/client/CLASSIC-CLIENT-772-001.md`. Historical client provenance remains `UNKNOWN`, independent functional repetition is `NOT_STARTED`, and no private key contents are documented.

## Source map

| Concern | Primary source / symbols | Initial confidence |
| --- | --- | --- |
| TCP and framing | game `src/communication.cc`: `OpenSocket`, `ReceiveCommand`, `WriteToSocket`, `ReadFromSocket`; login `src/connections.cc::CheckConnectionInput`, `PrepareXTEAResponse`, `SendXTEAResponse` | High; bounded client transport/framing implementation and deterministic fixtures PASS |
| RSA / XTEA | game `src/crypto.cc`: `TRSAPrivateKey`, `TXTEASymmetricKey`; `communication.cc::HandleLogin`; login `src/crypto.cc`, `connections.cc::ProcessLoginRequest/SendXTEAResponse` | High; bounded client implementation and byte fixtures PASS |
| Client/server opcode symbols | game `src/connections.hh`: `ClientCommand`, `ServerCommand` | High for selected source only |
| Client dispatch | game `src/receiving.cc`: `ReceiveData`, `C*` handlers | High; parser consumes one command per call |
| Server serialization | game `src/sending.cc`: `Send*` functions | High; payload specs not yet extracted |
| Game login / initial world | `communication.cc::HandleLogin`; `connections.cc::JoinGame`; `crplayer.cc`; `sending.cc::SendInitGame/SendFullScreen` | High; bounded classic live PASS, no byte fixture |
| Character list | login `src/connections.cc`, query path in `src/query.cc` | High; bounded classic live PASS, no byte fixture |
| Map / tile things | `sending.cc::SendMapObject/SendMapPoint/SendFullScreen/SendFloors`; `map.cc` | Medium |
| Movement/use/client commands | `receiving.cc`; `cract.cc`; `operate.cc`; `moveuse.cc` | Medium |
| Items/types | `objects.cc/.hh`, `map.cc/.hh`, runtime `dat/objects.srv`, `dat/conversion.lst` | Medium |
| Creatures/combat | `cr*.cc/.hh`, especially `crcombat.cc`; receiving/sending | Medium |
| Magic/effects | `magic.cc/.hh`; sending effect functions | Medium |
| Stats/skills | `crskill.cc`; player serialization | Medium |
| Chat/NPC/trade | receiving/sending, `operate.cc`, `crnonpl.cc`, runtime NPC data | Medium/low |
| Errors/disconnect | `communication.cc`, `receiving.cc::CQuitGame/CErrorFileEntry`, result/message senders | Medium |

## Framing/crypto interpretation

Game and Login receive an outer little-endian `uint16` size followed by exactly that many bytes; the size excludes its own two-byte header. `CLIENTCORE-TRANSPORT-772-001 = PASS` implements that bounded outer framing with separate source-derived endpoint/direction limits and deterministic fragment/coalescing/error fixtures. Detailed symbols and derivations are in `docs/protocol772/TRANSPORT.md`.

The first Game connection payload is unencrypted login. Subsequent Game payload size must align to eight bytes, is XTEA-decrypted, and begins with an inner little-endian plaintext length. Sending mirrors outer length, inner data length, data, and padding. RSA uses a 128-byte/1024-bit block with no padding; decrypted byte zero must be zero, followed by four little-endian XTEA words. `CLIENTCORE-CRYPTO-772-001 = PASS` implements and tests these bounded rules; detailed traceability is in `docs/protocol772/CRYPTO.md`. Application Login/Game Login fields remain outside that result.

## Known risks and unknowns

- Exact relationship among loose snapshots and the newer game bundle is unresolved.
- Character-list request/response is source-traced and bounded classic live `PASS`, but lacks golden byte fixtures.
- Fresh RSA public-modulus derivation and the IP Changer decimal representation are proven equivalent; live use is `PASS` for the exact recorded client hash. Checksum behavior if any, RSA padding-byte behavior beyond the observed path, and independent repetition remain unverified.
- `ReceiveData` dispatches one logical client opcode; multi-command packet behavior is unverified.
- Tile order begins from the map container linked list; `PlaceObject` orders priorities BANK, CLIP, BOTTOM, TOP, CREATURE, LOW. Reverse stack lookup and full flag semantics remain untraced.
- Selected-source startup against the bounded legacy data subset is runtime-smoke PASS. Archived binaries remain unexecuted and their 7.72 status is unknown.
