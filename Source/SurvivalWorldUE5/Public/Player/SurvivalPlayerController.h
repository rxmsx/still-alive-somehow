#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"

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

protected:
	virtual void BeginPlay() override;

	void SetUIState(ESurvivalUIState NewUIState);

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	ESurvivalUIState UIState = ESurvivalUIState::None;
};
