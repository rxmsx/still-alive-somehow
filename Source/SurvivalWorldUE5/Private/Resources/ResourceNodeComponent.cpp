#include "Resources/ResourceNodeComponent.h"

#include "Items/InventoryComponent.h"
#include "Items/ItemPickupActor.h"
#include "World/WorldSeedSubsystem.h"
#include "Engine/World.h"
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

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = MaxHealth;
	}

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
				const FName StableType = !ResourceNodeId.IsNone() ? ResourceNodeId : OutputItemId;
				StableResourceId = SeedSubsystem->MakeStableId(StableType, Owner->GetActorLocation());
			}
		}
	}
}

void UResourceNodeComponent::ConfigureFromDefinition(const FResourceNodeDef& ResourceDef)
{
	ResourceNodeId = ResourceDef.ResourceNodeId;
	DisplayName = ResourceDef.DisplayName;
	OutputItemId = ResourceDef.OutputItemId;
	RequiredToolItemId = ResourceDef.RequiredToolItemId;
	RequiredToolType = ResourceDef.RequiredToolType;
	AlternativeToolTypes = ResourceDef.AlternativeToolTypes;
	bAllowBareHands = ResourceDef.bAllowBareHands;
	AmountPerHarvest = FMath::Max(1, ResourceDef.AmountPerHarvest);
	MaxHarvests = FMath::Max(1, ResourceDef.MaxHarvests);
	MaxHealth = FMath::Max(1.0f, ResourceDef.MaxHealth);
	CurrentHealth = MaxHealth;
	BaseHarvestDamage = FMath::Max(0.1f, ResourceDef.BaseHarvestDamage);
	WrongToolDamageMultiplier = FMath::Max(0.0f, ResourceDef.WrongToolDamageMultiplier);
	ToolDurabilityCost = FMath::Max(0.0f, ResourceDef.ToolDurabilityCost);
	Loot = ResourceDef.Loot;
	RemainingHarvests = MaxHarvests;
	OnResourceDamaged.Broadcast(CurrentHealth, MaxHealth);
	OnResourceHarvested.Broadcast(RemainingHarvests);
}

bool UResourceNodeComponent::CanHarvest(const AActor* HarvestingActor) const
{
	if (!HarvestingActor || IsDepleted())
	{
		return false;
	}

	if (Loot.Num() <= 0 && OutputItemId.IsNone())
	{
		return false;
	}

	int32 ToolSlotIndex = INDEX_NONE;
	FItemDef ToolDef;
	bool bHasTool = false;
	bool bCorrectTool = false;
	return ResolveHarvestTool(HarvestingActor, ToolSlotIndex, ToolDef, bHasTool, bCorrectTool);
}

bool UResourceNodeComponent::Harvest(AActor* HarvestingActor, FName& OutItemId, int32& OutAmount)
{
	OutItemId = NAME_None;
	OutAmount = 0;

	int32 ToolSlotIndex = INDEX_NONE;
	FItemDef ToolDef;
	bool bHasTool = false;
	bool bCorrectTool = false;
	if (!HarvestingActor || !ResolveHarvestTool(HarvestingActor, ToolSlotIndex, ToolDef, bHasTool, bCorrectTool) || IsDepleted())
	{
		return false;
	}

	UInventoryComponent* Inventory = HarvestingActor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	const float ToolDamage = bHasTool ? FMath::Max(0.1f, ToolDef.ToolDamage) * FMath::Max(0.1f, ToolDef.HarvestEfficiency) : 1.0f;
	const float ToolMultiplier = bCorrectTool ? 1.0f : WrongToolDamageMultiplier;
	const float AppliedDamage = FMath::Max(0.0f, BaseHarvestDamage * ToolDamage * ToolMultiplier);
	if (AppliedDamage <= 0.0f)
	{
		return false;
	}

	if (bHasTool && ToolSlotIndex != INDEX_NONE)
	{
		Inventory->DamageItemDurability(ToolSlotIndex, ToolDurabilityCost * FMath::Max(0.1f, ToolDef.DurabilityLossPerUse), true);
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - AppliedDamage);
	RefreshRemainingHarvestsFromHealth();
	const bool bDepleted = IsDepleted();

	bool bGrantedAnyLoot = false;
	for (const FResourceLootEntry& LootEntry : Loot)
	{
		if (LootEntry.bOnlyWhenDepleted && !bDepleted)
		{
			continue;
		}
		if (GrantOrDropLoot(HarvestingActor, LootEntry, bDepleted, OutItemId, OutAmount))
		{
			bGrantedAnyLoot = true;
		}
	}

	if (!bGrantedAnyLoot && !OutputItemId.IsNone())
	{
		FResourceLootEntry FallbackLoot;
		FallbackLoot.ItemId = OutputItemId;
		FallbackLoot.MinCount = AmountPerHarvest;
		FallbackLoot.MaxCount = AmountPerHarvest;
		FallbackLoot.Chance = 1.0f;
		bGrantedAnyLoot = GrantOrDropLoot(HarvestingActor, FallbackLoot, bDepleted, OutItemId, OutAmount);
	}

	OnResourceDamaged.Broadcast(CurrentHealth, MaxHealth);
	OnResourceHarvested.Broadcast(RemainingHarvests);
	return bGrantedAnyLoot;
}

void UResourceNodeComponent::SetRemainingHarvests(int32 NewRemainingHarvests)
{
	RemainingHarvests = FMath::Clamp(NewRemainingHarvests, 0, MaxHarvests);
	const float HealthAlpha = MaxHarvests > 0 ? static_cast<float>(RemainingHarvests) / static_cast<float>(MaxHarvests) : 0.0f;
	CurrentHealth = FMath::Clamp(MaxHealth * HealthAlpha, 0.0f, MaxHealth);
	OnResourceDamaged.Broadcast(CurrentHealth, MaxHealth);
	OnResourceHarvested.Broadcast(RemainingHarvests);
}

void UResourceNodeComponent::OnRep_RemainingHarvests()
{
	OnResourceHarvested.Broadcast(RemainingHarvests);
}

void UResourceNodeComponent::OnRep_Health()
{
	OnResourceDamaged.Broadcast(CurrentHealth, MaxHealth);
}

bool UResourceNodeComponent::ResolveHarvestTool(const AActor* HarvestingActor, int32& OutToolSlotIndex, FItemDef& OutToolDef, bool& bOutHasTool, bool& bOutCorrectTool) const
{
	OutToolSlotIndex = INDEX_NONE;
	OutToolDef = FItemDef();
	bOutHasTool = false;
	bOutCorrectTool = false;

	const UInventoryComponent* Inventory = HarvestingActor ? HarvestingActor->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return false;
	}

	if (!RequiredToolItemId.IsNone())
	{
		for (const FInventoryStack& Stack : Inventory->GetSlots())
		{
			if (Stack.ItemId != RequiredToolItemId || Stack.IsEmpty() || !Inventory->IsSlotUsable(Stack.SlotIndex))
			{
				continue;
			}

			Inventory->GetItemDefinition(Stack.ItemId, OutToolDef);
			OutToolSlotIndex = Stack.SlotIndex;
			bOutHasTool = true;
			bOutCorrectTool = true;
			return true;
		}

		if (!bAllowBareHands)
		{
			return false;
		}
	}

	TArray<ESurvivalToolType> PreferredToolTypes;
	if (RequiredToolType != ESurvivalToolType::None && RequiredToolType != ESurvivalToolType::Hand)
	{
		PreferredToolTypes.Add(RequiredToolType);
	}
	for (ESurvivalToolType AlternativeToolType : AlternativeToolTypes)
	{
		PreferredToolTypes.AddUnique(AlternativeToolType);
	}

	for (ESurvivalToolType ToolType : PreferredToolTypes)
	{
		FInventoryStack ToolStack;
		int32 ToolSlotIndex = INDEX_NONE;
		if (Inventory->FindUsableTool(ToolType, ToolStack, ToolSlotIndex) && Inventory->GetItemDefinition(ToolStack.ItemId, OutToolDef))
		{
			OutToolSlotIndex = ToolSlotIndex;
			bOutHasTool = true;
			bOutCorrectTool = IsToolTypeAccepted(OutToolDef.ToolType);
			return bOutCorrectTool || bAllowBareHands;
		}
	}

	if (RequiredToolType == ESurvivalToolType::None || RequiredToolType == ESurvivalToolType::Hand)
	{
		bOutCorrectTool = true;
		return true;
	}

	if (bAllowBareHands)
	{
		bOutCorrectTool = false;
		return true;
	}

	return false;
}

bool UResourceNodeComponent::IsToolTypeAccepted(ESurvivalToolType ToolType) const
{
	if (RequiredToolType == ESurvivalToolType::None || RequiredToolType == ESurvivalToolType::Hand)
	{
		return ToolType == RequiredToolType || AlternativeToolTypes.Contains(ToolType);
	}

	return ToolType == RequiredToolType || AlternativeToolTypes.Contains(ToolType);
}

bool UResourceNodeComponent::GrantOrDropLoot(AActor* HarvestingActor, const FResourceLootEntry& LootEntry, bool bIsDepletionLoot, FName& OutFirstItemId, int32& OutFirstAmount) const
{
	if (LootEntry.ItemId.IsNone() || LootEntry.MaxCount <= 0 || FMath::FRand() > LootEntry.Chance)
	{
		return false;
	}

	const int32 MinCount = FMath::Clamp(LootEntry.MinCount, 0, LootEntry.MaxCount);
	const int32 Count = FMath::RandRange(MinCount, LootEntry.MaxCount);
	if (Count <= 0)
	{
		return false;
	}

	UInventoryComponent* Inventory = HarvestingActor ? HarvestingActor->FindComponentByClass<UInventoryComponent>() : nullptr;
	const bool bAddedToInventory = Inventory && Inventory->AddItem(LootEntry.ItemId, Count);
	if (!bAddedToInventory && !SpawnLootPickup(HarvestingActor, LootEntry.ItemId, Count))
	{
		return false;
	}

	if (OutFirstItemId.IsNone())
	{
		OutFirstItemId = LootEntry.ItemId;
		OutFirstAmount = Count;
	}
	return true;
}

bool UResourceNodeComponent::SpawnLootPickup(AActor* HarvestingActor, FName ItemId, int32 Count) const
{
	const AActor* Owner = GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World || ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	const FVector AwayFromHarvester = HarvestingActor ? (Owner->GetActorLocation() - HarvestingActor->GetActorLocation()).GetSafeNormal2D() : FVector::ForwardVector;
	const FVector SpawnLocation = Owner->GetActorLocation() + AwayFromHarvester * 80.0f + FVector(0.0f, 0.0f, 45.0f);
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = const_cast<AActor*>(Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const TSubclassOf<AItemPickupActor> PickupClass = LootPickupActorClass ? LootPickupActorClass.Get() : AItemPickupActor::StaticClass();
	AItemPickupActor* Pickup = World->SpawnActor<AItemPickupActor>(PickupClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Pickup)
	{
		return false;
	}

	FInventoryStack Stack;
	Stack.ItemId = ItemId;
	Stack.Count = Count;
	Stack.Freshness = 1.0f;
	if (const UInventoryComponent* Inventory = HarvestingActor ? HarvestingActor->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		FItemDef ItemDef;
		if (Inventory->GetItemDefinition(ItemId, ItemDef))
		{
			Stack.Durability = ItemDef.GetSpawnDurability();
		}
	}
	Pickup->InitializePickup(Stack, nullptr);
	return true;
}

void UResourceNodeComponent::RefreshRemainingHarvestsFromHealth()
{
	if (MaxHealth <= 0.0f)
	{
		RemainingHarvests = 0;
		return;
	}

	const float HealthAlpha = FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
	RemainingHarvests = CurrentHealth <= 0.0f ? 0 : FMath::Max(1, FMath::CeilToInt(HealthAlpha * MaxHarvests));
}

void UResourceNodeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UResourceNodeComponent, StableResourceId);
	DOREPLIFETIME(UResourceNodeComponent, ResourceNodeId);
	DOREPLIFETIME(UResourceNodeComponent, DisplayName);
	DOREPLIFETIME(UResourceNodeComponent, OutputItemId);
	DOREPLIFETIME(UResourceNodeComponent, RequiredToolItemId);
	DOREPLIFETIME(UResourceNodeComponent, RequiredToolType);
	DOREPLIFETIME(UResourceNodeComponent, AlternativeToolTypes);
	DOREPLIFETIME(UResourceNodeComponent, bAllowBareHands);
	DOREPLIFETIME(UResourceNodeComponent, AmountPerHarvest);
	DOREPLIFETIME(UResourceNodeComponent, MaxHarvests);
	DOREPLIFETIME(UResourceNodeComponent, MaxHealth);
	DOREPLIFETIME(UResourceNodeComponent, CurrentHealth);
	DOREPLIFETIME(UResourceNodeComponent, BaseHarvestDamage);
	DOREPLIFETIME(UResourceNodeComponent, WrongToolDamageMultiplier);
	DOREPLIFETIME(UResourceNodeComponent, ToolDurabilityCost);
	DOREPLIFETIME(UResourceNodeComponent, Loot);
	DOREPLIFETIME(UResourceNodeComponent, RemainingHarvests);
}
