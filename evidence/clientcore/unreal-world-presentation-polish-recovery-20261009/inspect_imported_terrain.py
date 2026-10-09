"""Read existing Unreal mesh settings; never save or modify a package."""
import json
from pathlib import Path
import unreal

rows = []
for item in [622, 4785, 4789, 4793, 4794, 4795, 4796]:
    mesh = unreal.load_asset(f'/Game/Experimental/V08/ID_{item:05}/SM_V08_{item:05}')
    bounds = mesh.get_bounding_box()
    rows.append(dict(item=item, nanite_triangles=mesh.get_num_nanite_triangles(),
        nanite_settings=str(mesh.get_editor_property('nanite_settings')),
        bounds_min=[bounds.min.x,bounds.min.y,bounds.min.z],
        bounds_max=[bounds.max.x,bounds.max.y,bounds.max.z],
        lod_count=mesh.get_num_lods(),
        material=str(mesh.get_material(0))))
Path(__file__).with_name('imported-terrain-settings.json').write_text(json.dumps(rows,indent=2))
print('READ_ONLY_TERRAIN_AUDIT_COMPLETE')
