#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Items/SurvivalItemTypes.h"
#include "SurvivalPlayerController.generated.h"

class UInventoryComponent;
class UInputMappingContext;

UENUM(BlueprintType)
enum class ESurvivalUIState : uint8
{
	None,
	Inventory,
	Map
};

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 MappingPriority = 0;

	UFUNCTION(BlueprintPure, Category = "UI")
	ESurvivalUIState GetUIState() const { return UIState; }

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsInventoryOpen() const { return UIState == ESurvivalUIState::Inventory; }

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsMapOpen() const { return UIState == ESurvivalUIState::Map; }

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsGameplayInputBlocked() const { return UIState != ESurvivalUIState::None; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleMap();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseOpenUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenWorldInventory(AActor* InventoryOwner, UInventoryComponent* WorldInventory, ECraftingStationType StationType = ECraftingStationType::None);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenCraftingStation(ECraftingStationType StationType, AActor* StationActor);

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void GiveItem(FName ItemId, int32 Count = 1);

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void SpawnResource(FName ResourceNodeId = NAME_None);

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void SaveSurvival();

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void LoadSurvival();

	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void SelectBuildPart(FName PartId);

	UFUNCTION(BlueprintPure, Category = "UI")
	UInventoryComponent* GetOpenedWorldInventory() const { return OpenedWorldInventory.Get(); }

	UFUNCTION(BlueprintPure, Category = "UI")
	AActor* GetOpenedWorldInventoryOwner() const { return OpenedWorldInventoryOwner.Get(); }

protected:
	virtual void BeginPlay() override;

	void SetUIState(ESurvivalUIState NewUIState);
	void ClearWorldInteractionContext();

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	ESurvivalUIState UIState = ESurvivalUIState::None;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInventoryComponent> OpenedWorldInventory;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<AActor> OpenedWorldInventoryOwner;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	ECraftingStationType OpenedCraftingStation = ECraftingStationType::None;
};
