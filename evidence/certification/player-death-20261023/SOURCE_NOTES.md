# UNREAL-PLAYER-DEATH-RESPAWN-001 source inspection

Audit 20db3a6 is the sole child of expected main 28b5524; audit normally pushed.
This worktree is fresh. Scope: independent live certification plus evidence and
documentation; production changes only if a real defect is demonstrated.

Read-only server source inspection corrects a detail in the audit:

- crplayer.cc TPlayer::Death sends PlayerData (HP zero), then EventMessage
  "You are dead.\n", then calls Connection::Die.
- connections.cc TConnection::Die only switches GAME to DEAD.
- crmain.cc TCreature destructor creates corpse/loot, removes the creature,
  then, for a player with a connection, calls Logout(30,true).
- connections.cc Logout clears the player's connection if the player still
  exists, marks LOGOUT and records the delayed disconnect round.
- TConnection::Process disconnects when that round is reached. The expected
  normal close is therefore approximately 30 seconds after destruction, not
  necessarily immediate at the death message.
- Unreal ClearWorld resets tiles, static sectors, creatures, local id, vitals,
  equipment, containers and combat. Disconnected first writes a before-cleanup
  snapshot, then clears, then writes AfterCleanup.
- Real33DReconnectPolicy treats an orderly close as final, regardless of the
  five-second retry timer for lost links. Relogin is an explicitly launched
  fresh client. Keeping the old disconnected client open tests that it does
  not reclaim ownership from the relogged client.

Tools: guarded real native keyboard/mouse gestures; read-only gdb hardware
breakpoints; unchanged ClientCore packet replay. Login observation records
only opcode, connection pointer and timestamp: no login body, keys or credentials.
No server data editing, creature spawning, teleporting or debug damage calls.

Live results and precise limitations will be recorded in README.md.
