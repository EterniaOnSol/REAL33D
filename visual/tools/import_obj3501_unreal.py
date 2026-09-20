"""Import the approved mailbox GLB into REAL33D's Unreal 5.8 project.

Run with UnrealEditor-Cmd -run=PythonScript. The source GLB is immutable;
Interchange performs the glTF metre/Y-up to Unreal centimetre/Z-up conversion.
"""

import hashlib
import json
import os
import struct
from pathlib import Path

import unreal


SOURCE_SHA256 = "ff172ce12b191920ad3767643eadc439fa56a15bf23c534810e9c8c7105316c6"
SOURCE_RELATIVE = Path(
    "visual/production/obj3501/export/decoration_bottom_overlay_obj3501_mailbox.glb"
)
DESTINATION = "/Game/Visual/obj3501"
MESH_PACKAGE = (
    DESTINATION
    + "/decoration_bottom_overlay_obj3501_mailbox/StaticMeshes/"
    + "decoration_bottom_overlay_obj3501_mailbox"
)


def validate_source_geometry(source):
    data = source.read_bytes()
    if data[:4] != b"glTF" or struct.unpack_from("<I", data, 4)[0] != 2:
        raise RuntimeError("approved mailbox is not a glTF 2.0 binary")
    json_length = struct.unpack_from("<I", data, 12)[0]
    document = json.loads(data[20 : 20 + json_length].decode("utf-8"))
    binary_offset = (20 + json_length + 3) & ~3
    binary_length = struct.unpack_from("<I", data, binary_offset)[0]
    binary = data[binary_offset + 8 : binary_offset + 8 + binary_length]

    def accessor(index):
        item = document["accessors"][index]
        view = document["bufferViews"][item["bufferView"]]
        start = view.get("byteOffset", 0) + item.get("byteOffset", 0)
        components = {"SCALAR": 1, "VEC2": 2, "VEC3": 3}[item["type"]]
        code = {5123: "H", 5125: "I", 5126: "f"}[item["componentType"]]
        size = struct.calcsize("<" + code * components)
        stride = view.get("byteStride", size)
        return [
            struct.unpack_from("<" + code * components, binary, start + row * stride)
            for row in range(item["count"])
        ]

    triangles = 0
    zero_area = 0
    for primitive in document["meshes"][0]["primitives"]:
        positions = accessor(primitive["attributes"]["POSITION"])
        indices = [row[0] for row in accessor(primitive["indices"])]
        triangles += len(indices) // 3
        for offset in range(0, len(indices), 3):
            a, b, c = (positions[indices[offset + n]] for n in range(3))
            ab = tuple(b[n] - a[n] for n in range(3))
            ac = tuple(c[n] - a[n] for n in range(3))
            cross = (
                ab[1] * ac[2] - ab[2] * ac[1],
                ab[2] * ac[0] - ab[0] * ac[2],
                ab[0] * ac[1] - ab[1] * ac[0],
            )
            if cross == (0.0, 0.0, 0.0):
                zero_area += 1
    if (triangles, zero_area) != (1216, 24):
        raise RuntimeError(
            f"approved mailbox source geometry changed: {triangles} triangles, "
            f"{zero_area} zero-area"
        )
    print("REAL33D_MAILBOX_SOURCE_GEOMETRY", "triangles=1216 zero_area=24")


def main():
    repo = Path(unreal.Paths.project_dir()).resolve().parents[1]
    source = repo / SOURCE_RELATIVE
    actual = hashlib.sha256(source.read_bytes()).hexdigest()
    if actual != SOURCE_SHA256:
        raise RuntimeError(f"approved mailbox GLB hash mismatch: {actual}")
    validate_source_geometry(source)

    if unreal.EditorAssetLibrary.does_asset_exist(MESH_PACKAGE):
        paths = list(unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True))
    else:
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = DESTINATION
        task.automated = True
        task.async_ = False
        task.replace_existing = False
        task.save = True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        paths = list(task.imported_object_paths)

    print("REAL33D_MAILBOX_IMPORT_PATHS", paths)
    if len(paths) != 4:
        raise RuntimeError(f"expected exactly four mailbox assets, found {len(paths)}")
    meshes = []
    for path in paths:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset is None:
            raise RuntimeError(f"missing imported asset: {path}")
        print("REAL33D_MAILBOX_IMPORT_ASSET", path, asset.get_class().get_name())
        package = str(path).split(".", 1)[0]
        package_file = repo / "unreal/REAL33D/Content" / (package.removeprefix("/Game/") + ".uasset")
        relative_source = os.path.relpath(source, package_file.parent).replace("\\", "/")
        import_data = asset.get_editor_property("asset_import_data")
        import_data.scripted_add_filename(relative_source, 0, "Approved obj:3501 GLB")
        resolved = [Path(value).resolve() for value in import_data.extract_filenames()]
        if source.resolve() not in resolved:
            raise RuntimeError(f"reimport path does not resolve to approved source: {path}: {resolved}")
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        print("REAL33D_MAILBOX_REIMPORT", path, relative_source)
        if isinstance(asset, unreal.StaticMesh):
            meshes.append(asset)
            bounds = asset.get_bounding_box()
            print("REAL33D_MAILBOX_IMPORT_BOUNDS", bounds.min, bounds.max)
    if len(meshes) != 1:
        raise RuntimeError(f"expected one mailbox static mesh, found {len(meshes)}")
    mesh = meshes[0]
    bounds = mesh.get_bounding_box()
    expected_min = (-33.0, -32.5, 0.0)
    expected_max = (33.0, 32.5, 136.5)
    for actual_vector, expected in ((bounds.min, expected_min), (bounds.max, expected_max)):
        for actual_coordinate, expected_coordinate in zip(
            (actual_vector.x, actual_vector.y, actual_vector.z), expected
        ):
            if abs(actual_coordinate - expected_coordinate) > 0.05:
                raise RuntimeError(f"mailbox scale/orientation/pivot mismatch: {bounds}")
    if len(mesh.get_editor_property("static_materials")) != 2:
        raise RuntimeError("mailbox does not have its two source material slots")
    if mesh.get_material(0) is None or mesh.get_material(1) is None:
        raise RuntimeError("mailbox has an unassigned source material")
    if mesh.get_num_nanite_triangles() != 1192:
        raise RuntimeError(
            f"mailbox Nanite geometry mismatch: {mesh.get_num_nanite_triangles()} triangles"
        )
    if unreal.EditorAssetLibrary.does_directory_exist("/Game/Visual/obj3508"):
        raise RuntimeError("unapproved obj:3508 art appeared in the Unreal project")
    if hashlib.sha256(source.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError("approved GLB changed during import")
    print(
        "REAL33D_MAILBOX_IMPORT_VALIDATED",
        MESH_PACKAGE,
        "nanite_triangles=1192 source_zero_area_triangles=24",
    )


main()
