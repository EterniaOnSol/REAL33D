"""One observed native gesture with before/after F9 snapshots and screenshot."""
import argparse, json, subprocess, sys
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('account',choices=['A','B'])
p.add_argument('label')
a,gesture=p.parse_known_args()
root=Path(__file__).resolve().parents[3]
out=root/'build/certification/live'/a.account
pid=json.loads((out/'launch.json').read_text())['pid']
driver=Path(__file__).with_name('native_ui_driver.py')
base=[sys.executable,str(driver),'--pid',str(pid),'--activate']
snap=out/'unreal_slice_evidence_Manual.json'
assert not (out/(a.label+'-after.json')).exists()
subprocess.run(base+['--snapshot'],check=True,stdout=subprocess.DEVNULL)
(out/(a.label+'-before.json')).write_bytes(snap.read_bytes())
subprocess.run(base+gesture+['--snapshot','--capture',str(out/(a.label+'.png')),'--record',str(out/(a.label+'-input.json'))],check=True,stdout=subprocess.DEVNULL)
(out/(a.label+'-after.json')).write_bytes(snap.read_bytes())
d=json.loads(snap.read_text())
print(json.dumps({'label':a.label,'position':d.get('local_position'),'counters':{k:v for k,v in d['clientcore'].items() if (v and k.endswith(('requested','accepted','rejected'))) or k in ['protocol_anomalies','residual_bytes','unsupported_opcodes']},'inventory':d.get('inventory'),'containers':d.get('containers')}))
