#include "Save/SurvivalSaveSubsystem.h"
#include "Save/SurvivalSaveGame.h"
#include "Building/BuildableActor.h"
#include "Building/CampfireActor.h"
#include "Building/StorageContainerActor.h"
#include "Building/SurvivalBuildTypes.h"
#include "Items/InventoryComponent.h"
#include "Resources/ResourceNodeActor.h"
#include "Resources/ResourceNodeComponent.h"
#include "Survival/SurvivalStatsComponent.h"
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
				SaveGame->InventorySlotSnapshot = Inventory->GetSlotSnapshot();
				SaveGame->HotbarSlotSnapshot = Inventory->GetHotbarSnapshot();
				SaveGame->EquippedSlotIndex = Inventory->GetEquippedSlotIndex();
			}
			if (const USurvivalStatsComponent* Stats = Pawn->FindComponentByClass<USurvivalStatsComponent>())
			{
				SaveGame->PlayerStats.Health = Stats->Health;
				SaveGame->PlayerStats.Hunger = Stats->Hunger;
				SaveGame->PlayerStats.Thirst = Stats->Thirst;
				SaveGame->PlayerStats.Stamina = Stats->Stamina;
				SaveGame->PlayerStats.TemperatureCelsius = Stats->TemperatureCelsius;
				SaveGame->PlayerStats.Fatigue = Stats->Fatigue;
				SaveGame->PlayerStats.Disease = Stats->Disease;
				SaveGame->PlayerStats.Bleeding = Stats->Bleeding;
				SaveGame->PlayerStats.Poison = Stats->Poison;
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

	for (TActorIterator<ABuildableActor> It(World); It; ++It)
	{
		const ABuildableActor* Buildable = *It;
		if (!Buildable || Buildable->bIsPreviewActor || !Buildable->bWasPlacedByPlayer || Buildable->PartId.IsNone())
		{
			continue;
		}

		FBuildableActorSaveState State;
		State.PartId = Buildable->PartId;
		State.StableBuildId = Buildable->StableBuildId;
		State.Transform = Buildable->GetActorTransform();

		if (const AStorageContainerActor* Storage = Cast<AStorageContainerActor>(Buildable))
		{
			if (Storage->StorageInventory)
			{
				State.InventorySlotSnapshot = Storage->StorageInventory->GetSlotSnapshot();
			}
		}
		if (const ACampfireActor* Campfire = Cast<ACampfireActor>(Buildable))
		{
			if (Campfire->CampfireInventory)
			{
				State.InventorySlotSnapshot = Campfire->CampfireInventory->GetSlotSnapshot();
			}
			State.bCampfireLit = Campfire->bIsLit;
			State.CampfireFuelSeconds = Campfire->FuelSecondsRemaining;
			State.CampfireCookSeconds = Campfire->CurrentCookSeconds;
		}

		SaveGame->BuiltActors.Add(State);
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
				if (LastLoadedSave->InventorySlotSnapshot.Num() > 0)
				{
					Inventory->SetSlotSnapshot(LastLoadedSave->InventorySlotSnapshot, LastLoadedSave->HotbarSlotSnapshot, LastLoadedSave->EquippedSlotIndex);
				}
				else
				{
					Inventory->SetSnapshot(LastLoadedSave->InventorySnapshot);
				}
			}
			if (USurvivalStatsComponent* Stats = Pawn->FindComponentByClass<USurvivalStatsComponent>())
			{
				Stats->Health = LastLoadedSave->PlayerStats.Health;
				Stats->Hunger = LastLoadedSave->PlayerStats.Hunger;
				Stats->Thirst = LastLoadedSave->PlayerStats.Thirst;
				Stats->Stamina = LastLoadedSave->PlayerStats.Stamina;
				Stats->TemperatureCelsius = LastLoadedSave->PlayerStats.TemperatureCelsius;
				Stats->Fatigue = LastLoadedSave->PlayerStats.Fatigue;
				Stats->Disease = LastLoadedSave->PlayerStats.Disease;
				Stats->Bleeding = LastLoadedSave->PlayerStats.Bleeding;
				Stats->Poison = LastLoadedSave->PlayerStats.Poison;
				Stats->ApplyHealthDelta(0.0f);
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

	for (TActorIterator<ABuildableActor> It(World); It; ++It)
	{
		ABuildableActor* Buildable = *It;
		if (Buildable && Buildable->bWasPlacedByPlayer)
		{
			Buildable->Destroy();
		}
	}

	for (const FBuildableActorSaveState& State : LastLoadedSave->BuiltActors)
	{
		const FSurvivalBuildPartDef* BuildPart = USurvivalBuildCatalog::FindDefaultBuildPart(State.PartId);
		if (!BuildPart)
		{
			continue;
		}

		const TSubclassOf<ABuildableActor> ActorClass = BuildPart->BuildActorClass ? BuildPart->BuildActorClass.Get() : ABuildableActor::StaticClass();
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ABuildableActor* Buildable = World->SpawnActor<ABuildableActor>(ActorClass, State.Transform, SpawnParams);
		if (!Buildable)
		{
			continue;
		}

		Buildable->InitializeBuildable(State.PartId, nullptr, true);
		Buildable->StableBuildId = State.StableBuildId;
		Buildable->SetPreviewState(false, true);

		if (AStorageContainerActor* Storage = Cast<AStorageContainerActor>(Buildable))
		{
			if (Storage->StorageInventory)
			{
				Storage->StorageInventory->SetSlotSnapshot(State.InventorySlotSnapshot, TArray<int32>(), INDEX_NONE);
			}
		}
		if (ACampfireActor* Campfire = Cast<ACampfireActor>(Buildable))
		{
			if (Campfire->CampfireInventory)
			{
				Campfire->CampfireInventory->SetSlotSnapshot(State.InventorySlotSnapshot, TArray<int32>(), INDEX_NONE);
			}
			Campfire->SetCampfireState(State.bCampfireLit, State.CampfireFuelSeconds, State.CampfireCookSeconds);
		}
	}

	return true;
}

bool USurvivalSaveSubsystem::LoadCurrentWorldFromDefaultSlot()
{
	return LoadCurrentWorld(DefaultSaveSlotName, DefaultSaveUserIndex);
}
