"""Quick editor-side sanity check for the Survival World UE5 project.

Run inside Unreal Editor with -ExecutePythonScript or from the Python Output Log.
"""

import unreal


EXPECTED_DIRECTORIES = (
    "/Game/OpenWorld",
    "/Game/OpenWorld/Maps",
    "/Game/OpenWorld/PCG",
    "/Game/OpenWorld/Data",
    "/Game/OpenWorld/Caves",
    "/Game/OpenWorld/Resources",
)


def log(message: str) -> None:
    unreal.log(f"[SurvivalWorld Audit] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[SurvivalWorld Audit] {message}")


def main() -> None:
    asset_library = unreal.EditorAssetLibrary

    log("Starting project audit.")

    missing_directories = [
        path for path in EXPECTED_DIRECTORIES
        if not asset_library.does_directory_exist(path)
    ]

    if missing_directories:
        warn("Missing expected content directories:")
        for path in missing_directories:
            warn(f"  {path}")
    else:
        log("Expected OpenWorld content directories exist.")

    assets = asset_library.list_assets("/Game", recursive=True, include_folder=False)
    log(f"Project asset count under /Game: {len(assets)}")

    log("Audit complete.")


if __name__ == "__main__":
    main()
