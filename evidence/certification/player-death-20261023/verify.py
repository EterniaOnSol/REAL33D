"""Audit recorded live evidence; never drives the game or alters server state."""
import argparse,json
from datetime import datetime
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('live',type=Path)
a=p.parse_args(); root=a.live
def read(path): return json.loads(path.read_text(encoding='utf-8-sig'))
def stamp(s): return datetime.fromisoformat(s.replace('Z','+00:00'))
def rows(path): return [json.loads(s) for s in path.read_text().splitlines() if s]
checks=[]
def check(name,ok):
    checks.append(dict(name=name,passed=bool(ok)))
    print(('PASS ' if ok else 'FAIL ')+name)
trace=rows(root/'server-trace.jsonl')
decoded=rows(root/'replayed.jsonl')
old=root/'A-cert'; new=root/'A-relogin'
old_launch=read(old/'launch.json'); new_launch=read(new/'launch.json')
start=stamp(old_launch['utc']); relogin=stamp(new_launch['utc'])
death_rows=[r for r in decoded if start<=stamp(r['utc'])<relogin]
fresh_rows=[r for r in decoded if stamp(r['utc'])>=relogin]
death_events=[r for r in trace if r['event']=='PLAYER_DEATH' and start<=stamp(r['utc'])<relogin]
messages=[r for r in death_rows if r.get('message')=='You are dead.\n' and r.get('message_mode')==19]
check('one observed server player death',len(death_events)==1)
check('one exact event message, including wire newline',len(messages)==1)
check('authoritative HP reaches zero',any(r['opcode']==160 and r['stats']['hp']==0 for r in death_rows))
check('death actor removed from authoritative visible set',not any(c['id']==1001 for c in death_rows[-1]['visible_creatures']))
check('death session one INIT_GAME',sum(r['opcode']==10 for r in death_rows)==1)
check('fresh relogin exactly one INIT_GAME',sum(r['opcode']==10 for r in fresh_rows)==1)
check('same unchanged audit DLL',old_launch['dll_sha256']==new_launch['dll_sha256'])
check('replay errors and anomalies zero',all('error' not in r and r.get('anomalies',0)==0 for r in decoded))
check('observer errors zero',not any(r['event']=='OBSERVER_ERROR' for r in trace))
death_time=stamp(messages[0]['utc'])
close=[r for r in trace if r['event']=='DISCONNECT' and death_time<=stamp(r['utc'])<relogin]
check('one server disconnect after death',len(close)==1)
delay=(stamp(close[0]['utc'])-death_time).total_seconds()
check('orderly delayed close about 30 seconds',25<=delay<=35)
old_log=(old/'REAL33D.log').read_text(errors='replace')
check('client reports orderly close', 'Fusion32 closed this connection; not reconnecting automatically' in old_log)
check('client releases ownership','ownership released: this client will not reconnect by itself' in old_log)
check('old client automatic reconnect attempts zero','reconnecting, attempt' not in old_log)
closed=read(old/'unreal_slice_evidence_AfterCleanup.json')
check('cleanup removes all presentation actors',closed['presentation']['tile_actors']==0 and closed['presentation']['creature_actors']==0)
check('cleanup clears local identity and origin',closed['local_creature_id']==0 and not closed['origin']['set'])
check('cleanup clears equipment and containers',not closed['inventory'] and not closed['containers'])
check('cleanup combat state neutral',closed['combat']['target_creature_id']==0 and not closed['combat']['following'])
fresh=read(new/'001-relogin-after.json')
check('relogin recreates exactly one local player',fresh['connected_at_close'] and fresh['local_creature_id']==1001 and sum(c['local'] for c in fresh['creatures'])==1)
check('initial world rebuilds',fresh['presentation']['tile_actors']>0 and fresh['clientcore']['viewport_synchronised'])
check('relogin combat neutral',fresh['combat']['target_creature_id']==0 and not fresh['combat']['following'])
before=read(new/'002-immediate-move-before.json'); after=read(new/'003-move-confirmed-after.json')
check('first independent post-relogin walk accepted',after['clientcore']['walks_accepted']==before['clientcore']['walks_accepted']+1 and before['local_position']!=after['local_position'])
for folder in (old,new):
    for path in sorted(folder.glob('*freshness.json')):
        f=read(path)
        if path.name=='002-immediate-move-freshness.json':
            check('stale immediate-move after snapshot explicitly excluded',f['before_fresh'] and not f['after_fresh'])
        else: check(path.parent.name+'/'+path.name,f['before_fresh'] and f['after_fresh'])
    for path in sorted(folder.glob('*.json')):
        s=read(path)
        if 'clientcore' not in s: continue
        check(path.parent.name+'/'+path.name+' protocol zero',all(s['clientcore'][k]==0 for k in ('residual_bytes','unsupported_opcodes','protocol_anomalies')))
        if not s['connected_at_close']:continue
        when=stamp(s['written_at'])
        candidates=[r for r in decoded if stamp(r['utc'])<=when]
        authoritative=candidates[-1]
        inventory=sorted((i['slot'],i['type_id'],i.get('amount') or 1) for i in s['inventory'])
        wire=sorted((i['slot'],i['item']['type_id'],i['item']['amount']) for i in authoritative['inventory'])
        check(path.parent.name+'/'+path.name+' authoritative inventory',inventory==wire)
        check(path.parent.name+'/'+path.name+' authoritative local id',s['local_creature_id']==authoritative['player_id'])
overlap=read(old/'ownership-after-relogin-after.json')
check('old window remains disconnected after new login',not overlap['connected_at_close'] and overlap['local_creature_id']==0 and stamp(overlap['written_at'])>stamp(fresh['written_at']))
check('no old client login before manual relaunch',not any(r['event']=='LOGIN_OBSERVED' and stamp(close[0]['utc'])<stamp(r['utc'])<relogin for r in trace))
stats=[r['stats'] for r in fresh_rows if r['opcode']==160]
check('authoritative respawn stats',bool(stats) and stats[0]['hp']==150 and stats[0]['max_hp']==150 and stats[0]['mana']==0 and stats[0]['max_mana']==0 and stats[0]['level']==1 and stats[0]['experience']==90)
initial_map=next(r for r in fresh_rows if r['opcode']==100)
check('authoritative temple respawn position',initial_map['anchor']==[32097,32219,7])
check('presentation origin matches server respawn',fresh['origin']==dict(set=True,x=32097,y=32219,z=7))
result=dict(checks=checks,passed=sum(c['passed'] for c in checks),failed=sum(not c['passed'] for c in checks),decoded_commands=len(decoded),disconnect_delay_seconds=delay)
(root/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='checks'}))
raise SystemExit(0 if result['failed']==0 else 1)
