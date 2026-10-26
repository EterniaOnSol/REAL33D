"""Independent certification probes. Never emit actual credential values."""
import importlib.util
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
RUNTIME=Path(r'\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0')
SCANNER=ROOT/'tests/scan_credential_values.py'
spec=importlib.util.spec_from_file_location('scanner',SCANNER)
scanner=importlib.util.module_from_spec(spec)
spec.loader.exec_module(scanner)
needles=scanner.read_needles(RUNTIME)
assert len(needles)==4
patterns={key:scanner.pattern_for(key,value) for key,value in needles.items()}

def run_probe():
    results=[]
    # TemporaryDirectory is under this worktree's ignored build tree. Values are
    # deleted by context cleanup; only subprocess exit codes/counts are retained.
    with tempfile.TemporaryDirectory(prefix='negative-',dir=OUT) as temp:
        target=Path(temp)/'planted.log'
        for key,value in needles.items():
            target.write_text('credential '+value+' end\n',encoding='utf-8')
            proc=subprocess.run([sys.executable,str(SCANNER),'--runtime',str(RUNTIME),str(target)],capture_output=True,text=True)
            report=json.JSONDecoder().raw_decode(proc.stdout)[0]
            results.append({'kind':key,'exit_code':proc.returncode,'account_occurrences':report['account_id_occurrences'],'password_occurrences':report['password_occurrences']})
            assert proc.returncode==1 and any(h['credential_kind']==key for h in report['hits'])
        target.write_text('clean control\n',encoding='utf-8')
        proc=subprocess.run([sys.executable,str(SCANNER),'--runtime',str(RUNTIME),str(target)],capture_output=True,text=True)
        assert proc.returncode==0
        results.append({'kind':'clean_control','exit_code':proc.returncode})
    (OUT/'scanner-negative.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
    print('SCANNER NEGATIVE: PASS 4/4; clean control PASS; planted files removed')

def scan(paths):
    hits=[]
    count=0
    for path in sorted(set(paths)):
        if not path.is_file(): continue
        data=path.read_bytes()
        if data.startswith((b'\xff\xfe',b'\xfe\xff')):
            body=data.decode('utf-16',errors='replace')
        elif b'\0' in data[:8192]:
            continue
        else:
            body=data.decode('utf-8-sig',errors='replace')
        # Exclude raster formats from textual claims; they are visually reviewed.
        if path.suffix.lower() in {'.png','.jpg','.jpeg','.gif','.webp','.bmp'}: continue
        count+=1
        for key,pattern in patterns.items():
            n=len(pattern.findall(body))
            if n: hits.append({'file':str(path.relative_to(ROOT)),'kind':key,'occurrences':n})
    return {'files_scanned':count,'account_occurrences':sum(h['occurrences'] for h in hits if h['kind'].endswith('_ID')),'password_occurrences':sum(h['occurrences'] for h in hits if h['kind'].endswith('_PASSWORD')),'hits':hits}

if __name__=='__main__':
    run_probe()
    tracked=subprocess.check_output(['git','ls-files','-z','--','evidence'],cwd=ROOT).decode().split('\0')
    result={'committed_evidence':scan([ROOT/p for p in tracked if p]),'generated_evidence':scan(OUT.rglob('*')),'runtime_logs':scan((OUT/'live').rglob('*.log'))}
    (OUT/'credential-audit.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
    sys.exit(1 if any(r['hits'] for r in result.values()) else 0)
