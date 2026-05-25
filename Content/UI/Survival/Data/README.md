# Survival Item and Crafting Data

This folder keeps sample data and visual metadata for the C++ survival item system.

## Runtime Architecture

- `USurvivalItemCatalog` is the data-driven source for item and recipe definitions. If no DataAsset is configured in `Open World Prototype > ItemCatalog`, the C++ default catalog is used.
- `UInventoryComponent` stores replicated item slots with `ItemId`, `Count`, `Durability`, `Freshness` and `SlotIndex`. It still exposes old count APIs for resource nodes and saves.
- `UCraftingComponent` owns known recipes, active station, manual crafting-input slots, validation errors and timed crafting progress.
- `USurvivalStatsComponent` receives item effects for health, hunger, thirst, stamina, temperature, fatigue, disease, bleeding and poison.
- `USurvivalHUDWidget` renders the existing dark survival HUD style, inventory slots, hotbar, context menu, recipe book, ingredient status and crafting surface.

## Adding an Item

1. Add a row to a `USurvivalItemCatalog` DataAsset, or mirror the field shape shown in `survival_item_catalog.sample.json`.
2. Use a stable `ItemId`; recipes, resource nodes, saves and UI all reference this value.
3. Set category, rarity, weight, max stack, durability/perishability and `UseType`.
4. Add effects for consumables and medicine.
5. Place the icon under `Content/UI/Survival/Textures/Items/` and point `AssetIconPath` to `/Game/UI/Survival/Textures/Items/<name>`.
6. If a world or crafting-table preview exists, set `WorldMesh`/`PreviewMesh` and the preview transform.

## Adding a Recipe

1. Add an `FCraftingRecipe` to the catalog.
2. Define `RecipeId`, category, station requirement, craft time, ingredient list and output.
3. Use `bUnlockedByDefault=false` for learned recipes; call `UnlockRecipe(RecipeId)` from books, skills or discovery logic.
4. For station recipes, call `SetActiveCraftingStation` when the player opens a workbench, campfire, forge, cooking station or medical table.
5. The UI can craft from manual surface inputs. If the surface is empty, `CraftRecipe` falls back to inventory validation for compatibility.

## Visual Asset Rules

- Icons are 512x512 transparent PNGs, centered, readable at hotbar size and authored from one lighting/camera setup.
- Use UI texture compression for inventory images.
- Use lit masked/opaque materials for 3D preview meshes on crafting surfaces.
- Replace the generated PNGs and placeholder mesh names as final scans/models become available; keep filenames stable to avoid breaking data references.
