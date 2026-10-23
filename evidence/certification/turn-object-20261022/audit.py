"""Independent assertions against recorded live evidence, not a gameplay replay."""
import hashlib
import json
import re
from datetime import datetime
from pathlib import Path

HERE = Path(__file__).resolve().parent
LIVE = HERE / 'live'
checks = []


def check(name, condition):
    assert condition, name
    checks.append(name)


def read(relative):
    return json.loads((LIVE / relative).read_text(encoding='utf-8-sig'))


def snap(run, label, when='after'):
    return read(f'{run}/{label}-{when}.json')


def stamp(text):
    return datetime.fromisoformat(text.replace('Z', '+00:00')).timestamp()


traces = [json.loads(line) for line in (LIVE / 'server-trace.jsonl').read_text().splitlines()]
turns = [e for e in traces if e['event'] == 'TURN_133']
changes = [e for e in traces if e['event'] == 'CHANGE_FIELD_107']
logs = {run: (LIVE / run / 'REAL33D.log').read_text(errors='replace') for run in ('B', 'B2', 'B3')}
rotates = []
pattern = r'\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\].*LogReal33D: rotate (\d+): object (\d+) at (\d+),(\d+),(\d+) stack (\d+)'
for run, log in logs.items():
    for match in re.finditer(pattern, log):
        d = datetime.strptime(match[1], '%Y.%m.%d-%H.%M.%S:%f')
        rotates.append(dict(run=run))
        # Log timestamps are UTC, independent of the auditing host timezone.
        rotates[-1].update(time=stamp(d.isoformat()+'Z'), id=int(match[2]),
                           type_id=int(match[3]), x=int(match[4]), y=int(match[5]),
                           z=int(match[6]), stack=int(match[7]))
untraced = []
for action in rotates:
    hits = [e for e in turns if all(e[k] == action[k] for k in ('x','y','z','type_id','stack'))
            and 0 <= stamp(e['utc']) - action['time'] < 2]
    if not hits:
        untraced.append(action)
    else:
        check(f"one opcode 133 for {action['run']} action {action['id']}", len(hits) == 1)
check('exactly one disclosed untraced cleanup activation',
      len(untraced) == 1 and untraced[0]['run'] == 'B3' and untraced[0]['id'] == 1)
check('all 9 observed opcode 133 requests matched to UI actions', len(turns) == len(rotates)-1 == 9)
statue = [e for e in changes if (e['x'],e['y'],e['z']) == (32081,32203,7)]
chest = [e for e in changes if (e['x'],e['y'],e['z']) == (32110,32203,6)]
check('statue complete cycle and restored', [e['type_id'] for e in statue] == [2048,2046,2047,2030])
check('chest accepted change and restored', [e['type_id'] for e in chest] == [2482,2481,2472])
check('both rotatable targets preserve stack 1', all(e['stack'] == 1 for e in statue+chest))
check('server field bytes encode opcode 107', all(bytes.fromhex(e['raw_hex'])[0] == 107 for e in changes))
held = next(e for e in traces if e['event'] == 'TURN_HELD')
released = next(e for e in traces if e['event'] == 'TURN_RELEASED')
check('authoritative first change follows 12-second hold',
      stamp(released['utc'])-stamp(held['utc']) >= 12 and stamp(statue[0]['utc']) > stamp(released['utc']))
check('client still resolves original type during hold',
      '[2026.10.23-03.05.43:824]' in logs['B'] and
      'world context menu opened for object 2030 at 32081,32203,7 stack 1' in logs['B'])
check('client resolves changed type after release',
      'world context menu opened for object 2048 at 32081,32203,7 stack 1' in logs['B'])
check('nonrotatable request targets ground stack 0',
      any(e['type_id'] == 408 and e['stack'] == 0 for e in turns))
check('authoritative NOTTURNABLE 57 and displayed refusal',
      any(e['event'] == 'RESULT' and e['result'] == 57 for e in traces)
      and 'You cannot turn this object.' in logs['B'])
check('range approach performed by server',
      snap('B','011-statue-held-rotate')['local_position'] == dict(x=32084,y=32204,z=7)
      and snap('B','013-statue-released')['local_position'] == dict(x=32082,y=32204,z=7)
      and snap('B','013-statue-released')['clientcore']['external_relocations'] == 2)
check('blocked chest approach refused NOWAY 44',
      any(e['event'] == 'RESULT' and e['result'] == 44 for e in traces)
      and 'There is no way.' in logs['B2'])
check('Look regression receives description', 'You see a dustbin.' in logs['B'] and 'You see wooden floor.' in logs['B'])
before = snap('B','027-drag-rope','before')
after = snap('B','027-drag-rope')
check('drag sends one move and server reorders rope',
      after['clientcore']['moves_requested']-before['clientcore']['moves_requested'] == 1
      and after['containers'][0]['objects'][0]['type_id'] == 3003)
check('drag bag order restored', snap('B','028-restore-bag-order')['containers'] == before['containers'])
check('Ctrl amount dialog sends no move until confirmed',
      snap('B','029-ctrl-stack-dialog')['clientcore']['moves_requested']
      == snap('B','029-ctrl-stack-dialog','before')['clientcore']['moves_requested'])
check('Ctrl amount cancel sends no move',
      snap('B2','041-stack-cancel')['clientcore']['moves_requested'] == 0
      and 'nothing reached Fusion32' in logs['B2'])
check('plain right click Use opens backpack once',
      snap('B3','052-backpack-use')['clientcore']['uses_requested'] == 1
      and snap('B3','052-backpack-use')['containers'][0]['type_id'] == 2854)
check('UseWith arm sends nothing',
      snap('B3','053-usewith-rope-arm')['clientcore']['uses_requested'] == 1)
check('Ctrl world target sends exactly one use and consumes targeting',
      snap('B3','054-usewith-ctrl-world-target')['clientcore']['uses_requested'] == 2
      and 'use-with 5: object 3003 on object 2472 at 32110,32203,6 stack 1' in logs['B3']
      and 'You cannot use this object.' in logs['B3'])
target_input = read('B3/054-usewith-ctrl-world-target-input.json')
check('UseWith was actual Ctrl right click', target_input['modifier'] == 'ctrl' and target_input['real_right_click'] == [591,389])
check('UseWith never opened a world menu for that target',
      'world context menu opened for object 2472 at 32110,32203,6 stack 1' not in logs['B3'])
check('door restored to original type', changes[-1]['type_id'] == 1632
      and (changes[-1]['x'],changes[-1]['y'],changes[-1]['z']) == (32108,32204,6))
final = read('B3/unreal_slice_evidence.json')
check('final inventory and backpack order restored',
      final['inventory'] == before['inventory'] and final['containers'] == before['containers'])
check('final 3 use sends match backpack, UseWith, door', final['clientcore']['uses_requested'] == 3)
snapshots = []
for path in sorted(LIVE.rglob('*.json')):
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    if 'clientcore' in data:
        snapshots.append(data)
check('every retained snapshot has zero protocol diagnostics', all(
    all(d['clientcore'][key] == 0 for key in ('residual_bytes','unsupported_opcodes','protocol_anomalies'))
    for d in snapshots))
report = json.loads((HERE/'validation/automation-report.json').read_text(encoding='utf-8-sig'))
check('retained Unreal automation 10/10 no warnings/errors',
      report['succeeded'] == 10 and all(report[k] == 0 for k in ('failed','notRun','inProcess','succeededWithWarnings')))
manifest = json.loads((HERE/'raw-manifest.json').read_text())
check('all published raw artifacts match preserved hashes', all(
    hashlib.sha256((LIVE / e['path']).read_bytes()).hexdigest() == e['sha256']
    for e in manifest if e['published']))
stale = []
for path in sorted(LIVE.rglob('*-input.json')):
    input_data = json.loads(path.read_text())
    after_path = path.with_name(path.name.replace('-input.json','-after.json'))
    if after_path.exists():
        data = json.loads(after_path.read_text())
        if stamp(data['written_at']) < stamp(input_data['utc']):
            stale.append(str(after_path.relative_to(LIVE)))
summary = dict(status='PASS', assertions=len(checks), checks=checks,
               snapshots=len(snapshots), ui_rotate_activations=len(rotates),
               independently_observed_turns=len(turns), duplicate_sends_observed=0,
               untraced_cleanup_activation=dict(run='B3',action=1,type_id=2482),
               stale_after_snapshots=stale,
               limitations=['UseWith covers target selection and authoritative refusal, not successful rope gameplay.',
                            'Placeholder presentation does not prove visible mesh orientation.',
                            'One cleanup rotation was outside the independent debugger observation window.',
                            'Stale F9 copies are retained; logs, screenshots and fresh subsequent captures support those steps.'])
(HERE/'audit-result.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k not in ('checks','stale_after_snapshots')}))
print('Stale after-capture copies disclosed:',len(stale))
