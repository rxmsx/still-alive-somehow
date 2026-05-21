# Survival World UE5

Unreal Engine 5.7 foundation for the open-world survival game. The repository now contains only the UE5 project and the project lives directly at the repository root.

## Open

Open the project file from the repository root:

```bash
open SurvivalWorldUE5.uproject
```

Or launch it directly with Unreal Editor:

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/SurvivalWorldUE5.uproject"
```

## Structure

- `SurvivalWorldUE5.uproject`: UE5 project file.
- `Config/`: project and editor configuration.
- `Content/`: Unreal content root.
- `Source/`: C++ gameplay module and targets.
- `Plugins/`: project plugins, kept at root for future UE plugins.
- `Scripts/Unreal/`: Python scripts for editor automation and asset pipeline work.

Generated UE folders such as `Binaries/`, `Intermediate/`, `Saved/`, and `DerivedDataCache/` are intentionally ignored.

## Current Milestone

- 2x2 km hybrid open-world prototype.
- World Partition plus PCG-ready folder/config structure.
- C++ gameplay core with Blueprint-tunable actors/components.
- Singleplayer first, with component-driven gameplay state for later networking.
- Cave mining v1 uses cave/mineshaft actors and ore veins, not deformable terrain.

## First Editor Pass

1. Finish installing Unreal Engine 5.7.x stable.
2. Open `SurvivalWorldUE5.uproject`.
3. Let Unreal generate project files and compile the `SurvivalWorldUE5` module.
4. Create `Content/OpenWorld/Maps/L_OpenWorld_Prototype.umap` as an Open World / World Partition map.
5. Create a 2x2 km Landscape, then add Data Layers for `DL_Surface`, `DL_Caves`, `DL_Resources`, and `DL_Debug`.
6. Create PCG graphs under `Content/OpenWorld/PCG/` for forest, rocky, and wet/river resource placement.
7. Create Blueprint children of `AResourceNodeActor` for wood, stone, iron, copper, and coal nodes.

## Tests

Run automation tests from the editor with:

```text
SurvivalWorld.Foundation
```

Python is reserved for editor automation and pipeline work. Runtime gameplay is C++ plus Blueprint tuning.

## Python Automation

Python is used only inside the Unreal Editor for tooling. Keep runtime gameplay in C++ and Blueprints.

After opening the project once, enable Python Developer Mode in Unreal's Python project settings. Unreal will generate local stubs under `Intermediate/PythonStub/`, and VS Code is already configured to look there for `unreal` API autocomplete.

Run editor scripts from the repository root with:

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/SurvivalWorldUE5.uproject" \
  -ExecutePythonScript="$PWD/Scripts/Unreal/audit_project.py"
```

Useful starter scripts:

- `Scripts/Unreal/audit_project.py`: prints a quick content/project sanity check.
- `Scripts/Unreal/bootstrap_open_world.py`: creates the expected Open World content folders.
