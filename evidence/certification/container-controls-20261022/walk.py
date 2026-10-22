"""Bounded real-key walk, checking server response after every step."""
import argparse,json,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('account');p.add_argument('label');p.add_argument('direction',choices=['north','south','east','west']);p.add_argument('steps',type=int)
a=p.parse_args();assert 0<a.steps<=20
here=Path(__file__).parent;out=here.parents[2]/'build/certification/live'/a.account
key={'north':'0x26','south':'0x28','east':'0x27','west':'0x25'}[a.direction]
for i in range(a.steps):
 label=f'{a.label}-{i+1:02}'
 subprocess.run([sys.executable,str(here/'probe.py'),a.account,label,'--key',key],check=True,stdout=subprocess.DEVNULL)
 before=json.loads((out/(label+'-before.json')).read_text());after=json.loads((out/(label+'-after.json')).read_text())
 print(label,after['local_position'],flush=True)
 if after['local_position']==before['local_position']:
  sent=after['clientcore']['walks_requested_from_unreal']-before['clientcore']['walks_requested_from_unreal']
  print('STOP: no position change; requests sent =',sent);break
