# Project Status

Current phase: `PHASE 1 - CLIENT CORE` (`IN_PROGRESS`; classic functional baseline retained, Transport, Crypto, Login, Game Login, Initial World, Movement and Player State `PASS`)
Current milestone: `TWO-CLIENT-VERTICAL-SLICE-001` (`NOT_STARTED`)
Branch: `main`
Classic baseline review commit: `f65f3a7645ff40b39b7cc8399760fd4f0b69ecee`
Transport implementation commit: `abd2d0a25bd9632f5aa3955e822876268c7ca96c`
Crypto implementation commit: `64e9217ef64181d44cdce815a36b6bb1d2aa9038`
Login implementation commit: `a56add56e7ad5a11fd2e28d642bada4390b16756`
Game Login implementation commit: `3db23f9`
Initial World implementation commit: `fedd536`
Movement implementation commit: `37fa5f6`
Player State implementation commit: `af3e3ac`
Worktree: clean after the focused Player State commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`; full history pushed to `main` (24 commits), `HEAD == origin/main`. No history was rewritten and no force push was used. The first attempt returned HTTP 403 because the stored credential belonged to `leodavidsoto`, which holds only `READ` on that repo; the operator re-authenticated `gh` as `EterniaOnSol`, which holds `admin`

Server reference source: immutable candidates selected, independently statically reviewed, materialized and `BUILD PASS`; `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` after independent live repetition
Protocol: `IN_PROGRESS`; Transport, Crypto, Login, Game Login, the `FULLSCREEN` initial world snapshot, cardinal movement with its incremental map updates, and the player/session command set are `PASS` for deterministic fixtures plus bounded local synthetic-account smoke. A whole login burst and ordinary session traffic now decode with zero residual bytes; chat, containers, trade, the request queue and the editors remain recognized by name but unparsed
Client core: Unreal-independent C++17 TCP/framing/crypto/Login/Game Login/Initial World/Movement/Player State components implemented; retained and new suites pass in normal and ASan/UBSan WSL builds; native Windows build remains unverified
Client: selected local Tibia 7.72 EXE/DAT/SPR/PIC set hashed and statically validated; Login, character list, Game entry and a session over 30 minutes are `PASS`; historical acquisition provenance is `UNKNOWN`
IP Changer: official Fusion32 revision `8215db18...` independently reviewed, materialized and Windows x86 `BUILD PASS`; exact 7.72 address table and fresh-modulus live patch are `PASS` for the selected client hash
Unreal: no Unreal project found (`NOT_STARTED`)
Parity: `NOT_STARTED`

Last certified test: `SERVER-RUNTIME-SMOKE-001`; independent reviewer repeated prepare, start, smoke and stop with all assertions passing
Latest functional tests: classic baseline IDs, `CLIENTCORE-TRANSPORT-772-001`, `CLIENTCORE-CRYPTO-772-001`, `CLIENTCORE-LOGIN-772-001`, `CLIENTCORE-GAMELOGIN-772-001`, `INITIALWORLD-772-001`, `MOVEMENT-772-001` and `PLAYERSTATE-772-001` are `PASS`
Current certification blockers: no verifiable original source/chain of custody for the operator-supplied local client copy; no independent repetition of the live client procedure; no retained sanitized character-list/world screenshots
Closing ritual from `PLAYERSTATE-772-001` onwards: tests + sanitizers + evidence + docs + commit + handoff + push, with `tests/secret_check.sh` run before every push
Next task: `UNREAL-SLICE-001` (plan/source-trace only; do not start automatically). Protocol772Core now decodes an entire ordinary session, so the remaining vertical-slice gap is the Unreal desktop project that consumes it through a network-thread event queue and applies WorldState on the game thread. Chat, containers and trade are the alternative next protocol step if presentation is deferred.

Updated: 2026-09-16
