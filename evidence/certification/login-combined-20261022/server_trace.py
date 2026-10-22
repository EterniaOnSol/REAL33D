"""GDB-only observation of actual server calls; no inferior calls or data writes.

Load after attaching to the identity-verified Fusion32 game PID. Four hardware
execution breakpoints leave executable bytes untouched. SysV AMD64 argument
registers are sampled at the exact function entry; no credentials are read.
"""
import gdb
import json
import os
from datetime import datetime, timezone
from pathlib import Path

root = Path('/mnt/c/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022')
out = root / 'build/certification/live/server-trace.jsonl'
out.parent.mkdir(parents=True, exist_ok=True)
(out.parent / 'gdb.pid').write_text(str(os.getpid()))

def record(**event):
    event['utc'] = datetime.now(timezone.utc).isoformat()
    with out.open('a') as f:
        f.write(json.dumps(event) + '\n')

class Observe(gdb.Breakpoint):
    def __init__(self, symbol, event, connection_register='rdi'):
        super().__init__("*'" + symbol + "'", type=gdb.BP_HARDWARE_BREAKPOINT, internal=False)
        self.event = event
        self.connection_register = connection_register
    def stop(self):
        data = dict(event=self.event, server_pid=gdb.selected_inferior().pid,
                    connection=hex(int(gdb.parse_and_eval('$' + self.connection_register))),
                    thread=gdb.selected_thread().ptid)
        if self.event == 'INIT_GAME':
            data['creature_id'] = int(gdb.parse_and_eval('$esi'))
        record(**data)
        return False

Observe('TPlayer::TakeOver(TConnection*)', 'TAKEOVER', 'rsi')
Observe('SendClearTarget(TConnection*)', 'CLEAR_TARGET')
Observe('SendInitGame(TConnection*, unsigned int)', 'INIT_GAME')
Observe('SendFullScreen(TConnection*)', 'FULLSCREEN')
record(event='TRACE_READY', server_pid=gdb.selected_inferior().pid, debugger_pid=os.getpid())
