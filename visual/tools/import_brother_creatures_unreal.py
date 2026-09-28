"""Import the unchanged, pinned 3DTIBIA creature delivery into a separate UE folder."""
import hashlib
import json
from pathlib import Path
import shlex
import unreal


def argument(name):
    for part in shlex.split(unreal.SystemLibrary.get_command_line(), posix=False):
        if part.startswith(name + "="):
            return part.split("=", 1)[1].strip('"')
    raise RuntimeError("missing " + name)


root = Path(unreal.Paths.project_dir()).resolve().parents[1]
source = Path(argument("-brother-creatures-source"))
spec_path = root / "visual/qa/brother_creatures/source.json"
spec = json.loads(spec_path.read_text(encoding="utf-8-sig"))
entries = []
for row in spec["entries"]:
    record = dict(row)
    record["status"] = "FAILED"
    try:
        file = source / row["delivery_file"]
        if hashlib.sha256(file.read_bytes()).hexdigest() != row["sha256"]:
            raise RuntimeError("source hash mismatch")
        folder = f'/Game/Experimental/BrotherCreatures/Outfit_{row["outfit_id"]:03d}'
        paths = unreal.EditorAssetLibrary.list_assets(folder, recursive=True)
        if not paths:
            task = unreal.AssetImportTask()
            task.filename = str(file)
            task.destination_path = folder
            task.automated = True
            task.async_ = False
            task.replace_existing = False
            task.save = True
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            paths = unreal.EditorAssetLibrary.list_assets(folder, recursive=True)
        assets = [(p, unreal.EditorAssetLibrary.load_asset(p)) for p in paths]
        meshes = [(p, a) for p, a in assets if isinstance(a, unreal.SkeletalMesh)]
        if not meshes:
            raise RuntimeError("no skeletal mesh parts imported")
        # The delivered GLBs contain separate skinned body parts. Preserve all
        # of them; never select one body part as if it were a complete creature.
        skeletons = {a.get_editor_property("skeleton").get_path_name() for p, a in meshes}
        if len(skeletons) != 1:
            raise RuntimeError("body parts do not share one skeleton")
        animations = {a.get_name().lower(): p for p, a in assets
                      if isinstance(a, unreal.AnimSequence)}
        clips = {}
        for name in ("idle", "caminar", "atacar", "morir"):
            matches = [p for n, p in animations.items() if name in n]
            if len(matches) != 1:
                raise RuntimeError(f"expected one {name} sequence, found {len(matches)}")
            clips[name] = matches[0]
            sequence = unreal.EditorAssetLibrary.load_asset(matches[0])
            if sequence.get_editor_property("skeleton").get_path_name() not in skeletons:
                raise RuntimeError(f"{name} skeleton differs from mesh skeleton")
        record.update(status="PASS", mesh_paths=sorted(p for p, a in meshes), clips=clips,
                      skeleton_path=next(iter(skeletons)),
                      imported_asset_count=len(paths))
    except Exception as error:
        record["error"] = str(error)
        unreal.log_error(f'Brother outfit {row["outfit_id"]}: {error}')
    entries.append(record)

out = {"schema": "real33d.brother-creatures.runtime.v1", "source": spec["source"],
       "entries": entries, "import_pass": sum(e["status"] == "PASS" for e in entries)}
(spec_path.parent / "runtime.json").write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
unreal.log(f'Brother creature import PASS {out["import_pass"]}/{len(entries)}')
if out["import_pass"] != len(entries):
    raise RuntimeError("creature import incomplete; see runtime.json")
