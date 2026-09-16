# Project Status

Current phase: `PHASE 0 - SERVER BASELINE` (`PASS` for static source baseline, sanitized runtime and classic-client functional entry; client provenance/review remains `IN_PROGRESS`)
Current milestone: `TWO-CLIENT-VERTICAL-SLICE-001` (`NOT_STARTED`)
Branch: `main`
Starting HEAD for the current evidence task: `7d0202f9190febc001b9e0be2fc3a094a8197e5d`
Dirty worktree: expected while `CLASSIC-CLIENT-772-001` evidence and documentation await review/commit

Server reference source: immutable candidates selected, independently statically reviewed, materialized and `BUILD PASS`; `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` after independent live repetition
Protocol: `IN_PROGRESS`; all `TIBIA772` guards and character-list/game-login source paths traced and the classic live path passed; packet fixtures remain unverified
Client: selected local Tibia 7.72 EXE/DAT/SPR/PIC set hashed and statically validated; Login, character list, Game entry and a session over 30 minutes are `PASS`; historical acquisition provenance is `UNKNOWN`
IP Changer: official Fusion32 revision `8215db18...` independently reviewed, materialized and Windows x86 `BUILD PASS`; exact 7.72 address table and fresh-modulus live patch are `PASS` for the selected client hash
Unreal: no Unreal project found (`NOT_STARTED`)
Parity: `NOT_STARTED`

Last certified test: `SERVER-RUNTIME-SMOKE-001`; independent reviewer repeated prepare, start, smoke and stop with all assertions passing
Latest functional tests: `CLASSIC-CLIENT-772-STATIC-001`, `IPCHANGER-772-LIVE-001`, `CLASSIC-LOGIN-772-001`, `CLASSIC-CHARLIST-772-001`, `CLASSIC-GAME-ENTRY-772-001` and `CLASSIC-SESSION-SUSTAIN-001` are `PASS`
Current certification blockers: no verifiable original source/chain of custody for the operator-supplied local client copy; no independent repetition of the live client procedure; no retained sanitized character-list/world screenshots
Next task: `PROTOCOL-LOGIN-001` - specify and fixture character-list/Game login framing, RSA/XTEA and typed events while an independent reviewer separately repeats the classic functional test

Updated: 2026-09-15
