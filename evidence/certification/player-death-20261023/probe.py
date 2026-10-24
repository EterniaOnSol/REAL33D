"""One observed native gesture with before/after F9 snapshots and screenshot."""
import argparse, json, subprocess, sys, time
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('account',choices=['A','B'])
p.add_argument('label')
p.add_argument('--run', default=None)
a,gesture=p.parse_known_args()
root=Path(__file__).resolve().parents[3]
out=root/'build/certification/live'/(a.run or a.account)
pid=json.loads((out/'launch.json').read_text())['pid']
driver=Path(__file__).with_name('native_ui_driver.py')
base=[sys.executable,str(driver),'--pid',str(pid),'--activate']
snap=out/'unreal_slice_evidence_Manual.json'
assert not (out/(a.label+'-after.json')).exists()
before_request=time.time()
subprocess.run(base+['--snapshot'],check=True,stdout=subprocess.DEVNULL)
before_fresh=snap.stat().st_mtime>=before_request
(out/(a.label+'-before.json')).write_bytes(snap.read_bytes())
gesture_start=time.time()
subprocess.run(base+gesture+['--snapshot','--capture',str(out/(a.label+'.png')),'--record',str(out/(a.label+'-input.json'))],check=True,stdout=subprocess.DEVNULL)
after_fresh=snap.stat().st_mtime>=gesture_start
(out/(a.label+'-after.json')).write_bytes(snap.read_bytes())
d=json.loads(snap.read_text())
(out/(a.label+'-freshness.json')).write_text(json.dumps(dict(before_fresh=before_fresh,after_fresh=after_fresh,before_request=before_request,gesture_start=gesture_start,snapshot_mtime=snap.stat().st_mtime),indent=2)+'\n')
print(json.dumps(dict(before_fresh=before_fresh,after_fresh=after_fresh)))
print(json.dumps({k:d.get(k) for k in ('written_at','connected_at_close','local_creature_id','player','vitals','combat','reconnect_attempts')}))
print(json.dumps({'label':a.label,'position':d.get('local_position'),'counters':{k:v for k,v in d['clientcore'].items() if (v and k.endswith(('requested','accepted','rejected'))) or k in ['protocol_anomalies','residual_bytes','unsupported_opcodes']},'inventory':d.get('inventory'),'containers':d.get('containers')}))
