[2026.10.22-05.26.27:581][  1]LogReal33D: Warning: client core: SV_CMD_INIT_GAME is not decoded; 1863 bytes of the payload left unwalked

Observed 2026-10-22 during UNREAL-DIAGONAL-INPUT-001 live QA, in a session
launched seconds after the previous one was killed. NOT caused by this
milestone: the warning fires on the first frame, at "connected:", before any
walk is requested, and opcode 10 is handled by the gamelogin/initial_world
path which this milestone does not touch. The two measured sessions of this
milestone (D1, D2) reported residual_bytes 0, unsupported_opcodes 0 and
protocol_anomalies 0.

The affected session measured nothing -- every walk in it was refused or
expired unanswered, because the client never received a coherent world -- so
its snapshot is deliberately not carried as milestone evidence. It is recorded
here instead so the observation is not lost.

Reproduction: kill a running REAL33D session and relaunch immediately against
the same Fusion32, so the server takes a re-login path for the same character.
Suggested follow-up: UNREAL-LOGIN-HANDOVER-001.
