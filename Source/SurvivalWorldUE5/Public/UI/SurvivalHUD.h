#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SurvivalHUD.generated.h"

class USurvivalHUDWidget;

UCLASS()
class SURVIVALWORLDUE5_API ASurvivalHUD : public AHUD
{
	GENERATED_BODY()

public:
	ASurvivalHUD();

	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USurvivalHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USurvivalHUDWidget> HUDWidget;
};
