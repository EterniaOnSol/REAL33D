"""WSL-only QA service lifecycle; normal login executable, no protocol/game data."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import signal
import subprocess
import time

p = argparse.ArgumentParser()
p.add_argument('action', choices=('stop', 'start'))
a = p.parse_args()
root = Path('/var/lib/fusion32-server-baseline-772-0/login')
pidfile = root / 'run/login.pid'
startfile = root / 'run/login.start'
exe = root / 'bin/login'

def alive(pid):
    stat = Path(f'/proc/{pid}/stat')
    return stat.exists() and stat.read_text().split()[2] != 'Z'

if a.action == 'stop':
    pid = int(pidfile.read_text())
    assert alive(pid)
    assert Path(f'/proc/{pid}/exe').resolve() == exe.resolve()
    assert Path(f'/proc/{pid}/cwd').resolve() == root.resolve()
    assert Path(f'/proc/{pid}/stat').read_text().split()[21] == startfile.read_text().strip()
    os.kill(pid, signal.SIGTERM)
    deadline = time.monotonic() + 15
    while alive(pid) and time.monotonic() < deadline:
        time.sleep(.1)
    assert not alive(pid), 'Login did not stop normally'
    pidfile.unlink()
    startfile.unlink()
else:
    assert not pidfile.exists(), 'Refusing duplicate login service'
    with (root / 'logs/login.log').open('ab') as log:
        proc = subprocess.Popen([str(exe)], cwd=root, stdin=subprocess.DEVNULL,
                                stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    pid = proc.pid
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        assert proc.poll() is None, 'Login exited'
        ports = subprocess.check_output(['ss', '-ltnpH', 'sport = :7171'], text=True)
        if f'pid={pid},' in ports:
            break
        time.sleep(.1)
    else:
        raise RuntimeError('Login listener did not start')
    pidfile.write_text(str(pid) + '\n')
    startfile.write_text(Path(f'/proc/{pid}/stat').read_text().split()[21] + '\n')
print(json.dumps(dict(action=a.action, pid=pid, utc=datetime.now(timezone.utc).isoformat())))
