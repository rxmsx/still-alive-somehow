#include "Building/StorageContainerActor.h"

#include "Items/InventoryComponent.h"
#include "Player/SurvivalPlayerController.h"

AStorageContainerActor::AStorageContainerActor()
{
	PartId = TEXT("StorageChest");
	StorageInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("StorageInventory"));
	StorageInventory->InventorySlotCount = 24;
	StorageInventory->HotbarSlotCount = 1;
	StorageInventory->MaxWeightKg = 120.0f;
}

FText AStorageContainerActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	return NSLOCTEXT("SurvivalWorld", "OpenStorageChestPrompt", "Lagerkiste oeffnen");
}

bool AStorageContainerActor::CanInteract_Implementation(const AActor* InteractingActor) const
{
	return !bIsPreviewActor && StorageInventory != nullptr;
}

bool AStorageContainerActor::Interact_Implementation(AActor* InteractingActor)
{
	if (!CanInteract_Implementation(InteractingActor))
	{
		return false;
	}

	if (APawn* Pawn = Cast<APawn>(InteractingActor))
	{
		if (ASurvivalPlayerController* Controller = Cast<ASurvivalPlayerController>(Pawn->GetController()))
		{
			Controller->OpenWorldInventory(this, StorageInventory, ECraftingStationType::None);
			return true;
		}
	}

	return false;
}
