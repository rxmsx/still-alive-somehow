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

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FSurvivalStatsSaveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float Health = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float Hunger = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float Thirst = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float Stamina = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float TemperatureCelsius = 37.0f;

	UPROPERTY(BlueprintReadWrite)
	float Fatigue = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float Disease = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float Bleeding = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float Poison = 0.0f;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FBuildableActorSaveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadWrite)
	FString StableBuildId;

	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(BlueprintReadWrite)
	TArray<FInventoryStack> InventorySlotSnapshot;

	UPROPERTY(BlueprintReadWrite)
	bool bCampfireLit = false;

	UPROPERTY(BlueprintReadWrite)
	float CampfireFuelSeconds = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float CampfireCookSeconds = 0.0f;
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
	FSurvivalStatsSaveState PlayerStats;

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

	UPROPERTY(BlueprintReadWrite)
	TArray<FBuildableActorSaveState> BuiltActors;
};
