#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "OpenWorldPrototypeSettings.generated.h"

class USurvivalItemCatalog;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Open World Prototype"))
class SURVIVALWORLDUE5_API UOpenWorldPrototypeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World", meta = (ClampMin = "100.0"))
	float WorldSizeMeters = 2000.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World", meta = (ClampMin = "1"))
	int32 TargetBiomeCount = 3;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World", meta = (ClampMin = "1"))
	int32 TargetCaveEntrances = 3;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World", meta = (ClampMin = "1"))
	int32 TargetOreTypes = 3;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World")
	bool bUseWorldPartition = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World")
	bool bUsePCGForResourcePlacement = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<USurvivalItemCatalog> ItemCatalog;
};
