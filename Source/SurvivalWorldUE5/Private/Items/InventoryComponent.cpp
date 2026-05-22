#include "Items/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UInventoryComponent::AddItem(FName ItemId, int32 Count)
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	const int32 CurrentCount = ItemCounts.FindRef(ItemId);
	ItemCounts.Add(ItemId, CurrentCount + Count);
	RebuildStacksFromMap();
	BroadcastInventoryChanged();
	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (ItemId.IsNone() || Count <= 0 || !HasItem(ItemId, Count))
	{
		return false;
	}

	const int32 NewCount = ItemCounts.FindRef(ItemId) - Count;
	if (NewCount <= 0)
	{
		ItemCounts.Remove(ItemId);
	}
	else
	{
		ItemCounts.Add(ItemId, NewCount);
	}

	RebuildStacksFromMap();
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
	TArray<FInventoryStack> SortedStacks = ReplicatedStacks;
	SortedStacks.Sort([](const FInventoryStack& Left, const FInventoryStack& Right)
	{
		return Left.ItemId.LexicalLess(Right.ItemId);
	});
	return SortedStacks;
}

void UInventoryComponent::SetSnapshot(const TMap<FName, int32>& NewItemCounts)
{
	ItemCounts.Empty();
	for (const TPair<FName, int32>& Pair : NewItemCounts)
	{
		if (!Pair.Key.IsNone() && Pair.Value > 0)
		{
			ItemCounts.Add(Pair.Key, Pair.Value);
		}
	}

	RebuildStacksFromMap();
	BroadcastInventoryChanged();
}

void UInventoryComponent::ClearInventory()
{
	ItemCounts.Empty();
	ReplicatedStacks.Empty();
	BroadcastInventoryChanged();
}

void UInventoryComponent::OnRep_ReplicatedStacks()
{
	RebuildMapFromStacks();
	BroadcastInventoryChanged();
}

void UInventoryComponent::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::RebuildStacksFromMap()
{
	ReplicatedStacks.Empty();
	for (const TPair<FName, int32>& Pair : ItemCounts)
	{
		if (!Pair.Key.IsNone() && Pair.Value > 0)
		{
			FInventoryStack Stack;
			Stack.ItemId = Pair.Key;
			Stack.Count = Pair.Value;
			ReplicatedStacks.Add(Stack);
		}
	}
}

void UInventoryComponent::RebuildMapFromStacks()
{
	ItemCounts.Empty();
	for (const FInventoryStack& Stack : ReplicatedStacks)
	{
		if (!Stack.ItemId.IsNone() && Stack.Count > 0)
		{
			ItemCounts.Add(Stack.ItemId, Stack.Count);
		}
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, ReplicatedStacks);
}
