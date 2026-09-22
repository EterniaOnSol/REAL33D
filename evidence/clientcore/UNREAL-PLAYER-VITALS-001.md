# UNREAL-PLAYER-VITALS-001

Status: IMPLEMENTED_UNVERIFIED (2026-09-21). The V08 catalog remains STANDBY; neither task is promoted to PASS.

## Source and scope

- `reference/game/src/sending.cc::SendPlayerData` writes hitpoints, maximum hitpoints, mana, maximum mana and level under `SV_CMD_PLAYER_DATA`.
- `clientcore/src/player_state.cpp::DecodePlayerData` and `clientcore/src/movement.cpp::ApplyServerUpdate` already decode and store these values in `WorldState::stats`.
- The Unreal worker now publishes a semantic `PlayerVitals` event after that state application. The game thread stores it; the player controller shows it in a small bottom-right HUD and clears it after a disconnect.
- No server, protocol decoder, ClientCore semantic state, or V08 asset was changed.

## Verification

- `Build.bat REAL33DEditor Win64 Development -Project=... -WaitMutex`: Result Succeeded after the final edit.
- The sanitized Query Manager, Game and Login services were restarted and their identity/port checks were ALIVE.
- `scripts/client/run_unreal_slice.cmd B` connected to Test Player B. `unreal/REAL33D/Saved/Logs/REAL33D.log` recorded:
  - `connected: Test Player B`
  - `HUD vitals: HP --/--   Mana --/--   Level --`
  - `HUD vitals: HP 145/145   Mana 0/65531   Level 1`
- `65531` is the decoded value received from this runtime. Its cause and desired presentation have not been established from source or classic-client observation. It was not silently changed.
- The log proves the label received and set the server values. No retained screenshot or independent visual review proves layout or readability, so the task remains IMPLEMENTED_UNVERIFIED.