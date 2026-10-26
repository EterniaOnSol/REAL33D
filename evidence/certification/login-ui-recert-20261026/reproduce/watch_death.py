"""Read-only capture of first literal-death and cleanup observations."""
import json,time,shutil
from datetime import datetime,timezone
from pathlib import Path
root=Path(__file__).resolve().parent
run=root/'live/death'
dest=run/'death-observed';dest.mkdir(exist_ok=True)
deadline=time.monotonic()+1800
seen=False
while time.monotonic()<deadline:
    log=(run/'REAL33D.log').read_text(encoding='utf-8',errors='replace')
    lines=[line for line in log.splitlines() if 'You are dead.' in line]
    if lines and not seen:
        seen=True
        (dest/'death-message.json').write_text(json.dumps({'observed_utc':datetime.now(timezone.utc).isoformat(),'lines':lines,'pid':11680},indent=2)+'\n')
        print('LITERAL DEATH OBSERVED',flush=True)
    cleanup=run/'unreal_slice_evidence_AfterCleanup.json'
    if seen and cleanup.exists():
        try:j=json.loads(cleanup.read_text())
        except (ValueError,OSError):time.sleep(.2);continue
        if j['session']['phase']=='character-select':
            for name in ('unreal_slice_evidence_AfterCleanup.json','unreal_slice_evidence_Disconnected.json','movement_journal.jsonl'):
                if (run/name).exists():shutil.copy2(run/name,dest/name)
            (dest/'observation.json').write_text(json.dumps({'utc':datetime.now(timezone.utc).isoformat(),'pid':11680,'session':j['session'],'presentation':j['presentation']},indent=2)+'\n')
            print('DEATH CLEANUP PRESERVED',flush=True)
            break
    time.sleep(.5)
else:raise RuntimeError('No literal death/cleanup before deadline')
