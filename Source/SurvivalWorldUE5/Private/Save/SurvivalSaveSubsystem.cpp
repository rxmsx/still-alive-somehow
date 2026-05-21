#include "Save/SurvivalSaveSubsystem.h"
#include "Save/SurvivalSaveGame.h"
#include "Items/InventoryComponent.h"
#include "Resources/ResourceNodeActor.h"
#include "Resources/ResourceNodeComponent.h"
#include "World/WorldSeedSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 DefaultSaveUserIndex = 0;
	const TCHAR* DefaultSaveSlotName = TEXT("OpenWorldPrototype");
}

bool USurvivalSaveSubsystem::SaveCurrentWorld(const FString& SlotName, int32 UserIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	USurvivalSaveGame* SaveGame = Cast<USurvivalSaveGame>(UGameplayStatics::CreateSaveGameObject(USurvivalSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	if (const UWorldSeedSubsystem* SeedSubsystem = GetGameInstance()->GetSubsystem<UWorldSeedSubsystem>())
	{
		SaveGame->WorldName = SeedSubsystem->GetWorldName();
		SaveGame->WorldSeed = SeedSubsystem->GetWorldSeed();
	}

	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (const APawn* Pawn = PlayerController->GetPawn())
		{
			SaveGame->PlayerTransform = Pawn->GetActorTransform();
			if (const UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
			{
				SaveGame->InventorySnapshot = Inventory->GetSnapshot();
			}
		}
	}

	for (TActorIterator<AResourceNodeActor> It(World); It; ++It)
	{
		const AResourceNodeActor* NodeActor = *It;
		const UResourceNodeComponent* NodeComponent = NodeActor ? NodeActor->ResourceNodeComponent : nullptr;
		if (!NodeComponent || NodeComponent->StableResourceId.IsEmpty())
		{
			continue;
		}

		FResourceNodeSaveState State;
		State.StableResourceId = NodeComponent->StableResourceId;
		State.RemainingHarvests = NodeComponent->RemainingHarvests;
		SaveGame->ResourceNodes.Add(State);
	}

	LastLoadedSave = SaveGame;
	return UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
}

bool USurvivalSaveSubsystem::SaveCurrentWorldToDefaultSlot()
{
	return SaveCurrentWorld(DefaultSaveSlotName, DefaultSaveUserIndex);
}

bool USurvivalSaveSubsystem::LoadCurrentWorld(const FString& SlotName, int32 UserIndex)
{
	UWorld* World = GetWorld();
	if (!World || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	LastLoadedSave = Cast<USurvivalSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!LastLoadedSave)
	{
		return false;
	}

	if (UWorldSeedSubsystem* SeedSubsystem = GetGameInstance()->GetSubsystem<UWorldSeedSubsystem>())
	{
		SeedSubsystem->ConfigureWorld(LastLoadedSave->WorldName, LastLoadedSave->WorldSeed);
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			Pawn->SetActorTransform(LastLoadedSave->PlayerTransform);
			if (UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
			{
				Inventory->SetSnapshot(LastLoadedSave->InventorySnapshot);
			}
		}
	}

	TMap<FString, int32> ResourceStateById;
	for (const FResourceNodeSaveState& State : LastLoadedSave->ResourceNodes)
	{
		ResourceStateById.Add(State.StableResourceId, State.RemainingHarvests);
	}

	for (TActorIterator<AResourceNodeActor> It(World); It; ++It)
	{
		AResourceNodeActor* NodeActor = *It;
		UResourceNodeComponent* NodeComponent = NodeActor ? NodeActor->ResourceNodeComponent : nullptr;
		if (!NodeComponent)
		{
			continue;
		}

		if (const int32* RemainingHarvests = ResourceStateById.Find(NodeComponent->StableResourceId))
		{
			NodeComponent->SetRemainingHarvests(*RemainingHarvests);
		}
	}

	return true;
}

bool USurvivalSaveSubsystem::LoadCurrentWorldFromDefaultSlot()
{
	return LoadCurrentWorld(DefaultSaveSlotName, DefaultSaveUserIndex);
}
