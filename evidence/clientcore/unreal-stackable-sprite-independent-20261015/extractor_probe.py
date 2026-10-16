"""Independent certification probe. No client data or generated sprites published."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from PIL import Image

repo = next(p for p in Path(__file__).resolve().parents if (p/'AGENTS.md').is_file())
things = Path(sys.argv[1])
out = repo / 'unreal/REAL33D/Resources/UI/Items'
stdout = 'Inspecting the fresh extraction already generated in this isolated worktree.\n'
if not out.exists():
    run = subprocess.run([sys.executable, str(repo/'scripts/client/extract_item_sprites.py'),
                          str(things), str(out)], capture_output=True, text=True, check=True)
    stdout = run.stdout
print(stdout)
spec = importlib.util.spec_from_file_location('independent_reader', repo/'visual/tools/tibia772.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
dat = reader.AppearanceFile.read(things/'Tibia.dat')
spr = reader.SpriteFile.read(things/'Tibia.spr')
items = dat.by_kind['item']
counts = {'cumulative_types': 0, 'cumulative_frames_checked': 0,
          'non_cumulative_single_output': 0, 'non_cumulative_multi_source': 0}
details = []
for tid, app in items.items():
    paths = list(out.glob(str(tid)+'_*.png'))
    base = out / (str(tid)+'.png')
    cumulative = 5 in app.options
    if not cumulative:
        assert not paths, (tid, 'non-cumulative emitted extra frames')
        if base.exists():
            counts['non_cumulative_single_output'] += 1
            if len(app.sprite_ids) > app.width*app.height:
                counts['non_cumulative_multi_source'] += 1
        continue
    assert (app.width, app.height, app.layers, app.pattern_z) == (1,1,1,1), app
    expected_frames = app.pattern_x * app.pattern_y * app.frames
    assert expected_frames == len(app.sprite_ids)
    for frame, sid in enumerate(app.sprite_ids):
        expected = spr.decode(sid)
        path = base if frame == 0 else out / f'{tid}_{frame}.png'
        assert path.is_file(), (tid, frame, 'frame missing')
        with Image.open(path) as png:
            assert png.size == (32,32)
            actual = list(png.convert('RGBA').get_flattened_data())
        assert actual == expected, (tid, frame, 'decoded pixels differ')
        counts['cumulative_frames_checked'] += 1
    assert len(paths)+1 == expected_frames, (tid, 'unexpected output frames')
    counts['cumulative_types'] += 1
    details.append({'type_id':tid, 'frames':expected_frames,
                    'amount_patterns':app.pattern_x * app.pattern_y,
                    'animation_phases':app.frames, 'sprite_ids':app.sprite_ids})
for tid in (3031,3035,3043,3277):
    print(next(x for x in details if x['type_id']==tid))
result = {'candidate':'68f2ffe488627d5da52981bd83a2e95a5846ecc3',
          'result':'PASS', 'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()
              for p in (things/'Tibia.dat',things/'Tibia.spr')},
          'counts':counts, 'cumulative_types':details, 'extractor_stdout':stdout}
(repo/'build/certification/extractor-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(counts))
