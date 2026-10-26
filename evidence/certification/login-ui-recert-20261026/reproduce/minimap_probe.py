"""Keep salt private; publish comparisons and account-value absence only."""
import json,sys,hashlib
from pathlib import Path
from security_audit import ROOT, OUT, needles, patterns
cache=ROOT/'unreal/REAL33D/Saved/Minimap'
salt=cache/'scope.salt'
maps=sorted(cache.glob('*.r33map'))
assert salt.is_file() and maps
state={'salt_hash':hashlib.sha256(salt.read_bytes()).hexdigest(),'files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in maps}}
private=OUT/'minimap-private.json'
if sys.argv[1]=='before':
    private.write_text(json.dumps(state),encoding='utf-8')
    print('Minimap baseline recorded privately')
else:
    baseline=json.loads(private.read_text())
    text='\n'.join(p.name+'\n'+p.read_text() for p in maps)
    result={'salt_persisted':state['salt_hash']==baseline['salt_hash'],'same_cache_names':set(state['files'])==set(baseline['files']),'cache_count':len(maps),'raw_account_occurrences':sum(len(patterns[k].findall(text)) for k in patterns if k.endswith('_ID')),'raw_password_occurrences':sum(len(patterns[k].findall(text)) for k in patterns if k.endswith('_PASSWORD')),'salt_not_published':True}
    (OUT/'minimap-restart.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    assert result['salt_persisted'] and result['same_cache_names'] and result['raw_account_occurrences']==result['raw_password_occurrences']==0
