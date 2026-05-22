#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/SurvivalItemTypes.h"
#include "CraftingComponent.generated.h"

class UInventoryComponent;
class USurvivalItemCatalog;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingChanged);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingComponent();

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingChanged OnCraftingChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<USurvivalItemCatalog> ItemCatalog;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetItemCatalog(USurvivalItemCatalog* NewItemCatalog);

	UFUNCTION(BlueprintPure, Category = "Crafting")
	USurvivalItemCatalog* GetItemCatalog() const { return ItemCatalog; }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FCraftingRecipe> GetKnownRecipes() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FCraftingRecipe> GetCraftableRecipes() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraft(FName RecipeId) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CraftRecipe(FName RecipeId);

	UFUNCTION(BlueprintPure, Category = "Crafting")
	FText GetItemDisplayName(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	ESurvivalItemCategory GetItemCategory(FName ItemId) const;

protected:
	virtual void BeginPlay() override;

	const FCraftingRecipe* FindRecipe(FName RecipeId, FCraftingRecipe& OutFallbackRecipe) const;
	UInventoryComponent* GetOwnerInventory() const;
	TArray<FCraftingRecipe> GetDefaultRecipes() const;
	TArray<FItemDef> GetDefaultItems() const;
};
