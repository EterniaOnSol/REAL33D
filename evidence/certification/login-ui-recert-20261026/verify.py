"""Replay assertions over retained certification artifacts; no runtime needed."""
import json,re,sys
from datetime import datetime
from pathlib import Path
root=Path(__file__).resolve().parent
checks=[]
def check(name,condition):checks.append({'name':name,'result':'PASS' if condition else 'FAILED'})
def read(path):return json.loads((root/path).read_text(encoding='utf-8-sig'))
def snapshot(path):
    j=read(path)
    check(str(path)+' protocol',all(j['clientcore'][k]==0 for k in ('unsupported_opcodes','residual_bytes','protocol_anomalies')))
    check(str(path)+' INIT_GAME',j['session']['init_games']==1)
    return j
for name,n in [('cycles',5),('restart',1)]:
    rows=[json.loads(x) for x in (root/'live'/name/'login_qa.jsonl').read_text().splitlines()]
    ev=lambda kind:[x['detail'] for x in rows if x['event']==kind]
    check(name+' QA result',ev('result')==['PASS'])
    check(name+' invalid credentials rejected',len(ev('wrong-password'))==1 and 'Accountnumber or password is not correct.' in ev('wrong-password')[0] and 'characters offered 0' in ev('wrong-password')[0])
    check(name+' password box cleared',len(ev('character-list'))==1 and 'password box cleared yes' in ev('character-list')[0])
    entries=ev('in-game');out=ev('logged-out')
    check(name+' expected sessions',len(entries)==n and len(out)==n)
    check(name+' one init per entry',all('init_games 1;' in s for s in entries))
    for i,s in enumerate(entries):
        m=re.search(r'worldstate tiles (\d+); tile actors (\d+)',s)
        check(name+f' session {i+1} rebuild',m is not None and int(m[1])>0 and m[1]==m[2])
    check(name+' all logout returns',all('tile actors 0; local player gone; reconnect attempts 0; rows still offered 1' in s for s in out))
    check(name+' movement every session',len(ev('movement'))==n)

for p in sorted((root/'live').rglob('*.json')):
    if p.name=='launch.json':continue
    try:j=json.loads(p.read_text(encoding='utf-8-sig'))
    except (ValueError,OSError):continue
    if not isinstance(j,dict) or 'clientcore' not in j:continue
    check(str(p.relative_to(root))+' no protocol errors',all(j['clientcore'][k]==0 for k in ('unsupported_opcodes','residual_bytes','protocol_anomalies')))
    check(str(p.relative_to(root))+' no reconnect',j['session']['reconnect_attempts']==0)
    if j['connected_at_close']:
        check(str(p.relative_to(root))+' connected session has one init',j['session']['init_games']==1)

d=snapshot(Path('live/takeover/displaced-held.json'))
check('displaced returns to selection',d['session']['phase']=='character-select')
check('displaced world empty',d['presentation']['tile_actors']==d['presentation']['creature_actors']==d['local_creature_id']==0)
closed=snapshot(Path('live/takeover/unreal_slice_evidence_Disconnected.json'))
check('displacement held beyond retry budget', (datetime.fromisoformat(d['written_at'])-datetime.fromisoformat(closed['written_at'])).total_seconds()>20)
owner=snapshot(Path('live/restart/manual-reclaim.json'))
check('manual reclaim stays in game',owner['session']['phase']=='in-game')
check('reclaim world rebuilt',owner['presentation']['tile_actors']==owner['clientcore']['worldstate_tiles']>0)
for name in ('restart','takeover'):
    log=(root/'live'/name/'REAL33D.log').read_text(errors='replace')
    check(name+' no automatic reconnect log','reconnecting, attempt' not in log)
    check(name+' no duplicate init diagnostic','INIT_GAME received' not in log)
death=root/'live/death/death-observed'
if (death/'death-message.json').exists():
    message=read('live/death/death-observed/death-message.json')
    check('literal server death message',any('server message [EventMessage]: You are dead.' in s for s in message['lines']))
    log=(root/'live/death/REAL33D.log').read_text(errors='replace')
    check('literal HP zero','HUD vitals: HP 0/150' in log)
    check('death no automatic retry','reconnecting, attempt' not in log)
    j=snapshot(Path('live/death/death-observed/unreal_slice_evidence_AfterCleanup.json'))
    check('death returns to selection',j['session']['phase']=='character-select')
    check('death removes world',j['presentation']['tile_actors']==j['presentation']['creature_actors']==j['local_creature_id']==0)
    check('death clears inventory/containers',not j['inventory'] and not j['containers'] and not j['inventory_known'])
    j=snapshot(Path('live/death/relogin.json'))
    check('death reentry',j['session']['phase']=='in-game' and j['connected_at_close'])
    check('death rebuild',j['presentation']['tile_actors']==j['clientcore']['worldstate_tiles']>0 and j['presentation']['creature_actors']==j['clientcore']['worldstate_visible_creatures'])
    process=read('live/death/process-after-relogin.json');launch=read('live/death/launch.json')
    check('same process after death',process['pid']==launch['pid'] and process['process_start_utc']==launch['process_start_utc'])
    walk=read('live/death/relogin-walk.json')
    check('first postdeath movement accepted',bool(walk) and walk[0]['responses'][0]['phase']=='accepted')
else:check('literal death evidence exists',False)

mini=read('minimap-restart.json')
check('minimap stable and private',mini['salt_persisted'] and mini['same_cache_names'] and mini['raw_account_occurrences']==mini['raw_password_occurrences']==0)
log=(root/'live/restart/REAL33D.log').read_text(errors='replace')
check('minimap loaded prior process cache',bool(re.search(r'minimap cache load: accepted cells=[1-9]\d*',log)))
negative=read('scanner-negative.json')
check('scanner fails every planted value',len(negative)==5 and all(x['exit_code']==1 for x in negative[:4]) and negative[-1]['exit_code']==0)
audit=read('credential-audit.json')
for name,j in audit.items():check(name+' credentials absent',j['files_scanned']>0 and j['account_occurrences']==j['password_occurrences']==0)
automation=read('automation-desktop/report/index.json')
check('Unreal 15+',automation['succeeded']>=15 and automation['failed']==automation['notRun']==automation['inProcess']==0)
ancestry=read('ancestry.json')
check('correct ancestry and normal push',ancestry['normal_push_verified'] and ancestry['main_unchanged'] and not ancestry['failed_head_is_ancestor'])
report={'pass':sum(x['result']=='PASS' for x in checks),'failed':sum(x['result']=='FAILED' for x in checks),'checks':checks}
(root/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='checks'}))
for c in checks:
    if c['result']!='PASS':print(c)
sys.exit(bool(report['failed']))
