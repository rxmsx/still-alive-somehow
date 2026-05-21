# Unreal Python Scripts

These scripts are for Unreal Editor automation only. Do not put runtime gameplay logic here.

## Setup

1. Open `SurvivalWorldUE5.uproject` in Unreal Engine.
2. Confirm the `Python Editor Script Plugin` and `Editor Scripting Utilities` plugins are enabled.
3. In Project Settings > Python, enable Developer Mode.
4. Restart the editor once so Unreal can generate `Intermediate/PythonStub/unreal.py`.
5. Open the repository in VS Code. Pylance should then resolve `import unreal`.

## Run

From the repository root:

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/SurvivalWorldUE5.uproject" \
  -ExecutePythonScript="$PWD/Scripts/Unreal/audit_project.py"
```

You can also run snippets in Unreal's Output Log with the log mode set to Python.
