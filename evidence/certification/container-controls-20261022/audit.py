"""Recheck recorded authoritative snapshots, never generate gameplay evidence."""
import argparse,json
from pathlib import Path
from datetime import datetime
p=argparse.ArgumentParser();p.add_argument('--live',type=Path);a=p.parse_args()
here=Path(__file__).resolve().parent
live=a.live or here/'live'
def read(session,label,phase='after'):
 return json.loads((live/session/(label+'-'+phase+'.json')).read_text())
def counter(d,k): return d['clientcore'][k]
def ids(d): return [c['number'] for c in d['containers']]
def check(value,label):
 assert value,label
 checks.append(label)
checks=[]
old=lambda label,phase='after':read('B-round1',label,phase)
now=lambda label,phase='after':read('B',label,phase)
held=old('02-close-held');closed=old('03-close-response')
check(ids(old('01-open'))==[0],'real container opens')
check(ids(held)==[0] and counter(held,'container_closes_requested')==1,'Close intent preserves window')
check(ids(closed)==[],'server Close removes window')
check(ids(old('04-reopen'))==[0],'reopen succeeds')
trace=[json.loads(l) for l in (live/'server-trace.jsonl').read_text().splitlines()]
stamp=lambda s:datetime.fromisoformat(s.replace('Z','+00:00'))
pause=next(t for t in trace if t['event']=='RESPONSE_HELD')
resume=next(t for t in trace if t['event']=='RESPONSE_RELEASED')
check(stamp(pause['utc'])<stamp(held['written_at'])<stamp(resume['utc'])<stamp(closed['written_at']),'window persists during independently held server response')
requests=[t for t in trace if t['event']=='CLOSE_135']
check(requests[0]['container_id']==0 and requests[1]['server_open_containers'][0]==requests[0]['server_open_containers'][0]!=0,'minimized container remains open in server table')
for label,get in [('05-minimize',old),('07-minimize',now),('08-expand',now)]:
 before,after=get(label,'before'),get(label)
 keys=[k for k in before['clientcore'] if k.endswith('_requested')]
 check(before['containers']==after['containers'] and all(counter(before,k)==counter(after,k) for k in keys),label+' changes no gameplay state or request counters')
check(counter(old('08-drag'),'moves_requested')==1 and old('08-drag')['containers'][0]['objects'][0]['type_id']==3003,'drag receives authoritative content reorder')
check(counter(old('09-ctrl-amount'),'moves_requested')==counter(old('10-cancel'),'moves_requested')==1,'Ctrl amount and Cancel send no move')
check(counter(now('12-amount-one'),'moves_requested')==0 and counter(now('13-amount-confirm'),'moves_requested')==1,'Ctrl amount confirmation sends exactly one move')
check([o['amount'] for o in now('13-amount-confirm')['containers'][0]['objects'] if o['type_id']==3031]==[1,1],'server splits two coins into one plus one')
check(now('14-stack-restore')['containers']==now('11-ctrl-amount','before')['containers'],'server merges stack back')
check(ids(now('03-third'))==[0,1,2] and [c['has_parent'] for c in now('03-third')['containers']]==[False,True,True],'three legitimate containers and nested parent flags')
check(ids(now('04-close-middle'))==[0,2],'closing middle preserves IDs 0 and 2')
check(now('04-close-middle')['containers'][0]==now('03-third')['containers'][0] and now('04-close-middle')['containers'][1]==now('03-third')['containers'][2],'closing middle preserves other contents')
check(ids(now('05-up-id2'))==[0,2] and now('05-up-id2')['containers'][1]['type_id']==2854,'Up navigates bag to parent at ID 2')
check(ids(now('06-close-id2'))==[0],'shifted window Close routes to ID 2')
for label,event,number in [('04-close-middle','CLOSE_135',1),('05-up-id2','UP_136',2),('06-close-id2','CLOSE_135',2)]:
 before,after=now(label,'before'),now(label)
 found=[t for t in trace if t['event']==event and stamp(before['written_at'])<stamp(t['utc'])<stamp(after['written_at'])]
 check(len(found)==1 and found[0]['container_id']==number,label+' independently observed opcode and ID')
check(old('18-up-parent')['containers'][1]['objects']==old('18-up-parent')['containers'][0]['objects'] and not old('18-up-parent')['containers'][1]['has_parent'],'Up reaches root and clears parent flag')
check(counter(now('10-channel-join'),'channels_opened')==1,'Channels opens server-authorized chat tab')
check(read('A','09-restored')['inventory']==read('A-round1','00-baseline')['inventory'],'A inventory restored by normal Trade')
check(now('24-order-restore')['inventory']==old('01-open')['inventory'] and now('24-order-restore')['containers']==old('01-open')['containers'],'B inventory and container contents restored')
valid=0;invalid=[];bad=[]
for f in sorted(live.rglob('*.json')):
 try: d=json.loads(f.read_text())
 except (ValueError,UnicodeError): invalid.append(str(f.relative_to(live)));continue
 if not isinstance(d,dict) or 'clientcore' not in d: continue
 valid+=1
 for k in ['residual_bytes','unsupported_opcodes','protocol_anomalies']:
  if d['clientcore'].get(k)!=0: bad.append([str(f.relative_to(live)),k,d['clientcore'].get(k)])
check(valid>100 and not bad,'all valid snapshots have zero protocol diagnostics')
result=dict(checks=checks,passed=len(checks),valid_protocol_snapshots=valid,protocol_errors=bad,incomplete_artifacts=invalid)
(here/'audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
