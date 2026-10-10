import json, struct, pathlib, hashlib
root=pathlib.Path('C:/Users/dell/3DTIBIA_leo/worklog/ORQUESTADOR/generaciones_assets/refinamiento_en_curso_v8/samples/models')
rows=[]
for item in [373,377]:
    data=(root/f'{item:05}.glb').read_bytes(); chunks={}; at=12
    while at<len(data):
        size,kind=struct.unpack_from('<II',data,at); chunks[kind]=data[at+8:at+8+size]; at+=8+size
    doc=json.loads(chunks[0x4e4f534a]); binary=chunks[0x004e4942]
    def read(index):
        a=doc['accessors'][index]; v=doc['bufferViews'][a['bufferView']]
        fmt={5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']]
        n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
        size=struct.calcsize('<'+fmt*n); start=v.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from('<'+fmt*n,binary,start+i*v.get('byteStride',size)) for i in range(a['count'])]
    row={'item_id':item,'source_sha256':hashlib.sha256(data).hexdigest(),'triangles':0,'constant_uv_triangles':0,'nodes':doc.get('nodes',[])}
    positions=[]
    for mesh in doc['meshes']:
        for p in mesh['primitives']:
            pos=read(p['attributes']['POSITION']); positions+=pos
            uv=read(p['attributes']['TEXCOORD_0']); ids=[x[0] for x in read(p['indices'])]
            for i in range(0,len(ids),3):
                a,b,c=[uv[k] for k in ids[i:i+3]]; row['triangles']+=1
                if a==b==c: row['constant_uv_triangles']+=1
    row['source_bounds_metres']={'min':[min(p[i] for p in positions) for i in range(3)],'max':[max(p[i] for p in positions) for i in range(3)]}
    row['runtime_height_uu']=218
    row['runtime_vertical_scale']=218/(100*(row['source_bounds_metres']['max'][1]-row['source_bounds_metres']['min'][1]))
    rows.append(row)
print(json.dumps(rows,indent=2))