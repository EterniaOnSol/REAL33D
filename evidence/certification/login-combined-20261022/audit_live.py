"""Recompute certification gates from raw logs, real-input journals and snapshots."""
import argparse
from collections import Counter
from datetime import datetime
import json
from pathlib import Path
import re

p = argparse.ArgumentParser()
p.add_argument('live', type=Path)
p.add_argument('output', type=Path)
a = p.parse_args()
result = {'sessions': {}}
def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))
def timevalue(text):
    return datetime.fromisoformat(text.replace('Z', '+00:00'))
def event_times(text, phrase):
    pattern = r'\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\].*' + re.escape(phrase)
    return [datetime.strptime(t, '%Y.%m.%d-%H.%M.%S:%f') for t in re.findall(pattern, text)]
hashes = set()
for name in ('C0', 'H1', 'H2', 'H3', 'H4', 'Recovery'):
    directory = a.live / name
    log = (directory / 'REAL33D.log').read_text(encoding='utf-8-sig', errors='replace')
    meta = load(directory / 'launch.json')
    hashes.add(meta['dll_sha256'])
    connects = event_times(log, 'connected: Test Player B')
    inits = event_times(log, 'local player is creature 1002')
    worlds = event_times(log, '"Test Player B" appeared at')
    attempts = event_times(log, 'reconnecting, attempt')
    assert len(connects) == len(inits) == len(worlds) == 1, name
    assert len(attempts) == (1 if name == 'Recovery' else 0), name
    stable_path = directory / 'stable-owner-resumed.json'
    stable = load(stable_path if stable_path.exists() else directory / 'stable-owner.json')
    hold = (timevalue(meta['hold_finished_utc']) - timevalue(meta['hold_started_utc'])).total_seconds()
    assert hold > 30, (name, hold)
    assert stable['connected_at_close'] and stable['local_creature_id'] == 1002
    assert stable['clientcore']['walks_requested_from_unreal'] == stable['clientcore']['walks_accepted'] == 2
    assert stable['clientcore']['worldstate_tiles'] > 0
    assert stable['clientcore']['walks_unanswered'] == stable['clientcore']['external_relocations'] == 0
    journal = [json.loads(line) for line in (directory / 'movement_journal.jsonl').read_text().splitlines()]
    assert len(journal) == 4, (name, journal)
    for index, (label, delta) in enumerate((('01-south', (0, 1, 0)), ('02-north', (0, -1, 0)))):
        probe = load(directory / (label + '-result.json'))
        rows = probe['journal']
        assert len(rows) == 2 and [r['phase'] for r in rows] == ['sent', 'accepted'], name
        assert rows[0]['input_id'] == rows[1]['input_id'] == index + 1, name
        assert tuple(probe['after'][v] - probe['before'][v] for v in ('x', 'y', 'z')) == delta, name
    first_key = event_times(log, 'input 1: walk south requested from Unreal')[0]
    latency = (first_key - worlds[0]).total_seconds()
    assert 0 <= latency <= 10, (name, latency)
    item = dict(connects=1, inits=1, worlds=1, reconnects=len(attempts), hold_seconds=hold,
                first_cardinal_seconds_after_world=latency, accepted_walks=2,
                authoritative_position=stable['local_position'])
    if name in ('C0', 'H1', 'H2', 'H3'):
        old = load(directory / 'displaced-after-hold.json')
        assert not old['connected_at_close']
        assert log.count('ownership released: this client will not reconnect by itself') == 1
        assert 'Fusion32 closed this connection; not reconnecting automatically' in log
        item['displaced_no_reconnect'] = True
    if name == 'Recovery':
        restored = timevalue(meta['service_recovery']['utc'])
        noticed = timevalue(meta['failure_observed_utc'])
        item['service_restored_seconds_after_observed_failure'] = (restored - noticed).total_seconds()
        assert 0 < item['service_restored_seconds_after_observed_failure'] < 5
        remote = load(directory / 'remote-closed-final.json')
        assert not remote['connected_at_close']
        assert 'Fusion32 closed this connection; not reconnecting automatically' in log
        closed = event_times(log, 'Fusion32 closed this connection; not reconnecting automatically')
        assert len(closed) == 1
        # Preserve the already used recovery retry; require no new retry after closure.
        assert attempts[0] < closed[0]
        observed = datetime.fromisoformat(remote['written_at'].replace('Z', '+00:00')).replace(tzinfo=None)
        item['remote_closed_observation_seconds'] = (observed - closed[0]).total_seconds()
        assert item['remote_closed_observation_seconds'] > 30
    result['sessions'][name] = item

budget = (a.live / 'Budget/REAL33D.log').read_text(encoding='utf-8-sig', errors='replace')
fails = event_times(budget, 'session ended: could not reach the login service')
retries = event_times(budget, 'reconnecting, attempt')
assert len(fails) == 4 and len(retries) == 3
delays = [(retry - fails[n]).total_seconds() for n, retry in enumerate(retries)]
assert all(5 <= value < 7 for value in delays), delays
assert 'connected: Test Player B' not in budget
result['login_service_retry'] = dict(initial_failure=1, retries=3, delays_after_failure_seconds=delays)

events = [json.loads(line) for line in (a.live / 'server-trace.jsonl').read_text().splitlines()]
events = [e for e in events if e['event'] != 'TRACE_READY' and e.get('connection') != '0x0']
counts = Counter(e['event'] for e in events)
assert counts == dict(TAKEOVER=4, CLEAR_TARGET=4, INIT_GAME=6, FULLSCREEN=6), counts
for n, event in enumerate(events):
    if event['event'] == 'TAKEOVER':
        group = events[n:n+4]
        assert [v['event'] for v in group] == ['TAKEOVER', 'CLEAR_TARGET', 'INIT_GAME', 'FULLSCREEN']
        assert len({v['connection'] for v in group}) == 1
assert len(hashes) == 1
result['dll_sha256'] = hashes.pop()
result['server_call_counts'] = dict(counts)
scanned = 0
for path in sorted(a.live.rglob('*')):
    if path.suffix not in ('.json', '.jsonl', '.log', '.txt'):
        continue
    scanned += 1
    data = path.read_text(encoding='utf-8-sig', errors='replace')
    values = re.findall(r'"(?:residual_bytes|unsupported_opcodes|protocol_anomalies)"\s*:\s*(\d+)', data)
    assert all(int(value) == 0 for value in values), path
    assert not re.search(r'is not decoded|could not be decoded|left unwalked|desynchronis', data), path
result['protocol_scan'] = dict(files=scanned, residual_bytes=0, unsupported_opcodes=0, protocol_anomalies=0)
result['certification'] = 'CERTIFIED'
a.output.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
