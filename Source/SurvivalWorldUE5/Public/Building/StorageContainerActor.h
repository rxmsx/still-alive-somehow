#pragma once

#include "CoreMinimal.h"
#include "Building/BuildableActor.h"
#include "StorageContainerActor.generated.h"

class UInventoryComponent;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API AStorageContainerActor : public ABuildableActor
{
	GENERATED_BODY()

public:
	AStorageContainerActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage")
	TObjectPtr<UInventoryComponent> StorageInventory;

	virtual FText GetInteractionPrompt_Implementation(const AActor* InteractingActor) const override;
	virtual bool CanInteract_Implementation(const AActor* InteractingActor) const override;
	virtual bool Interact_Implementation(AActor* InteractingActor) override;
};
