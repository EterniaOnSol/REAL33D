"""Publish unchanged, scanned evidence only; exclude private minimap baseline."""
import hashlib,json,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
source=Path(__file__).resolve().parent
dest=ROOT/'evidence/certification/login-ui-recert-20261026'
dest.mkdir(parents=True,exist_ok=True)
files=[]
for sub in ('live','harness','final-harness'):
    files.extend(p for p in (source/sub).rglob('*') if p.is_file() and p.suffix.lower() in {'.json','.jsonl','.txt','.log','.md','.png'})
files += [source/'automation-desktop/automation.log',source/'automation-desktop/report/index.json',source/'automation/automation.log']
for name in ('credential-audit.json','scanner-negative.json','minimap-restart.json','verification.json','verify.py','ancestry.json','server-final-status.txt','screenshots.json'):
    files.append(source/name)
manifest=[]
for p in files:
    if not p.is_file():raise RuntimeError('Required evidence missing: '+str(p))
    rel=p.relative_to(source);out=dest/rel;out.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,out)
    manifest.append({'file':str(rel).replace('\\','/'),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size,'redacted':False})
for name in ('launch.ps1','automation.ps1','run-harness.cmd','run-final.cmd','security_audit.py','minimap_probe.py','native_ui_driver.py','walk_route.py','watch_death.py','package.py'):
    out=dest/'reproduce'/name;out.parent.mkdir(exist_ok=True);shutil.copy2(source/name,out)
(dest/'manifest.json').write_text(json.dumps({'tested_head':'444a89e268289cad31fd64260b04762bef73dfdc','files':manifest,'private_exclusions':['Saved/Minimap/scope.salt','minimap-private.json'],'runtime_credentials_published':False},indent=2)+'\n')
print('Packaged',len(manifest),'artifacts without redaction')
