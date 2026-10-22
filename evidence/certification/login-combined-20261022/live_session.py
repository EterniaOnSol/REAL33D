"""Launch unchanged REAL33D; observe logs, drive real Win32 input, preserve evidence."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

p = argparse.ArgumentParser()
p.add_argument('--name', required=True, choices=('C0', 'H1', 'H2', 'H3', 'H4', 'Budget', 'Recovery'))
p.add_argument('--previous', choices=('C0', 'H1', 'H2', 'H3'))
p.add_argument('--mode', choices=('world', 'budget', 'recovery'), default='world')
a = p.parse_args()
root = Path(__file__).resolve().parents[3]
live_root = root / 'build/certification/live'
live = live_root / a.name
live.mkdir(parents=True, exist_ok=True)
assert not (live / 'launch.json').exists(), 'Refusing to overwrite session'
exe = r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
args = [exe, str(root / 'unreal/REAL33D/REAL33D.uproject'), '-game', '-windowed',
        '-ResX=1280', '-ResY=800', '-nosound', '-nosplash', '-ExecCmds=t.MaxFPS 20',
        r'-real33d-runtime=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0',
        '-real33d-account=B', '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
        '-real33d-evidence=' + str(live), '-abslog=' + str(live / 'REAL33D.log')]
proc = subprocess.Popen(args)
meta = dict(name=a.name, pid=proc.pid, args=args, mode=a.mode,
            implementation='c7b3c3dc2b31d5e476e4a5039e0959d99a9ed4b8',
            launched_utc=datetime.now(timezone.utc).isoformat(),
            dll_sha256=hashlib.sha256((root / 'unreal/REAL33D/Binaries/Win64/UnrealEditor-REAL33D.dll').read_bytes()).hexdigest())
def save():
    (live / 'launch.json').write_text(json.dumps(meta, indent=2) + '\n')
def logfile(directory=live):
    path = directory / 'REAL33D.log'
    return path.read_text(encoding='utf-8-sig', errors='replace') if path.exists() else ''
def wait_for(predicate, seconds=240):
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        assert proc.poll() is None, 'Unreal exited during probe'
        if predicate():
            return
        time.sleep(.25)
    raise RuntimeError('Timed out; session retained for inspection')
def capture(directory, pid, label):
    subprocess.run([sys.executable, str(Path(__file__).with_name('native_ui_driver.py')),
                    '--pid', str(pid), '--activate', '--snapshot',
                    '--capture', str(directory / (label + '.png')),
                    '--record', str(directory / (label + '-input.json'))], check=True, stdout=subprocess.DEVNULL)
    data = json.loads((directory / 'unreal_slice_evidence_Manual.json').read_text())
    (directory / (label + '.json')).write_text(json.dumps(data, indent=2) + '\n')
    return data

save()
print('Launched', a.name, proc.pid, flush=True)
if a.mode == 'budget':
    wait_for(lambda: logfile().count('session ended: could not reach the login service') == 4)
    time.sleep(12)
    data = capture(live, proc.pid, 'budget-exhausted')
    assert logfile().count('reconnecting, attempt') == 3
    assert 'connected: Test Player B' not in logfile()
    meta['budget_checked_utc'] = datetime.now(timezone.utc).isoformat()
    save()
    print('Budget exhausted: exactly three retries, no connection', flush=True)
    sys.exit(0)
if a.mode == 'recovery':
    wait_for(lambda: 'session ended: could not reach the login service' in logfile())
    meta['failure_observed_utc'] = datetime.now(timezone.utc).isoformat()
    time.sleep(1)
    helper = '/mnt/c/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022/evidence/certification/login-combined-20261022/login_service.py'
    result = subprocess.run(['wsl.exe', '-d', 'Ubuntu-26.04', '--', 'python3', helper, 'start'],
                            capture_output=True, text=True, check=True)
    meta['service_recovery'] = json.loads(result.stdout)
    save()
    print('Login service restored', result.stdout.strip(), flush=True)

wait_for(lambda: 'creature 1002 "Test Player B" appeared' in logfile())
meta['world_seen_utc'] = datetime.now(timezone.utc).isoformat()
save()
print('World observed', a.name, meta['world_seen_utc'], flush=True)
for label, key, delta in [('01-south', '0x28', (0, 1, 0)), ('02-north', '0x26', (0, -1, 0))]:
    subprocess.run([sys.executable, str(Path(__file__).with_name('run_probe.py')),
                    '--pid', str(proc.pid), '--live', str(live), '--label', label, '--key', key], check=True)
    result = json.loads((live / (label + '-result.json')).read_text())
    journal = result['journal']
    assert len(journal) == 2 and [v['phase'] for v in journal] == ['sent', 'accepted'], result
    assert journal[0]['input_id'] == journal[1]['input_id']
    assert result['requested_after'] == result['requested_before'] + 1
    assert tuple(result['after'][v] - result['before'][v] for v in ('x', 'y', 'z')) == delta
    snapshot = json.loads((live / (label + '-after.json')).read_text())
    assert snapshot['connected_at_close'] and snapshot['local_creature_id'] == 1002

meta['hold_started_utc'] = datetime.now(timezone.utc).isoformat()
save()
print('Holding ownership for 36 seconds', a.name, flush=True)
time.sleep(36)
new = capture(live, proc.pid, 'stable-owner')
assert new['connected_at_close'] and new['local_creature_id'] == 1002
assert new['clientcore']['worldstate_tiles'] > 0
assert logfile().count('connected: Test Player B') == 1
assert logfile().count('local player is creature 1002') == 1
assert 'session ended:' not in logfile()[logfile().index('connected: Test Player B'):]
assert logfile().count('reconnecting, attempt') == (1 if a.mode == 'recovery' else 0)
if a.previous:
    previous = live_root / a.previous
    old_meta = json.loads((previous / 'launch.json').read_text())
    old = capture(previous, old_meta['pid'], 'displaced-after-hold')
    assert not old['connected_at_close']
    assert 'Fusion32 closed this connection; not reconnecting automatically' in logfile(previous)
    assert logfile(previous).count('reconnecting, attempt') == 0
    assert logfile(previous).count('connected: Test Player B') == 1
meta['hold_finished_utc'] = datetime.now(timezone.utc).isoformat()
save()
print('PASS stable owner and displaced-client checks', a.name, flush=True)
