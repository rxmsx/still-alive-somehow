"""Import generated survival item PNGs into Unreal as UI textures.

Run from the repository root with Unreal Editor:

    UnrealEditor SurvivalWorldUE5.uproject -ExecutePythonScript=Scripts/Unreal/import_survival_item_assets.py
"""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = PROJECT_ROOT / "Content" / "UI" / "Survival" / "Textures" / "Items"
DESTINATION_PATH = "/Game/UI/Survival/Textures/Items"


def make_import_task(filename: Path) -> unreal.AssetImportTask:
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = DESTINATION_PATH
    task.automated = True
    task.replace_existing = True
    task.save = True
    return task


def configure_texture(asset_path: str) -> None:
    texture = unreal.load_asset(asset_path)
    if not texture:
        return

    texture.set_editor_property("srgb", True)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def main() -> None:
    files = sorted(SOURCE_DIR.glob("item_*.png"))
    if not files:
        unreal.log_warning(f"No generated item PNGs found in {SOURCE_DIR}")
        return

    tasks = [make_import_task(path) for path in files]
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for path in files:
        configure_texture(f"{DESTINATION_PATH}/{path.stem}")

    unreal.log(f"Imported {len(files)} survival item textures to {DESTINATION_PATH}")


if __name__ == "__main__":
    main()
