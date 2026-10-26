from pathlib import Path
import hashlib, json, re, shutil, subprocess

root = Path(__file__).resolve().parents[2]
work = root/'build/certification'
out = root/'evidence/certification/login-ui-independent-20261025'
out.mkdir(parents=True, exist_ok=True)
runtime = Path(r'\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0')
env = dict(l.split('=',1) for l in (runtime/'secrets/credentials.env').read_text().splitlines() if '=' in l)
secrets = {k:v.strip() for k,v in env.items() if k in ('ACCOUNT_A_ID','ACCOUNT_B_ID','ACCOUNT_A_PASSWORD','ACCOUNT_B_PASSWORD') and v.strip()}
manifest=[]
hits=[]
for source in sorted((work/'live').rglob('*')):
    if source.is_file() and source.suffix in ('.log','.json','.jsonl','.txt'):
        text=source.read_text(errors='replace')
        for key,value in secrets.items():
            pattern=r'(?<!\d)'+re.escape(value)+r'(?!\d)' if key.endswith('_ID') else re.escape(value)
            n=len(re.findall(pattern,text))
            if n: hits.append(dict(file=str(source.relative_to(work)),kind=key,occurrences=n))
(out/'credential-scan.json').write_text(json.dumps(dict(password_occurrences=sum(h['occurrences'] for h in hits if h['kind'].endswith('_PASSWORD')),account_id_occurrences=sum(h['occurrences'] for h in hits if h['kind'].endswith('_ID')),hits=hits),indent=2)+'\n')

def copy(source, dest):
    source=work/source
    target=out/dest
    target.parent.mkdir(parents=True,exist_ok=True)
    data=source.read_bytes()
    raw_hash=hashlib.sha256(data).hexdigest()
    redacted=False
    if source.suffix != '.png':
        text=data.decode('utf-8-sig',errors='replace')
        for key,value in secrets.items():
            pattern=r'(?<!\d)'+re.escape(value)+r'(?!\d)' if key.endswith('_ID') else re.escape(value)
            text,n=re.subn(pattern,'[REDACTED_'+key+']',text)
            redacted=redacted or n>0
        data=text.encode('utf-8')
    target.write_bytes(data)
    manifest.append(dict(file=str(target.relative_to(out)),sha256=hashlib.sha256(data).hexdigest(),original_sha256=raw_hash,redacted=redacted))

for run in ('cycles-desktop','takeover-new','A-art'):
    for name in ('launch.json','REAL33D.log','movement_journal.jsonl','login_qa.jsonl','unreal_slice_evidence_Disconnected.json','unreal_slice_evidence_AfterCleanup.json'):
        if (work/'live'/run/name).exists(): copy(f'live/{run}/{name}',f'live/{run}/{name}')
for stage in ('death','relogin'):
    for p in sorted((work/'live/A-art'/stage).glob('*')):
        if p.suffix in ('.json','.jsonl','.png'):
            copy(str(p.relative_to(work)),str(p.relative_to(work)))
for stage in ('initial','final'):
    for p in sorted((work/stage).rglob('*')):
        if p.is_file() and p.suffix in ('.md','.txt'):
            copy(str(p.relative_to(work)),f'tests/{p.relative_to(work)}')
for name in ('build-recovery.log','automation-result.txt','automation/automation-report/index.json','automation/automation.log'):
    copy(name,f'tests/{name}')
for name in ('launch.ps1','launch-art.ps1','automation.ps1','run-final.cmd','scan_credentials.py'):
    copy(name,f'reproduce/{name}')
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'Packaged {len(manifest)} files; account identifiers redacted; original hashes retained.')
