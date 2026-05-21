#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldSeedSubsystem.generated.h"

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API UWorldSeedSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "World Seed")
	void ConfigureWorld(const FString& InWorldName, int32 InWorldSeed);

	UFUNCTION(BlueprintPure, Category = "World Seed")
	FString GetWorldName() const { return WorldName; }

	UFUNCTION(BlueprintPure, Category = "World Seed")
	int32 GetWorldSeed() const { return WorldSeed; }

	UFUNCTION(BlueprintCallable, Category = "World Seed")
	void GenerateRandomSeed();

	UFUNCTION(BlueprintPure, Category = "World Seed")
	FRandomStream MakeStream(FName Channel, int32 Salt = 0) const;

	UFUNCTION(BlueprintPure, Category = "World Seed")
	FString MakeStableId(FName Category, const FVector& WorldLocation, int32 Salt = 0) const;

private:
	UPROPERTY()
	FString WorldName = TEXT("Neue Welt");

	UPROPERTY()
	int32 WorldSeed = 1337;
};
