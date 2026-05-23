# Generated Survival Item Icons

The PNG files in this folder are transparent 512x512 starter icons generated from one photorealistic chroma-key atlas and cropped into item-specific inventory images.

Source atlas:

`/Users/ramsiado/.codex/generated_images/019e5276-a8fa-7c82-bbb1-86c8c67ec165/ig_088f6e87b5c66829016a110d9362908191b876ee41d1a62719.png`

Generation prompt summary:

Photorealistic isolated survival item icons, equal 6x3 atlas, flat `#00ff00` background, consistent three-quarter camera, soft upper-left studio light, rugged used materials, no text, no hands, no fantasy styling.

Import into Unreal with:

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$PWD/SurvivalWorldUE5.uproject" \
  -ExecutePythonScript="$PWD/Scripts/Unreal/import_survival_item_assets.py"
```
