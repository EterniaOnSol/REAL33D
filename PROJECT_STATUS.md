# Project Status

Current phase: `PHASE 1 - CLIENT CORE` (`IN_PROGRESS`; classic functional baseline retained, Transport and Crypto `PASS`)
Current milestone: `TWO-CLIENT-VERTICAL-SLICE-001` (`NOT_STARTED`)
Branch: `main`
Classic baseline review commit: `f65f3a7645ff40b39b7cc8399760fd4f0b69ecee`
Transport implementation commit: `abd2d0a25bd9632f5aa3955e822876268c7ca96c`
Worktree: Crypto implementation in progress; final clean-state check pending

Server reference source: immutable candidates selected, independently statically reviewed, materialized and `BUILD PASS`; `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` after independent live repetition
Protocol: `IN_PROGRESS`; `CLIENTCORE-TRANSPORT-772-001 = PASS` and `CLIENTCORE-CRYPTO-772-001 = PASS`; Login payloads and opcode fixtures remain unverified
Client core: Unreal-independent C++17 TCP/framing/crypto components implemented; 20 Transport plus 25 Crypto cases pass in normal and ASan/UBSan WSL builds; native Windows build remains unverified
Client: selected local Tibia 7.72 EXE/DAT/SPR/PIC set hashed and statically validated; Login, character list, Game entry and a session over 30 minutes are `PASS`; historical acquisition provenance is `UNKNOWN`
IP Changer: official Fusion32 revision `8215db18...` independently reviewed, materialized and Windows x86 `BUILD PASS`; exact 7.72 address table and fresh-modulus live patch are `PASS` for the selected client hash
Unreal: no Unreal project found (`NOT_STARTED`)
Parity: `NOT_STARTED`

Last certified test: `SERVER-RUNTIME-SMOKE-001`; independent reviewer repeated prepare, start, smoke and stop with all assertions passing
Latest functional tests: `CLASSIC-CLIENT-772-STATIC-001`, `IPCHANGER-772-LIVE-001`, `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001`, `CLASSIC-GAME-ENTRY-772-001`, `CLASSIC-SESSION-SUSTAIN-001`, `CLIENTCORE-TRANSPORT-772-001` and `CLIENTCORE-CRYPTO-772-001` are `PASS`
Current certification blockers: no verifiable original source/chain of custody for the operator-supplied local client copy; no independent repetition of the live client procedure; no retained sanitized character-list/world screenshots
Next task: `CLIENTCORE-LOGIN-772-001` - implement only the source-traced character-list Login request/response using the established Transport/Framing/Crypto layers and deterministic byte fixtures before bounded live use

Updated: 2026-09-15
