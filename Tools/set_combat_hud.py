import unreal


GAME_MODE_PATH = "/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode"
HUD_CLASS_PATH = "/Script/UnfriendBlur.UBCombatHud"


def get_blueprint_cdo(blueprint):
    generated_class = None
    try:
        generated_class = blueprint.get_editor_property("generated_class")
    except Exception:
        generated_class = None

    if not generated_class:
        getter = getattr(blueprint, "generated_class", None)
        if callable(getter):
            generated_class = getter()

    if not generated_class:
        raise RuntimeError("Could not resolve generated class for {}".format(blueprint.get_name()))

    return unreal.get_default_object(generated_class)


def main():
    game_mode = unreal.EditorAssetLibrary.load_asset(GAME_MODE_PATH)
    if not game_mode:
        raise RuntimeError("Missing GameMode asset {}".format(GAME_MODE_PATH))

    hud_class = unreal.load_class(None, HUD_CLASS_PATH)
    if not hud_class:
        raise RuntimeError("Missing HUD class {}".format(HUD_CLASS_PATH))

    cdo = get_blueprint_cdo(game_mode)
    before = cdo.get_editor_property("hud_class")
    cdo.set_editor_property("hud_class", hud_class)
    after = cdo.get_editor_property("hud_class")
    unreal.log("Set {} HUD class: {} -> {}".format(GAME_MODE_PATH, before, after))

    unreal.EditorAssetLibrary.save_loaded_asset(game_mode, False)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


if __name__ == "__main__":
    main()
