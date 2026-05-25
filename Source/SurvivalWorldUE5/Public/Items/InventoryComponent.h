#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FInventoryStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;
};

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemId, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemId, int32 Count = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemId, int32 Count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TMap<FName, int32> GetInventorySnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryStack> GetSortedStacks() const;

	const TMap<FName, int32>& GetSnapshot() const { return ItemCounts; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSnapshot(const TMap<FName, int32>& NewItemCounts);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedStacks, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryStack> ReplicatedStacks;

	TMap<FName, int32> ItemCounts;

	UFUNCTION()
	void OnRep_ReplicatedStacks();

	void BroadcastInventoryChanged();
	void RebuildStacksFromMap();
	void RebuildMapFromStacks();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
