#include "UI/SurvivalHUDWidget.h"

#include "Crafting/CraftingComponent.h"
#include "Interfaces/Interactable.h"
#include "Items/InventoryComponent.h"
#include "Map/WorldMapSubsystem.h"
#include "Player/SurvivalCharacter.h"
#include "Player/SurvivalPlayerController.h"
#include "Survival/SurvivalStatsComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr float HudMargin = 28.0f;
	constexpr float MinimapOuterRadius = 108.0f;
	constexpr float MinimapRadius = 76.0f;
	constexpr float MinimapWorldRadius = 4500.0f;

	const FSlateBrush* GetWhiteBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	FSlateFontInfo GetFont(int32 Size, bool bBold = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size);
	}

	ASurvivalPlayerController* GetSurvivalController(const UUserWidget* Widget)
	{
		return Widget ? Cast<ASurvivalPlayerController>(Widget->GetOwningPlayer()) : nullptr;
	}

	ASurvivalCharacter* GetSurvivalCharacter(const UUserWidget* Widget)
	{
		const ASurvivalPlayerController* Controller = GetSurvivalController(Widget);
		return Controller ? Cast<ASurvivalCharacter>(Controller->GetPawn()) : nullptr;
	}

	void DrawBox(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position))),
			GetWhiteBrush(),
			ESlateDrawEffect::None,
			Color);
	}

	void DrawText(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FString& Text,
		const FVector2D& Position,
		const FLinearColor& Color,
		int32 Size = 16,
		bool bBold = false,
		float Width = 620.0f)
	{
		if (Text.IsEmpty())
		{
			return;
		}

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(FVector2f(Width, static_cast<float>(Size + 12)), FSlateLayoutTransform(FVector2f(Position))),
			Text,
			GetFont(Size, bBold),
			ESlateDrawEffect::None,
			Color);
	}

	void DrawLine(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Start,
		const FVector2D& End,
		const FLinearColor& Color,
		float Thickness = 1.0f)
	{
		TArray<FVector2D> Points;
		Points.Add(Start);
		Points.Add(End);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			Color,
			true,
			Thickness);
	}

	void DrawPolyline(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const TArray<FVector2D>& Points,
		const FLinearColor& Color,
		float Thickness = 1.0f)
	{
		if (Points.Num() < 2)
		{
			return;
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			Color,
			true,
			Thickness);
	}

	void DrawFilledCircle(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius,
		const FLinearColor& Color)
	{
		const int32 Rows = FMath::Max(8, FMath::RoundToInt(Radius * 1.4f));
		for (int32 Row = -Rows; Row <= Rows; ++Row)
		{
			const float NormalizedY = static_cast<float>(Row) / static_cast<float>(Rows);
			const float Y = NormalizedY * Radius;
			const float HalfWidth = FMath::Sqrt(FMath::Max(0.0f, (Radius * Radius) - (Y * Y)));
			DrawBox(
				Geometry,
				OutDrawElements,
				LayerId,
				FVector2D(Center.X - HalfWidth, Center.Y + Y),
				FVector2D(HalfWidth * 2.0f, 2.0f),
				Color);
		}
	}

	void DrawRingArc(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius,
		float StartDegrees,
		float EndDegrees,
		const FLinearColor& Color,
		float Thickness)
	{
		const float ArcLength = EndDegrees - StartDegrees;
		const int32 Segments = FMath::Max(3, FMath::CeilToInt(FMath::Abs(ArcLength) / 5.0f));
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);

		for (int32 SegmentIndex = 0; SegmentIndex <= Segments; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(Segments);
			const float Degrees = FMath::Lerp(StartDegrees, EndDegrees, Alpha);
			Points.Add(FVector2D(
				Center.X + FMath::Cos(FMath::DegreesToRadians(Degrees)) * Radius,
				Center.Y + FMath::Sin(FMath::DegreesToRadians(Degrees)) * Radius));
		}

		DrawPolyline(Geometry, OutDrawElements, LayerId, Points, Color, Thickness);
	}

	void DrawCircleOutline(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius,
		const FLinearColor& Color,
		float Thickness = 1.0f)
	{
		DrawRingArc(Geometry, OutDrawElements, LayerId, Center, Radius, 0.0f, 360.0f, Color, Thickness);
	}

	FVector2D WorldToMap(const FVector& WorldLocation, const FVector& PlayerLocation, const FVector2D& Center, float Radius, float WorldRadius = MinimapWorldRadius)
	{
		const FVector Delta = WorldLocation - PlayerLocation;
		return FVector2D(
			Center.X + FMath::Clamp(Delta.Y / WorldRadius, -1.0f, 1.0f) * Radius,
			Center.Y - FMath::Clamp(Delta.X / WorldRadius, -1.0f, 1.0f) * Radius);
	}

	FString CategoryLabel(ESurvivalItemCategory Category)
	{
		switch (Category)
		{
		case ESurvivalItemCategory::Resource:
			return TEXT("Rohstoffe");
		case ESurvivalItemCategory::Tool:
			return TEXT("Werkzeuge");
		case ESurvivalItemCategory::Food:
			return TEXT("Nahrung");
		case ESurvivalItemCategory::Building:
			return TEXT("Bauen");
		case ESurvivalItemCategory::Ore:
			return TEXT("Erze");
		default:
			return TEXT("Sonstiges");
		}
	}

	FLinearColor ItemAccentColor(FName ItemId, ESurvivalItemCategory Category)
	{
		if (ItemId == TEXT("Wood"))
		{
			return FLinearColor(0.24f, 0.58f, 0.28f, 1.0f);
		}
		if (ItemId == TEXT("Stone"))
		{
			return FLinearColor(0.70f, 0.70f, 0.64f, 1.0f);
		}
		if (ItemId == TEXT("StoneBlade"))
		{
			return FLinearColor(0.74f, 0.76f, 0.86f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Tool)
		{
			return FLinearColor(0.92f, 0.62f, 0.24f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Food)
		{
			return FLinearColor(0.82f, 0.22f, 0.20f, 1.0f);
		}
		return FLinearColor(0.42f, 0.58f, 0.72f, 1.0f);
	}

	void DrawMapBackground(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius)
	{
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center, Radius, FLinearColor(0.34f, 0.48f, 0.35f, 1.0f));
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center + FVector2D(-20.0f, 12.0f), Radius * 0.46f, FLinearColor(0.43f, 0.56f, 0.38f, 0.8f));
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center + FVector2D(26.0f, -22.0f), Radius * 0.38f, FLinearColor(0.27f, 0.43f, 0.34f, 0.72f));

		const FLinearColor RoadColor(0.78f, 0.73f, 0.56f, 0.72f);
		DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-62.0f, -8.0f), Center + FVector2D(-26.0f, 8.0f), RoadColor, 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-26.0f, 8.0f), Center + FVector2D(10.0f, -4.0f), RoadColor, 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(10.0f, -4.0f), Center + FVector2D(58.0f, -22.0f), RoadColor, 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-18.0f, 18.0f), Center + FVector2D(38.0f, 52.0f), RoadColor, 2.0f);

		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 2, Center, Radius, FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 3.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 3, TEXT("N"), Center + FVector2D(-5.0f, -Radius + 7.0f), FLinearColor::White, 12, true);
	}

	void DrawPlayerArrow(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		const ASurvivalCharacter* SurvivalCharacter)
	{
		if (!SurvivalCharacter)
		{
			return;
		}

		const FVector Forward3D = SurvivalCharacter->GetActorForwardVector();
		FVector2D Forward(Forward3D.Y, -Forward3D.X);
		Forward.Normalize();
		const FVector2D Right(-Forward.Y, Forward.X);

		const FVector2D Tip = Center + Forward * 15.0f;
		const FVector2D Left = Center - Forward * 10.0f - Right * 7.0f;
		const FVector2D RightPoint = Center - Forward * 10.0f + Right * 7.0f;

		DrawLine(Geometry, OutDrawElements, LayerId, Tip, Left, FLinearColor::White, 3.0f);
		DrawLine(Geometry, OutDrawElements, LayerId, Tip, RightPoint, FLinearColor::White, 3.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 1, Left, RightPoint, FLinearColor(0.1f, 0.25f, 1.0f, 1.0f), 4.0f);
	}

	void DrawStatusIcon(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Value,
		const FLinearColor& StatusColor,
		bool bDrawThirstIcon)
	{
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center, 17.0f, FLinearColor(0.06f, 0.08f, 0.09f, 0.94f));
		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 1, Center, 17.0f, FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 2.0f);
		DrawRingArc(Geometry, OutDrawElements, LayerId + 2, Center, 20.0f, -90.0f, -90.0f + (360.0f * FMath::Clamp(Value, 0.0f, 100.0f) / 100.0f), StatusColor, 3.0f);

		if (bDrawThirstIcon)
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(0.0f, -8.0f), Center + FVector2D(-6.0f, 2.0f), FLinearColor::White, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(0.0f, -8.0f), Center + FVector2D(6.0f, 2.0f), FLinearColor::White, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-6.0f, 2.0f), Center + FVector2D(0.0f, 9.0f), FLinearColor::White, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(6.0f, 2.0f), Center + FVector2D(0.0f, 9.0f), FLinearColor::White, 2.0f);
		}
		else
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-5.0f, -8.0f), Center + FVector2D(-5.0f, 8.0f), FLinearColor::White, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-9.0f, -8.0f), Center + FVector2D(-9.0f, -2.0f), FLinearColor::White, 1.5f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-1.0f, -8.0f), Center + FVector2D(-1.0f, -2.0f), FLinearColor::White, 1.5f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(6.0f, -8.0f), Center + FVector2D(9.0f, 8.0f), FLinearColor::White, 2.0f);
		}
	}

	void DrawHealthAndStaminaRings(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius,
		float Health,
		float Stamina)
	{
		const FLinearColor RingBackground(0.025f, 0.03f, 0.035f, 0.95f);
		DrawRingArc(Geometry, OutDrawElements, LayerId, Center, Radius, 104.0f, 256.0f, RingBackground, 14.0f);
		DrawRingArc(Geometry, OutDrawElements, LayerId, Center, Radius, -76.0f, 76.0f, RingBackground, 14.0f);
		DrawRingArc(Geometry, OutDrawElements, LayerId + 1, Center, Radius, 104.0f, 104.0f + (152.0f * FMath::Clamp(Health, 0.0f, 100.0f) / 100.0f), FLinearColor(0.92f, 0.08f, 0.12f, 1.0f), 12.0f);
		DrawRingArc(Geometry, OutDrawElements, LayerId + 1, Center, Radius, -76.0f, -76.0f + (152.0f * FMath::Clamp(Stamina, 0.0f, 100.0f) / 100.0f), FLinearColor(0.0f, 0.72f, 0.82f, 1.0f), 12.0f);
	}

	void DrawMapMarkers(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const UUserWidget* Widget,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& Center,
		float Radius,
		float WorldRadius = MinimapWorldRadius,
		bool bLargeMap = false)
	{
		if (!Widget || !SurvivalCharacter || !Widget->GetWorld())
		{
			return;
		}

		const UGameInstance* GameInstance = Widget->GetWorld()->GetGameInstance();
		const UWorldMapSubsystem* MapSubsystem = GameInstance ? GameInstance->GetSubsystem<UWorldMapSubsystem>() : nullptr;
		if (!MapSubsystem)
		{
			return;
		}

		const FVector PlayerLocation = SurvivalCharacter->GetActorLocation();
		const TArray<FMapMarkerSnapshot> Markers = bLargeMap
			? MapSubsystem->GetAllMarkers()
			: MapSubsystem->GetMarkersAround(PlayerLocation, WorldRadius);

		for (const FMapMarkerSnapshot& Marker : Markers)
		{
			const FVector2D MarkerPosition = WorldToMap(Marker.WorldLocation, PlayerLocation, Center, Radius, WorldRadius);
			if (FVector2D::Distance(MarkerPosition, Center) > Radius - 8.0f)
			{
				continue;
			}

			DrawFilledCircle(Geometry, OutDrawElements, LayerId, MarkerPosition, bLargeMap ? 6.0f : 4.0f, Marker.MarkerColor);
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 1, MarkerPosition, bLargeMap ? 6.0f : 4.0f, FLinearColor(0.02f, 0.02f, 0.02f, 1.0f), 1.0f);
		}
	}

	void DrawGameplayHud(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const UUserWidget* Widget,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& ViewSize)
	{
		if (!SurvivalCharacter || !SurvivalCharacter->SurvivalStatsComponent)
		{
			return;
		}

		const USurvivalStatsComponent* Stats = SurvivalCharacter->SurvivalStatsComponent;
		const FVector2D Center(HudMargin + MinimapOuterRadius, HudMargin + MinimapOuterRadius);
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center, MinimapOuterRadius + 9.0f, FLinearColor(0.015f, 0.018f, 0.022f, 0.76f));
		DrawMapBackground(Geometry, OutDrawElements, LayerId + 1, Center, MinimapRadius);
		DrawMapMarkers(Geometry, OutDrawElements, LayerId + 4, Widget, SurvivalCharacter, Center, MinimapRadius);
		DrawPlayerArrow(Geometry, OutDrawElements, LayerId + 7, Center, SurvivalCharacter);
		DrawHealthAndStaminaRings(Geometry, OutDrawElements, LayerId + 8, Center, MinimapRadius + 16.0f, Stats->Health, Stats->Stamina);
		DrawStatusIcon(Geometry, OutDrawElements, LayerId + 10, Center + FVector2D(44.0f, -110.0f), Stats->Hunger, FLinearColor(0.96f, 0.62f, 0.15f, 0.95f), false);
		DrawStatusIcon(Geometry, OutDrawElements, LayerId + 10, Center + FVector2D(84.0f, -84.0f), Stats->Thirst, FLinearColor(0.12f, 0.62f, 0.95f, 0.95f), true);

		if (SurvivalCharacter->FirstPersonCamera)
		{
			const FVector2D CrosshairCenter(ViewSize.X * 0.5f, ViewSize.Y * 0.5f);
			DrawLine(Geometry, OutDrawElements, LayerId + 11, CrosshairCenter + FVector2D(-6.0f, 0.0f), CrosshairCenter + FVector2D(6.0f, 0.0f), FLinearColor::White, 1.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 11, CrosshairCenter + FVector2D(0.0f, -6.0f), CrosshairCenter + FVector2D(0.0f, 6.0f), FLinearColor::White, 1.0f);

			const FVector TraceStart = SurvivalCharacter->FirstPersonCamera->GetComponentLocation();
			const FVector TraceEnd = TraceStart + SurvivalCharacter->FirstPersonCamera->GetForwardVector() * SurvivalCharacter->InteractionRange;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalHudWidgetInteractTrace), false, SurvivalCharacter);
			FHitResult HitResult;
			if (Widget && Widget->GetWorld() && Widget->GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
			{
				AActor* HitActor = HitResult.GetActor();
				if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
				{
					const bool bCanInteract = IInteractable::Execute_CanInteract(HitActor, SurvivalCharacter);
					const FText Prompt = IInteractable::Execute_GetInteractionPrompt(HitActor, SurvivalCharacter);
					const FString PromptText = FString::Printf(TEXT("F - %s"), *Prompt.ToString());
					const FLinearColor PromptColor = bCanInteract ? FLinearColor::White : FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);
					DrawText(Geometry, OutDrawElements, LayerId + 12, PromptText, CrosshairCenter + FVector2D(-80.0f, 24.0f), PromptColor, 16, true);
				}
			}
		}
	}

	void DrawInventoryStack(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size,
		const FString& Label,
		int32 Count,
		const FLinearColor& Accent)
	{
		const bool bCompact = Size.Y < 52.0f;
		const float Border = bCompact ? 2.0f : 3.0f;
		const float IconRadius = bCompact ? 11.0f : 16.0f;
		const float TextX = bCompact ? 38.0f : 52.0f;
		const int32 NameFont = bCompact ? 11 : 13;
		const int32 CountFont = bCompact ? 10 : 12;

		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(4.0f, 5.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.22f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.045f, 0.046f, 0.042f, 0.96f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position, FVector2D(5.0f, Size.Y), Accent.CopyWithNewOpacity(0.88f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(Border, Border), Size - FVector2D(Border * 2.0f, Border * 2.0f), FLinearColor(0.15f, 0.145f, 0.125f, 0.92f));

		const FVector2D IconCenter = Position + FVector2D(bCompact ? 21.0f : 28.0f, Size.Y * 0.5f);
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 4, IconCenter, IconRadius, Accent);
		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 5, IconCenter, IconRadius, FLinearColor(0.015f, 0.014f, 0.012f, 1.0f), 2.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 6, Label, Position + FVector2D(TextX, bCompact ? 8.0f : 12.0f), FLinearColor::White, NameFont, true, Size.X - TextX - 44.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 6, FString::Printf(TEXT("x%d"), Count), Position + FVector2D(Size.X - 36.0f, Size.Y - (bCompact ? 24.0f : 26.0f)), FLinearColor(0.86f, 0.84f, 0.74f, 1.0f), CountFont, true, 34.0f);
	}

	void DrawInventorySection(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FString& Title,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(5.0f, 7.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.18f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.035f, 0.036f, 0.033f, 0.72f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position, FVector2D(Size.X, 2.0f), FLinearColor(0.64f, 0.56f, 0.38f, 0.42f));
		DrawText(Geometry, OutDrawElements, LayerId + 3, Title, Position + FVector2D(14.0f, 10.0f), FLinearColor(0.88f, 0.84f, 0.74f, 1.0f), 13, true, Size.X - 28.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(12.0f, 38.0f), Position + FVector2D(Size.X - 12.0f, 38.0f), FLinearColor(0.64f, 0.56f, 0.38f, 0.52f), 1.5f);
	}

	FString IngredientListText(const FCraftingRecipe& Recipe, const UCraftingComponent* Crafting, const UInventoryComponent* Inventory)
	{
		TArray<FString> Parts;
		for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
		{
			const FString Name = Crafting ? Crafting->GetItemDisplayName(Ingredient.ItemId).ToString() : Ingredient.ItemId.ToString();
			const int32 Have = Inventory ? Inventory->GetItemCount(Ingredient.ItemId) : 0;
			Parts.Add(FString::Printf(TEXT("%s %d/%d"), *Name, Have, Ingredient.Count));
		}
		return FString::Join(Parts, TEXT("   "));
	}

	void DrawRecipeCard(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		const FVector2D& Position,
		const FVector2D& Size,
		const FCraftingRecipe& Recipe,
		const UCraftingComponent* Crafting,
		const UInventoryComponent* Inventory)
	{
		const bool bCanCraft = Crafting && Crafting->CanCraft(Recipe.RecipeId);
		const FLinearColor CardColor = bCanCraft
			? FLinearColor(0.075f, 0.105f, 0.090f, 0.96f)
			: FLinearColor(0.070f, 0.071f, 0.067f, 0.95f);
		const FLinearColor Accent = bCanCraft
			? FLinearColor(0.20f, 0.78f, 0.48f, 1.0f)
			: FLinearColor(0.72f, 0.59f, 0.34f, 1.0f);

		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(4.0f, 6.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.22f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, CardColor);
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position, FVector2D(5.0f, Size.Y), Accent);

		const FString DisplayName = !Recipe.DisplayName.IsEmpty() ? Recipe.DisplayName.ToString() : Recipe.RecipeId.ToString();
		const FString OutputName = Crafting ? Crafting->GetItemDisplayName(Recipe.OutputItemId).ToString() : Recipe.OutputItemId.ToString();
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(30.0f, 35.0f), 14.0f, Accent.CopyWithNewOpacity(0.85f));
		DrawText(Geometry, OutDrawElements, LayerId + 4, DisplayName, Position + FVector2D(54.0f, 12.0f), FLinearColor::White, 15, true, Size.X - 170.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 4, FString::Printf(TEXT("%s x%d"), *OutputName, Recipe.OutputCount), Position + FVector2D(54.0f, 36.0f), FLinearColor(0.88f, 0.84f, 0.70f, 1.0f), 12, false, Size.X - 170.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 4, IngredientListText(Recipe, Crafting, Inventory), Position + FVector2D(18.0f, Size.Y - 30.0f), FLinearColor(0.73f, 0.73f, 0.67f, 1.0f), 11, false, Size.X - 142.0f);

		const FVector2D ButtonSize(112.0f, 34.0f);
		const FVector2D ButtonPosition(Position.X + Size.X - ButtonSize.X - 14.0f, Position.Y + Size.Y - ButtonSize.Y - 12.0f);
		DrawBox(
			Geometry,
			OutDrawElements,
			LayerId + 4,
			ButtonPosition,
			ButtonSize,
			bCanCraft ? FLinearColor(0.17f, 0.52f, 0.32f, 1.0f) : FLinearColor(0.21f, 0.20f, 0.17f, 1.0f));
		DrawText(
			Geometry,
			OutDrawElements,
			LayerId + 5,
			bCanCraft ? TEXT("Craften") : TEXT("Material"),
			ButtonPosition + FVector2D(22.0f, 8.0f),
			FLinearColor::White,
			12,
			true,
			ButtonSize.X - 20.0f);

		if (bCanCraft)
		{
			USurvivalHUDWidget::FRecipeHitBox HitBox;
			HitBox.RecipeId = Recipe.RecipeId;
			HitBox.Bounds = FSlateRect(ButtonPosition.X, ButtonPosition.Y, ButtonPosition.X + ButtonSize.X, ButtonPosition.Y + ButtonSize.Y);
			RecipeHitBoxes.Add(HitBox);
		}
	}

	void DrawInventoryOverlay(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& ViewSize)
	{
		RecipeHitBoxes.Reset();
		if (!SurvivalCharacter || !SurvivalCharacter->InventoryComponent)
		{
			return;
		}

		const UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
		const UCraftingComponent* Crafting = SurvivalCharacter->CraftingComponent;
		const float PanelMarginX = FMath::Clamp(ViewSize.X * 0.045f, 38.0f, 72.0f);
		const float PanelMarginY = FMath::Clamp(ViewSize.Y * 0.070f, 46.0f, 72.0f);
		const FVector2D PanelPosition(PanelMarginX, PanelMarginY);
		const FVector2D PanelSize(FMath::Max(760.0f, ViewSize.X - PanelMarginX * 2.0f), FMath::Max(540.0f, ViewSize.Y - PanelMarginY * 2.0f - 18.0f));

		DrawBox(Geometry, OutDrawElements, LayerId, FVector2D::ZeroVector, ViewSize, FLinearColor(0.006f, 0.007f, 0.008f, 0.78f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, PanelPosition + FVector2D(9.0f, 12.0f), PanelSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.30f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, PanelPosition, PanelSize, FLinearColor(0.105f, 0.095f, 0.078f, 0.96f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, PanelPosition + FVector2D(14.0f, 14.0f), PanelSize - FVector2D(28.0f, 28.0f), FLinearColor(0.045f, 0.047f, 0.044f, 0.94f));
		DrawBox(Geometry, OutDrawElements, LayerId + 4, PanelPosition + FVector2D(24.0f, 24.0f), PanelSize - FVector2D(48.0f, 48.0f), FLinearColor(0.085f, 0.080f, 0.066f, 0.40f));
		DrawText(Geometry, OutDrawElements, LayerId + 5, TEXT("INVENTAR"), PanelPosition + FVector2D(34.0f, 28.0f), FLinearColor::White, 20, true, 280.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 5, TEXT("Tab / Esc schliessen"), PanelPosition + FVector2D(PanelSize.X - 190.0f, 34.0f), FLinearColor(0.76f, 0.74f, 0.66f, 1.0f), 13, false, 170.0f);

		const float TopY = PanelPosition.Y + 84.0f;
		const float BottomHeight = 108.0f;
		const float BottomY = PanelPosition.Y + PanelSize.Y - BottomHeight - 28.0f;
		const float ContentHeight = FMath::Max(280.0f, BottomY - TopY - 20.0f);
		const float CraftWidth = FMath::Clamp(PanelSize.X * 0.42f, 420.0f, 560.0f);
		const float SideGap = 30.0f;
		const float SideWidth = FMath::Max(210.0f, (PanelSize.X - CraftWidth - (SideGap * 4.0f)) * 0.5f);
		const FVector2D LeftSectionPosition(PanelPosition.X + SideGap, TopY);
		const FVector2D CraftPanelPosition(PanelPosition.X + (PanelSize.X - CraftWidth) * 0.5f, TopY);
		const FVector2D RightSectionPosition(PanelPosition.X + PanelSize.X - SideGap - SideWidth, TopY);
		const FVector2D BottomSectionPosition(PanelPosition.X + SideGap, BottomY);
		const FVector2D CraftPanelSize(CraftWidth, ContentHeight);

		DrawInventorySection(Geometry, OutDrawElements, LayerId + 5, TEXT("Rucksack"), LeftSectionPosition, FVector2D(SideWidth, ContentHeight));
		DrawInventorySection(Geometry, OutDrawElements, LayerId + 5, TEXT("Werkzeuge"), RightSectionPosition, FVector2D(SideWidth, ContentHeight));
		DrawInventorySection(Geometry, OutDrawElements, LayerId + 5, TEXT("Schnellablage"), BottomSectionPosition, FVector2D(PanelSize.X - SideGap * 2.0f, BottomHeight));

		DrawBox(Geometry, OutDrawElements, LayerId + 5, CraftPanelPosition + FVector2D(8.0f, 10.0f), CraftPanelSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.24f));
		DrawBox(Geometry, OutDrawElements, LayerId + 6, CraftPanelPosition, CraftPanelSize, FLinearColor(0.105f, 0.125f, 0.118f, 0.90f));
		DrawBox(Geometry, OutDrawElements, LayerId + 7, CraftPanelPosition + FVector2D(12.0f, 12.0f), CraftPanelSize - FVector2D(24.0f, 24.0f), FLinearColor(0.155f, 0.170f, 0.160f, 0.66f));
		DrawText(Geometry, OutDrawElements, LayerId + 8, TEXT("CRAFTING"), CraftPanelPosition + FVector2D(22.0f, 20.0f), FLinearColor(0.035f, 0.038f, 0.035f, 1.0f), 18, true, CraftPanelSize.X - 44.0f);

		TArray<FInventoryStack> Stacks = Inventory->GetSortedStacks();
		TArray<FInventoryStack> ToolStacks;
		TArray<FInventoryStack> OtherStacks;
		for (const FInventoryStack& Stack : Stacks)
		{
			const ESurvivalItemCategory Category = Crafting ? Crafting->GetItemCategory(Stack.ItemId) : ESurvivalItemCategory::Misc;
			if (Category == ESurvivalItemCategory::Tool)
			{
				ToolStacks.Add(Stack);
			}
			else
			{
				OtherStacks.Add(Stack);
			}
		}

		const FVector2D StackSize(SideWidth - 36.0f, 58.0f);
		for (int32 Index = 0; Index < OtherStacks.Num(); ++Index)
		{
			const FInventoryStack& Stack = OtherStacks[Index];
			const ESurvivalItemCategory Category = Crafting ? Crafting->GetItemCategory(Stack.ItemId) : ESurvivalItemCategory::Misc;
			const FString Label = Crafting ? Crafting->GetItemDisplayName(Stack.ItemId).ToString() : Stack.ItemId.ToString();
			DrawInventoryStack(
				Geometry,
				OutDrawElements,
				LayerId + 9,
				LeftSectionPosition + FVector2D(18.0f, 52.0f + Index * 68.0f),
				StackSize,
				Label,
				Stack.Count,
				ItemAccentColor(Stack.ItemId, Category));
		}

		for (int32 Index = 0; Index < ToolStacks.Num(); ++Index)
		{
			const FInventoryStack& Stack = ToolStacks[Index];
			const ESurvivalItemCategory Category = Crafting ? Crafting->GetItemCategory(Stack.ItemId) : ESurvivalItemCategory::Tool;
			const FString Label = Crafting ? Crafting->GetItemDisplayName(Stack.ItemId).ToString() : Stack.ItemId.ToString();
			DrawInventoryStack(
				Geometry,
				OutDrawElements,
				LayerId + 9,
				RightSectionPosition + FVector2D(18.0f, 52.0f + Index * 68.0f),
				FVector2D(SideWidth - 36.0f, 58.0f),
				Label,
				Stack.Count,
				ItemAccentColor(Stack.ItemId, Category));
		}

		for (int32 Index = 0; Index < Stacks.Num(); ++Index)
		{
			const FInventoryStack& Stack = Stacks[Index];
			const ESurvivalItemCategory Category = Crafting ? Crafting->GetItemCategory(Stack.ItemId) : ESurvivalItemCategory::Misc;
			const float SlotWidth = 132.0f;
			const float X = BottomSectionPosition.X + 18.0f + Index * (SlotWidth + 12.0f);
			if (X + SlotWidth > BottomSectionPosition.X + PanelSize.X - SideGap * 2.0f - 18.0f)
			{
				break;
			}

			DrawInventoryStack(
				Geometry,
				OutDrawElements,
				LayerId + 9,
				FVector2D(X, BottomSectionPosition.Y + 40.0f),
				FVector2D(SlotWidth, 46.0f),
				Crafting ? Crafting->GetItemDisplayName(Stack.ItemId).ToString() : Stack.ItemId.ToString(),
				Stack.Count,
				ItemAccentColor(Stack.ItemId, Category));
		}

		if (Crafting)
		{
			const TArray<FCraftingRecipe> Recipes = Crafting->GetKnownRecipes();
			const FVector2D RecipeStart = CraftPanelPosition + FVector2D(22.0f, 62.0f);
			const FVector2D RecipeSize(CraftPanelSize.X - 44.0f, 92.0f);
			for (int32 Index = 0; Index < Recipes.Num(); ++Index)
			{
				if (RecipeStart.Y + Index * (RecipeSize.Y + 12.0f) + RecipeSize.Y > CraftPanelPosition.Y + CraftPanelSize.Y - 18.0f)
				{
					break;
				}

				DrawRecipeCard(
					Geometry,
					OutDrawElements,
					LayerId + 9,
					RecipeHitBoxes,
					RecipeStart + FVector2D(0.0f, Index * (RecipeSize.Y + 12.0f)),
					RecipeSize,
					Recipes[Index],
					Crafting,
					Inventory);
			}
		}
	}

	void DrawLargeMapOverlay(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const UUserWidget* Widget,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& ViewSize)
	{
		if (!SurvivalCharacter)
		{
			return;
		}

		const FVector2D ScreenCenter(ViewSize.X * 0.5f, ViewSize.Y * 0.5f);
		const float Radius = FMath::Min(ViewSize.X, ViewSize.Y) * 0.34f;
		DrawBox(Geometry, OutDrawElements, LayerId, FVector2D::ZeroVector, ViewSize, FLinearColor(0.008f, 0.010f, 0.012f, 0.88f));
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, ScreenCenter, Radius + 24.0f, FLinearColor(0.02f, 0.026f, 0.028f, 0.96f));
		DrawMapBackground(Geometry, OutDrawElements, LayerId + 2, ScreenCenter, Radius);
		DrawMapMarkers(Geometry, OutDrawElements, LayerId + 5, Widget, SurvivalCharacter, ScreenCenter, Radius, MinimapWorldRadius * 3.5f, true);
		DrawPlayerArrow(Geometry, OutDrawElements, LayerId + 8, ScreenCenter, SurvivalCharacter);
		DrawText(Geometry, OutDrawElements, LayerId + 9, TEXT("WELTKARTE"), ScreenCenter + FVector2D(-56.0f, -Radius - 70.0f), FLinearColor::White, 20, true);
		DrawText(Geometry, OutDrawElements, LayerId + 9, TEXT("M / Esc schliessen"), ScreenCenter + FVector2D(-60.0f, Radius + 42.0f), FLinearColor(0.76f, 0.72f, 0.64f, 1.0f), 13);
	}
}

USurvivalHUDWidget::USurvivalHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

int32 USurvivalHUDWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	RecipeHitBoxes.Reset();

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const ASurvivalPlayerController* Controller = GetSurvivalController(this);
	const ASurvivalCharacter* SurvivalCharacter = GetSurvivalCharacter(this);

	if (!Controller || !SurvivalCharacter)
	{
		return CurrentLayer;
	}

	if (Controller->IsInventoryOpen())
	{
		DrawInventoryOverlay(AllottedGeometry, OutDrawElements, CurrentLayer + 1, RecipeHitBoxes, SurvivalCharacter, ViewSize);
		return CurrentLayer + 30;
	}

	if (Controller->IsMapOpen())
	{
		DrawLargeMapOverlay(AllottedGeometry, OutDrawElements, CurrentLayer + 1, this, SurvivalCharacter, ViewSize);
		return CurrentLayer + 20;
	}

	DrawGameplayHud(AllottedGeometry, OutDrawElements, CurrentLayer + 1, this, SurvivalCharacter, ViewSize);
	return CurrentLayer + 20;
}

FReply USurvivalHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ASurvivalPlayerController* Controller = GetSurvivalController(this);
	ASurvivalCharacter* SurvivalCharacter = GetSurvivalCharacter(this);
	if (!Controller || !Controller->IsInventoryOpen() || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D LocalMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	for (const FRecipeHitBox& HitBox : RecipeHitBoxes)
	{
		if (LocalMousePosition.X >= HitBox.Bounds.Left && LocalMousePosition.X <= HitBox.Bounds.Right
			&& LocalMousePosition.Y >= HitBox.Bounds.Top && LocalMousePosition.Y <= HitBox.Bounds.Bottom)
		{
			if (SurvivalCharacter && SurvivalCharacter->CraftingComponent)
			{
				SurvivalCharacter->CraftingComponent->CraftRecipe(HitBox.RecipeId);
			}
			return FReply::Handled();
		}
	}

	return FReply::Handled();
}
