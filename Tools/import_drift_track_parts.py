import unreal


SOURCE_FBX = r"C:/GamesDev/UnfriendBlur/Saved/ImportedAssets/DriftTrackShort/RaceTrackExport/Track.fbx"
DESTINATION = "/Game/UnfriendBlur/Downloaded/DriftTrackShortParts"


def configure_fbx_import():
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    static_mesh_options = options.get_editor_property("static_mesh_import_data")
    static_mesh_options.set_editor_property("combine_meshes", False)
    static_mesh_options.set_editor_property("build_nanite", False)
    static_mesh_options.set_editor_property("generate_lightmap_u_vs", True)
    static_mesh_options.set_editor_property("auto_generate_collision", False)

    return options


def main():
    unreal.log("Importing Drift Track Short as separated static meshes from {}".format(SOURCE_FBX))

    if unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        unreal.EditorAssetLibrary.delete_directory(DESTINATION)
    unreal.EditorAssetLibrary.make_directory(DESTINATION)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE_FBX)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", configure_fbx_import())

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    unreal.log("Part import paths: {}".format(list(task.get_editor_property("imported_object_paths") or [])))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


if __name__ == "__main__":
    main()
