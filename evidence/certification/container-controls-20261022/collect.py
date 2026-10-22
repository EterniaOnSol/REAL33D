"""Preserve original evidence bytes plus a manifest of all local raw artifacts."""
import hashlib,json,shutil,subprocess
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
raw=root/'build/certification';live=raw/'live'
manifest=[]
for f in sorted(raw.rglob('*')):
 if f.is_file() and f.suffix in ['.json','.jsonl','.png','.log','.txt','.md','.cmd']:
  manifest.append(dict(path=f.relative_to(raw).as_posix(),bytes=f.stat().st_size,sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
(here/'raw-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def copy(src,dst):
 dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
for f in live.rglob('*'):
 if f.is_file() and f.suffix in ['.json','.jsonl','.log']:
  copy(f,here/'live'/f.relative_to(live))
shots={
 'B-round1':['02-close-held','03-close-response','04-reopen','09-ctrl-amount','10-cancel'],
 'B':['00-resumed','03-third','04-close-middle','05-up-id2','06-close-id2','07-minimize','08-expand','09-channels','10-channel-join','11-ctrl-amount','12-amount-one','13-amount-confirm','14-stack-restore','21-return-both','23-restored','24-order-restore'],
 'A':['00-return-invited','03-return-both','04-reject-wrong-slot','07-rope-offer','09-restored']}
for session,labels in shots.items():
 for label in labels: copy(live/session/(label+'.png'),here/'live'/session/(label+'.png'))
for name in ['harness','harness-final']:
 for f in (raw/name).rglob('*'):
  if f.is_file(): copy(f,here/'validation'/name/f.relative_to(raw/name))
for name in ['unreal-rebuild.txt','unreal-corrupt-pch.txt','server-disassembly.txt','rebuild.cmd','run-harness-final.cmd','automation-console.txt']:
 copy(raw/name,here/'validation'/name)
copy(raw/'automation/automation-report/index.json',here/'validation/automation-report.json')
for name in ['unreal-rebuild-sandbox-failed.txt','server-observer-console.txt','server-observer-console-resumed.txt','server-observer-console-signals.txt']:
 lines=(raw/name).read_text(errors='replace').splitlines()
 (here/'validation'/(name+'.excerpt')).write_text('\n'.join(lines[:35]+['[... middle omitted; full SHA256 in raw-manifest.json ...]']+lines[-35:])+'\n')
git=lambda *args:subprocess.check_output(['git',*args],cwd=root,text=True).strip()
impl='2136e6f63dd12f73c2575a87d7b44af22aa0c66a';main='af251c9b2de0372c97ecbd128af0ada0c92bc166'
assert git('show','-s','--format=%P',impl)==main
assert git('rev-list','--count',main+'..'+impl)=='1'
remote=git('ls-remote','origin','refs/heads/main','refs/heads/milestone/unreal-container-controls-001')
assert main+'\trefs/heads/main' in remote and impl+'\trefs/heads/milestone/' in remote
dll=root/'unreal/REAL33D/Binaries/Win64/UnrealEditor-REAL33D.dll'
provenance=dict(main=main,implementation=impl,branch=git('branch','--show-current'),remote_refs=remote,
 linear_parent_verified=True,no_main_merge=True,production_diff=git('diff',impl,'--','clientcore','tests','unreal/REAL33D/Source','scripts'),
 live_dll_sha256=json.loads((live/'B/launch.json').read_text())['dll_sha256'],final_harness_dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest())
assert not provenance['production_diff']
(here/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print('Preserved',len(manifest),'raw artifact hashes; production diff empty; remote heads verified')
