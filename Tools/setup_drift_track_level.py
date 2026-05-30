import unreal


MESH_PATH = "/Game/UnfriendBlur/Downloaded/DriftTrackShort/SM_DriftTrackShort.SM_DriftTrackShort"
LEVEL_PATH = "/Game/UnfriendBlur/Maps/L_DriftTrackShort"


def configure_track_mesh(mesh):
    body_setup = mesh.get_editor_property("body_setup")
    if body_setup:
        body_setup.set_editor_property(
            "collision_trace_flag",
            unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
        )
    mesh.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)


def spawn_actor(actor_class, location, rotation=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        location,
        rotation or unreal.Rotator(0.0, 0.0, 0.0),
    )
    return actor


def main():
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not mesh:
        raise RuntimeError("Could not load {}".format(MESH_PATH))

    configure_track_mesh(mesh)

    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.EditorAssetLibrary.delete_asset(LEVEL_PATH)

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level_editor.new_level(LEVEL_PATH)

    track_actor = spawn_actor(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
    track_actor.set_actor_label("Drift Track Short")
    track_component = track_actor.get_editor_property("static_mesh_component")
    track_component.set_static_mesh(mesh)
    track_component.set_collision_profile_name("BlockAll")
    track_component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)

    sun = spawn_actor(
        unreal.DirectionalLight,
        unreal.Vector(-3000.0, -3000.0, 5000.0),
        unreal.Rotator(-45.0, -35.0, 0.0),
    )
    sun.set_actor_label("Sun")
    sun.get_editor_property("light_component").set_editor_property("intensity", 5.0)

    skylight = spawn_actor(unreal.SkyLight, unreal.Vector(0.0, 0.0, 500.0))
    skylight.set_actor_label("Skylight")
    skylight.get_editor_property("light_component").set_editor_property("intensity", 1.2)

    player_start = spawn_actor(
        unreal.PlayerStart,
        unreal.Vector(0.0, -1200.0, 250.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    player_start.set_actor_label("Player Start")

    level_editor.save_current_level()
    unreal.log("Created {}".format(LEVEL_PATH))


if __name__ == "__main__":
    main()
