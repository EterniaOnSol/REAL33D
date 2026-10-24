"""Capture the actual death message and cleanup without generating gameplay.

The raw string is only a trigger; acceptance uses full packet replay at command
boundaries. F9 and screenshots are the only UI input from this watcher.
"""
import argparse,json,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[3]
parser=argparse.ArgumentParser()
parser.add_argument('--run', required=True)
args=parser.parse_args()
out=root/'build/certification/live'/args.run
trace=root/'build/certification/live/server-trace.jsonl'
marker=out/'death-watch.json'
assert not marker.exists()
started=time.time()
needle=b'You are dead.\n'.hex()
print('Watching for real death; F9/capture only.',flush=True)
while time.time()-started<1200:
    lines=trace.read_text().splitlines()
    hits=[json.loads(s) for s in lines if needle in s]
    if hits:
        hit=hits[-1];observed=time.time()
        marker.write_text(json.dumps(dict(started=started,observed=observed,server_event=hit),indent=2)+'\n')
        for label,delay in [('death-message',1),('death-cleanup',36),('death-no-reconnect',60)]:
            while time.time()-observed<delay:time.sleep(.25)
            cmd=[sys.executable,str(Path(__file__).with_name('probe.py')),'A',label,'--run',args.run]
            r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            (out/(label+'-watch-output.txt')).write_text(r.stdout)
            print(label,'exit',r.returncode,flush=True)
        break
    time.sleep(.5)
else:
    raise SystemExit('No death within 20-minute observation window')
