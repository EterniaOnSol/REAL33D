"""Read-only search for existing rotatable objects near QA player B."""
import re,sys,json
from pathlib import Path
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'scripts/client'))
from build_wide_world_cache import parse_sector
from audit_wide_world_sources import load_disguises
runtime=Path(r'\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0/game')
objects=runtime/'reference/dat/objects.srv'
raw=objects.read_text(encoding='latin-1');types={}
for block in re.split(r'(?=\bTypeID\s*=)',raw)[1:]:
 ident=int(re.search(r'TypeID\s*=\s*(\d+)',block)[1])
 flags=re.search(r'Flags\s*=\s*\{([^}]*)}',block)
 name=re.search(r'Name\s*=\s*"([^"]*)"',block)
 target=re.search(r'RotateTarget\s*=\s*(\d+)',block)
 if flags and re.search(r'\bRotate\b',flags[1]):
  types[ident]=dict(name=name[1] if name else '',target=int(target[1]) if target else None,flags=flags[1])
disguises=load_disguises(objects);found=[]
for floor in range(0,16):
 for sx in range(999,1008):
  for sy in range(1002,1011):
   name=f'{sx:04}-{sy:04}-{floor:02}.sec';path=runtime/'state/map'/name
   if not path.exists(): path=runtime/'reference/origmap'/name
   if not path.exists(): continue
   for x,y,z,stack,display,ident in parse_sector(path.read_text(encoding='latin-1'),sx,sy,floor,disguises):
    if ident in types:found.append(dict(position=[x,y,z],file_stack=stack,type_id=ident,display=display,**types[ident],distance=max(abs(x-32096),abs(y-32218))+10*abs(z-7)))
found.sort(key=lambda x:x['distance'])
Path(__file__).with_name('object-candidates-allfloors.json').write_text(json.dumps(found,indent=2)+'\n')
print(json.dumps(found[:25],indent=2))
