#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/SurvivalItemTypes.h"
#include "InventoryComponent.generated.h"

class USurvivalItemCatalog;
class AItemPickupActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemChanged, FName, ItemId, int32, Count, int32, SlotIndex);

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FInventoryStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Durability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Freshness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SlotIndex = INDEX_NONE;

	bool IsEmpty() const { return ItemId.IsNone() || Count <= 0; }
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FInventoryOperationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly)
	FText Message;
};

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChanged OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChanged OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChanged OnItemUsed;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChanged OnItemEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChanged OnItemDropped;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USurvivalItemCatalog> ItemCatalog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 InventorySlotCount = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 HotbarSlotCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0.0"))
	float MaxWeightKg = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|World")
	TSubclassOf<AItemPickupActor> DroppedItemActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|World", meta = (ClampMin = "25.0"))
	float DropDistance = 140.0f;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemCatalog(USurvivalItemCatalog* NewItemCatalog);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemId, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemWithState(FName ItemId, int32 Count, float Durability, float Freshness = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemId, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveFromSlot(int32 SlotIndex, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemId, int32 Count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TMap<FName, int32> GetInventorySnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryStack> GetSortedStacks() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryStack> GetSlots() const { return ReplicatedStacks; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryStack> GetHotbarStacks() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryStack GetSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryStack> GetSlotSnapshot() const { return ReplicatedStacks; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<int32> GetHotbarSnapshot() const { return HotbarSlotIndices; }

	const TMap<FName, int32>& GetSnapshot() const { return ItemCounts; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSnapshot(const TMap<FName, int32>& NewItemCounts);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSlotSnapshot(const TArray<FInventoryStack>& NewSlots, const TArray<int32>& NewHotbarSlotIndices, int32 NewEquippedSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveStack(int32 FromSlotIndex, int32 ToSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MergeStack(int32 FromSlotIndex, int32 ToSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStack(int32 FromSlotIndex, int32 ToSlotIndex, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStackHalf(int32 FromSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TransferSlotTo(UInventoryComponent* TargetInventory, int32 FromSlotIndex, int32 Count = -1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropSlot(int32 SlotIndex, int32 Count = -1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AssignSlotToHotbar(int32 SlotIndex, int32 HotbarIndex);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeightKg() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetMaxWeightKg() const { return MaxWeightKg; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(FName ItemId, int32 Count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItemToSlotSnapshot(FName ItemId, int32 Count, const TArray<FInventoryStack>& SlotSnapshot) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetItemDefinition(FName ItemId, FItemDef& OutItem) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEquippedSlotIndex() const { return EquippedSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryStack GetEquippedStack() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindUsableTool(ESurvivalToolType ToolType, FInventoryStack& OutToolStack, int32& OutSlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DamageItemDurability(int32 SlotIndex, float Amount, bool bKeepBrokenItem = true);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotUsable(int32 SlotIndex) const;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_InventoryState, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryStack> ReplicatedStacks;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<int32> HotbarSlotIndices;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 EquippedSlotIndex = INDEX_NONE;

	TMap<FName, int32> ItemCounts;

	UFUNCTION()
	void OnRep_InventoryState();

	void BroadcastInventoryChanged();
	void EnsureSlotArrays();
	void RefreshCachedCounts();
	bool AddItemToSlots(FName ItemId, int32 Count, float Durability, float Freshness, TArray<FInventoryStack>& Slots) const;
	bool CanAddItemToSlots(FName ItemId, int32 Count, const TArray<FInventoryStack>& Slots) const;
	float CalculateWeightKgForSlots(const TArray<FInventoryStack>& Slots) const;
	int32 FindFirstEmptySlot(const TArray<FInventoryStack>& Slots) const;
	int32 GetMaxStackForItem(FName ItemId) const;
	float GetSpawnDurabilityForItem(FName ItemId) const;
	bool IsValidInventorySlot(int32 SlotIndex) const;
	const FItemDef* ResolveItemDefinition(FName ItemId) const;
	bool SpawnDroppedItem(const FInventoryStack& Stack) const;
	void SanitizeSlotIndices();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
