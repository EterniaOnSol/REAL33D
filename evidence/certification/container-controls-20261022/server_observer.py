"""Read-only hardware breakpoints; no inferior calls, memory or data edits.

Offsets verified from this runtime's disassembly (retained alongside evidence).
CClose+64: ebp=decoded id,r12=player before SetOpenContainer; CUp+112:
esi=decoded id,rbp=player. GetOpenContainer reads uint32 at this+0xd98+4*id.
A hold-next-close marker delays that handler once, at most 12 seconds, so the
real client can be photographed before any response. No state is fabricated.
"""
import gdb, json, os, struct, time
from pathlib import Path
from datetime import datetime, timezone
root=Path('/mnt/c/Users/dell/Desktop/fusion32/build/cert-container-independent-20261022')
out=root/'build/certification/live'
out.mkdir(parents=True,exist_ok=True)
(out/'gdb.pid').write_text(str(os.getpid()))
def record(**d):
 d['utc']=datetime.now(timezone.utc).isoformat()
 with (out/'server-trace.jsonl').open('a') as f: f.write(json.dumps(d)+'\n')
def reg(name): return int(gdb.parse_and_eval('$'+name))
class Observe(gdb.Breakpoint):
 def __init__(self,symbol,event,offset=0):
  super().__init__("*'"+symbol+"'+"+str(offset),type=gdb.BP_HARDWARE_BREAKPOINT)
  self.event=event
 def stop(self):
  d=dict(event=self.event,server_pid=gdb.selected_inferior().pid)
  if self.event in ['CLOSE_135','UP_136']:
   close=self.event=='CLOSE_135'
   d['container_id']=reg('ebp' if close else 'esi')
   d['connection']=hex(reg('rbx'))
   player=reg('r12' if close else 'rbp')
   d['server_open_containers']=list(struct.unpack('<16I',gdb.selected_inferior().read_memory(player+0xd98,64)))
  else:
   d['container_id']=reg('esi'); d['connection']=hex(reg('rdi'))
  record(**d)
  marker=out/'hold-next-close'
  if self.event=='CLOSE_135' and marker.exists():
   marker.unlink()
   record(event='RESPONSE_HELD',seconds=12)
   time.sleep(12)
   record(event='RESPONSE_RELEASED')
  return False
Observe('CCloseContainer(TConnection*, TReadBuffer*)','CLOSE_135',64)
Observe('CUpContainer(TConnection*, TReadBuffer*)','UP_136',112)
Observe('SendCloseContainer(TConnection*, int)','CLOSE_111')
Observe('SendContainer(TConnection*, int)','CONTAINER_110')
record(event='TRACE_READY',debugger_pid=os.getpid(),server_pid=gdb.selected_inferior().pid)
