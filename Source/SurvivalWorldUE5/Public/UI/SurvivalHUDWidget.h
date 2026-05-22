#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SurvivalHUDWidget.generated.h"

UCLASS()
class SURVIVALWORLDUE5_API USurvivalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	explicit USurvivalHUDWidget(const FObjectInitializer& ObjectInitializer);

	struct FRecipeHitBox
	{
		FName RecipeId = NAME_None;
		bool bCraftButton = false;
		FSlateRect Bounds;
	};

	struct FInventoryItemHitBox
	{
		FName ItemId = NAME_None;
		FString DisplayName;
		FString Category;
		FString Description;
		int32 Count = 0;
		FSlateRect Bounds;
	};

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	mutable TArray<FRecipeHitBox> RecipeHitBoxes;
	mutable TArray<FInventoryItemHitBox> InventoryItemHitBoxes;
	mutable FName HoveredItemId = NAME_None;
	mutable FName SelectedItemId = NAME_None;
	mutable FName SelectedRecipeId = NAME_None;
};
