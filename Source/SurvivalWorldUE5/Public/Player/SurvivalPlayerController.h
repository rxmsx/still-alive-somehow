#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"

class UInputMappingContext;

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

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryOpen() const { return bInventoryOpen; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

protected:
	virtual void BeginPlay() override;

	void SetInventoryOpen(bool bNewInventoryOpen);

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bInventoryOpen = false;
};
