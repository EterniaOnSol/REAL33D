"""Read-only hardware breakpoints; no inferior calls/writes, no login capture.

SysV arguments and InData offset verified against this server's disassembly.
Capture plaintext outgoing game payloads before encryption and only allowlisted
incoming gameplay commands. Passwords/login/key material are never read.
"""
import gdb, json, os, struct
from pathlib import Path
from datetime import datetime, timezone
root = Path('/mnt/c/Users/dell/Desktop/fusion32/build/cert-player-death-20261023')
out = root/'build/certification/live'
out.mkdir(parents=True, exist_ok=True)
(out/'gdb.pid').write_text(str(os.getpid()))
allowed = {20,30,100,101,102,103,104,105,106,107,108,109,111,112,113,114,120,130,131,132,133,135,136,140,160,161,162,190,201,202}
def reg(n): return int(gdb.parse_and_eval('$'+n))
def mem(a,n): return bytes(gdb.selected_inferior().read_memory(a,n))
def record(**d):
    d['utc'] = datetime.now(timezone.utc).isoformat()
    with (out/'server-trace.jsonl').open('a') as f: f.write(json.dumps(d)+'\n')
    if 'raw_hex' in d:
        with (out/'packets.tsv').open('a') as f:
            f.write('\t'.join([d['utc'],d['event'],d['connection'],d['raw_hex']])+'\n')
class Watch(gdb.Breakpoint):
    def __init__(self,sym,event):
        super().__init__('*'+sym,type=gdb.BP_HARDWARE_BREAKPOINT)
        self.event=event
    def stop(self):
        try:
            if self.event=='OUT':
                size=reg('edx')-4
                if 0<size<65532:
                    record(event='OUT',connection=hex(reg('rdi')),raw_hex=mem(reg('rsi')+4,size).hex())
            elif self.event=='IN':
                conn=reg('rdi')
                opcode=mem(conn+2,1)[0]
                if opcode==11:
                    record(event='LOGIN_OBSERVED',connection=hex(conn),opcode=opcode)
                if opcode in allowed:
                    size=struct.unpack('<H',mem(conn,2))[0]
                    if 0<size<=2046:
                        record(event='IN',connection=hex(conn),opcode=opcode,raw_hex=mem(conn+2,size).hex())
            else:
                record(event=self.event,object_pointer=hex(reg('rdi')))
        except Exception as exc:
            record(event='OBSERVER_ERROR',detail=str(exc))
        return False
Watch('_Z13WriteToSocketP11TConnectionPhii','OUT')
Watch('_Z11ReceiveDataP11TConnection','IN')
Watch('_ZN7TPlayer5DeathEv','PLAYER_DEATH')
Watch('_ZN11TConnection10DisconnectEv','DISCONNECT')
record(event='TRACE_READY',debugger_pid=os.getpid(),server_pid=gdb.selected_inferior().pid)
