"""Create starter Open World content folders in the Unreal project.

Run inside Unreal Editor. This script is intentionally conservative: it creates
folders only and does not overwrite assets.
"""

import unreal


DIRECTORIES = (
    "/Game/OpenWorld",
    "/Game/OpenWorld/Maps",
    "/Game/OpenWorld/PCG",
    "/Game/OpenWorld/Data",
    "/Game/OpenWorld/Caves",
    "/Game/OpenWorld/Resources",
)


def main() -> None:
    asset_library = unreal.EditorAssetLibrary

    for path in DIRECTORIES:
        if asset_library.does_directory_exist(path):
            unreal.log(f"[SurvivalWorld Bootstrap] Exists: {path}")
            continue

        created = asset_library.make_directory(path)
        if created:
            unreal.log(f"[SurvivalWorld Bootstrap] Created: {path}")
        else:
            unreal.log_warning(f"[SurvivalWorld Bootstrap] Could not create: {path}")


if __name__ == "__main__":
    main()
