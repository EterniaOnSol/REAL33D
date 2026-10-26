from pathlib import Path
import json, re, sys

root = Path(__file__).resolve().parents[2]
runtime = Path(r'\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0')
values = dict(line.split('=', 1) for line in (runtime/'secrets/credentials.env').read_text().splitlines() if '=' in line)
needles = {k: v.strip() for k, v in values.items() if k in ('ACCOUNT_A_ID', 'ACCOUNT_B_ID', 'ACCOUNT_A_PASSWORD', 'ACCOUNT_B_PASSWORD') and v.strip()}
target = Path(sys.argv[1]) if len(sys.argv) > 1 else root/'build/certification/live'
hits = []
files = 0
for p in target.rglob('*'):
    if not p.is_file() or p.suffix.lower() not in ('.log', '.txt', '.json', '.jsonl', '.md'):
        continue
    files += 1
    text = p.read_text(errors='replace')
    for key, value in needles.items():
        count = len(re.findall(r'(?<!\d)' + re.escape(value) + r'(?!\d)', text)) if key.endswith('_ID') else text.count(value)
        if count:
            hits.append(dict(file=str(p.relative_to(target)), credential_kind=key, occurrences=count))
result = dict(files=files, password_occurrences=sum(h['occurrences'] for h in hits if h['credential_kind'].endswith('_PASSWORD')), account_id_occurrences=sum(h['occurrences'] for h in hits if h['credential_kind'].endswith('_ID')), hits=hits)
print(json.dumps(result, indent=2))
