"""Observe an established real client after normal server shutdown; real F9 only."""
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[3]
live = root / 'build/certification/live/Recovery'
meta = json.loads((live / 'launch.json').read_text())
deadline = time.monotonic() + 60
needle = 'Fusion32 closed this connection; not reconnecting automatically'
while time.monotonic() < deadline:
    log = (live / 'REAL33D.log').read_text(encoding='utf-8-sig', errors='replace')
    if needle in log:
        break
    time.sleep(.2)
else:
    raise RuntimeError('No RemoteClosed classification after server shutdown')
started = datetime.now(timezone.utc).isoformat()
print('RemoteClosed observed; holding client open for 36 more seconds', flush=True)
time.sleep(36)
subprocess.run([sys.executable, str(Path(__file__).with_name('native_ui_driver.py')),
                '--pid', str(meta['pid']), '--activate', '--snapshot',
                '--capture', str(live / 'remote-closed-final.png'),
                '--record', str(live / 'remote-closed-final-input.json')],
               check=True, stdout=subprocess.DEVNULL)
data = json.loads((live / 'unreal_slice_evidence_Manual.json').read_text())
(live / 'remote-closed-final.json').write_text(json.dumps(data, indent=2) + '\n')
assert not data['connected_at_close']
log = (live / 'REAL33D.log').read_text(encoding='utf-8-sig', errors='replace')
assert log.count('reconnecting, attempt') == 1, 'New reconnect after RemoteClosed'
assert log.count('connected: Test Player B') == 1
result = dict(started_utc=started, finished_utc=datetime.now(timezone.utc).isoformat(),
              new_reconnects_after_remote_closed=0)
(live / 'remote-close-result.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: no new reconnect after established-session RemoteClosed', flush=True)
