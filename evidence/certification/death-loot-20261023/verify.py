"""Recheck retained live evidence; no game input or production changes."""
import json, struct
from pathlib import Path
from datetime import datetime
here=Path(__file__).resolve().parent
live=here/'live'
if not live.exists():live=here.parents[2]/'build/certification/live'
rows=[json.loads(s) for s in (live/'replayed.jsonl').read_text().splitlines()]
trace=[json.loads(s) for s in (live/'server-trace.jsonl').read_text().splitlines()]
checks=[]
def check(name,condition):
    checks.append(dict(check=name,passed=bool(condition)))
    if not condition:raise AssertionError(name)
def snap(run,label):return json.loads((live/run/(label+'-after.json')).read_text())
def inv(d):return [(i['slot'],i['type_id'],i['amount'] or 1) for i in d['inventory']]
def containers(d):return [(c['number'],c['type_id'],[(i['type_id'],i['amount'] or 1) for i in c['objects']]) for c in d['containers']]
def timestamp(s):return datetime.fromisoformat(s.replace('Z','+00:00'))
check('observer has no errors',not any(t['event']=='OBSERVER_ERROR' for t in trace))
check('all observed server commands decode and apply cleanly',all('error' not in r and r['anomalies']==0 for r in rows))
for cid,position in [(1073743323,[32112,32248,7]),(1073743321,[32105,32249,7])]:
    deaths=[r for r in rows if r['opcode']==140 and r['creature_id']==cid and r['health']==0]
    check(f'{cid} health reaches zero',bool(deaths))
    births=[r for r in rows if r['opcode']==106 and r['after']['position']==position and any(i.get('type_id')==4173 for i in r['after']['stack'])]
    check(f'{cid} corpse on death tile',bool(births) and births[0]['utc']>=deaths[0]['utc'])
    removed=[r for r in rows if r['opcode']==108 and r['before']['position']==position
             and any(i.get('creature_id')==cid for i in r['before']['stack'])
             and not any(i.get('creature_id')==cid for i in r['after']['stack'])]
    check(f'{cid} authoritative creature removal retains corpse',bool(removed) and any(i.get('type_id')==4173 for i in removed[0]['after']['stack']))
# The optimized binary did not hit the standalone Death symbol (calls may be
# inlined); health-zero, corpse-add and creature-delete wire commands are the
# evidence of death. Do not promote an unhit breakpoint into a PASS.
matched=0
for f in sorted(live.glob('*/*-after.json')):
    d=json.loads(f.read_text())
    check(str(f.relative_to(live))+' protocol counters zero',all(d['clientcore'][k]==0 for k in ('unsupported_opcodes','residual_bytes','protocol_anomalies')))
    if not d.get('connected_at_close'):continue
    prior=[r for r in rows if timestamp(r['utc'])<=timestamp(d['written_at'])]
    r=prior[-1]
    check(str(f.relative_to(live))+' inventory matches observed server bytes',
          inv(d)==[(i['slot'],i['item']['type_id'],i['item']['amount']) for i in r['inventory']])
    check(str(f.relative_to(live))+' containers match observed server bytes',
          containers(d)==[(c['number'],c['type_id'],[(i['type_id'],i['amount']) for i in c['items']]) for c in r['containers']])
    matched+=1
for f in live.glob('*/*-freshness.json'):
    d=json.loads(f.read_text());check(str(f.relative_to(live))+' snapshots fresh',d['before_fresh'] and d['after_fresh'])
a=snap('A2','019-corpse-use')
b=snap('A2','020-loot-one-inventory')
c=snap('A2','021-loot-backpack')
check('corpse opened with meat x2',containers(a)[1]==(1,4173,[(3577,2)]))
check('corpse to inventory: one item', (10,3577,1) in inv(b) and containers(b)[1]==(1,4173,[(3577,1)]))
check('corpse to backpack: remaining item',containers(c)[0]==(0,2854,[(3577,1),(2853,1)]) and containers(c)[1]==(1,4173,[]))
check('empty corpse reopens empty',containers(snap('A2','024-reopen-empty'))[1]==(1,4173,[]))
check('Look corpse description is authoritative',any(r.get('message','').startswith('You see a dead rabbit') for r in rows))
for label in ('026-amount-dialog','027-select-one'):
    check(label+' sends no move',snap('A2',label)['clientcore']['moves_requested']==4)
check('amount confirmation sends one move',snap('A2','028-confirm-one')['clientcore']['moves_requested']==5)
check('amount confirmed inventory=1 backpack=1',(10,3577,1) in inv(snap('A2','028-confirm-one')) and containers(snap('A2','028-confirm-one'))[0][2][0]==(3577,1))
decay=[r for r in rows if r['opcode']==107 and r['after']['position']==[32112,32248,7] and any(i.get('type_id')==4174 for i in r['after']['stack'])]
check('natural corpse decay 4173 to 4174',bool(decay))
check('decay closes open corpse from server',any(r['opcode']==111 and r['utc']==decay[0]['utc'] for r in rows))
inputs=[t for t in trace if t['event']=='IN']
moves=[bytes.fromhex(t['raw_hex']) for t in inputs if t['opcode']==120]
check('five move commands observed total',len(moves)==5)
check('four successful loot/stack moves plus one documented stale-coordinate refusal',sum(struct.unpack('<H',b[6:8])[0]==3577 for b in moves)==4)
check('one attack per live combat session',sum(t['opcode']==161 for t in inputs)==2)
report=dict(passed=len(checks),failed=0,server_commands=len(rows),matched_live_snapshots=matched,
    standalone_death_breakpoint_hits=sum(t['event']=='TCreature::Death' for t in trace),checks=checks,
    limitations=['Final corpse disappearance after remaining decay stages not observed.',
                 'Only rabbit corpses, floor 7 and meat stacks exercised live.',
                 'Creature-specific visual meshes/animations not certified.'])
(here/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='checks'},indent=2))
