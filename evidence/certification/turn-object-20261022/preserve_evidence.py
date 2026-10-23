"""Copy compact review evidence; never replace or delete the original captures."""
import hashlib
import json
import shutil
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
LIVE = ROOT / 'build/certification/live'
SELECTED = {
    '011-statue-held-rotate', '012-statue-held-inspection',
    '014-statue-updated-menu', '023-ground-refusal', '027-drag-rope',
    '029-ctrl-stack-dialog', '037-upper-chest-rotate',
    '043-chest-turn-success', '044-chest-second-menu',
    '051-chest-restored', '052-backpack-use', '053-usewith-rope-arm',
    '054-usewith-ctrl-world-target', '056-door-restored',
}


def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        assert destination.read_bytes() == source.read_bytes(), destination
    else:
        shutil.copy2(source, destination)


manifest = []
for source in sorted(LIVE.rglob('*')):
    if not source.is_file():
        continue
    relative = source.relative_to(LIVE)
    retain = source.suffix in ('.json', '.jsonl', '.log') or source.stem in SELECTED
    manifest.append(dict(path=str(relative).replace('\\', '/'),
                         bytes=source.stat().st_size,
                         sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                         published=retain))
    if retain:
        copy(source, HERE / 'live' / relative)
(HERE / 'raw-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
for name in ('unreal-rebuild.txt', 'unreal-rebuild-attempt2.txt',
             'server-observer-console.txt', 'server-observer-console-2.txt',
             'server-observer-console-3.txt', 'final-harness-resume.cmd',
             'turn-disassembly.txt', 'update-disassembly.txt'):
    source = ROOT / 'build/certification' / name
    if source.exists():
        copy(source, HERE / 'validation' / name)
copy(ROOT / 'build/certification/automation/automation-report/index.json',
     HERE / 'validation/automation-report.json')
copy(ROOT / 'build/certification/automation/automation.log',
     HERE / 'validation/automation.log')
print('Preserved', len(manifest), 'original artifacts;',
      sum(m['published'] for m in manifest), 'published copies')
