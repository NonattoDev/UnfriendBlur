import unreal


ASSETS = {
    "front_wheel": "/Game/VehicleTemplate/Blueprints/SportsCar/SportsCar_WheelsFront",
    "rear_wheel": "/Game/VehicleTemplate/Blueprints/SportsCar/SportsCar_WheelsRear",
}


def get_default_object(asset):
    generated_class = None
    try:
        generated_class = asset.get_editor_property("generated_class")
    except Exception:
        generated_class = None

    if not generated_class:
        generated_class_getter = getattr(asset, "generated_class", None)
        if callable(generated_class_getter):
            try:
                generated_class = generated_class_getter()
            except Exception:
                generated_class = None

    if generated_class:
        return unreal.get_default_object(generated_class)
    return asset


def try_get(obj, prop):
    try:
        return obj.get_editor_property(prop)
    except Exception:
        return None


def try_set(obj, prop, value):
    try:
        before = obj.get_editor_property(prop)
        obj.set_editor_property(prop, value)
        after = obj.get_editor_property(prop)
        unreal.log("Set {}.{}: {} -> {}".format(obj.get_name(), prop, before, after))
        return True
    except Exception as exc:
        unreal.log_warning("Skipped {}.{}: {}".format(obj.get_name(), prop, exc))
        return False


def tune_wheel(asset_path, settings):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        unreal.log_error("Missing asset {}".format(asset_path))
        return False

    cdo = get_default_object(asset)
    if not cdo:
        unreal.log_error("Could not resolve CDO for {}".format(asset_path))
        return False

    changed = False
    for prop, value in settings.items():
        changed = try_set(cdo, prop, value) or changed

    if changed:
        unreal.EditorAssetLibrary.save_loaded_asset(asset, False)

    return changed


def main():
    # Arcade tuning: more front steering and grip, easier handbrake rotation, less truck-like feel.
    front_settings = {
        "max_steer_angle": 52.0,
        "friction_force_multiplier": 3.0,
        "cornering_stiffness": 1250.0,
        "slip_threshold": 34.0,
        "skid_threshold": 34.0,
    }

    rear_settings = {
        "friction_force_multiplier": 2.35,
        "cornering_stiffness": 780.0,
        "slip_threshold": 24.0,
        "skid_threshold": 24.0,
    }

    changed_front = tune_wheel(ASSETS["front_wheel"], front_settings)
    changed_rear = tune_wheel(ASSETS["rear_wheel"], rear_settings)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("Vehicle handling asset tuning complete. front_changed={} rear_changed={}".format(changed_front, changed_rear))


if __name__ == "__main__":
    main()
