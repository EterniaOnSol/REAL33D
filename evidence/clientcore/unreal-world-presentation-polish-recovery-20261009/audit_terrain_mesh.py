"""Read-only source GLB audit: planar surface winding/coverage, no asset writes."""
import json, pathlib, struct, hashlib

base = pathlib.Path('C:/Users/dell/3DTIBIA_leo/worklog/ORQUESTADOR/generaciones_assets/piloto_completo_4913/models')
results = []
for item in [622, 4785, 4789, 4793, 4794, 4795, 4796]:
    path = base / f'{item:05}.glb'
    data = path.read_bytes()
    at, chunks = 12, {}
    while at < len(data):
        size, kind = struct.unpack_from('<II', data, at)
        chunks[kind] = data[at+8:at+8+size]
        at += 8+size
    doc = json.loads(chunks[0x4e4f534a])
    binary = chunks[0x004e4942]
    def read(index):
        a = doc['accessors'][index]; v = doc['bufferViews'][a['bufferView']]
        fmt = {5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']]
        n = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
        size = struct.calcsize('<'+fmt*n)
        start = v.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from('<'+fmt*n,binary,start+i*v.get('byteStride',size)) for i in range(a['count'])]
    row = dict(item=item,sha256=hashlib.sha256(data).hexdigest(),top_up_area=0,top_down_area=0,top_triangles=0,other_triangles=0)
    for mesh in doc['meshes']:
        for prim in mesh['primitives']:
            pos = read(prim['attributes']['POSITION']); ids = [x[0] for x in read(prim['indices'])]
            top = max(p[1] for p in pos)
            for i in range(0,len(ids),3):
                a,b,c = [pos[k] for k in ids[i:i+3]]
                if all(abs(p[1]-top)<1e-6 for p in [a,b,c]):
                    signed = ((b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]))/2
                    row['top_up_area' if signed>0 else 'top_down_area'] += abs(signed)
                    row['top_triangles'] += 1
                else: row['other_triangles'] += 1
    row['nodes'] = doc.get('nodes', [])
    row['materials'] = [{k:v for k,v in m.items() if k not in ['pbrMetallicRoughness']} for m in doc.get('materials',[])]
    row['position_min'] = [min(p[i] for p in pos) for i in range(3)]
    row['position_max'] = [max(p[i] for p in pos) for i in range(3)]
    results.append(row)
print(json.dumps(results,indent=2))
