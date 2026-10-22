"""Read-only certification audit of recorded real UI probes and server responses."""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('live', type=Path)
a = p.parse_args()
live = a.live

def read(name):
    return json.loads((live / name).read_text(encoding='utf-8-sig'))

def pos(value):
    return tuple(value[k] for k in ('x', 'y', 'z'))

directions = {'north': (0, -1, 0), 'east': (1, 0, 0), 'south': (0, 1, 0),
              'west': (-1, 0, 0), 'northwest': (-1, -1, 0),
              'northeast': (1, -1, 0), 'southwest': (-1, 1, 0), 'southeast': (1, 1, 0)}
expected = {
    '06-north-W': 'north', '07-Q-north': 'northwest', '08-E-north': 'northeast',
    '09-Z-north': 'southwest', '10-C-north': 'southeast',
    '11-repeat-Q': 'northwest', '12-repeat-Q': 'northwest', '13-repeat-Q': 'northwest',
    '14-cardinal-D': 'east', '15-diagonal-C': 'southeast', '16-cardinal-S': 'south',
    '17-diagonal-Q': 'northwest', '18-cardinal-A': 'west', '19-diagonal-C': 'southeast',
    '20-arrow-up': 'north', '21-arrow-right': 'east', '22-arrow-down': 'south',
    '23-arrow-left': 'west', '37-south-W': 'south', '38-Q-south': 'southeast',
    '39-E-south': 'southwest', '40-return-east': 'east', '41-return-origin': 'southeast'}
controlled = []
rows = []
for label, direction in expected.items():
    r = read(label + '-result.json')
    before, after = read(label + '-before.json'), read(label + '-after.json')
    assert before['written_at'] < after['written_at'], label + ': stale snapshot'
    sent, terminal = r['journal']  # exactly two entries: no extra walk/response
    outcome = 'rejected' if label == '13-repeat-Q' else 'accepted'
    assert sent['phase'] == 'sent' and terminal['phase'] == outcome, label
    assert sent['input_id'] == terminal['input_id'] and sent['request_id'] == terminal['request_id'], label
    assert sent['direction'] == terminal['direction'] == direction, label
    delta = directions[direction] if outcome == 'accepted' else (0, 0, 0)
    assert pos(r['before']) == pos(sent['from']) == pos(terminal['from']), label
    assert tuple(b + d for b, d in zip(pos(r['before']), delta)) == pos(r['after']) == pos(terminal['to']), label
    assert r['requested_after'] - r['requested_before'] == 1, label
    controlled.append(sent['request_id'])
    rows.append(dict(probe=label, direction=direction, outcome=outcome,
                     request_id=sent['request_id'], input_id=sent['input_id'],
                     before=r['before'], after=r['after'], delta=delta))

for label in ('24-chat-Q', '25-chat-E', '26-chat-Z', '27-chat-C', '28-chat-escape', '30-world-look'):
    r = read(label + '-result.json')
    assert not r['journal'] and r['before'] == r['after'], label
    assert r['requested_before'] == r['requested_after'], label
    assert read(label + '-before.json')['written_at'] < read(label + '-after.json')['written_at'], label
look_before, look_after = read('30-world-look-before.json'), read('30-world-look-after.json')
assert look_after['clientcore']['looks_requested'] - look_before['clientcore']['looks_requested'] == 1
assert any('You see grass.' in x['line'] for x in look_after['chat_transcript'])

mouse = read('29-mouse-walk-result.json')
assert [x['phase'] for x in mouse['journal']] == ['sent', 'accepted', 'sent', 'accepted']
assert all(x['direction'] == 'north' for x in mouse['journal'])
assert pos(mouse['after']) == tuple(b + d for b, d in zip(pos(mouse['before']), (0, -2, 0)))
controlled.extend(x['request_id'] for x in mouse['journal'] if x['phase'] == 'sent')

north = read('07-Q-north-before.json')['camera']['yaw']
south = read('38-Q-south-before.json')['camera']['yaw']
rotation = (north - south) % 360
assert abs(rotation - 180) < 1, rotation
assert pos(read('07-Q-north-before.json')['local_position']) == pos(read('10-C-north-after.json')['local_position'])
assert pos(read('11-repeat-Q-before.json')['local_position']) == (32087, 32227, 7)
assert pos(read('12-repeat-Q-after.json')['local_position']) == (32085, 32225, 7)

journal = [json.loads(s) for s in (live / 'movement_journal.jsonl').read_text().splitlines() if s]
groups = defaultdict(list)
for entry in journal:
    groups[entry['request_id']].append(entry)
for req, entries in groups.items():
    assert len(entries) == 2 and entries[0]['phase'] == 'sent', req
    assert entries[1]['phase'] in ('accepted', 'rejected'), req
    assert entries[0]['input_id'] == entries[1]['input_id'], req
assert len({entries[0]['input_id'] for entries in groups.values()}) == len(groups)
assert len(controlled) == len(set(controlled)) == 25
assert set(groups) - set(controlled) == {1}  # unexplained setup input, never claimed as a controlled test
assert read('05-orbit-north-fine-result.json')['journal'] == groups[1]

snapshots = [read(f.name) for f in live.glob('*.json') if f.name.endswith(('-before.json', '-after.json'))]
end = read('unreal_slice_evidence.json')
snapshots.append(end)
for s in snapshots:
    assert s['connected_at_close']
    for key in ('residual_bytes', 'unsupported_opcodes', 'protocol_anomalies', 'walks_unanswered', 'external_relocations'):
        assert s['clientcore'][key] == 0, (s['written_at'], key)
    assert s['last_diagnostic'] == 'none'
cc = end['clientcore']
assert cc['walks_requested_from_unreal'] == len(groups) == 26
assert cc['walks_accepted'] == cc['local_player_moves_total'] == 25
assert cc['walks_rejected'] == 1
assert cc['moves_requested'] == cc['uses_requested'] == cc['says_requested'] == 0
assert pos(end['local_position']) == pos(read('00-baseline.json')['local_position'])
report = dict(status='PASS', scope='25 controlled walk requests; setup request 1 excluded from gesture attribution',
              controlled_requests=controlled, controlled_key_probes=rows,
              north_yaw=north, south_yaw=south, camera_rotation_degrees=rotation,
              chat_no_walk=True, chat_text='qezc visually inspected in 27-chat-C.png',
              mouse_walk_requests=2, look_response='You see grass.',
              end_counters=cc, snapshots_checked=len(snapshots),
              duplicate_sends=0, duplicate_terminal_responses=0,
              excluded_setup_request=groups[1],
              limitation='Origin of setup input 1 UNVERIFIED; aborted orbit 33 lost foreground and is not a passing probe.')
(live / 'audit.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k: v for k, v in report.items() if k not in ('controlled_key_probes', 'excluded_setup_request', 'end_counters', 'controlled_requests')}))
