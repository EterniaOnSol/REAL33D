"""Compare opaque, non-counter pixels of captured slots against all gold frames.

Read-only: no screenshot is modified. Run from the certification checkout root.
"""
import json
from pathlib import Path
from PIL import Image

root = Path('evidence/clientcore/unreal-stackable-sprite-independent-20261015')
items = Path('unreal/REAL33D/Resources/UI/Items')
cases = [('03-inventory-one.png', (1824, 407), 3031, 0),
         ('04-container-twelve.png', (1561, 231), 3031, 5),
         ('05-inventory-twelve.png', (1824, 407), 3031, 5),
         ('06-worms-six.png', (1561, 231), 3492, 4)]
out = []
for filename, (left, top), type_id, expected in cases:
    shot = Image.open(root / filename).convert('RGB')
    scores = []
    for frame in range(8):
        sprite = Image.open(items / (f'{type_id}.png' if frame == 0 else f'{type_id}_{frame}.png')).convert('RGBA')
        # Omit the counter's bottom-right overlay and transparent blend pixels.
        samples = [(x, y, sprite.getpixel((x, y))[:3]) for y in range(32) for x in range(32)
                   if sprite.getpixel((x, y))[3] == 255 and not (x >= 20 and y >= 20)]
        best = (float('inf'), None)
        for dy in range(-4, 7):
            for dx in range(-4, 7):
                error = sum(sum(abs(a-b) for a, b in zip(rgb, shot.getpixel((left+dx+x, top+dy+y))))
                            for x, y, rgb in samples) / (3 * len(samples))
                if error < best[0]:
                    best = (error, [left+dx, top+dy])
        scores.append({'frame': frame, 'mean_absolute_error': round(best[0], 5), 'origin': best[1], 'opaque_pixels': len(samples)})
    selected = min(scores, key=lambda s: s['mean_absolute_error'])['frame']
    out.append({'capture': filename, 'expected': expected, 'best_match': selected, 'scores': scores})
print(json.dumps(out, indent=2))
assert all(x['best_match'] == x['expected'] for x in out)
