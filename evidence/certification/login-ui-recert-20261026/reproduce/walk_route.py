"""Real keyboard walking; every step waits for Fusion32's accepted movement."""
import argparse,ctypes as c,json,time
from pathlib import Path
from datetime import datetime,timezone
import native_ui_driver as d
p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);p.add_argument('--run',required=True);p.add_argument('--directions',required=True);p.add_argument('--record',required=True);a=p.parse_args()
root=Path(__file__).resolve().parent
run=root/'live'/a.run
win=[x for x in d.windows(a.pid) if x['size'][0]>=800]
assert len(win)==1
hwnd=win[0]['hwnd']
d.u.ShowWindow(hwnd,9);d.u.SetForegroundWindow(hwnd)
if d.u.GetForegroundWindow()!=hwnd:
    d.u.SwitchToThisWindow.argtypes=[d.w.HWND,d.w.BOOL];d.u.SwitchToThisWindow(hwnd,True);time.sleep(.5)
assert d.u.GetForegroundWindow()==hwnd,'Target not foreground'
keys={'N':0x26,'E':0x27,'S':0x28,'W':0x25,'Q':0x67,'R':0x69,'Z':0x61,'C':0x63}
def press(vk):
    assert d.u.GetForegroundWindow()==hwnd,'Foreground changed'
    scan=d.u.MapVirtualKeyW(vk,0)
    extra=1 if vk in (0x25,0x26,0x27,0x28) else 0
    for flag in (8|extra,8|extra|2):
        ev=d.INPUT(type=1,ki=d.KEYBDINPUT(wScan=scan,dwFlags=flag))
        assert d.u.SendInput(1,c.byref(ev),c.sizeof(ev))==1
        time.sleep(.06)
results=[]
try:
    for direction in a.directions:
        journal=run/'movement_journal.jsonl'
        lines=journal.read_text().splitlines() if journal.exists() else []
        start=len(lines); press(keys[direction]); deadline=time.monotonic()+5
        while time.monotonic()<deadline:
            records=[json.loads(x) for x in journal.read_text().splitlines()[start:]] if journal.exists() else []
            answers=[x for x in records if x['phase'] in ('accepted','rejected','external')]
            if answers:break
            time.sleep(.1)
        else: raise RuntimeError('No observed movement answer')
        results.append({'utc':datetime.now(timezone.utc).isoformat(),'key':direction,'responses':answers})
        if any(x['phase']=='rejected' for x in answers):raise RuntimeError('Server refused step; route stopped')
        time.sleep(.3)
finally:
    (run/a.record).write_text(json.dumps(results,indent=2)+'\n')
    if results:print(json.dumps(results[-1]))
