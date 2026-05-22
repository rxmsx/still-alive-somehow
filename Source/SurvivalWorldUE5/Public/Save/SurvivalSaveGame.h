#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Survival/BodyConditionComponent.h"
#include "World/WorldTimeWeatherSubsystem.h"
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
	float PlayerHealth = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float PlayerHunger = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float PlayerThirst = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	float PlayerStamina = 100.0f;

	UPROPERTY(BlueprintReadWrite)
	FBodyConditionState BodyCondition;

	UPROPERTY(BlueprintReadWrite)
	FWorldTimeWeatherState WorldTimeWeather;

	UPROPERTY(BlueprintReadWrite)
	TMap<FName, int32> InventorySnapshot;

	UPROPERTY(BlueprintReadWrite)
	TArray<FResourceNodeSaveState> ResourceNodes;
};
