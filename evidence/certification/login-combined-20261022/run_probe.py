"""Cert-only wrapper: real Win32 input, F9 captures and read-only journal attribution."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

p = argparse.ArgumentParser()
p.add_argument('--pid', required=True)
p.add_argument('--label', required=True)
p.add_argument('--live', default='build/certification/live')
a, gesture = p.parse_known_args()
live = Path(a.live).resolve()
prefix = live / a.label
driver = Path(__file__).with_name('native_ui_driver.py')
base = [sys.executable, str(driver), '--pid', a.pid, '--activate']
snapshot = live / 'unreal_slice_evidence_Manual.json'
journal = live / 'movement_journal.jsonl'

def entries():
    return [json.loads(line) for line in journal.read_text().splitlines() if line] if journal.exists() else []

def run(args):
    subprocess.run(base + args, check=True, stdout=subprocess.DEVNULL)

run(['--snapshot'])
before = json.loads(snapshot.read_text())
shutil.copyfile(snapshot, str(prefix) + '-before.json')
past = entries()
time.sleep(.4)
if entries() != past:
    raise RuntimeError('Movement during quiet attribution window; refusing gesture')
run(gesture + ['--snapshot', '--capture', str(prefix) + '.png', '--record', str(prefix) + '-input.json'])
after = json.loads(snapshot.read_text())
shutil.copyfile(snapshot, str(prefix) + '-after.json')
new = entries()[len(past):]
result = dict(label=a.label, gesture=gesture, before=before['local_position'], after=after['local_position'],
              camera_before=before['camera'], camera_after=after['camera'], journal=new,
              requested_before=before['clientcore']['walks_requested_from_unreal'],
              requested_after=after['clientcore']['walks_requested_from_unreal'])
Path(str(prefix) + '-result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
