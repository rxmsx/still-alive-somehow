#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SurvivalHUD.generated.h"

class ASurvivalCharacter;

UCLASS()
class SURVIVALWORLDUE5_API ASurvivalHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawSurvivalMinimap(const ASurvivalCharacter* SurvivalCharacter);
	void DrawInventorySummary(const ASurvivalCharacter* SurvivalCharacter);
	void DrawInventoryOverlay(const ASurvivalCharacter* SurvivalCharacter);
	void DrawInventorySlot(const FVector2D& Position, const FVector2D& Size, const FString& Label, int32 Count, const FLinearColor& AccentColor);
	void DrawCraftingPanel(const FVector2D& PanelPosition, const FVector2D& PanelSize);
	void DrawCrosshairAndPrompt(const ASurvivalCharacter* SurvivalCharacter);
	void DrawMapBackground(const FVector2D& Center, float Radius);
	void DrawMapResources(const FVector2D& Center, float Radius, const ASurvivalCharacter* SurvivalCharacter);
	void DrawPlayerArrow(const FVector2D& Center, const ASurvivalCharacter* SurvivalCharacter);
	void DrawStatusIcon(const FVector2D& Center, float Value, const FLinearColor& StatusColor, bool bDrawThirstIcon);
	void DrawFilledCircle(const FVector2D& Center, float Radius, const FLinearColor& Color);
	void DrawCircleOutline(const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness = 1.0f);
	void DrawRingArc(const FVector2D& Center, float Radius, float StartDegrees, float EndDegrees, const FLinearColor& Color, float Thickness);
	void DrawHealthAndStaminaRings(const FVector2D& Center, float Radius, float Health, float Stamina);
	FVector2D WorldToMap(const FVector& WorldLocation, const FVector& PlayerLocation, const FVector2D& Center, float Radius) const;
};
