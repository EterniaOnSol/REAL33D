import importlib.util, json, pathlib, struct
root = pathlib.Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('extractor', root/'scripts/client/extract_item_sprites.py')
e = importlib.util.module_from_spec(spec); spec.loader.exec_module(e)
things = pathlib.Path('C:/Users/dell/Desktop/fusion32/build/classic-client-772/app')
items = e.parse_dat(str(things/'Tibia.dat'))
data, offsets = e.parse_spr(str(things/'Tibia.spr'))
rows=[]
for item in range(4785,4797):
 w,h,ids=items[item]; width,height,rgba=e.compose(w,h,ids,data,offsets)
 pixels=[(x,y) for y in range(height) for x in range(width) if rgba[(y*width+x)*4+3]>127]
 lo=[min(p[i] for p in pixels) for i in (0,1)]
 hi=[max(p[i] for p in pixels)+1 for i in (0,1)]
 center=[((lo[i]+hi[i])/2/32-.5)*100 for i in (0,1)]
 rows.append(dict(type_id=item,pixel_min=lo,pixel_max_exclusive=hi,offset_uu=center))
print(json.dumps(rows,indent=2))
