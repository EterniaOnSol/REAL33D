import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
log = (root / 'REAL33D.log').read_text(encoding='utf-8', errors='replace')
assert 'OUTFIT_CERT_RESULT=PASS' in log, 'live run did not complete'
assert not re.search(r'OUTFIT_CERT_ASSERT FAILED', log), 'live assertion failed'
assertions = re.findall(r'OUTFIT_CERT_ASSERT PASS (.+)', log)
wire = re.findall(r'OUTFIT_CERT_WIRE opcode=(\d+) bytes=(\d+) hex=([0-9a-f]+)', log)
get = [x for x in wire if x[0] == '210']
sets = [x for x in wire if x[0] == '211']
assert len(get) == 4 and all(x[1:] == ('1', 'd2') for x in get)
assert len(sets) == 2 and all(x[1] == '7' and len(x[2]) == 14 for x in sets)
states = {}
for label, oid, a, b, c, d, disguise, obj in re.findall(
        r'OUTFIT_CERT_STATE (.+?) id=(\d+) colors=(\d+),(\d+),(\d+),(\d+) disguise=(\d+) object=(\d+)', log):
    states[label] = [int(oid), int(a), int(b), int(c), int(d), int(disguise), int(obj)]
assert states['draft selected'] == states['authoritative'] == states['reopened']
assert all(states['baseline'][i] != states['draft selected'][i] for i in range(5))
def encode(state):
    return (bytes([211]) + state[0].to_bytes(2, 'little') + bytes(state[1:5])).hex()
assert sets[0][2] == encode(states['draft selected'])
assert sets[1][2] == encode(states['baseline'])
assert 'final reopen confirms restored outfit and four colours' in assertions
snapshots = []
for p in sorted(root.glob('*.json')):
    data = json.loads(p.read_text(encoding='utf-8-sig'))
    def walk(node):
        if isinstance(node, dict):
            for k,v in node.items():
                if k in ('residual_bytes', 'unsupported_opcodes', 'protocol_anomalies'):
                    assert v == 0, (p.name,k,v)
                walk(v)
        elif isinstance(node, list):
            for v in node: walk(v)
    walk(data)
    snapshots.append(p.name)
assert snapshots
summary = dict(result='PASS', assertions=len(assertions), wire=wire, states=states,
               snapshots=snapshots, limitations=['RGB rendering NOT_IMPLEMENTED',
               'disguise rendering NOT_IMPLEMENTED', 'OS input injection not exercised'])
print(json.dumps(summary, indent=2))
