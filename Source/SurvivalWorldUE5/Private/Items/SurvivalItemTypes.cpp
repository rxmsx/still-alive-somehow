#include "Items/SurvivalItemTypes.h"

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
