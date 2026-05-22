#include "UI/SurvivalHUD.h"

#include "UI/SurvivalHUDWidget.h"
#include "Blueprint/UserWidget.h"

ASurvivalHUD::ASurvivalHUD()
{
	HUDWidgetClass = USurvivalHUDWidget::StaticClass();
}

void ASurvivalHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!HUDWidgetClass || !PlayerOwner)
	{
		return;
	}

	HUDWidget = CreateWidget<USurvivalHUDWidget>(PlayerOwner, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void ASurvivalHUD::DrawHUD()
{
	Super::DrawHUD();
}
