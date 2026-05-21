#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SurvivalPlayerController.generated.h"

class UInputMappingContext;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API ASurvivalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 MappingPriority = 0;

protected:
	virtual void BeginPlay() override;
};
