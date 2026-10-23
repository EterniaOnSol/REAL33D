"""Launch the unchanged build on a normal QA account, recording provenance."""
import argparse, hashlib, json, subprocess
from datetime import datetime, timezone
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('account', choices=['A','B'])
p.add_argument('--run', default=None)
a=p.parse_args()
root=Path(__file__).resolve().parents[3]
out=root/'build/certification/live'/(a.run or a.account)
out.mkdir(parents=True, exist_ok=True)
assert not (out/'launch.json').exists()
args=[r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',str(root/'unreal/REAL33D/REAL33D.uproject'),
 '-game','-windowed','-ResX=1280','-ResY=900','-nosound','-nosplash','-ExecCmds=t.MaxFPS 20',
 r'-real33d-runtime=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0',
 '-real33d-account='+a.account,'-real33d-host=127.0.0.1','-real33d-loginport=7171',
 '-real33d-evidence='+str(out),'-abslog='+str(out/'REAL33D.log')]
proc=subprocess.Popen(args)
data=dict(pid=proc.pid,account=a.account,args=args,utc=datetime.now(timezone.utc).isoformat(),
 implementation=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
 dll_sha256=hashlib.sha256((root/'unreal/REAL33D/Binaries/Win64/UnrealEditor-REAL33D.dll').read_bytes()).hexdigest())
(out/'launch.json').write_text(json.dumps(data,indent=2)+'\n')
print(json.dumps(data))
