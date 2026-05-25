#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SurvivalGameMode.generated.h"

UCLASS()
class SURVIVALWORLDUE5_API ASurvivalGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASurvivalGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bSpawnMilestoneTestContent = true;

protected:
	virtual void BeginPlay() override;
};
