#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/InventoryComponent.h"
#include "Items/SurvivalItemTypes.h"
#include "CraftingComponent.generated.h"

class USurvivalItemCatalog;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeCrafted, FName, RecipeId, int32, OutputCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftingFailed, FName, RecipeId, FText, Message);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingChanged OnCraftingChanged;

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnRecipeCrafted OnRecipeCrafted;

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FOnCraftingFailed OnCraftingFailed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<USurvivalItemCatalog> ItemCatalog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting", meta = (ClampMin = "1"))
	int32 CraftingInputSlotCount = 6;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetItemCatalog(USurvivalItemCatalog* NewItemCatalog);

	UFUNCTION(BlueprintPure, Category = "Crafting")
	USurvivalItemCatalog* GetItemCatalog() const { return ItemCatalog; }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FCraftingRecipe> GetKnownRecipes() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FCraftingRecipe> GetVisibleRecipes(bool bIncludeLocked = true) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FCraftingRecipe> GetRecipesFiltered(const FString& SearchText, ECraftingRecipeCategory Category, bool bIncludeLocked = true) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FCraftingRecipe> GetCraftableRecipes() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraft(FName RecipeId) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	FCraftingValidationResult ValidateCraftRecipe(FName RecipeId) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CraftRecipe(FName RecipeId);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool UnlockRecipe(FName RecipeId);

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsRecipeKnown(FName RecipeId) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetActiveCraftingStation(ECraftingStationType NewStation);

	UFUNCTION(BlueprintPure, Category = "Crafting")
	ECraftingStationType GetActiveCraftingStation() const { return ActiveCraftingStation; }

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool AddInventorySlotToCrafting(int32 InventorySlotIndex, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool RemoveCraftingInput(int32 CraftingSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void ClearCraftingInputs();

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FInventoryStack> GetCraftingInputSlots() const { return CraftingInputSlots; }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	TMap<FName, int32> GetCraftingInputCounts() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool FindMatchingRecipeFromInputs(FCraftingRecipe& OutRecipe) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsCrafting() const { return !ActiveRecipeId.IsNone(); }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	float GetCraftingProgress() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	FText GetLastCraftingMessage() const { return LastCraftingMessage; }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsLastCraftingMessageError() const { return bLastCraftingMessageIsError; }

	UFUNCTION(BlueprintPure, Category = "Crafting")
	FText GetItemDisplayName(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	ESurvivalItemCategory GetItemCategory(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool GetItemDefinition(FName ItemId, FItemDef& OutItem) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TArray<FInventoryStack> CraftingInputSlots;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TArray<FName> UnlockedRecipeIds;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	ECraftingStationType ActiveCraftingStation = ECraftingStationType::None;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	FName ActiveRecipeId = NAME_None;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	float ActiveRecipeElapsedSeconds = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	float ActiveRecipeTotalSeconds = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	FText LastCraftingMessage;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingState, VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	bool bLastCraftingMessageIsError = false;

	UFUNCTION()
	void OnRep_CraftingState();

	const FCraftingRecipe* FindRecipe(FName RecipeId) const;
	UInventoryComponent* GetOwnerInventory() const;
	TArray<FCraftingRecipe> GetAllRecipes() const;
	bool HasRecipeIngredientsInInventory(const FCraftingRecipe& Recipe) const;
	bool HasRecipeIngredientsOnSurface(const FCraftingRecipe& Recipe) const;
	bool ConsumeRecipeIngredientsFromInventory(const FCraftingRecipe& Recipe);
	bool ConsumeRecipeIngredientsFromSurface(const FCraftingRecipe& Recipe);
	bool ConsumeRecipeIngredientsFromSlots(const FCraftingRecipe& Recipe, TArray<FInventoryStack>& Slots) const;
	bool CanInventoryAcceptRecipeOutput(const FCraftingRecipe& Recipe, bool bUseSurface) const;
	bool HasAnyCraftingInput() const;
	bool AddItemToCraftingSlots(FName ItemId, int32 Count, float Durability, float Freshness, TArray<FInventoryStack>& Slots) const;
	void EnsureCraftingSlots();
	void FinishActiveRecipe();
	void SetCraftingMessage(const FText& Message, bool bIsError);
	void FailCrafting(FName RecipeId, ECraftingFailureReason Reason, const FText& Message);
	const FItemDef* ResolveItemDefinition(FName ItemId) const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
