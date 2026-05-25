#include "Save/SurvivalSaveSubsystem.h"
#include "Save/SurvivalSaveGame.h"
#include "Items/InventoryComponent.h"
#include "Resources/ResourceNodeActor.h"
#include "Resources/ResourceNodeComponent.h"
#include "Survival/BodyConditionComponent.h"
#include "Survival/SurvivalStatsComponent.h"
#include "World/WorldTimeWeatherSubsystem.h"
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

	if (const UWorldTimeWeatherSubsystem* TimeWeatherSubsystem = GetGameInstance()->GetSubsystem<UWorldTimeWeatherSubsystem>())
	{
		SaveGame->WorldTimeWeather = TimeWeatherSubsystem->GetState();
	}

	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (const APawn* Pawn = PlayerController->GetPawn())
		{
			SaveGame->PlayerTransform = Pawn->GetActorTransform();
			if (const USurvivalStatsComponent* Stats = Pawn->FindComponentByClass<USurvivalStatsComponent>())
			{
				SaveGame->PlayerHealth = Stats->Health;
				SaveGame->PlayerHunger = Stats->Hunger;
				SaveGame->PlayerThirst = Stats->Thirst;
				SaveGame->PlayerStamina = Stats->Stamina;
			}
			if (const UBodyConditionComponent* BodyCondition = Pawn->FindComponentByClass<UBodyConditionComponent>())
			{
				SaveGame->BodyCondition = BodyCondition->GetBodyConditionState();
			}
			if (const UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
			{
				SaveGame->InventorySnapshot = Inventory->GetSnapshot();
				SaveGame->InventorySlotSnapshot = Inventory->GetSlotSnapshot();
				SaveGame->HotbarSlotSnapshot = Inventory->GetHotbarSnapshot();
				SaveGame->EquippedSlotIndex = Inventory->GetEquippedSlotIndex();
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

	if (UWorldTimeWeatherSubsystem* TimeWeatherSubsystem = GetGameInstance()->GetSubsystem<UWorldTimeWeatherSubsystem>())
	{
		TimeWeatherSubsystem->RestoreState(LastLoadedSave->WorldTimeWeather);
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			Pawn->SetActorTransform(LastLoadedSave->PlayerTransform);
			if (USurvivalStatsComponent* Stats = Pawn->FindComponentByClass<USurvivalStatsComponent>())
			{
				Stats->SetSurvivalStats(LastLoadedSave->PlayerHealth, LastLoadedSave->PlayerHunger, LastLoadedSave->PlayerThirst, LastLoadedSave->PlayerStamina);
			}
			if (UBodyConditionComponent* BodyCondition = Pawn->FindComponentByClass<UBodyConditionComponent>())
			{
				BodyCondition->RestoreBodyCondition(LastLoadedSave->BodyCondition);
			}
			if (UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
			{
				if (LastLoadedSave->InventorySlotSnapshot.Num() > 0)
				{
					Inventory->SetSlotSnapshot(LastLoadedSave->InventorySlotSnapshot, LastLoadedSave->HotbarSlotSnapshot, LastLoadedSave->EquippedSlotIndex);
				}
				else
				{
					Inventory->SetSnapshot(LastLoadedSave->InventorySnapshot);
				}
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
