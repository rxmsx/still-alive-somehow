#include "Resources/ResourceNodeComponent.h"
#include "Items/InventoryComponent.h"
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

	if (RequiredToolItemId.IsNone())
	{
		return true;
	}

	const UInventoryComponent* Inventory = HarvestingActor->FindComponentByClass<UInventoryComponent>();
	return Inventory && Inventory->HasItem(RequiredToolItemId);
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
	if (!Inventory || !Inventory->AddItem(OutputItemId, AmountPerHarvest))
	{
		return false;
	}

	RemainingHarvests = FMath::Max(0, RemainingHarvests - 1);
	OutItemId = OutputItemId;
	OutAmount = AmountPerHarvest;
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
	DOREPLIFETIME(UResourceNodeComponent, RequiredToolItemId);
	DOREPLIFETIME(UResourceNodeComponent, AmountPerHarvest);
	DOREPLIFETIME(UResourceNodeComponent, MaxHarvests);
	DOREPLIFETIME(UResourceNodeComponent, RemainingHarvests);
}
