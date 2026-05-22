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

	FCraftingRecipe StoneBlade;
	StoneBlade.RecipeId = TEXT("StoneBlade");
	StoneBlade.DisplayName = NSLOCTEXT("SurvivalWorld", "RecipeStoneBlade", "Stone Blade");
	StoneBlade.Description = NSLOCTEXT("SurvivalWorld", "RecipeStoneBladeDescription", "A sharp starter blade made from stone.");
	StoneBlade.OutputItemId = TEXT("StoneBlade");
	StoneBlade.OutputCount = 1;
	FCraftingIngredient StoneIngredient;
	StoneIngredient.ItemId = TEXT("Stone");
	StoneIngredient.Count = 2;
	StoneBlade.Ingredients.Add(StoneIngredient);
	Recipes.Add(StoneBlade);

	FCraftingRecipe Stick;
	Stick.RecipeId = TEXT("Stick");
	Stick.DisplayName = NSLOCTEXT("SurvivalWorld", "RecipeStick", "Stick");
	Stick.Description = NSLOCTEXT("SurvivalWorld", "RecipeStickDescription", "A simple crafting base made from wood.");
	Stick.OutputItemId = TEXT("Stick");
	Stick.OutputCount = 2;
	FCraftingIngredient WoodIngredient;
	WoodIngredient.ItemId = TEXT("Wood");
	WoodIngredient.Count = 1;
	Stick.Ingredients.Add(WoodIngredient);
	Recipes.Add(Stick);

	return Recipes;
}

TArray<FItemDef> UCraftingComponent::GetDefaultItems() const
{
	TArray<FItemDef> Items;

	auto AddItem = [&Items](FName ItemId, const FText& DisplayName, const FText& Description, ESurvivalItemCategory Category, bool bIsTool = false, int32 SortOrder = 0)
	{
		FItemDef Item;
		Item.ItemId = ItemId;
		Item.DisplayName = DisplayName;
		Item.Description = Description;
		Item.Category = Category;
		Item.bIsTool = bIsTool;
		Item.SortOrder = SortOrder;
		Items.Add(Item);
	};

	AddItem(TEXT("Axe"), NSLOCTEXT("SurvivalWorld", "ItemAxe", "Axe"), NSLOCTEXT("SurvivalWorld", "ItemAxeDescription", "A worn chopping tool for gathering wood and breaking light obstacles."), ESurvivalItemCategory::Tool, true, 10);
	AddItem(TEXT("Pickaxe"), NSLOCTEXT("SurvivalWorld", "ItemPickaxe", "Pickaxe"), NSLOCTEXT("SurvivalWorld", "ItemPickaxeDescription", "A heavy tool for stone and ore work."), ESurvivalItemCategory::Tool, true, 20);
	AddItem(TEXT("Wood"), NSLOCTEXT("SurvivalWorld", "ItemWood", "Wood"), NSLOCTEXT("SurvivalWorld", "ItemWoodDescription", "Dry salvaged wood suitable for fire, repair and basic crafting."), ESurvivalItemCategory::Resource, false, 100);
	AddItem(TEXT("Stone"), NSLOCTEXT("SurvivalWorld", "ItemStone", "Stone"), NSLOCTEXT("SurvivalWorld", "ItemStoneDescription", "A dense stone with sharp workable edges."), ESurvivalItemCategory::Resource, false, 110);
	AddItem(TEXT("Stick"), NSLOCTEXT("SurvivalWorld", "ItemStick", "Stick"), NSLOCTEXT("SurvivalWorld", "ItemStickDescription", "A straight stick for lashings, handles and simple tools."), ESurvivalItemCategory::Resource, false, 120);
	AddItem(TEXT("StoneBlade"), NSLOCTEXT("SurvivalWorld", "ItemStoneBlade", "Stone Blade"), NSLOCTEXT("SurvivalWorld", "ItemStoneBladeDescription", "A primitive chipped blade. Fragile, but better than bare hands."), ESurvivalItemCategory::Tool, true, 130);

	return Items;
}
