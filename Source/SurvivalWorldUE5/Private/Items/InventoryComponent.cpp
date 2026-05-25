#include "Items/InventoryComponent.h"

#include "Items/ItemPickupActor.h"
#include "Survival/SurvivalStatsComponent.h"
#include "World/OpenWorldPrototypeSettings.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalInventory, Log, All);

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::SetItemCatalog(USurvivalItemCatalog* NewItemCatalog)
{
	ItemCatalog = NewItemCatalog;
	BroadcastInventoryChanged();
}

bool UInventoryComponent::AddItem(FName ItemId, int32 Count)
{
	return AddItemWithState(ItemId, Count, GetSpawnDurabilityForItem(ItemId), 1.0f);
}

bool UInventoryComponent::AddItemWithState(FName ItemId, int32 Count, float Durability, float Freshness)
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	EnsureSlotArrays();
	TArray<FInventoryStack> CandidateSlots = ReplicatedStacks;
	if (!AddItemToSlots(ItemId, Count, Durability, Freshness, CandidateSlots))
	{
		return false;
	}

	ReplicatedStacks = CandidateSlots;
	SanitizeSlotIndices();
	RefreshCachedCounts();
	OnItemAdded.Broadcast(ItemId, Count, INDEX_NONE);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (ItemId.IsNone() || Count <= 0 || !HasItem(ItemId, Count))
	{
		return false;
	}

	EnsureSlotArrays();
	int32 Remaining = Count;
	for (FInventoryStack& Slot : ReplicatedStacks)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Slot.ItemId != ItemId || Slot.Count <= 0)
		{
			continue;
		}

		const int32 Removed = FMath::Min(Slot.Count, Remaining);
		Slot.Count -= Removed;
		Remaining -= Removed;
		if (Slot.Count <= 0)
		{
			Slot = FInventoryStack();
		}
	}

	SanitizeSlotIndices();
	RefreshCachedCounts();
	OnItemRemoved.Broadcast(ItemId, Count, INDEX_NONE);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::RemoveFromSlot(int32 SlotIndex, int32 Count)
{
	if (!IsValidInventorySlot(SlotIndex) || Count <= 0 || ReplicatedStacks[SlotIndex].IsEmpty())
	{
		return false;
	}

	FInventoryStack& Slot = ReplicatedStacks[SlotIndex];
	const int32 Removed = FMath::Min(Count, Slot.Count);
	const FName RemovedItemId = Slot.ItemId;
	Slot.Count -= Removed;
	if (Slot.Count <= 0)
	{
		Slot = FInventoryStack();
	}

	SanitizeSlotIndices();
	RefreshCachedCounts();
	OnItemRemoved.Broadcast(RemovedItemId, Removed, SlotIndex);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::HasItem(FName ItemId, int32 Count) const
{
	return !ItemId.IsNone() && Count > 0 && ItemCounts.FindRef(ItemId) >= Count;
}

int32 UInventoryComponent::GetItemCount(FName ItemId) const
{
	return ItemCounts.FindRef(ItemId);
}

TMap<FName, int32> UInventoryComponent::GetInventorySnapshot() const
{
	return ItemCounts;
}

TArray<FInventoryStack> UInventoryComponent::GetSortedStacks() const
{
	TArray<FInventoryStack> SortedStacks;
	for (const FInventoryStack& Slot : ReplicatedStacks)
	{
		if (!Slot.IsEmpty())
		{
			SortedStacks.Add(Slot);
		}
	}

	SortedStacks.Sort([this](const FInventoryStack& Left, const FInventoryStack& Right)
	{
		const FItemDef* LeftItem = ResolveItemDefinition(Left.ItemId);
		const FItemDef* RightItem = ResolveItemDefinition(Right.ItemId);
		const int32 LeftOrder = LeftItem ? LeftItem->SortOrder : 9999;
		const int32 RightOrder = RightItem ? RightItem->SortOrder : 9999;
		if (LeftOrder != RightOrder)
		{
			return LeftOrder < RightOrder;
		}
		return Left.ItemId.LexicalLess(Right.ItemId);
	});

	return SortedStacks;
}

TArray<FInventoryStack> UInventoryComponent::GetHotbarStacks() const
{
	TArray<FInventoryStack> HotbarStacks;
	HotbarStacks.Reserve(HotbarSlotCount);
	for (int32 Index = 0; Index < HotbarSlotCount; ++Index)
	{
		const int32 SlotIndex = HotbarSlotIndices.IsValidIndex(Index) ? HotbarSlotIndices[Index] : INDEX_NONE;
		HotbarStacks.Add(GetSlot(SlotIndex));
	}
	return HotbarStacks;
}

FInventoryStack UInventoryComponent::GetSlot(int32 SlotIndex) const
{
	return ReplicatedStacks.IsValidIndex(SlotIndex) ? ReplicatedStacks[SlotIndex] : FInventoryStack();
}

void UInventoryComponent::SetSnapshot(const TMap<FName, int32>& NewItemCounts)
{
	ReplicatedStacks.Empty();
	ReplicatedStacks.SetNum(FMath::Max(1, InventorySlotCount));
	HotbarSlotIndices.Empty();
	HotbarSlotIndices.SetNum(FMath::Max(1, HotbarSlotCount));
	for (int32& SlotIndex : HotbarSlotIndices)
	{
		SlotIndex = INDEX_NONE;
	}

	for (const TPair<FName, int32>& Pair : NewItemCounts)
	{
		if (!Pair.Key.IsNone() && Pair.Value > 0)
		{
			AddItemToSlots(Pair.Key, Pair.Value, GetSpawnDurabilityForItem(Pair.Key), 1.0f, ReplicatedStacks);
		}
	}

	SanitizeSlotIndices();
	RefreshCachedCounts();
	BroadcastInventoryChanged();
}

void UInventoryComponent::SetSlotSnapshot(const TArray<FInventoryStack>& NewSlots, const TArray<int32>& NewHotbarSlotIndices, int32 NewEquippedSlotIndex)
{
	ReplicatedStacks = NewSlots;
	ReplicatedStacks.SetNum(FMath::Max(1, InventorySlotCount));
	HotbarSlotIndices = NewHotbarSlotIndices;
	HotbarSlotIndices.SetNum(FMath::Max(1, HotbarSlotCount));
	EquippedSlotIndex = NewEquippedSlotIndex;
	SanitizeSlotIndices();
	RefreshCachedCounts();
	BroadcastInventoryChanged();
}

void UInventoryComponent::ClearInventory()
{
	ReplicatedStacks.Empty();
	ReplicatedStacks.SetNum(FMath::Max(1, InventorySlotCount));
	HotbarSlotIndices.Empty();
	HotbarSlotIndices.SetNum(FMath::Max(1, HotbarSlotCount));
	for (int32& SlotIndex : HotbarSlotIndices)
	{
		SlotIndex = INDEX_NONE;
	}
	EquippedSlotIndex = INDEX_NONE;
	RefreshCachedCounts();
	BroadcastInventoryChanged();
}

bool UInventoryComponent::MoveStack(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (!IsValidInventorySlot(FromSlotIndex) || !IsValidInventorySlot(ToSlotIndex) || FromSlotIndex == ToSlotIndex)
	{
		return false;
	}

	if (ReplicatedStacks[FromSlotIndex].IsEmpty())
	{
		return false;
	}

	if (!ReplicatedStacks[ToSlotIndex].IsEmpty() && ReplicatedStacks[ToSlotIndex].ItemId == ReplicatedStacks[FromSlotIndex].ItemId)
	{
		return MergeStack(FromSlotIndex, ToSlotIndex);
	}

	Swap(ReplicatedStacks[FromSlotIndex], ReplicatedStacks[ToSlotIndex]);
	SanitizeSlotIndices();
	RefreshCachedCounts();
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::MergeStack(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (!IsValidInventorySlot(FromSlotIndex) || !IsValidInventorySlot(ToSlotIndex) || FromSlotIndex == ToSlotIndex)
	{
		return false;
	}

	FInventoryStack& From = ReplicatedStacks[FromSlotIndex];
	FInventoryStack& To = ReplicatedStacks[ToSlotIndex];
	if (From.IsEmpty() || To.IsEmpty() || From.ItemId != To.ItemId)
	{
		return false;
	}

	const int32 MaxStack = GetMaxStackForItem(To.ItemId);
	const int32 Space = FMath::Max(0, MaxStack - To.Count);
	if (Space <= 0)
	{
		return false;
	}

	const int32 Moved = FMath::Min(Space, From.Count);
	const float TotalCount = static_cast<float>(To.Count + Moved);
	To.Durability = TotalCount > 0.0f ? ((To.Durability * To.Count) + (From.Durability * Moved)) / TotalCount : To.Durability;
	To.Freshness = TotalCount > 0.0f ? ((To.Freshness * To.Count) + (From.Freshness * Moved)) / TotalCount : To.Freshness;
	To.Count += Moved;
	From.Count -= Moved;
	if (From.Count <= 0)
	{
		From = FInventoryStack();
	}

	SanitizeSlotIndices();
	RefreshCachedCounts();
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::SplitStack(int32 FromSlotIndex, int32 ToSlotIndex, int32 Count)
{
	if (!IsValidInventorySlot(FromSlotIndex) || !IsValidInventorySlot(ToSlotIndex) || FromSlotIndex == ToSlotIndex || Count <= 0)
	{
		return false;
	}

	FInventoryStack& From = ReplicatedStacks[FromSlotIndex];
	FInventoryStack& To = ReplicatedStacks[ToSlotIndex];
	if (From.IsEmpty() || !To.IsEmpty() || From.Count <= Count)
	{
		return false;
	}

	To = From;
	To.Count = Count;
	From.Count -= Count;
	SanitizeSlotIndices();
	RefreshCachedCounts();
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::SplitStackHalf(int32 FromSlotIndex)
{
	if (!IsValidInventorySlot(FromSlotIndex) || ReplicatedStacks[FromSlotIndex].Count < 2)
	{
		return false;
	}

	const int32 EmptySlot = FindFirstEmptySlot(ReplicatedStacks);
	if (EmptySlot == INDEX_NONE)
	{
		return false;
	}

	return SplitStack(FromSlotIndex, EmptySlot, ReplicatedStacks[FromSlotIndex].Count / 2);
}

bool UInventoryComponent::DropSlot(int32 SlotIndex, int32 Count)
{
	if (!IsValidInventorySlot(SlotIndex) || ReplicatedStacks[SlotIndex].IsEmpty())
	{
		return false;
	}

	const int32 DropCount = Count <= 0 ? ReplicatedStacks[SlotIndex].Count : Count;
	FInventoryStack DroppedStack = ReplicatedStacks[SlotIndex];
	DroppedStack.Count = FMath::Min(DropCount, ReplicatedStacks[SlotIndex].Count);
	if (!SpawnDroppedItem(DroppedStack))
	{
		UE_LOG(LogSurvivalInventory, Warning, TEXT("DropSlot failed for %s x%d: could not spawn pickup actor."), *DroppedStack.ItemId.ToString(), DroppedStack.Count);
		return false;
	}

	if (!RemoveFromSlot(SlotIndex, DroppedStack.Count))
	{
		return false;
	}

	OnItemDropped.Broadcast(DroppedStack.ItemId, DroppedStack.Count, SlotIndex);
	return true;
}

bool UInventoryComponent::UseSlot(int32 SlotIndex)
{
	if (!IsValidInventorySlot(SlotIndex) || ReplicatedStacks[SlotIndex].IsEmpty())
	{
		return false;
	}

	FInventoryStack& Slot = ReplicatedStacks[SlotIndex];
	const FItemDef* ItemDef = ResolveItemDefinition(Slot.ItemId);
	if (!ItemDef)
	{
		return false;
	}

	if (ItemDef->UseType == ESurvivalItemUseType::Equip || ItemDef->bEquippable)
	{
		return EquipSlot(SlotIndex);
	}

	if (ItemDef->UseType != ESurvivalItemUseType::Consume && ItemDef->UseType != ESurvivalItemUseType::UseTool)
	{
		return false;
	}

	if (AActor* Owner = GetOwner())
	{
		if (USurvivalStatsComponent* Stats = Owner->FindComponentByClass<USurvivalStatsComponent>())
		{
			Stats->ApplyItemEffects(ItemDef->Effects);
		}
	}

	if (ItemDef->UseType == ESurvivalItemUseType::Consume)
	{
		const FName UsedItemId = Slot.ItemId;
		if (!RemoveFromSlot(SlotIndex, 1))
		{
			return false;
		}
		OnItemUsed.Broadcast(UsedItemId, 1, SlotIndex);
		return true;
	}

	if (ItemDef->bHasDurability)
	{
		Slot.Durability = FMath::Max(0.0f, Slot.Durability - 1.0f);
		if (Slot.Durability <= 0.0f)
		{
			Slot = FInventoryStack();
		}
	}

	SanitizeSlotIndices();
	RefreshCachedCounts();
	OnItemUsed.Broadcast(ItemDef->ItemId, 1, SlotIndex);
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::UseItem(FName ItemId)
{
	if (ItemId.IsNone())
	{
		return false;
	}

	for (const FInventoryStack& Slot : ReplicatedStacks)
	{
		if (Slot.ItemId == ItemId && Slot.Count > 0)
		{
			return UseSlot(Slot.SlotIndex);
		}
	}

	return false;
}

bool UInventoryComponent::EquipSlot(int32 SlotIndex)
{
	if (!IsValidInventorySlot(SlotIndex) || ReplicatedStacks[SlotIndex].IsEmpty())
	{
		return false;
	}

	const FItemDef* ItemDef = ResolveItemDefinition(ReplicatedStacks[SlotIndex].ItemId);
	if (!ItemDef || (!ItemDef->bEquippable && ItemDef->UseType != ESurvivalItemUseType::Equip && ItemDef->UseType != ESurvivalItemUseType::UseTool))
	{
		return false;
	}

	EquippedSlotIndex = SlotIndex;
	OnItemEquipped.Broadcast(ReplicatedStacks[SlotIndex].ItemId, ReplicatedStacks[SlotIndex].Count, SlotIndex);

	if (ItemDef->bHotbarAllowed)
	{
		for (int32 HotbarIndex = 0; HotbarIndex < HotbarSlotIndices.Num(); ++HotbarIndex)
		{
			if (HotbarSlotIndices[HotbarIndex] == SlotIndex)
			{
				BroadcastInventoryChanged();
				return true;
			}
		}

		AssignSlotToHotbar(SlotIndex, 0);
	}

	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::AssignSlotToHotbar(int32 SlotIndex, int32 HotbarIndex)
{
	if (!IsValidInventorySlot(SlotIndex) || !HotbarSlotIndices.IsValidIndex(HotbarIndex) || ReplicatedStacks[SlotIndex].IsEmpty())
	{
		return false;
	}

	const FItemDef* ItemDef = ResolveItemDefinition(ReplicatedStacks[SlotIndex].ItemId);
	if (ItemDef && !ItemDef->bHotbarAllowed)
	{
		return false;
	}

	HotbarSlotIndices[HotbarIndex] = SlotIndex;
	BroadcastInventoryChanged();
	return true;
}

float UInventoryComponent::GetCurrentWeightKg() const
{
	return CalculateWeightKgForSlots(ReplicatedStacks);
}

bool UInventoryComponent::CanAddItem(FName ItemId, int32 Count) const
{
	return !ItemId.IsNone() && Count > 0 && CanAddItemToSlots(ItemId, Count, ReplicatedStacks);
}

bool UInventoryComponent::CanAddItemToSlotSnapshot(FName ItemId, int32 Count, const TArray<FInventoryStack>& SlotSnapshot) const
{
	return !ItemId.IsNone() && Count > 0 && CanAddItemToSlots(ItemId, Count, SlotSnapshot);
}

bool UInventoryComponent::GetItemDefinition(FName ItemId, FItemDef& OutItem) const
{
	if (const FItemDef* ItemDef = ResolveItemDefinition(ItemId))
	{
		OutItem = *ItemDef;
		return true;
	}
	return false;
}

void UInventoryComponent::OnRep_InventoryState()
{
	SanitizeSlotIndices();
	RefreshCachedCounts();
	BroadcastInventoryChanged();
}

void UInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::EnsureSlotArrays()
{
	if (!ItemCatalog)
	{
		if (const UOpenWorldPrototypeSettings* Settings = GetDefault<UOpenWorldPrototypeSettings>())
		{
			ItemCatalog = Settings->ItemCatalog.LoadSynchronous();
		}
	}

	ReplicatedStacks.SetNum(FMath::Max(1, InventorySlotCount));
	HotbarSlotIndices.SetNum(FMath::Max(1, HotbarSlotCount));
	for (int32& HotbarSlotIndex : HotbarSlotIndices)
	{
		if (!ReplicatedStacks.IsValidIndex(HotbarSlotIndex))
		{
			HotbarSlotIndex = INDEX_NONE;
		}
	}
	SanitizeSlotIndices();
	RefreshCachedCounts();
}

void UInventoryComponent::RefreshCachedCounts()
{
	ItemCounts.Empty();
	for (const FInventoryStack& Slot : ReplicatedStacks)
	{
		if (!Slot.IsEmpty())
		{
			ItemCounts.FindOrAdd(Slot.ItemId) += Slot.Count;
		}
	}

	if (AActor* Owner = GetOwner())
	{
		if (USurvivalStatsComponent* Stats = Owner->FindComponentByClass<USurvivalStatsComponent>())
		{
			Stats->SetEncumbranceRatio(MaxWeightKg > 0.0f ? GetCurrentWeightKg() / MaxWeightKg : 0.0f);
		}
	}
}

bool UInventoryComponent::AddItemToSlots(FName ItemId, int32 Count, float Durability, float Freshness, TArray<FInventoryStack>& Slots) const
{
	if (!CanAddItemToSlots(ItemId, Count, Slots))
	{
		return false;
	}

	int32 Remaining = Count;
	const int32 MaxStack = GetMaxStackForItem(ItemId);
	for (FInventoryStack& Slot : Slots)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Slot.ItemId != ItemId || Slot.Count <= 0 || Slot.Count >= MaxStack)
		{
			continue;
		}

		const int32 Space = MaxStack - Slot.Count;
		const int32 Added = FMath::Min(Space, Remaining);
		const float TotalCount = static_cast<float>(Slot.Count + Added);
		Slot.Durability = TotalCount > 0.0f ? ((Slot.Durability * Slot.Count) + (Durability * Added)) / TotalCount : Durability;
		Slot.Freshness = TotalCount > 0.0f ? ((Slot.Freshness * Slot.Count) + (Freshness * Added)) / TotalCount : Freshness;
		Slot.Count += Added;
		Remaining -= Added;
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && Remaining > 0; ++SlotIndex)
	{
		if (!Slots[SlotIndex].IsEmpty())
		{
			continue;
		}

		const int32 Added = FMath::Min(MaxStack, Remaining);
		FInventoryStack NewStack;
		NewStack.ItemId = ItemId;
		NewStack.Count = Added;
		NewStack.Durability = Durability;
		NewStack.Freshness = FMath::Clamp(Freshness, 0.0f, 1.0f);
		NewStack.SlotIndex = SlotIndex;
		Slots[SlotIndex] = NewStack;
		Remaining -= Added;
	}

	return Remaining <= 0;
}

bool UInventoryComponent::CanAddItemToSlots(FName ItemId, int32 Count, const TArray<FInventoryStack>& Slots) const
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	const FItemDef* ItemDef = ResolveItemDefinition(ItemId);
	const float ItemWeight = ItemDef ? ItemDef->WeightKg : 0.1f;
	if (MaxWeightKg > 0.0f && CalculateWeightKgForSlots(Slots) + ItemWeight * Count > MaxWeightKg)
	{
		return false;
	}

	int32 Remaining = Count;
	const int32 MaxStack = GetMaxStackForItem(ItemId);
	for (const FInventoryStack& Slot : Slots)
	{
		if (Slot.ItemId == ItemId && Slot.Count > 0 && Slot.Count < MaxStack)
		{
			Remaining -= MaxStack - Slot.Count;
			if (Remaining <= 0)
			{
				return true;
			}
		}
	}

	for (const FInventoryStack& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Remaining -= MaxStack;
			if (Remaining <= 0)
			{
				return true;
			}
		}
	}

	return false;
}

float UInventoryComponent::CalculateWeightKgForSlots(const TArray<FInventoryStack>& Slots) const
{
	float TotalWeight = 0.0f;
	for (const FInventoryStack& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		const FItemDef* ItemDef = ResolveItemDefinition(Slot.ItemId);
		TotalWeight += (ItemDef ? ItemDef->WeightKg : 0.1f) * Slot.Count;
	}
	return TotalWeight;
}

int32 UInventoryComponent::FindFirstEmptySlot(const TArray<FInventoryStack>& Slots) const
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (Slots[SlotIndex].IsEmpty())
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

int32 UInventoryComponent::GetMaxStackForItem(FName ItemId) const
{
	if (const FItemDef* ItemDef = ResolveItemDefinition(ItemId))
	{
		return ItemDef->GetEffectiveMaxStack();
	}
	return 99;
}

float UInventoryComponent::GetSpawnDurabilityForItem(FName ItemId) const
{
	if (const FItemDef* ItemDef = ResolveItemDefinition(ItemId))
	{
		return ItemDef->GetSpawnDurability();
	}
	return 0.0f;
}

bool UInventoryComponent::IsValidInventorySlot(int32 SlotIndex) const
{
	return ReplicatedStacks.IsValidIndex(SlotIndex);
}

const FItemDef* UInventoryComponent::ResolveItemDefinition(FName ItemId) const
{
	if (ItemCatalog)
	{
		if (const FItemDef* ItemDef = ItemCatalog->FindItem(ItemId))
		{
			return ItemDef;
		}
	}
	return USurvivalItemCatalog::FindDefaultItem(ItemId);
}

bool UInventoryComponent::SpawnDroppedItem(const FInventoryStack& Stack) const
{
	if (Stack.IsEmpty())
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	const TSubclassOf<AItemPickupActor> PickupClass = DroppedItemActorClass ? DroppedItemActorClass.Get() : AItemPickupActor::StaticClass();
	const FVector Forward = Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
	const FVector SpawnLocation = Owner ? Owner->GetActorLocation() + Forward * DropDistance + FVector(0.0f, 0.0f, 35.0f) : FVector::ZeroVector;
	const FRotator SpawnRotation = Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = const_cast<AActor*>(Owner);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemPickupActor* Pickup = World->SpawnActor<AItemPickupActor>(PickupClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Pickup)
	{
		return false;
	}

	Pickup->InitializePickup(Stack, ItemCatalog);
	return true;
}

void UInventoryComponent::SanitizeSlotIndices()
{
	for (int32 SlotIndex = 0; SlotIndex < ReplicatedStacks.Num(); ++SlotIndex)
	{
		if (ReplicatedStacks[SlotIndex].IsEmpty())
		{
			ReplicatedStacks[SlotIndex] = FInventoryStack();
		}
		ReplicatedStacks[SlotIndex].SlotIndex = SlotIndex;
	}

	if (!ReplicatedStacks.IsValidIndex(EquippedSlotIndex) || ReplicatedStacks[EquippedSlotIndex].IsEmpty())
	{
		EquippedSlotIndex = INDEX_NONE;
	}

	for (int32& HotbarSlotIndex : HotbarSlotIndices)
	{
		if (!ReplicatedStacks.IsValidIndex(HotbarSlotIndex) || ReplicatedStacks[HotbarSlotIndex].IsEmpty())
		{
			HotbarSlotIndex = INDEX_NONE;
		}
	}
}



bool UInventoryComponent::DamageEquippedTool(float DurabilityDamage, ESurvivalToolType RequiredToolType, bool bRequireMatchingTool, float& OutEfficiency, FName& OutToolItemId)
{
	OutEfficiency = 0.0f;
	OutToolItemId = NAME_None;
	if (!IsValidInventorySlot(EquippedSlotIndex))
	{
		return !bRequireMatchingTool;
	}

	FInventoryStack& Slot = ReplicatedStacks[EquippedSlotIndex];
	if (Slot.IsEmpty())
	{
		return !bRequireMatchingTool;
	}

	const FItemDef* ItemDef = ResolveItemDefinition(Slot.ItemId);
	if (!ItemDef || !ItemDef->bIsTool || (RequiredToolType != ESurvivalToolType::None && ItemDef->ToolType != RequiredToolType))
	{
		return !bRequireMatchingTool;
	}

	if (ItemDef->bHasDurability && Slot.Durability <= 0.0f)
	{
		return false;
	}

	OutToolItemId = Slot.ItemId;
	OutEfficiency = FMath::Max(0.0f, ItemDef->HarvestEfficiency);
	if (ItemDef->bHasDurability)
	{
		Slot.Durability = FMath::Max(0.0f, Slot.Durability - FMath::Max(0.0f, DurabilityDamage));
		if (Slot.Durability <= 0.0f)
		{
			Slot.Durability = 0.0f;
		}
		BroadcastInventoryChanged();
	}
	return true;
}
void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, ReplicatedStacks);
	DOREPLIFETIME(UInventoryComponent, HotbarSlotIndices);
	DOREPLIFETIME(UInventoryComponent, EquippedSlotIndex);
}
