# ASSET_PROMPTS

These prompts are prepared for later bitmap generation/import into Unreal. No text should be baked into generated images. For UI backgrounds, generate clean square or rectangular assets that can be converted to `UTexture2D` and used as nine-slice/brush imagery in UMG or Slate.

## Shared UI Style

Dark realistic survival game UI, desaturated palette, worn dark grey painted steel, blackened leather, coarse faded fabric, subtle scratches, dust, edge wear, grime in corners, restrained highlights, high readability, no fantasy ornament, no mobile game colors, no text, no logos, no buttons baked into the image.

## Inventory Panel

Realistic survival game inventory panel background, dark worn gunmetal frame combined with faded olive-black canvas insert, subtle stitched fabric texture, chipped painted steel edges, dust and fine scratches, slightly recessed center surface, scalable nine-slice friendly border, orthographic front view, no text, no icons, no buttons, no strong shadows, transparent outside edges if possible.

## Inventory Slot Frames

Base style: square realistic survival inventory slot frame, dark inset cavity, worn blackened steel and leather rim, subtle bevel, soft edge wear, clean transparent outside, centered square composition, no item inside, no text.

- Normal: subdued dark metal, low contrast.
- Hover: same frame with restrained warm edge highlight and slightly brighter inset.
- Selected: same frame with thin desaturated brass highlight, no glow-heavy arcade effect.
- Disabled: darker desaturated frame, low opacity feel, dusty inset.
- Equipped: same frame with a narrow muted green status strip on one edge.

## Crafting Panel

Realistic survival crafting panel background, dark workbench surface, scratched painted steel edge, worn plywood or oil-stained canvas insert, faint tool marks, recessed areas for recipe list, ingredient list and progress area, subtle practical workshop structure, no text, no icon, no buttons baked in, nine-slice friendly.

## Tooltip Panel

Compact realistic survival tooltip panel, dark matte metal and smoked glass look, subtle thin worn brass/steel rim, high contrast readable center area, faint dust and hairline scratches, no text, no icon, transparent outside, scalable UI background.

## HUD Symbols

Shared symbol style: compact realistic survival HUD icon, monochrome off-white stencil or etched metal icon, slightly worn edges, transparent background, no text, square composition, readable at small size.

- Health: simple medical cross or bandage mark, practical survival stencil style.
- Stamina: compact breath/lung or chevron endurance symbol, no lightning bolt arcade style.
- Hunger: minimal fork/knife or ration mark, rugged stencil style.
- Thirst: clean water drop symbol, slightly worn stencil style.

## Minimap / Compass Frame

Realistic dark survival minimap circular frame, blackened metal ring with subtle compass tick marks, muted brass north indicator, worn paint, scratches, transparent center and outside, no map baked in, no text except optional tiny `N` marker if separate variant is needed.

## Hotbar / Quickslot

Compact realistic survival quickslot frame, dark square inset, worn steel/leather border, small area reserved for key number overlay, active selection with restrained brass edge highlight, transparent outside, no item inside, no baked text.

## Item Icon Base Prompt

Photorealistic survival game inventory item icon, isolated single object, centered, slightly worn used condition, soft neutral studio lighting from upper left, consistent three-quarter angle, realistic material detail, transparent background if possible, no text, no border, no hands, square composition.

## Current Fallback Items

These are the built-in fallback item IDs currently present in `USurvivalItemCatalog::GetDefaultItems`. If a `USurvivalItemCatalog` DataAsset adds more items later, generate additional icons using the base prompt and the same camera/light setup.

- Wood: rough salvaged dry wood log, splintered cut end, weathered bark remnants.
- Stick: straight dry branch or stick, bark wear, small cracks.
- Stone: dense grey field stone, chipped edges, subtle dust.
- PlantFiber: bundle of dry plant fibers, fibrous strands, olive-tan color.
- Rope: rough hemp rope coil, frayed ends, worn fibers.
- Cloth: torn dirty cloth scrap, canvas weave, frayed stitched edge.
- RawMeat: raw red meat cut, realistic fat and moisture, no plate.
- CookedMeat: grilled cooked meat, browned surface, no plate.
- WaterBottle: clear plastic bottle with clean water, worn blue cap.
- DirtyWater: plastic bottle with brown cloudy water, muddy residue.
- Bandage: folded dirty survival bandage with small metal pin.
- Alcohol: small amber medical alcohol bottle, no label text.
- Hide: rough animal hide, fur side visible, irregular cut.
- PrimitiveTool: rough tied stone-and-stick starter tool.
- StoneAxe: primitive stone axe, wooden handle, fiber binding.
- SharpenedStone: chipped sharp stone blade, matte grey.
- Spear: wooden spear with sharpened or stone tip, fiber wrap.
- Torch: wrapped wooden torch, soot-dark cloth head.
- Campfire: compact campfire kit with stones, kindling and cord.
- SimpleBackpack: worn canvas survival backpack.
- PrimitiveClothing: rough hide-and-fiber primitive clothing bundle.
- Axe: worn survival axe, dark wooden handle, scratched steel head, taped grip.
- Pickaxe: heavy worn pickaxe, dark handle, chipped steel head, dust on metal.
