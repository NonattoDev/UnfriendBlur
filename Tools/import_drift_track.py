import unreal


SOURCE_FBX = r"C:/GamesDev/UnfriendBlur/Saved/ImportedAssets/DriftTrackShort/RaceTrackExport/Track.fbx"
DESTINATION = "/Game/UnfriendBlur/Downloaded/DriftTrackShort"


def configure_fbx_import():
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    static_mesh_options = options.get_editor_property("static_mesh_import_data")
    static_mesh_options.set_editor_property("combine_meshes", True)
    static_mesh_options.set_editor_property("build_nanite", True)
    static_mesh_options.set_editor_property("generate_lightmap_u_vs", True)
    static_mesh_options.set_editor_property("auto_generate_collision", False)

    return options


def set_complex_collision(asset_paths):
    for asset_path in asset_paths:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            body_setup = asset.get_editor_property("body_setup")
            if body_setup:
                body_setup.set_editor_property(
                    "collision_trace_flag",
                    unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
                )
                asset.modify()
                unreal.EditorAssetLibrary.save_loaded_asset(asset)
                unreal.log("Configured complex collision: {}".format(asset_path))


def main():
    unreal.log("Importing Drift Track Short from {}".format(SOURCE_FBX))

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE_FBX)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", "SM_DriftTrackShort")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", configure_fbx_import())

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = list(task.get_editor_property("imported_object_paths") or [])
    unreal.log("Imported Drift Track assets: {}".format(imported_paths))
    set_complex_collision(imported_paths)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("Drift Track import finished")


if __name__ == "__main__":
    main()
