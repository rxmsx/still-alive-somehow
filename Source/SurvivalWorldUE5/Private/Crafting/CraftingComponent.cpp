#include "Crafting/CraftingComponent.h"

#include "Items/InventoryComponent.h"
#include "World/OpenWorldPrototypeSettings.h"

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ItemCatalog)
	{
		if (const UOpenWorldPrototypeSettings* Settings = GetDefault<UOpenWorldPrototypeSettings>())
		{
			ItemCatalog = Settings->ItemCatalog.LoadSynchronous();
		}
	}
}

void UCraftingComponent::SetItemCatalog(USurvivalItemCatalog* NewItemCatalog)
{
	ItemCatalog = NewItemCatalog;
	OnCraftingChanged.Broadcast();
}

TArray<FCraftingRecipe> UCraftingComponent::GetKnownRecipes() const
{
	if (ItemCatalog && ItemCatalog->Recipes.Num() > 0)
	{
		TArray<FCraftingRecipe> Recipes;
		for (const FCraftingRecipe& Recipe : ItemCatalog->Recipes)
		{
			if (Recipe.bUnlockedByDefault)
			{
				Recipes.Add(Recipe);
			}
		}
		return Recipes;
	}

	return GetDefaultRecipes();
}

TArray<FCraftingRecipe> UCraftingComponent::GetCraftableRecipes() const
{
	TArray<FCraftingRecipe> CraftableRecipes;
	for (const FCraftingRecipe& Recipe : GetKnownRecipes())
	{
		if (CanCraft(Recipe.RecipeId))
		{
			CraftableRecipes.Add(Recipe);
		}
	}
	return CraftableRecipes;
}

bool UCraftingComponent::CanCraft(FName RecipeId) const
{
	FCraftingRecipe FallbackRecipe;
	const FCraftingRecipe* Recipe = FindRecipe(RecipeId, FallbackRecipe);
	const UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Recipe || !Inventory || Recipe->OutputItemId.IsNone())
	{
		return false;
	}

	for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
	{
		if (Ingredient.ItemId.IsNone() || Ingredient.Count <= 0 || !Inventory->HasItem(Ingredient.ItemId, Ingredient.Count))
		{
			return false;
		}
	}

	return true;
}

bool UCraftingComponent::CraftRecipe(FName RecipeId)
{
	if (!CanCraft(RecipeId))
	{
		return false;
	}

	FCraftingRecipe FallbackRecipe;
	const FCraftingRecipe* Recipe = FindRecipe(RecipeId, FallbackRecipe);
	UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Recipe || !Inventory)
	{
		return false;
	}

	for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
	{
		if (!Inventory->RemoveItem(Ingredient.ItemId, Ingredient.Count))
		{
			return false;
		}
	}

	const bool bAddedOutput = Inventory->AddItem(Recipe->OutputItemId, Recipe->OutputCount);
	if (bAddedOutput)
	{
		OnCraftingChanged.Broadcast();
	}
	return bAddedOutput;
}

FText UCraftingComponent::GetItemDisplayName(FName ItemId) const
{
	if (ItemCatalog)
	{
		if (const FItemDef* Item = ItemCatalog->FindItem(ItemId))
		{
			if (!Item->DisplayName.IsEmpty())
			{
				return Item->DisplayName;
			}
		}
	}

	for (const FItemDef& Item : GetDefaultItems())
	{
		if (Item.ItemId == ItemId && !Item.DisplayName.IsEmpty())
		{
			return Item.DisplayName;
		}
	}

	return FText::FromName(ItemId);
}

ESurvivalItemCategory UCraftingComponent::GetItemCategory(FName ItemId) const
{
	if (ItemCatalog)
	{
		if (const FItemDef* Item = ItemCatalog->FindItem(ItemId))
		{
			return Item->Category;
		}
	}

	for (const FItemDef& Item : GetDefaultItems())
	{
		if (Item.ItemId == ItemId)
		{
			return Item.Category;
		}
	}

	return ESurvivalItemCategory::Misc;
}

bool UCraftingComponent::GetItemDefinition(FName ItemId, FItemDef& OutItem) const
{
	if (ItemCatalog)
	{
		if (const FItemDef* Item = ItemCatalog->FindItem(ItemId))
		{
			OutItem = *Item;
			return true;
		}
	}

	for (const FItemDef& Item : GetDefaultItems())
	{
		if (Item.ItemId == ItemId)
		{
			OutItem = Item;
			return true;
		}
	}

	return false;
}

const FCraftingRecipe* UCraftingComponent::FindRecipe(FName RecipeId, FCraftingRecipe& OutFallbackRecipe) const
{
	if (ItemCatalog)
	{
		if (const FCraftingRecipe* Recipe = ItemCatalog->FindRecipe(RecipeId))
		{
			return Recipe;
		}
	}

	for (const FCraftingRecipe& Recipe : GetDefaultRecipes())
	{
		if (Recipe.RecipeId == RecipeId)
		{
			OutFallbackRecipe = Recipe;
			return &OutFallbackRecipe;
		}
	}

	return nullptr;
}

UInventoryComponent* UCraftingComponent::GetOwnerInventory() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

TArray<FCraftingRecipe> UCraftingComponent::GetDefaultRecipes() const
{
	TArray<FCraftingRecipe> Recipes;

	auto Ingredient = [](const TCHAR* ItemId, int32 Count)
	{
		FCraftingIngredient NewIngredient;
		NewIngredient.ItemId = FName(ItemId);
		NewIngredient.Count = Count;
		return NewIngredient;
	};

	auto AddRecipe = [&Recipes](const TCHAR* RecipeId, const TCHAR* DisplayName, const TCHAR* Description, const TCHAR* OutputItemId, int32 OutputCount, TArray<FCraftingIngredient> Ingredients)
	{
		FCraftingRecipe Recipe;
		Recipe.RecipeId = FName(RecipeId);
		Recipe.DisplayName = FText::FromString(DisplayName);
		Recipe.Description = FText::FromString(Description);
		Recipe.OutputItemId = FName(OutputItemId);
		Recipe.OutputCount = OutputCount;
		Recipe.Ingredients = MoveTemp(Ingredients);
		Recipes.Add(Recipe);
	};

	AddRecipe(
		TEXT("Bow"),
		TEXT("Bogen"),
		TEXT("Ein einfacher Jagdbogen aus Stoecken, Seil und Tiersehnen."),
		TEXT("Bow"),
		1,
		{
			Ingredient(TEXT("Stick"), 2),
			Ingredient(TEXT("Rope"), 1),
			Ingredient(TEXT("AnimalSinew"), 2)
		});

	AddRecipe(
		TEXT("Arrow"),
		TEXT("Pfeile"),
		TEXT("Leichte Pfeile mit Feuersteinspitze und Federfuehrung."),
		TEXT("Arrow"),
		4,
		{
			Ingredient(TEXT("Stick"), 1),
			Ingredient(TEXT("Feather"), 1),
			Ingredient(TEXT("Flint"), 1)
		});

	AddRecipe(
		TEXT("Axe"),
		TEXT("Axt"),
		TEXT("Ein robustes Basiswerkzeug zum Faellen und Zerlegen von Holz."),
		TEXT("Axe"),
		1,
		{
			Ingredient(TEXT("Stick"), 1),
			Ingredient(TEXT("Stone"), 1),
			Ingredient(TEXT("Rope"), 1)
		});

	AddRecipe(
		TEXT("Pickaxe"),
		TEXT("Spitzhacke"),
		TEXT("Ein schweres Werkzeug fuer Stein, Erz und harte Ressourcenknoten."),
		TEXT("Pickaxe"),
		1,
		{
			Ingredient(TEXT("Stick"), 2),
			Ingredient(TEXT("Stone"), 3),
			Ingredient(TEXT("Rope"), 1)
		});

	AddRecipe(
		TEXT("Campfire"),
		TEXT("Lagerfeuer"),
		TEXT("Eine einfache Feuerstelle zum Kochen, Waermen und als Lichtquelle."),
		TEXT("Campfire"),
		1,
		{
			Ingredient(TEXT("Wood"), 5),
			Ingredient(TEXT("Stone"), 3)
		});

	AddRecipe(
		TEXT("IronKnife"),
		TEXT("Eisenmesser"),
		TEXT("Ein scharfes Messer fuer Jagd, Verarbeitung und Nahkampf."),
		TEXT("IronKnife"),
		1,
		{
			Ingredient(TEXT("IronIngot"), 1),
			Ingredient(TEXT("WoodGrip"), 1),
			Ingredient(TEXT("Leather"), 1)
		});

	AddRecipe(
		TEXT("FishingRod"),
		TEXT("Angel"),
		TEXT("Eine einfache Angel zum Fischen an Seen, Fluesse und Kueste."),
		TEXT("FishingRod"),
		1,
		{
			Ingredient(TEXT("Stick"), 2),
			Ingredient(TEXT("Rope"), 1),
			Ingredient(TEXT("IronHook"), 1)
		});

	AddRecipe(
		TEXT("StoneBlade"),
		TEXT("Steinklinge"),
		TEXT("Eine scharfe Starterklinge aus Stein."),
		TEXT("StoneBlade"),
		1,
		{
			Ingredient(TEXT("Stone"), 2)
		});

	AddRecipe(
		TEXT("Stick"),
		TEXT("Stock"),
		TEXT("Ein einfacher Grundbestandteil aus einem Ast."),
		TEXT("Stick"),
		2,
		{
			Ingredient(TEXT("Branch"), 1)
		});

	return Recipes;
}

TArray<FItemDef> UCraftingComponent::GetDefaultItems() const
{
	TArray<FItemDef> Items;

	auto AddItem = [&Items](
		const TCHAR* ItemId,
		const TCHAR* DisplayName,
		ESurvivalItemCategory Category,
		const TCHAR* Description,
		int32 MaxStack,
		float WeightKg,
		ESurvivalItemRarity Rarity,
		bool bIsEdible,
		int32 NutritionValue,
		bool bIsFuel,
		float BurnDurationSeconds,
		bool bCanBeProcessed,
		const TCHAR* ProcessingOutputItemId,
		int32 ProcessingOutputCount,
		bool bIsTool,
		int32 SortOrder)
	{
		FItemDef Item;
		Item.ItemId = FName(ItemId);
		Item.DisplayName = FText::FromString(DisplayName);
		Item.Description = FText::FromString(Description);
		Item.Category = Category;
		Item.MaxStack = MaxStack;
		Item.WeightKg = WeightKg;
		Item.Rarity = Rarity;
		Item.bIsEdible = bIsEdible;
		Item.NutritionValue = NutritionValue;
		Item.bIsFuel = bIsFuel;
		Item.BurnDurationSeconds = BurnDurationSeconds;
		Item.bCanBeProcessed = bCanBeProcessed;
		if (ProcessingOutputItemId && ProcessingOutputItemId[0] != TEXT('\0'))
		{
			Item.ProcessingOutputItemId = FName(ProcessingOutputItemId);
			Item.ProcessingOutputCount = ProcessingOutputCount;
		}
		Item.bIsTool = bIsTool;
		Item.SortOrder = SortOrder;
		Items.Add(Item);
	};

	AddItem(TEXT("Wood"), TEXT("Holz"), ESurvivalItemCategory::RawResource, TEXT("Ein brauchbares Stueck Holz fuer Bau, Feuer und einfache Verarbeitung."), 50, 1.20f, ESurvivalItemRarity::Common, false, 0, true, 90.0f, true, TEXT("WoodPlank"), 2, false, 100);
	AddItem(TEXT("Branch"), TEXT("Ast"), ESurvivalItemCategory::RawResource, TEXT("Ein trockener Ast, der sich gut zu Stoecken weiterverarbeiten laesst."), 60, 0.20f, ESurvivalItemRarity::Common, false, 0, true, 25.0f, true, TEXT("Stick"), 1, false, 110);
	AddItem(TEXT("Stone"), TEXT("Stein"), ESurvivalItemCategory::RawResource, TEXT("Ein harter Stein fuer einfache Werkzeuge, Feuerstellen und Bauplaetze."), 50, 0.80f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 120);
	AddItem(TEXT("Flint"), TEXT("Feuerstein"), ESurvivalItemCategory::RawResource, TEXT("Scharfkantiger Stein fuer Klingen, Pfeilspitzen und Feuerstarter."), 50, 0.25f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, false, 130);
	AddItem(TEXT("Clay"), TEXT("Lehm"), ESurvivalItemCategory::RawResource, TEXT("Feuchtes Erdmaterial fuer spaetere Bau-, Ofen- und Keramiksysteme."), 100, 0.40f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 140);
	AddItem(TEXT("Sand"), TEXT("Sand"), ESurvivalItemCategory::RawResource, TEXT("Feines Mineralmaterial fuer Bau, Gussformen und spaetere Glasverarbeitung."), 100, 0.20f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 150);
	AddItem(TEXT("OreChunk"), TEXT("Erzbrocken"), ESurvivalItemCategory::RawResource, TEXT("Ein unsortierter Erzbrocken mit kleinen Metallanteilen."), 40, 1.10f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, false, 160);
	AddItem(TEXT("CopperOre"), TEXT("Kupfererz"), ESurvivalItemCategory::RawResource, TEXT("Kupferhaltiges Gestein, das in einem Ofen zu Barren geschmolzen werden kann."), 40, 1.30f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, true, TEXT("CopperIngot"), 1, false, 170);
	AddItem(TEXT("IronOre"), TEXT("Eisenerz"), ESurvivalItemCategory::RawResource, TEXT("Eisenhaltiges Gestein fuer robuste Werkzeuge und Waffen."), 40, 1.50f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, true, TEXT("IronIngot"), 1, false, 180);

	AddItem(TEXT("WoodPlank"), TEXT("Holzbrett"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein zugeschnittenes Brett fuer Bau- und Werkzeugteile."), 40, 0.70f, ESurvivalItemRarity::Common, false, 0, true, 60.0f, true, TEXT("WoodGrip"), 1, false, 200);
	AddItem(TEXT("Stick"), TEXT("Stock"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein gerader Stock fuer Werkzeuge, Waffen und einfache Konstruktionen."), 60, 0.15f, ESurvivalItemRarity::Common, false, 0, true, 35.0f, false, nullptr, 1, false, 210);
	AddItem(TEXT("Rope"), TEXT("Seil"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Geflochtenes Material zum Binden, Spannen und Verbinden."), 30, 0.25f, ESurvivalItemRarity::Common, false, 0, true, 20.0f, false, nullptr, 1, false, 220);
	AddItem(TEXT("ClothScrap"), TEXT("Stofffetzen"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Kleine Stoffreste fuer Kleidung, Bandagen und leichte Reparaturen."), 50, 0.05f, ESurvivalItemRarity::Common, false, 0, true, 15.0f, false, nullptr, 1, false, 230);
	AddItem(TEXT("Leather"), TEXT("Leder"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Gegerbtes Leder fuer Griffe, Ruestung und haltbare Ausruestung."), 20, 0.40f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, false, 240);
	AddItem(TEXT("CopperIngot"), TEXT("Kupferbarren"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein gegossener Kupferbarren fuer einfache Metallteile."), 20, 1.00f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, false, 250);
	AddItem(TEXT("IronIngot"), TEXT("Eisenbarren"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein stabiler Eisenbarren fuer Werkzeuge, Klingen und Beschlaege."), 20, 1.10f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, true, TEXT("Nail"), 6, false, 260);
	AddItem(TEXT("Nail"), TEXT("Nagel"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein kleiner Eisenverbinder fuer Bau- und Reparaturrezepte."), 100, 0.03f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 270);
	AddItem(TEXT("Coal"), TEXT("Kohle"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Dichter Brennstoff fuer Feuerstellen, Schmelzoefen und spaetere Maschinen."), 50, 0.40f, ESurvivalItemRarity::Common, false, 0, true, 300.0f, false, nullptr, 1, false, 280);
	AddItem(TEXT("WoodGrip"), TEXT("Holzgriff"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein geformter Griff fuer Messer, Haken und feine Werkzeuge."), 25, 0.25f, ESurvivalItemRarity::Common, false, 0, true, 25.0f, false, nullptr, 1, false, 290);
	AddItem(TEXT("IronHook"), TEXT("Eisenhaken"), ESurvivalItemCategory::ProcessedMaterial, TEXT("Ein gebogener Eisenhaken fuer Angel- und Mechanikrezepte."), 50, 0.08f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, false, 300);

	AddItem(TEXT("Feather"), TEXT("Feder"), ESurvivalItemCategory::NaturalMaterial, TEXT("Leichte Feder fuer Pfeile, Schmuck und feine Polsterung."), 100, 0.02f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 400);
	AddItem(TEXT("Bone"), TEXT("Knochen"), ESurvivalItemCategory::NaturalMaterial, TEXT("Harter Knochen fuer Werkzeuge, Nadeln oder primitive Waffen."), 40, 0.30f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 410);
	AddItem(TEXT("AnimalSinew"), TEXT("Tiersehne"), ESurvivalItemCategory::NaturalMaterial, TEXT("Zaehes Gewebe fuer Boegen, Bindungen und robuste Naehte."), 30, 0.08f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, false, 420);
	AddItem(TEXT("Fur"), TEXT("Fell"), ESurvivalItemCategory::NaturalMaterial, TEXT("Warmes Fell fuer Kleidung, Lagerstaetten und Handel."), 20, 0.50f, ESurvivalItemRarity::Uncommon, false, 0, true, 30.0f, false, nullptr, 1, false, 430);
	AddItem(TEXT("Resin"), TEXT("Harz"), ESurvivalItemCategory::NaturalMaterial, TEXT("Klebriges Harz fuer Kleber, Abdichtung und schnell entzuendliche Rezepte."), 30, 0.12f, ESurvivalItemRarity::Common, false, 0, true, 75.0f, false, nullptr, 1, false, 440);
	AddItem(TEXT("PlantFiber"), TEXT("Pflanzenfaser"), ESurvivalItemCategory::NaturalMaterial, TEXT("Faseriges Pflanzenmaterial fuer Seile, Stoff und einfache Bindungen."), 60, 0.04f, ESurvivalItemRarity::Common, false, 0, true, 10.0f, true, TEXT("Rope"), 1, false, 450);

	AddItem(TEXT("RawChickenMeat"), TEXT("Rohes Huehnerfleisch"), ESurvivalItemCategory::Food, TEXT("Rohes Fleisch. Essbar im Notfall, gekocht aber deutlich sicherer und nahrhafter."), 10, 0.35f, ESurvivalItemRarity::Common, true, 12, false, 0.0f, true, TEXT("CookedChickenMeat"), 1, false, 500);
	AddItem(TEXT("CookedChickenMeat"), TEXT("Gebratenes Huehnerfleisch"), ESurvivalItemCategory::Food, TEXT("Durchgebratenes Fleisch mit gutem Naehrwert."), 10, 0.30f, ESurvivalItemRarity::Common, true, 35, false, 0.0f, false, nullptr, 1, false, 510);
	AddItem(TEXT("RawFish"), TEXT("Roher Fisch"), ESurvivalItemCategory::Food, TEXT("Frischer roher Fisch. Gekocht als verlaessliche Nahrung nutzbar."), 10, 0.45f, ESurvivalItemRarity::Common, true, 10, false, 0.0f, true, TEXT("CookedFish"), 1, false, 520);
	AddItem(TEXT("CookedFish"), TEXT("Gebratener Fisch"), ESurvivalItemCategory::Food, TEXT("Gebratener Fisch mit leichtem Gewicht und gutem Naehrwert."), 10, 0.40f, ESurvivalItemRarity::Common, true, 30, false, 0.0f, false, nullptr, 1, false, 530);
	AddItem(TEXT("Berries"), TEXT("Beeren"), ESurvivalItemCategory::Food, TEXT("Essbare Waldbeeren fuer schnelle Energie unterwegs."), 25, 0.05f, ESurvivalItemRarity::Common, true, 8, false, 0.0f, false, nullptr, 1, false, 540);
	AddItem(TEXT("Mushrooms"), TEXT("Pilze"), ESurvivalItemCategory::Food, TEXT("Sammelbare Pilze fuer Nahrung und spaetere Kochrezepte."), 20, 0.06f, ESurvivalItemRarity::Common, true, 10, false, 0.0f, false, nullptr, 1, false, 550);
	AddItem(TEXT("Water"), TEXT("Wasser"), ESurvivalItemCategory::Food, TEXT("Trinkbares Wasser. Stillt Durst, liefert aber keinen Naehrwert."), 10, 1.00f, ESurvivalItemRarity::Common, true, 0, false, 0.0f, false, nullptr, 1, false, 560);

	if (FItemDef* Water = Items.FindByPredicate([](const FItemDef& Item) { return Item.ItemId == TEXT("Water"); }))
	{
		Water->HydrationValue = 45;
	}

	AddItem(TEXT("Bow"), TEXT("Bogen"), ESurvivalItemCategory::Tool, TEXT("Ein einfacher Bogen fuer Jagd und Verteidigung."), 1, 1.00f, ESurvivalItemRarity::Uncommon, false, 0, true, 60.0f, false, nullptr, 1, true, 700);
	AddItem(TEXT("Arrow"), TEXT("Pfeil"), ESurvivalItemCategory::Tool, TEXT("Ein leichter Pfeil fuer Boegen."), 40, 0.05f, ESurvivalItemRarity::Common, false, 0, true, 5.0f, false, nullptr, 1, false, 710);
	AddItem(TEXT("Axe"), TEXT("Axt"), ESurvivalItemCategory::Tool, TEXT("Ein einfaches Werkzeug zum Faellen von Baeumen und Bearbeiten von Holz."), 1, 1.60f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, true, 720);
	AddItem(TEXT("Pickaxe"), TEXT("Spitzhacke"), ESurvivalItemCategory::Tool, TEXT("Ein Werkzeug zum Abbauen von Stein und Erz."), 1, 2.20f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, true, 730);
	AddItem(TEXT("Campfire"), TEXT("Lagerfeuer"), ESurvivalItemCategory::Building, TEXT("Eine platzierbare Feuerstelle fuer Licht, Waerme und Kochen."), 1, 4.00f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, false, 740);
	AddItem(TEXT("IronKnife"), TEXT("Eisenmesser"), ESurvivalItemCategory::Tool, TEXT("Eine haltbare Klinge fuer Jagd und Verarbeitung."), 1, 0.70f, ESurvivalItemRarity::Uncommon, false, 0, false, 0.0f, false, nullptr, 1, true, 750);
	AddItem(TEXT("FishingRod"), TEXT("Angel"), ESurvivalItemCategory::Tool, TEXT("Ein Werkzeug zum Angeln an geeigneten Wasserstellen."), 1, 0.80f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, true, 760);
	AddItem(TEXT("StoneBlade"), TEXT("Steinklinge"), ESurvivalItemCategory::Tool, TEXT("Eine primitive Klinge aus Stein fuer fruehe Verarbeitung."), 10, 0.20f, ESurvivalItemRarity::Common, false, 0, false, 0.0f, false, nullptr, 1, true, 770);

	return Items;
}
