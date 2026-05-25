#include "Resources/ResourceNodeComponent.h"
#include "Items/InventoryComponent.h"
#include "Items/ItemPickupActor.h"
#include "Player/SurvivalCharacter.h"
#include "World/WorldSeedSubsystem.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UResourceNodeComponent::UResourceNodeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UResourceNodeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (RemainingHarvests <= 0)
	{
		RemainingHarvests = MaxHarvests;
	}

	if (StableResourceId.IsEmpty())
	{
		const UWorld* World = GetWorld();
		const AActor* Owner = GetOwner();
		if (World && Owner && World->GetGameInstance())
		{
			if (const UWorldSeedSubsystem* SeedSubsystem = World->GetGameInstance()->GetSubsystem<UWorldSeedSubsystem>())
			{
				StableResourceId = SeedSubsystem->MakeStableId(OutputItemId, Owner->GetActorLocation());
			}
		}
	}
}

bool UResourceNodeComponent::CanHarvest(const AActor* HarvestingActor) const
{
	if (!HarvestingActor || IsDepleted() || OutputItemId.IsNone())
	{
		return false;
	}

	if (RequiredToolType == ESurvivalToolType::None)
	{
		return true;
	}

	const UInventoryComponent* Inventory = HarvestingActor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	float Efficiency = 0.0f;
	FName ToolItemId;
	return const_cast<UInventoryComponent*>(Inventory)->DamageEquippedTool(0.0f, RequiredToolType, bRequireMatchingTool, Efficiency, ToolItemId);
}

bool UResourceNodeComponent::Harvest(AActor* HarvestingActor, FName& OutItemId, int32& OutAmount)
{
	OutItemId = NAME_None;
	OutAmount = 0;

	if (!CanHarvest(HarvestingActor))
	{
		return false;
	}

	UInventoryComponent* Inventory = HarvestingActor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	float Efficiency = 1.0f;
	FName ToolItemId;
	if (!Inventory->DamageEquippedTool(1.0f, RequiredToolType, bRequireMatchingTool, Efficiency, ToolItemId))
	{
		return false;
	}

	const float Penalty = (RequiredToolType == ESurvivalToolType::None || ToolItemId != NAME_None) ? 1.0f : 0.35f;
	const int32 EffectiveAmount = FMath::Max(1, FMath::RoundToInt(static_cast<float>(AmountPerHarvest) * FMath::Max(0.1f, Efficiency) * Penalty));
	if (!Inventory->AddItem(OutputItemId, EffectiveAmount))
	{
		return false;
	}

	if (LootTable.Num() > 0)
	{
		for (const FResourceDropEntry& Drop : LootTable)
		{
			if (Drop.ItemId.IsNone()) continue;
			const int32 C = FMath::RandRange(Drop.MinCount, FMath::Max(Drop.MinCount, Drop.MaxCount));
			Inventory->AddItem(Drop.ItemId, C);
		}
	}

	RemainingHarvests = FMath::Max(0, RemainingHarvests - 1);
	OutItemId = OutputItemId;
	OutAmount = EffectiveAmount;
	OnResourceHarvested.Broadcast(RemainingHarvests);
	return true;
}

void UResourceNodeComponent::SetRemainingHarvests(int32 NewRemainingHarvests)
{
	RemainingHarvests = FMath::Clamp(NewRemainingHarvests, 0, MaxHarvests);
	OnResourceHarvested.Broadcast(RemainingHarvests);
}

void UResourceNodeComponent::OnRep_RemainingHarvests()
{
	OnResourceHarvested.Broadcast(RemainingHarvests);
}

void UResourceNodeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UResourceNodeComponent, StableResourceId);
	DOREPLIFETIME(UResourceNodeComponent, OutputItemId);
	DOREPLIFETIME(UResourceNodeComponent, RequiredToolType);
	DOREPLIFETIME(UResourceNodeComponent, bRequireMatchingTool);
	DOREPLIFETIME(UResourceNodeComponent, LootTable);
	DOREPLIFETIME(UResourceNodeComponent, AmountPerHarvest);
	DOREPLIFETIME(UResourceNodeComponent, MaxHarvests);
	DOREPLIFETIME(UResourceNodeComponent, RemainingHarvests);
}
