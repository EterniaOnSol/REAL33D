"""Read-only audit of captured production F9 snapshots and production logs."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parent
expected = [0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,7,8,9,10,11,12,12,12,13,14,15,15,16,17,17,17,18,19]
files = sorted((root / 'run2').glob('*-state.json'))
assert len(files) == len(expected), (len(files), len(expected))
rows = []
for path, moves in zip(files, expected):
    state = json.loads(path.read_text(encoding='utf-8-sig'))
    counters = state['clientcore']
    assert counters['moves_requested'] == moves, path.name
    assert state['connected_at_close'], path.name
    assert all(counters[k] == 0 for k in ('residual_bytes', 'unsupported_opcodes', 'protocol_anomalies')), path.name
    rows.append(dict(snapshot=path.name, moves=moves,
                     uses=counters['uses_requested'], looks=counters['looks_requested'],
                     inventory=state['inventory'], containers=state['containers']))
end = json.loads((root / 'run2/unreal_slice_evidence.json').read_text(encoding='utf-8-sig'))
assert end['reason'] == 'EndPlay' and end['connected_at_close']
assert end['clientcore']['moves_requested'] == 19
assert end['inventory'] == rows[1]['inventory']
assert end['containers'] == rows[1]['containers']
assert all(end['clientcore'][k] == 0 for k in ('residual_bytes', 'unsupported_opcodes', 'protocol_anomalies'))
log = (root / 'run2/application-state.txt').read_text(encoding='utf-8-sig')
requests = re.findall(r'LogReal33D: move (\d+): (.*)', log)
assert len(requests) == 19 and len(set(i for i, _ in requests)) == 19
result = dict(status='PASS', snapshots=len(rows), intended_moves=19, unique_move_requests=len(requests),
              protocol_errors=0, restored_inventory_and_container=True, rows=rows, move_requests=requests)
(root / 'audit.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print('PASS: 34 snapshots; 19 unique moves; inventory/container restored; protocol counters zero')
