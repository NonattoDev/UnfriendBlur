import unreal


ASSET_ROOT = "/Game/UnfriendBlur/Downloaded/DriftTrackShortParts"
LEVEL_PATH = "/Game/UnfriendBlur/Maps/L_DriftTrackShort"

SKIP_MESH_NAME_PARTS = [
    "collidior",
    "collider",
    "collision",
]


def should_skip_mesh(mesh):
    mesh_name = mesh.get_name().lower()
    return any(skip in mesh_name for skip in SKIP_MESH_NAME_PARTS)


def configure_collision(mesh):
    body_setup = mesh.get_editor_property("body_setup")
    if body_setup:
        body_setup.set_editor_property(
            "collision_trace_flag",
            unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
        )
        mesh.modify()


def spawn_actor(actor_class, location, rotation=None):
    return unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        location,
        rotation or unreal.Rotator(0.0, 0.0, 0.0),
    )


def spawn_track_part(mesh):
    actor = spawn_actor(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
    actor.set_actor_label("DriftTrack_{}".format(mesh.get_name()))

    component = actor.get_editor_property("static_mesh_component")
    component.set_static_mesh(mesh)
    component.set_collision_profile_name("BlockAll")
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    return actor


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.EditorAssetLibrary.delete_asset(LEVEL_PATH)

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level_editor.new_level(LEVEL_PATH)

    spawned_count = 0
    skipped = []

    for asset_path in unreal.EditorAssetLibrary.list_assets(ASSET_ROOT, recursive=False, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.StaticMesh):
            continue

        if should_skip_mesh(asset):
            skipped.append(asset.get_name())
            continue

        configure_collision(asset)
        spawn_track_part(asset)
        spawned_count += 1

    sun = spawn_actor(
        unreal.DirectionalLight,
        unreal.Vector(-3000.0, -3000.0, 5500.0),
        unreal.Rotator(-42.0, -35.0, 0.0),
    )
    sun.set_actor_label("Sun")
    sun.get_editor_property("light_component").set_editor_property("intensity", 5.0)
    try:
        sun.get_editor_property("light_component").set_editor_property("atmosphere_sun_light", True)
    except Exception as exc:
        unreal.log_warning("Could not bind sun to atmosphere: {}".format(exc))

    skylight = spawn_actor(unreal.SkyLight, unreal.Vector(0.0, 0.0, 1000.0))
    skylight.set_actor_label("Skylight")
    skylight.get_editor_property("light_component").set_editor_property("intensity", 1.3)

    try:
        sky = spawn_actor(unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0))
        sky.set_actor_label("Sky Atmosphere")
    except Exception as exc:
        unreal.log_warning("Could not spawn SkyAtmosphere: {}".format(exc))

    try:
        fog = spawn_actor(unreal.ExponentialHeightFog, unreal.Vector(0.0, 0.0, 0.0))
        fog.set_actor_label("Soft Track Fog")
        fog_component = fog.get_editor_property("component")
        fog_component.set_editor_property("fog_density", 0.006)
        fog_component.set_editor_property("fog_height_falloff", 0.18)
    except Exception as exc:
        unreal.log_warning("Could not spawn ExponentialHeightFog: {}".format(exc))

    player_start = spawn_actor(
        unreal.PlayerStart,
        unreal.Vector(-2000.0, 1450.0, 260.0),
        unreal.Rotator(0.0, 20.0, 0.0),
    )
    player_start.set_actor_label("Player Start")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    level_editor.save_current_level()
    unreal.log("Created {} with {} static mesh parts; skipped {}".format(LEVEL_PATH, spawned_count, skipped))


if __name__ == "__main__":
    main()
