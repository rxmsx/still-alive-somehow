#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Items/InventoryComponent.h"
#include "SurvivalSaveGame.generated.h"

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FResourceNodeSaveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString StableResourceId;

	UPROPERTY(BlueprintReadWrite)
	int32 RemainingHarvests = 0;
};

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API USurvivalSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FString WorldName = TEXT("Neue Welt");

	UPROPERTY(BlueprintReadWrite)
	int32 WorldSeed = 1337;

	UPROPERTY(BlueprintReadWrite)
	FTransform PlayerTransform;

	UPROPERTY(BlueprintReadWrite)
	TMap<FName, int32> InventorySnapshot;

	UPROPERTY(BlueprintReadWrite)
	TArray<FInventoryStack> InventorySlotSnapshot;

	UPROPERTY(BlueprintReadWrite)
	TArray<int32> HotbarSlotSnapshot;

	UPROPERTY(BlueprintReadWrite)
	int32 EquippedSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	TArray<FResourceNodeSaveState> ResourceNodes;
};
