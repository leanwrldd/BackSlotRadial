# Creates the mod's game feature data and its rebindable key. Re-runnable. Run headlessly with:
#   UnrealEditor-Cmd.exe C:\SF\SML\FactoryGame.uproject -run=pythonscript -script="<this file>" -unattended -nosplash
# A brand new plugin needs two runs: the first logs Plugin_Missing_GameFeatureData and exits 1.
import unreal

MOD_ROOT = "/BackSlotRadial"
INPUT_ROOT = f"{MOD_ROOT}/Input"
# Bound while the player is on foot. The subsystem rules out building, weapons and so on itself.
PARENT_CONTEXT = "/Game/FactoryGame/Inputs/Player/MC_PlayerActions"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
# A commandlet hasn't scanned plugin content yet, so create_asset would refuse to overwrite.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([MOD_ROOT], True)


def get_or_create(root, name, cls):
    path = f"{root}/{name}"
    asset = lib.load_asset(path) if lib.does_asset_exist(path) else unreal.load_object(None, f"{path}.{name}")
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", cls)
        asset = tools.create_asset(name, root, cls, factory)
    if asset is None:
        raise RuntimeError(f"Could not create {path}")
    return asset


feature = get_or_create(MOD_ROOT, "BackSlotRadial", unreal.load_class(None, "/Script/FactoryGame.FGGameFeatureData"))

action = get_or_create(INPUT_ROOT, "IA_BSR_OpenMenu", unreal.InputAction)
action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
action.set_editor_property("action_description", "Open Back Slot Menu (hold)")
# R means other things to the game; a tap must still reach them.
action.set_editor_property("consume_input", False)

keybinds = get_or_create(INPUT_ROOT, "MC_BSR_Keybinds", unreal.FGChildInputMappingContext)
keybinds.set_editor_property("m_display_name", "Back Slot Radial")
keybinds.set_editor_property("m_parent_context", unreal.load_asset(PARENT_CONTEXT))

key = unreal.Key()
key.set_editor_property("key_name", "R")
settings = unreal.new_object(unreal.PlayerMappableKeySettings, outer=keybinds)
settings.set_editor_property("name", "BSR.OpenMenu")
settings.set_editor_property("display_name", "Open Back Slot Menu (hold)")
mapping = unreal.EnhancedActionKeyMapping()
mapping.set_editor_property("action", action)
mapping.set_editor_property("key", key)
mapping.set_editor_property("setting_behavior", unreal.PlayerMappableKeySettingBehaviors.OVERRIDE_SETTINGS)
mapping.set_editor_property("player_mappable_key_settings", settings)
keybinds.set_editor_property("mappings", [mapping])

for asset in (feature, action, keybinds):
    if not lib.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save {asset.get_path_name()}")
    unreal.log(f"BSR: saved {asset.get_path_name()}")
