#include "Items/SurvivalItemTypes.h"

#include <initializer_list>

const FItemDef* USurvivalItemCatalog::FindItem(FName ItemId) const
{
	return Items.FindByPredicate([ItemId](const FItemDef& Item)
	{
		return Item.ItemId == ItemId;
	});
}

const FCraftingRecipe* USurvivalItemCatalog::FindRecipe(FName RecipeId) const
{
	return Recipes.FindByPredicate([RecipeId](const FCraftingRecipe& Recipe)
	{
		return Recipe.RecipeId == RecipeId;
	});
}

const TArray<FItemDef>& USurvivalItemCatalog::GetDefaultItems()
{
	static const TArray<FItemDef> DefaultItems = []
	{
		TArray<FItemDef> Items;

		auto AddEffect = [](FItemDef& Item, ESurvivalItemEffectType Type, float Magnitude, bool bRemovesNegativeState = false, float DurationSeconds = 0.0f)
		{
			FSurvivalItemEffect Effect;
			Effect.EffectType = Type;
			Effect.Magnitude = Magnitude;
			Effect.bRemovesNegativeState = bRemovesNegativeState;
			Effect.DurationSeconds = DurationSeconds;
			Item.Effects.Add(Effect);
		};

		auto IconNameForItem = [](FName ItemId)
		{
			static const TMap<FName, FString> IconNames = {
				{ TEXT("Wood"), TEXT("item_wood_log") },
				{ TEXT("PlantFiber"), TEXT("item_plant_fiber") },
				{ TEXT("RawMeat"), TEXT("item_raw_meat") },
				{ TEXT("CookedMeat"), TEXT("item_cooked_meat") },
				{ TEXT("WaterBottle"), TEXT("item_water_bottle") },
				{ TEXT("DirtyWater"), TEXT("item_dirty_water") },
				{ TEXT("StoneAxe"), TEXT("item_primitive_axe") },
				{ TEXT("SimpleBackpack"), TEXT("item_simple_backpack") },
				{ TEXT("PrimitiveClothing"), TEXT("item_primitive_clothing") },
				{ TEXT("PrimitiveTool"), TEXT("item_primitive_tool") },
				{ TEXT("SharpenedStone"), TEXT("item_sharpened_stone") }
			};

			if (const FString* IconName = IconNames.Find(ItemId))
			{
				return *IconName;
			}

			return FString::Printf(TEXT("item_%s"), *ItemId.ToString().ToLower());
		};

		auto AddItem = [&Items, &IconNameForItem](FName ItemId, const FText& DisplayName, const FText& Description, ESurvivalItemCategory Category, int32 MaxStack, float WeightKg, ESurvivalItemRarity Rarity, int32 SortOrder)
		{
			FItemDef Item;
			Item.ItemId = ItemId;
			Item.DisplayName = DisplayName;
			Item.Description = Description;
			Item.Category = Category;
			Item.MaxStack = MaxStack;
			Item.bStackable = MaxStack > 1;
			Item.WeightKg = WeightKg;
			Item.Rarity = Rarity;
			Item.SortOrder = SortOrder;
			Item.AssetIconPath = FName(*FString::Printf(TEXT("/Game/UI/Survival/Textures/Items/%s"), *IconNameForItem(ItemId)));
			Items.Add(Item);
			return Items.Num() - 1;
		};

		auto ConfigureDurability = [&Items](int32 Index, float MaxDurability)
		{
			FItemDef& Item = Items[Index];
			Item.bHasDurability = true;
			Item.MaxDurability = MaxDurability;
			Item.bStackable = false;
			Item.MaxStack = 1;
		};

		auto ConfigureUse = [&Items](int32 Index, ESurvivalItemUseType UseType, bool bEquippable = false, bool bHotbarAllowed = true)
		{
			FItemDef& Item = Items[Index];
			Item.UseType = UseType;
			Item.bEquippable = bEquippable;
			Item.bHotbarAllowed = bHotbarAllowed;
			Item.bIsTool = UseType == ESurvivalItemUseType::UseTool || bEquippable;
			if (!Item.bIsTool)
			{
				Item.ToolType = ESurvivalToolType::None;
				Item.HarvestEfficiency = 0.0f;
			}
		};

		AddItem(TEXT("Wood"), NSLOCTEXT("SurvivalWorld", "ItemWoodLog", "Holzscheit"), NSLOCTEXT("SurvivalWorld", "ItemWoodLogDescription", "Trockenes, raues Holz fuer Feuer, Reparaturen und einfache Konstruktionen."), ESurvivalItemCategory::Resource, 30, 0.85f, ESurvivalItemRarity::Common, 100);
		AddItem(TEXT("Stick"), NSLOCTEXT("SurvivalWorld", "ItemStick", "Stock"), NSLOCTEXT("SurvivalWorld", "ItemStickDescription", "Ein gerader trockener Ast fuer Griffe, Schienen und einfache Werkzeuge."), ESurvivalItemCategory::Resource, 40, 0.18f, ESurvivalItemRarity::Common, 110);
		AddItem(TEXT("Stone"), NSLOCTEXT("SurvivalWorld", "ItemStone", "Stein"), NSLOCTEXT("SurvivalWorld", "ItemStoneDescription", "Ein dichter Feldstein mit brauchbaren Kanten."), ESurvivalItemCategory::Resource, 40, 0.45f, ESurvivalItemRarity::Common, 120);
		AddItem(TEXT("PlantFiber"), NSLOCTEXT("SurvivalWorld", "ItemPlantFiber", "Pflanzenfaser"), NSLOCTEXT("SurvivalWorld", "ItemPlantFiberDescription", "Getrocknete Fasern zum Binden, Flechten und Abdichten."), ESurvivalItemCategory::Resource, 60, 0.03f, ESurvivalItemRarity::Common, 130);
		AddItem(TEXT("Rope"), NSLOCTEXT("SurvivalWorld", "ItemRope", "Seil"), NSLOCTEXT("SurvivalWorld", "ItemRopeDescription", "Ein raues, belastbares Seil aus Pflanzenfasern."), ESurvivalItemCategory::Resource, 20, 0.32f, ESurvivalItemRarity::Common, 140);
		AddItem(TEXT("Cloth"), NSLOCTEXT("SurvivalWorld", "ItemCloth", "Stoff"), NSLOCTEXT("SurvivalWorld", "ItemClothDescription", "Verschmutzter, aber brauchbarer Stoff fuer Verband und einfache Ausruestung."), ESurvivalItemCategory::Resource, 30, 0.08f, ESurvivalItemRarity::Common, 150);
		AddItem(TEXT("Hide"), NSLOCTEXT("SurvivalWorld", "ItemHide", "Fell"), NSLOCTEXT("SurvivalWorld", "ItemHideDescription", "Grob gereinigtes Fell, warm und widerstandsfaehig."), ESurvivalItemCategory::Resource, 10, 0.65f, ESurvivalItemRarity::Uncommon, 160);

		const int32 RawMeatIndex = AddItem(TEXT("RawMeat"), NSLOCTEXT("SurvivalWorld", "ItemRawMeat", "Rohes Fleisch"), NSLOCTEXT("SurvivalWorld", "ItemRawMeatDescription", "Verderbliches rohes Fleisch. Gekocht ist es deutlich sicherer."), ESurvivalItemCategory::Food, 8, 0.35f, ESurvivalItemRarity::Common, 220);
		Items[RawMeatIndex].bPerishable = true;
		Items[RawMeatIndex].SpoilTimeHours = 6.0f;
		ConfigureUse(RawMeatIndex, ESurvivalItemUseType::Consume);
		AddEffect(Items[RawMeatIndex], ESurvivalItemEffectType::Hunger, 10.0f);
		AddEffect(Items[RawMeatIndex], ESurvivalItemEffectType::Disease, 18.0f);

		const int32 CookedMeatIndex = AddItem(TEXT("CookedMeat"), NSLOCTEXT("SurvivalWorld", "ItemCookedMeat", "Gekochtes Fleisch"), NSLOCTEXT("SurvivalWorld", "ItemCookedMeatDescription", "Durchgegartes Fleisch mit sicherem Naehrwert."), ESurvivalItemCategory::Food, 8, 0.30f, ESurvivalItemRarity::Common, 230);
		Items[CookedMeatIndex].bPerishable = true;
		Items[CookedMeatIndex].SpoilTimeHours = 18.0f;
		ConfigureUse(CookedMeatIndex, ESurvivalItemUseType::Consume);
		AddEffect(Items[CookedMeatIndex], ESurvivalItemEffectType::Hunger, 34.0f);
		AddEffect(Items[CookedMeatIndex], ESurvivalItemEffectType::Health, 2.0f);

		const int32 WaterIndex = AddItem(TEXT("WaterBottle"), NSLOCTEXT("SurvivalWorld", "ItemWaterBottle", "Wasserflasche"), NSLOCTEXT("SurvivalWorld", "ItemWaterBottleDescription", "Eine wiederverwendbare Flasche mit sauberem Trinkwasser."), ESurvivalItemCategory::Drink, 5, 0.55f, ESurvivalItemRarity::Common, 240);
		ConfigureUse(WaterIndex, ESurvivalItemUseType::Consume);
		AddEffect(Items[WaterIndex], ESurvivalItemEffectType::Thirst, 38.0f);

		const int32 DirtyWaterIndex = AddItem(TEXT("DirtyWater"), NSLOCTEXT("SurvivalWorld", "ItemDirtyWater", "Schmutziges Wasser"), NSLOCTEXT("SurvivalWorld", "ItemDirtyWaterDescription", "Ungefiltertes Wasser. Loescht Durst, kann aber krank machen."), ESurvivalItemCategory::Drink, 5, 0.58f, ESurvivalItemRarity::Common, 250);
		ConfigureUse(DirtyWaterIndex, ESurvivalItemUseType::Consume);
		AddEffect(Items[DirtyWaterIndex], ESurvivalItemEffectType::Thirst, 18.0f);
		AddEffect(Items[DirtyWaterIndex], ESurvivalItemEffectType::Disease, 24.0f);

		const int32 BandageIndex = AddItem(TEXT("Bandage"), NSLOCTEXT("SurvivalWorld", "ItemBandage", "Verband"), NSLOCTEXT("SurvivalWorld", "ItemBandageDescription", "Improvisierter Verband zum Stoppen von Blutungen und kleinen Wunden."), ESurvivalItemCategory::Medicine, 10, 0.05f, ESurvivalItemRarity::Common, 300);
		ConfigureUse(BandageIndex, ESurvivalItemUseType::Consume);
		AddEffect(Items[BandageIndex], ESurvivalItemEffectType::Health, 12.0f);
		AddEffect(Items[BandageIndex], ESurvivalItemEffectType::Bleeding, -100.0f, true);

		const int32 AlcoholIndex = AddItem(TEXT("Alcohol"), NSLOCTEXT("SurvivalWorld", "ItemAlcohol", "Alkohol"), NSLOCTEXT("SurvivalWorld", "ItemAlcoholDescription", "Medizinischer Alkohol zur Desinfektion und Herstellung von Verband."), ESurvivalItemCategory::Medicine, 6, 0.30f, ESurvivalItemRarity::Uncommon, 310);
		ConfigureUse(AlcoholIndex, ESurvivalItemUseType::Consume, false, false);
		AddEffect(Items[AlcoholIndex], ESurvivalItemEffectType::Disease, -35.0f, true);
		AddEffect(Items[AlcoholIndex], ESurvivalItemEffectType::Thirst, -10.0f);

		const int32 PrimitiveToolIndex = AddItem(TEXT("PrimitiveTool"), NSLOCTEXT("SurvivalWorld", "ItemPrimitiveTool", "Primitives Werkzeug"), NSLOCTEXT("SurvivalWorld", "ItemPrimitiveToolDescription", "Ein grobes Werkzeug aus Stock und Stein. Zerbrechlich, aber nuetzlich."), ESurvivalItemCategory::Tool, 1, 0.65f, ESurvivalItemRarity::Common, 400);
		ConfigureDurability(PrimitiveToolIndex, 35.0f);
		ConfigureUse(PrimitiveToolIndex, ESurvivalItemUseType::UseTool, true);

		const int32 StoneAxeIndex = AddItem(TEXT("StoneAxe"), NSLOCTEXT("SurvivalWorld", "ItemStoneAxe", "Steinaxt"), NSLOCTEXT("SurvivalWorld", "ItemStoneAxeDescription", "Eine einfache Axt mit Steinblatt und Faserbindung."), ESurvivalItemCategory::Tool, 1, 1.10f, ESurvivalItemRarity::Common, 410);
		ConfigureDurability(StoneAxeIndex, 70.0f);
		ConfigureUse(StoneAxeIndex, ESurvivalItemUseType::UseTool, true);
		Items[StoneAxeIndex].ToolType = ESurvivalToolType::Axe;
		Items[StoneAxeIndex].HarvestEfficiency = 1.0f;

		const int32 SharpenedStoneIndex = AddItem(TEXT("SharpenedStone"), NSLOCTEXT("SurvivalWorld", "ItemSharpenedStone", "Geschaerfter Stein"), NSLOCTEXT("SurvivalWorld", "ItemSharpenedStoneDescription", "Eine scharfe Steinkante zum Schneiden und Kratzen."), ESurvivalItemCategory::Tool, 1, 0.38f, ESurvivalItemRarity::Common, 420);
		ConfigureDurability(SharpenedStoneIndex, 25.0f);
		ConfigureUse(SharpenedStoneIndex, ESurvivalItemUseType::UseTool, true);

		const int32 TorchIndex = AddItem(TEXT("Torch"), NSLOCTEXT("SurvivalWorld", "ItemTorch", "Fackel"), NSLOCTEXT("SurvivalWorld", "ItemTorchDescription", "Eine primitive Lichtquelle aus Holz und trockener Faser."), ESurvivalItemCategory::Tool, 1, 0.55f, ESurvivalItemRarity::Common, 430);
		ConfigureDurability(TorchIndex, 120.0f);
		ConfigureUse(TorchIndex, ESurvivalItemUseType::UseTool, true);
		Items[TorchIndex].ToolType = ESurvivalToolType::Hand;
		Items[TorchIndex].HarvestEfficiency = 0.2f;

		const int32 SpearIndex = AddItem(TEXT("Spear"), NSLOCTEXT("SurvivalWorld", "ItemSpear", "Speer"), NSLOCTEXT("SurvivalWorld", "ItemSpearDescription", "Ein gebundener Holzspeer fuer Jagd und Verteidigung."), ESurvivalItemCategory::Weapon, 1, 1.25f, ESurvivalItemRarity::Common, 500);
		ConfigureDurability(SpearIndex, 80.0f);
		ConfigureUse(SpearIndex, ESurvivalItemUseType::Equip, true);

		const int32 AxeIndex = AddItem(TEXT("Axe"), NSLOCTEXT("SurvivalWorld", "ItemAxe", "Axt"), NSLOCTEXT("SurvivalWorld", "ItemAxeDescription", "Eine abgenutzte Axt mit Stahlkopf fuer Holz und leichte Hindernisse."), ESurvivalItemCategory::Tool, 1, 1.45f, ESurvivalItemRarity::Uncommon, 510);
		ConfigureDurability(AxeIndex, 120.0f);
		ConfigureUse(AxeIndex, ESurvivalItemUseType::UseTool, true);
		Items[AxeIndex].ToolType = ESurvivalToolType::Axe;
		Items[AxeIndex].HarvestEfficiency = 1.25f;

		const int32 PickaxeIndex = AddItem(TEXT("Pickaxe"), NSLOCTEXT("SurvivalWorld", "ItemPickaxe", "Spitzhacke"), NSLOCTEXT("SurvivalWorld", "ItemPickaxeDescription", "Schweres Werkzeug fuer Stein, Erz und harte Hindernisse."), ESurvivalItemCategory::Tool, 1, 2.10f, ESurvivalItemRarity::Uncommon, 520);
		ConfigureDurability(PickaxeIndex, 140.0f);
		ConfigureUse(PickaxeIndex, ESurvivalItemUseType::UseTool, true);
		Items[PickaxeIndex].ToolType = ESurvivalToolType::Pickaxe;
		Items[PickaxeIndex].HarvestEfficiency = 1.1f;

		const int32 CampfireIndex = AddItem(TEXT("Campfire"), NSLOCTEXT("SurvivalWorld", "ItemCampfire", "Lagerfeuer"), NSLOCTEXT("SurvivalWorld", "ItemCampfireDescription", "Eine einfache Feuerstelle zum Kochen, Trocknen und Reinigen von Wasser."), ESurvivalItemCategory::Building, 1, 3.0f, ESurvivalItemRarity::Common, 600);
		ConfigureUse(CampfireIndex, ESurvivalItemUseType::PlaceWorldObject, false, false);

		const int32 BackpackIndex = AddItem(TEXT("SimpleBackpack"), NSLOCTEXT("SurvivalWorld", "ItemSimpleBackpack", "Einfacher Rucksack"), NSLOCTEXT("SurvivalWorld", "ItemSimpleBackpackDescription", "Ein kleiner Stoffrucksack, der spaeter als Kapazitaetsbonus ausgeruestet werden kann."), ESurvivalItemCategory::Armor, 1, 0.95f, ESurvivalItemRarity::Uncommon, 700);
		ConfigureDurability(BackpackIndex, 80.0f);
		ConfigureUse(BackpackIndex, ESurvivalItemUseType::Equip, true);

		const int32 ClothingIndex = AddItem(TEXT("PrimitiveClothing"), NSLOCTEXT("SurvivalWorld", "ItemPrimitiveClothing", "Primitive Kleidung"), NSLOCTEXT("SurvivalWorld", "ItemPrimitiveClothingDescription", "Grobe Fellkleidung gegen Kaelte und Kratzer."), ESurvivalItemCategory::Armor, 1, 1.35f, ESurvivalItemRarity::Common, 710);
		ConfigureDurability(ClothingIndex, 65.0f);
		ConfigureUse(ClothingIndex, ESurvivalItemUseType::Equip, true);

		return Items;
	}();

	return DefaultItems;
}

const TArray<FCraftingRecipe>& USurvivalItemCatalog::GetDefaultRecipes()
{
	static const TArray<FCraftingRecipe> DefaultRecipes = []
	{
		TArray<FCraftingRecipe> Recipes;

		auto Ingredient = [](FName ItemId, int32 Count)
		{
			FCraftingIngredient IngredientDef;
			IngredientDef.ItemId = ItemId;
			IngredientDef.Count = Count;
			return IngredientDef;
		};

		auto AddRecipe = [&Recipes, &Ingredient](FName RecipeId, const FText& DisplayName, const FText& Description, ECraftingRecipeCategory Category, ECraftingStationType Station, float CraftTimeSeconds, FName OutputItemId, int32 OutputCount, std::initializer_list<FCraftingIngredient> Ingredients)
		{
			FCraftingRecipe Recipe;
			Recipe.RecipeId = RecipeId;
			Recipe.DisplayName = DisplayName;
			Recipe.Description = Description;
			Recipe.Category = Category;
			Recipe.RequiredStation = Station;
			Recipe.CraftTimeSeconds = CraftTimeSeconds;
			Recipe.OutputItemId = OutputItemId;
			Recipe.OutputCount = OutputCount;
			for (const FCraftingIngredient& IngredientDef : Ingredients)
			{
				Recipe.Ingredients.Add(IngredientDef);
			}
			Recipes.Add(Recipe);
		};

		AddRecipe(TEXT("PrimitiveTool"), NSLOCTEXT("SurvivalWorld", "RecipePrimitiveTool", "Primitives Werkzeug"), NSLOCTEXT("SurvivalWorld", "RecipePrimitiveToolDescription", "Stock und Stein zu einem groben Starterwerkzeug verbinden."), ECraftingRecipeCategory::Tools, ECraftingStationType::None, 0.0f, TEXT("PrimitiveTool"), 1, { Ingredient(TEXT("Stick"), 1), Ingredient(TEXT("Stone"), 1) });
		AddRecipe(TEXT("StoneAxe"), NSLOCTEXT("SurvivalWorld", "RecipeStoneAxe", "Steinaxt"), NSLOCTEXT("SurvivalWorld", "RecipeStoneAxeDescription", "Steinblatt mit Pflanzenfasern an einem Stock befestigen."), ECraftingRecipeCategory::Tools, ECraftingStationType::None, 4.0f, TEXT("StoneAxe"), 1, { Ingredient(TEXT("Stick"), 1), Ingredient(TEXT("Stone"), 1), Ingredient(TEXT("PlantFiber"), 2) });
		AddRecipe(TEXT("Torch"), NSLOCTEXT("SurvivalWorld", "RecipeTorch", "Fackel"), NSLOCTEXT("SurvivalWorld", "RecipeTorchDescription", "Trockenes Holz mit Faser umwickeln."), ECraftingRecipeCategory::Tools, ECraftingStationType::None, 2.5f, TEXT("Torch"), 1, { Ingredient(TEXT("Wood"), 1), Ingredient(TEXT("PlantFiber"), 2) });
		AddRecipe(TEXT("SharpenedStone"), NSLOCTEXT("SurvivalWorld", "RecipeSharpenedStone", "Geschaerfter Stein"), NSLOCTEXT("SurvivalWorld", "RecipeSharpenedStoneDescription", "Zwei Steine gegeneinander schlagen, bis eine scharfe Kante entsteht."), ECraftingRecipeCategory::Tools, ECraftingStationType::None, 1.2f, TEXT("SharpenedStone"), 1, { Ingredient(TEXT("Stone"), 2) });
		AddRecipe(TEXT("Spear"), NSLOCTEXT("SurvivalWorld", "RecipeSpear", "Speer"), NSLOCTEXT("SurvivalWorld", "RecipeSpearDescription", "Holz und Seil zu einem einfachen Jagdspeer verbinden."), ECraftingRecipeCategory::Weapons, ECraftingStationType::None, 4.5f, TEXT("Spear"), 1, { Ingredient(TEXT("Wood"), 1), Ingredient(TEXT("Rope"), 1) });
		AddRecipe(TEXT("Bandage"), NSLOCTEXT("SurvivalWorld", "RecipeBandage", "Verband"), NSLOCTEXT("SurvivalWorld", "RecipeBandageDescription", "Stoff mit Alkohol desinfizieren und zu einem Notverband wickeln."), ECraftingRecipeCategory::Medicine, ECraftingStationType::MedicalTable, 3.0f, TEXT("Bandage"), 2, { Ingredient(TEXT("Cloth"), 1), Ingredient(TEXT("Alcohol"), 1) });
		AddRecipe(TEXT("CookedMeat"), NSLOCTEXT("SurvivalWorld", "RecipeCookedMeat", "Gekochtes Fleisch"), NSLOCTEXT("SurvivalWorld", "RecipeCookedMeatDescription", "Rohes Fleisch am Feuer garen."), ECraftingRecipeCategory::Food, ECraftingStationType::Campfire, 5.0f, TEXT("CookedMeat"), 1, { Ingredient(TEXT("RawMeat"), 1) });
		AddRecipe(TEXT("CleanWater"), NSLOCTEXT("SurvivalWorld", "RecipeCleanWater", "Sauberes Wasser"), NSLOCTEXT("SurvivalWorld", "RecipeCleanWaterDescription", "Schmutziges Wasser abkochen."), ECraftingRecipeCategory::Food, ECraftingStationType::Campfire, 4.0f, TEXT("WaterBottle"), 1, { Ingredient(TEXT("DirtyWater"), 1) });
		AddRecipe(TEXT("Campfire"), NSLOCTEXT("SurvivalWorld", "RecipeCampfire", "Lagerfeuer"), NSLOCTEXT("SurvivalWorld", "RecipeCampfireDescription", "Holz und Steine zu einer kleinen Feuerstelle aufschichten."), ECraftingRecipeCategory::Building, ECraftingStationType::None, 6.0f, TEXT("Campfire"), 1, { Ingredient(TEXT("Wood"), 3), Ingredient(TEXT("Stone"), 4) });
		AddRecipe(TEXT("SimpleBackpack"), NSLOCTEXT("SurvivalWorld", "RecipeSimpleBackpack", "Einfacher Rucksack"), NSLOCTEXT("SurvivalWorld", "RecipeSimpleBackpackDescription", "Stoff mit Seil auf einem Holzrahmen befestigen."), ECraftingRecipeCategory::Clothing, ECraftingStationType::Workbench, 8.0f, TEXT("SimpleBackpack"), 1, { Ingredient(TEXT("Wood"), 1), Ingredient(TEXT("Rope"), 2), Ingredient(TEXT("Cloth"), 3) });
		AddRecipe(TEXT("PrimitiveClothing"), NSLOCTEXT("SurvivalWorld", "RecipePrimitiveClothing", "Primitive Kleidung"), NSLOCTEXT("SurvivalWorld", "RecipePrimitiveClothingDescription", "Fell mit Pflanzenfasern zu einfacher Schutzkleidung verschnueren."), ECraftingRecipeCategory::Clothing, ECraftingStationType::None, 6.0f, TEXT("PrimitiveClothing"), 1, { Ingredient(TEXT("Hide"), 2), Ingredient(TEXT("PlantFiber"), 4) });

		return Recipes;
	}();

	return DefaultRecipes;
}

const FItemDef* USurvivalItemCatalog::FindDefaultItem(FName ItemId)
{
	return GetDefaultItems().FindByPredicate([ItemId](const FItemDef& Item)
	{
		return Item.ItemId == ItemId;
	});
}

const FCraftingRecipe* USurvivalItemCatalog::FindDefaultRecipe(FName RecipeId)
{
	return GetDefaultRecipes().FindByPredicate([RecipeId](const FCraftingRecipe& Recipe)
	{
		return Recipe.RecipeId == RecipeId;
	});
}
