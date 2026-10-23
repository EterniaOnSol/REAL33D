"""Four hardware breakpoints; read-only decoded input and outgoing field bytes.

Offsets are verified against retained disassembly of this exact running server.
No inferior calls, writes, config/data/inventory edits. Optional 12s pre-Turn hold.
"""
import gdb,json,os,time,struct
from pathlib import Path
from datetime import datetime,timezone
root=Path('/mnt/c/Users/dell/Desktop/fusion32/build/cert-turn-object-independent-20261022')
out=root/'build/certification/live';out.mkdir(parents=True,exist_ok=True)
(out/'gdb.pid').write_text(str(os.getpid()))
pending={}
def reg(n):return int(gdb.parse_and_eval('$'+n))
def mem(a,n):return bytes(gdb.selected_inferior().read_memory(a,n))
def u32(a):return struct.unpack('<I',mem(a,4))[0]
def ring(conn,start,size):return bytes(mem(conn+0x806+((start+i)&0x3fff),1)[0] for i in range(size))
def record(**d):
 d['utc']=datetime.now(timezone.utc).isoformat()
 with (out/'server-trace.jsonl').open('a') as f:f.write(json.dumps(d)+'\n')
class Watch(gdb.Breakpoint):
 def __init__(self,sym,offset,event):
  super().__init__("*'"+sym+"'+"+str(offset),type=gdb.BP_HARDWARE_BREAKPOINT);self.event=event
 def stop(self):
  thread=gdb.selected_thread().global_num
  if self.event=='TURN_133':
   record(event=self.event,server_pid=gdb.selected_inferior().pid,connection=hex(reg('rbx')),x=reg('r15d')&65535,y=reg('r13d')&65535,z=mem(reg('rsp')+8,1)[0],type_id=reg('r14d')&65535,stack=mem(reg('rsp')+15,1)[0])
   marker=out/'hold-next-turn'
   if marker.exists():
    marker.unlink();record(event='TURN_HELD',seconds=12);time.sleep(12);record(event='TURN_RELEASED')
  elif self.event=='FIELD_BEGIN':
   conn=reg('rbx');pos=u32(conn+0x4810)
   pending[thread]=(conn,pos,ring(conn,pos-7,7))
  elif self.event=='FIELD_END':
   entry=pending.pop(thread,None)
   if entry:
    conn,pos,header=entry;end=u32(conn+0x4810);size=(end-pos)&0xffffffff
    if 0<size<256:
     payload=header+ring(conn,pos,size)
     if header[0]==107:
      x,y,z,stack=struct.unpack('<HHBB',header[1:])
      type_id=struct.unpack('<H',payload[7:9])[0]
      if type_id>100:record(event='CHANGE_FIELD_107',connection=hex(conn),x=x,y=y,z=z,stack=stack,type_id=type_id,raw_hex=payload.hex())
  else:record(event='RESULT',connection=hex(reg('rdi')),result=reg('esi'))
  return False
Watch('CTurnObject(TConnection*, TReadBuffer*)',122,'TURN_133')
Watch('SendChangeField(TConnection*, int, int, int, Object)',428,'FIELD_BEGIN')
Watch('SendChangeField(TConnection*, int, int, int, Object)',440,'FIELD_END')
Watch('SendResult(TConnection*, RESULT)',0,'RESULT')
record(event='TRACE_READY',debugger_pid=os.getpid(),server_pid=gdb.selected_inferior().pid)
