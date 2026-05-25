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
		case ESurvivalItemCategory::RawResource:
			return TEXT("Rohstoffe");
		case ESurvivalItemCategory::ProcessedMaterial:
			return TEXT("Materialien");
		case ESurvivalItemCategory::NaturalMaterial:
			return TEXT("Natur");
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
		if (Category == ESurvivalItemCategory::ProcessedMaterial)
		{
			return FLinearColor(0.66f, 0.58f, 0.40f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::NaturalMaterial)
		{
			return FLinearColor(0.34f, 0.66f, 0.42f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::RawResource)
		{
			return FLinearColor(0.48f, 0.54f, 0.50f, 1.0f);
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

	FString CompactIngredientListText(const FCraftingRecipe& Recipe, const UCraftingComponent* Crafting)
	{
		TArray<FString> Parts;
		for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
		{
			const FString Name = Crafting ? Crafting->GetItemDisplayName(Ingredient.ItemId).ToString() : Ingredient.ItemId.ToString();
			Parts.Add(FString::Printf(TEXT("%dx %s"), Ingredient.Count, *Name));
		}
		return FString::Join(Parts, TEXT("  "));
	}

	float StableNoise(FName ItemId, int32 Salt)
	{
		uint32 Hash = GetTypeHash(ItemId) ^ (0x9E3779B9u + static_cast<uint32>(Salt) * 0x85EBCA6Bu);
		Hash ^= Hash >> 16;
		Hash *= 0x7FEB352Du;
		Hash ^= Hash >> 15;
		Hash *= 0x846CA68Bu;
		Hash ^= Hash >> 16;
		return static_cast<float>(Hash & 0xFFFFu) / 65535.0f;
	}

	FLinearColor ScaleColor(const FLinearColor& Color, float Multiplier, float Alpha)
	{
		return FLinearColor(Color.R * Multiplier, Color.G * Multiplier, Color.B * Multiplier, Alpha);
	}

	bool IsToolLike(FName ItemId, ESurvivalItemCategory Category)
	{
		return Category == ESurvivalItemCategory::Tool
			|| Category == ESurvivalItemCategory::Building
			|| ItemId == TEXT("Bow")
			|| ItemId == TEXT("Arrow")
			|| ItemId == TEXT("Axe")
			|| ItemId == TEXT("Pickaxe")
			|| ItemId == TEXT("FishingRod")
			|| ItemId == TEXT("IronKnife");
	}

	FVector2D PhysicalItemSize(FName ItemId, ESurvivalItemCategory Category)
	{
		if (ItemId == TEXT("Bow") || ItemId == TEXT("FishingRod"))
		{
			return FVector2D(116.0f, 42.0f);
		}
		if (ItemId == TEXT("Axe") || ItemId == TEXT("Pickaxe") || ItemId == TEXT("IronKnife") || ItemId == TEXT("Arrow"))
		{
			return FVector2D(92.0f, 38.0f);
		}
		if (ItemId == TEXT("Campfire"))
		{
			return FVector2D(94.0f, 68.0f);
		}
		if (Category == ESurvivalItemCategory::Food)
		{
			return FVector2D(62.0f, 52.0f);
		}
		if (Category == ESurvivalItemCategory::RawResource || Category == ESurvivalItemCategory::ProcessedMaterial)
		{
			return FVector2D(76.0f, 48.0f);
		}
		return FVector2D(64.0f, 48.0f);
	}

	void DrawFoldedClothSurface(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size,
		float TimeSeconds)
	{
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(14.0f, 18.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.30f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.045f, 0.052f, 0.046f, 0.98f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(16.0f, 14.0f), Size - FVector2D(32.0f, 28.0f), FLinearColor(0.078f, 0.088f, 0.072f, 0.92f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(34.0f, 32.0f), Size - FVector2D(68.0f, 64.0f), FLinearColor(0.030f, 0.034f, 0.031f, 0.42f));

		const FLinearColor SeamColor(0.18f, 0.20f, 0.16f, 0.80f);
		DrawLine(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(22.0f, 26.0f), Position + FVector2D(Size.X - 24.0f, 22.0f), SeamColor, 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(20.0f, Size.Y - 26.0f), Position + FVector2D(Size.X - 28.0f, Size.Y - 34.0f), SeamColor, 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(24.0f, 28.0f), Position + FVector2D(32.0f, Size.Y - 30.0f), SeamColor, 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(Size.X - 26.0f, 28.0f), Position + FVector2D(Size.X - 38.0f, Size.Y - 28.0f), SeamColor, 2.0f);

		for (int32 FoldIndex = 0; FoldIndex < 9; ++FoldIndex)
		{
			const float Alpha = static_cast<float>(FoldIndex + 1) / 10.0f;
			const float Wave = FMath::Sin(TimeSeconds * 0.55f + FoldIndex * 0.84f) * 4.0f;
			const FVector2D Start = Position + FVector2D(Size.X * Alpha, 44.0f + Wave);
			const FVector2D End = Position + FVector2D(Size.X * Alpha + 24.0f, Size.Y - 46.0f - Wave);
			DrawLine(Geometry, OutDrawElements, LayerId + 5, Start, End, FLinearColor(0.14f, 0.16f, 0.13f, 0.22f), 1.0f);
		}

		for (int32 StitchIndex = 0; StitchIndex < 22; ++StitchIndex)
		{
			const float X = Position.X + 42.0f + StitchIndex * ((Size.X - 84.0f) / 21.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 6, FVector2D(X, Position.Y + 19.0f), FVector2D(X + 8.0f, Position.Y + 19.0f), FLinearColor(0.42f, 0.38f, 0.26f, 0.36f), 1.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 6, FVector2D(X, Position.Y + Size.Y - 24.0f), FVector2D(X + 8.0f, Position.Y + Size.Y - 24.0f), FLinearColor(0.42f, 0.38f, 0.26f, 0.30f), 1.0f);
		}
	}

	void DrawOpenBackpack(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(10.0f, 16.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.26f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.052f, 0.047f, 0.036f, 0.94f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(18.0f, 18.0f), Size - FVector2D(36.0f, 36.0f), FLinearColor(0.018f, 0.020f, 0.018f, 0.82f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(12.0f, 18.0f), FVector2D(10.0f, Size.Y - 36.0f), FLinearColor(0.18f, 0.15f, 0.10f, 0.86f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(Size.X - 22.0f, 18.0f), FVector2D(10.0f, Size.Y - 36.0f), FLinearColor(0.18f, 0.15f, 0.10f, 0.86f));
		DrawLine(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(22.0f, 36.0f), Position + FVector2D(Size.X - 24.0f, 30.0f), FLinearColor(0.62f, 0.55f, 0.38f, 0.46f), 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(26.0f, Size.Y - 42.0f), Position + FVector2D(Size.X - 28.0f, Size.Y - 36.0f), FLinearColor(0.62f, 0.55f, 0.38f, 0.36f), 2.0f);
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 5, Position + FVector2D(Size.X * 0.22f, Size.Y * 0.18f), 12.0f, FLinearColor(0.45f, 0.39f, 0.24f, 0.70f));
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 5, Position + FVector2D(Size.X * 0.78f, Size.Y * 0.18f), 12.0f, FLinearColor(0.45f, 0.39f, 0.24f, 0.70f));
	}

	void DrawPhysicalItemObject(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		const FVector2D& Size,
		FName ItemId,
		ESurvivalItemCategory Category,
		const FLinearColor& Accent,
		bool bHovered)
	{
		const FVector2D ItemPosition = Center - Size * 0.5f;
		const FLinearColor Body = ScaleColor(Accent, bHovered ? 0.88f : 0.58f, 0.98f);
		const FLinearColor DarkEdge = ScaleColor(Accent, 0.28f, 0.92f);
		const FLinearColor Highlight = ScaleColor(Accent, 1.34f, 0.72f);

		DrawBox(Geometry, OutDrawElements, LayerId, ItemPosition + FVector2D(8.0f, 11.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, bHovered ? 0.36f : 0.24f));

		if (ItemId == TEXT("Bow"))
		{
			DrawRingArc(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-8.0f, 0.0f), Size.X * 0.42f, -56.0f, 56.0f, Body, 4.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(28.0f, -34.0f), Center + FVector2D(28.0f, 34.0f), FLinearColor(0.82f, 0.78f, 0.60f, 0.92f), 1.2f);
		}
		else if (ItemId == TEXT("FishingRod"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(10.0f, Size.Y - 10.0f), ItemPosition + FVector2D(Size.X - 12.0f, 8.0f), Body, 4.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(Size.X - 18.0f, 11.0f), ItemPosition + FVector2D(Size.X - 5.0f, 24.0f), FLinearColor(0.76f, 0.75f, 0.66f, 0.70f), 1.0f);
			DrawRingArc(Geometry, OutDrawElements, LayerId + 3, ItemPosition + FVector2D(Size.X - 3.0f, 29.0f), 8.0f, 90.0f, 250.0f, FLinearColor(0.76f, 0.75f, 0.66f, 0.70f), 1.5f);
		}
		else if (ItemId == TEXT("Axe"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(14.0f, Size.Y - 9.0f), ItemPosition + FVector2D(Size.X - 20.0f, 10.0f), Body, 5.0f);
			DrawBox(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(Size.X - 34.0f, 5.0f), FVector2D(28.0f, 22.0f), FLinearColor(0.55f, 0.55f, 0.50f, 0.95f));
			DrawLine(Geometry, OutDrawElements, LayerId + 3, ItemPosition + FVector2D(Size.X - 32.0f, 9.0f), ItemPosition + FVector2D(Size.X - 8.0f, 24.0f), Highlight, 1.0f);
		}
		else if (ItemId == TEXT("Pickaxe"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(18.0f, Size.Y - 8.0f), ItemPosition + FVector2D(Size.X - 26.0f, 10.0f), Body, 5.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(Size.X - 54.0f, 8.0f), ItemPosition + FVector2D(Size.X - 4.0f, 17.0f), FLinearColor(0.58f, 0.58f, 0.54f, 0.95f), 5.0f);
		}
		else if (ItemId == TEXT("IronKnife") || ItemId == TEXT("StoneBlade") || ItemId == TEXT("Arrow"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(10.0f, Size.Y * 0.58f), ItemPosition + FVector2D(Size.X - 12.0f, Size.Y * 0.40f), ItemId == TEXT("Arrow") ? Body : FLinearColor(0.66f, 0.66f, 0.60f, 0.96f), 4.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(Size.X - 24.0f, Size.Y * 0.34f), ItemPosition + FVector2D(Size.X - 8.0f, Size.Y * 0.40f), Highlight, 1.2f);
			DrawBox(Geometry, OutDrawElements, LayerId + 3, ItemPosition + FVector2D(7.0f, Size.Y * 0.54f), FVector2D(20.0f, 7.0f), DarkEdge);
		}
		else if (ItemId == TEXT("Campfire"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-28.0f, 18.0f), Center + FVector2D(28.0f, -16.0f), Body, 8.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-30.0f, -15.0f), Center + FVector2D(30.0f, 17.0f), Body, 8.0f);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 2, Center, 18.0f, FLinearColor(0.86f, 0.34f, 0.13f, 0.42f));
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(0.0f, -2.0f), 9.0f, FLinearColor(1.0f, 0.72f, 0.20f, 0.50f));
		}
		else if (ItemId == TEXT("Rope"))
		{
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 1, Center, Size.Y * 0.34f, Body, 4.0f);
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(7.0f, 2.0f), Size.Y * 0.25f, DarkEdge, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-18.0f, 14.0f), Center + FVector2D(22.0f, 16.0f), Highlight, 1.0f);
		}
		else if (ItemId == TEXT("Stone") || ItemId == TEXT("Flint") || ItemId == TEXT("OreChunk") || ItemId == TEXT("CopperOre") || ItemId == TEXT("IronOre") || ItemId == TEXT("Coal"))
		{
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-12.0f, 4.0f), Size.Y * 0.34f, Body);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(10.0f, -3.0f), Size.Y * 0.28f, ScaleColor(Accent, 0.48f, 0.95f));
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-21.0f, -2.0f), Center + FVector2D(-4.0f, -11.0f), Highlight, 1.0f);
		}
		else if (ItemId == TEXT("Feather"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(8.0f, Size.Y - 8.0f), ItemPosition + FVector2D(Size.X - 8.0f, 9.0f), Body, 2.0f);
			for (int32 BarbIndex = 0; BarbIndex < 5; ++BarbIndex)
			{
				const float T = static_cast<float>(BarbIndex + 1) / 6.0f;
				const FVector2D Spine = FMath::Lerp(ItemPosition + FVector2D(12.0f, Size.Y - 10.0f), ItemPosition + FVector2D(Size.X - 12.0f, 12.0f), T);
				DrawLine(Geometry, OutDrawElements, LayerId + 2, Spine, Spine + FVector2D(-12.0f, -6.0f), Body, 1.2f);
				DrawLine(Geometry, OutDrawElements, LayerId + 2, Spine, Spine + FVector2D(12.0f, 6.0f), Body, 1.2f);
			}
		}
		else if (Category == ESurvivalItemCategory::Food)
		{
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center, Size.Y * 0.32f, Body);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-8.0f, -8.0f), Size.Y * 0.11f, Highlight);
		}
		else if (ItemId == TEXT("Wood") || ItemId == TEXT("Branch") || ItemId == TEXT("Stick") || ItemId == TEXT("WoodPlank") || ItemId == TEXT("WoodGrip"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(8.0f, Size.Y * 0.32f), FVector2D(Size.X - 16.0f, Size.Y * 0.32f), Body);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(13.0f, Size.Y * 0.42f), ItemPosition + FVector2D(Size.X - 14.0f, Size.Y * 0.36f), Highlight, 1.2f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(18.0f, Size.Y * 0.58f), ItemPosition + FVector2D(Size.X - 20.0f, Size.Y * 0.54f), DarkEdge, 1.0f);
		}
		else if (ItemId == TEXT("CopperIngot") || ItemId == TEXT("IronIngot") || ItemId == TEXT("Nail") || ItemId == TEXT("IronHook"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 1, ItemPosition + FVector2D(10.0f, 13.0f), Size - FVector2D(20.0f, 26.0f), Body);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(14.0f, 16.0f), ItemPosition + FVector2D(Size.X - 14.0f, 16.0f), Highlight, 1.2f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, ItemPosition + FVector2D(14.0f, Size.Y - 15.0f), ItemPosition + FVector2D(Size.X - 14.0f, Size.Y - 15.0f), DarkEdge, 1.2f);
		}
		else
		{
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center, Size.Y * 0.34f, Body);
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 2, Center, Size.Y * 0.34f, DarkEdge, 1.5f);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-7.0f, -8.0f), Size.Y * 0.09f, Highlight);
		}

		if (bHovered)
		{
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 5, Center, FMath::Max(Size.X, Size.Y) * 0.42f, FLinearColor(0.94f, 0.86f, 0.58f, 0.68f), 2.0f);
		}
	}

	void DrawInventoryCountTag(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		int32 Count)
	{
		if (Count <= 1)
		{
			return;
		}

		const FVector2D Position = Center + FVector2D(18.0f, 12.0f);
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(2.0f, 3.0f), FVector2D(34.0f, 20.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.24f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, FVector2D(34.0f, 20.0f), FLinearColor(0.055f, 0.050f, 0.040f, 0.88f));
		DrawText(Geometry, OutDrawElements, LayerId + 2, FString::Printf(TEXT("x%d"), Count), Position + FVector2D(7.0f, 2.0f), FLinearColor(0.92f, 0.88f, 0.74f, 1.0f), 10, true, 30.0f);
	}

	void DrawLaidOutInventoryItem(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		FName HoveredItemId,
		const FInventoryStack& Stack,
		const UCraftingComponent* Crafting,
		const FVector2D& Center,
		float TimeSeconds)
	{
		const ESurvivalItemCategory Category = Crafting ? Crafting->GetItemCategory(Stack.ItemId) : ESurvivalItemCategory::Misc;
		const FString Label = Crafting ? Crafting->GetItemDisplayName(Stack.ItemId).ToString() : Stack.ItemId.ToString();
		const FLinearColor Accent = ItemAccentColor(Stack.ItemId, Category);
		const bool bHovered = Stack.ItemId == HoveredItemId;
		const FVector2D Size = PhysicalItemSize(Stack.ItemId, Category) * (bHovered ? 1.08f : 1.0f);
		const float Lift = bHovered ? -5.0f : FMath::Sin(TimeSeconds * 0.82f + StableNoise(Stack.ItemId, 7) * 6.28f) * 0.8f;
		const FVector2D DrawCenter = Center + FVector2D(0.0f, Lift);

		DrawPhysicalItemObject(Geometry, OutDrawElements, LayerId, DrawCenter, Size, Stack.ItemId, Category, Accent, bHovered);
		DrawInventoryCountTag(Geometry, OutDrawElements, LayerId + 7, DrawCenter, Stack.Count);

		USurvivalHUDWidget::FInventoryItemHitBox HitBox;
		HitBox.ItemId = Stack.ItemId;
		HitBox.DisplayName = Label;
		HitBox.Category = CategoryLabel(Category);
		HitBox.Count = Stack.Count;
		HitBox.Bounds = FSlateRect(DrawCenter.X - Size.X * 0.52f, DrawCenter.Y - Size.Y * 0.58f, DrawCenter.X + Size.X * 0.52f, DrawCenter.Y + Size.Y * 0.64f);
		ItemHitBoxes.Add(HitBox);
	}

	void DrawInventoryCluster(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		FName HoveredItemId,
		const FString& Label,
		const TArray<FInventoryStack>& Stacks,
		const UCraftingComponent* Crafting,
		const FVector2D& Center,
		const FVector2D& Extent,
		float TimeSeconds)
	{
		if (Stacks.Num() == 0)
		{
			return;
		}

		DrawText(Geometry, OutDrawElements, LayerId, Label, Center - Extent + FVector2D(4.0f, -22.0f), FLinearColor(0.74f, 0.70f, 0.56f, 0.68f), 11, true, 180.0f);
		DrawLine(Geometry, OutDrawElements, LayerId, Center - Extent + FVector2D(4.0f, -2.0f), Center - Extent + FVector2D(92.0f, -4.0f), FLinearColor(0.64f, 0.58f, 0.38f, 0.26f), 1.0f);

		const int32 Columns = FMath::Clamp(FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Stacks.Num()))), 1, 4);
		const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Stacks.Num()) / static_cast<float>(Columns)));
		const FVector2D CellSize(Extent.X * 2.0f / static_cast<float>(Columns), Extent.Y * 2.0f / static_cast<float>(Rows));

		for (int32 Index = 0; Index < Stacks.Num(); ++Index)
		{
			const int32 Column = Index % Columns;
			const int32 Row = Index / Columns;
			const FInventoryStack& Stack = Stacks[Index];
			const FVector2D Base = Center - Extent + FVector2D(CellSize.X * (Column + 0.5f), CellSize.Y * (Row + 0.5f));
			const FVector2D OrganicOffset(
				(StableNoise(Stack.ItemId, 1) - 0.5f) * FMath::Min(34.0f, CellSize.X * 0.28f),
				(StableNoise(Stack.ItemId, 2) - 0.5f) * FMath::Min(26.0f, CellSize.Y * 0.26f));

			DrawLaidOutInventoryItem(Geometry, OutDrawElements, LayerId + 2 + Index * 9, ItemHitBoxes, HoveredItemId, Stack, Crafting, Base + OrganicOffset, TimeSeconds);
		}
	}

	void DrawInventoryTooltip(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		FName HoveredItemId)
	{
		if (HoveredItemId.IsNone())
		{
			return;
		}

		for (const USurvivalHUDWidget::FInventoryItemHitBox& HitBox : ItemHitBoxes)
		{
			if (HitBox.ItemId != HoveredItemId)
			{
				continue;
			}

			const FVector2D Position(FMath::Min(HitBox.Bounds.Right + 14.0f, HitBox.Bounds.Left + 42.0f), HitBox.Bounds.Top - 8.0f);
			const FVector2D Size(190.0f, 58.0f);
			DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(4.0f, 5.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.32f));
			DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.025f, 0.026f, 0.024f, 0.94f));
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Position, FVector2D(Size.X, 2.0f), FLinearColor(0.68f, 0.58f, 0.34f, 0.54f));
			DrawText(Geometry, OutDrawElements, LayerId + 3, HitBox.DisplayName, Position + FVector2D(12.0f, 8.0f), FLinearColor::White, 12, true, Size.X - 24.0f);
			DrawText(Geometry, OutDrawElements, LayerId + 3, FString::Printf(TEXT("%s   x%d"), *HitBox.Category, HitBox.Count), Position + FVector2D(12.0f, 31.0f), FLinearColor(0.76f, 0.72f, 0.62f, 1.0f), 10, false, Size.X - 24.0f);
			return;
		}
	}

	void DrawCraftingScrap(
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
		const FLinearColor Accent = bCanCraft
			? FLinearColor(0.42f, 0.72f, 0.42f, 1.0f)
			: FLinearColor(0.62f, 0.52f, 0.34f, 0.78f);

		const FString DisplayName = !Recipe.DisplayName.IsEmpty() ? Recipe.DisplayName.ToString() : Recipe.RecipeId.ToString();
		const FString OutputName = Crafting ? Crafting->GetItemDisplayName(Recipe.OutputItemId).ToString() : Recipe.OutputItemId.ToString();
		const FString Ingredients = Inventory ? IngredientListText(Recipe, Crafting, Inventory) : CompactIngredientListText(Recipe, Crafting);

		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(4.0f, 5.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.24f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.42f, 0.37f, 0.26f, bCanCraft ? 0.88f : 0.68f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(5.0f, 5.0f), Size - FVector2D(10.0f, 10.0f), FLinearColor(0.16f, 0.14f, 0.10f, 0.42f));
		DrawLine(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(12.0f, 10.0f), Position + FVector2D(Size.X - 16.0f, 7.0f), Accent.CopyWithNewOpacity(0.56f), 2.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 4, DisplayName, Position + FVector2D(13.0f, 12.0f), FLinearColor::White, 12, true, Size.X - 24.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 4, FString::Printf(TEXT("%s x%d"), *OutputName, Recipe.OutputCount), Position + FVector2D(13.0f, 34.0f), FLinearColor(0.91f, 0.86f, 0.68f, 1.0f), 10, false, Size.X - 24.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 4, Ingredients, Position + FVector2D(13.0f, 55.0f), FLinearColor(0.76f, 0.72f, 0.60f, 1.0f), 9, false, Size.X - 26.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 4, bCanCraft ? TEXT("klick") : TEXT("fehlend"), Position + FVector2D(Size.X - 48.0f, Size.Y - 23.0f), Accent, 9, true, 42.0f);

		if (bCanCraft)
		{
			USurvivalHUDWidget::FRecipeHitBox HitBox;
			HitBox.RecipeId = Recipe.RecipeId;
			HitBox.Bounds = FSlateRect(Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y);
			RecipeHitBoxes.Add(HitBox);
		}
	}

	void DrawCraftingArea(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		const UCraftingComponent* Crafting,
		const UInventoryComponent* Inventory,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(8.0f, 10.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.24f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.075f, 0.064f, 0.044f, 0.72f));
		DrawLine(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(18.0f, 18.0f), Position + FVector2D(Size.X - 22.0f, 12.0f), FLinearColor(0.52f, 0.43f, 0.25f, 0.48f), 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(18.0f, Size.Y - 16.0f), Position + FVector2D(Size.X - 24.0f, Size.Y - 22.0f), FLinearColor(0.52f, 0.43f, 0.25f, 0.38f), 2.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 3, TEXT("Werkflaeche"), Position + FVector2D(18.0f, 18.0f), FLinearColor(0.74f, 0.70f, 0.56f, 0.68f), 11, true, 150.0f);

		if (!Crafting)
		{
			return;
		}

		const TArray<FCraftingRecipe> Recipes = Crafting->GetKnownRecipes();
		const FVector2D ScrapSize(FMath::Clamp(Size.X * 0.30f, 138.0f, 178.0f), 88.0f);
		for (int32 Index = 0; Index < Recipes.Num(); ++Index)
		{
			if (Index >= 7)
			{
				break;
			}

			const int32 Column = Index % 3;
			const int32 Row = Index / 3;
			const FVector2D Base = Position + FVector2D(24.0f + Column * (ScrapSize.X + 16.0f), 48.0f + Row * (ScrapSize.Y + 12.0f));
			if (Base.X + ScrapSize.X > Position.X + Size.X - 18.0f || Base.Y + ScrapSize.Y > Position.Y + Size.Y - 12.0f)
			{
				continue;
			}

			const FVector2D Offset((StableNoise(Recipes[Index].RecipeId, 12) - 0.5f) * 12.0f, (StableNoise(Recipes[Index].RecipeId, 13) - 0.5f) * 8.0f);
			DrawCraftingScrap(Geometry, OutDrawElements, LayerId + 5 + Index * 6, RecipeHitBoxes, Base + Offset, ScrapSize, Recipes[Index], Crafting, Inventory);
		}
	}

	void DrawInventoryOverlay(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		FName HoveredItemId,
		const UUserWidget* Widget,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& ViewSize,
		float OpenAlpha)
	{
		RecipeHitBoxes.Reset();
		ItemHitBoxes.Reset();
		if (!SurvivalCharacter || !SurvivalCharacter->InventoryComponent)
		{
			return;
		}

		const UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
		const UCraftingComponent* Crafting = SurvivalCharacter->CraftingComponent;
		const float TimeSeconds = Widget && Widget->GetWorld() ? Widget->GetWorld()->GetTimeSeconds() : 0.0f;
		const float Ease = 1.0f - FMath::Pow(1.0f - FMath::Clamp(OpenAlpha, 0.0f, 1.0f), 3.0f);
		const float PlaneWidth = FMath::Clamp(ViewSize.X * 0.82f, 700.0f, 1240.0f);
		const float PlaneHeight = FMath::Clamp(ViewSize.Y * 0.72f, 470.0f, 740.0f);
		const FVector2D PlaneSize(PlaneWidth, PlaneHeight);
		const FVector2D CameraDrift(FMath::Sin(TimeSeconds * 0.18f) * 5.0f, (1.0f - Ease) * 48.0f + FMath::Sin(TimeSeconds * 0.13f) * 4.0f);
		const FVector2D PlanePosition((ViewSize.X - PlaneSize.X) * 0.5f + CameraDrift.X, (ViewSize.Y - PlaneSize.Y) * 0.56f + CameraDrift.Y);

		TArray<FInventoryStack> Stacks = Inventory->GetSortedStacks();
		TArray<FInventoryStack> RawStacks;
		TArray<FInventoryStack> ProcessedStacks;
		TArray<FInventoryStack> NaturalStacks;
		TArray<FInventoryStack> FoodStacks;
		TArray<FInventoryStack> ToolStacks;
		TArray<FInventoryStack> UtilityStacks;
		for (const FInventoryStack& Stack : Stacks)
		{
			const ESurvivalItemCategory Category = Crafting ? Crafting->GetItemCategory(Stack.ItemId) : ESurvivalItemCategory::Misc;
			if (IsToolLike(Stack.ItemId, Category))
			{
				ToolStacks.Add(Stack);
			}
			else if (Category == ESurvivalItemCategory::Food)
			{
				FoodStacks.Add(Stack);
			}
			else if (Category == ESurvivalItemCategory::RawResource || Category == ESurvivalItemCategory::Resource || Category == ESurvivalItemCategory::Ore)
			{
				RawStacks.Add(Stack);
			}
			else if (Category == ESurvivalItemCategory::NaturalMaterial)
			{
				NaturalStacks.Add(Stack);
			}
			else if (Category == ESurvivalItemCategory::ProcessedMaterial)
			{
				ProcessedStacks.Add(Stack);
			}
			else
			{
				UtilityStacks.Add(Stack);
			}
		}

		DrawBox(Geometry, OutDrawElements, LayerId, FVector2D::ZeroVector, ViewSize, FLinearColor(0.002f, 0.003f, 0.003f, 0.62f + Ease * 0.22f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, FVector2D::ZeroVector, FVector2D(ViewSize.X, ViewSize.Y * 0.16f), FLinearColor(0.0f, 0.0f, 0.0f, 0.18f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, FVector2D(0.0f, ViewSize.Y * 0.84f), FVector2D(ViewSize.X, ViewSize.Y * 0.16f), FLinearColor(0.0f, 0.0f, 0.0f, 0.22f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, FVector2D::ZeroVector, FVector2D(ViewSize.X * 0.10f, ViewSize.Y), FLinearColor(0.0f, 0.0f, 0.0f, 0.14f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, FVector2D(ViewSize.X * 0.90f, 0.0f), FVector2D(ViewSize.X * 0.10f, ViewSize.Y), FLinearColor(0.0f, 0.0f, 0.0f, 0.16f));
		DrawFoldedClothSurface(Geometry, OutDrawElements, LayerId + 2, PlanePosition, PlaneSize, TimeSeconds);

		const FVector2D BackpackSize(PlaneSize.X * 0.20f, PlaneSize.Y * 0.50f);
		DrawOpenBackpack(Geometry, OutDrawElements, LayerId + 10, PlanePosition + FVector2D(PlaneSize.X * 0.04f, PlaneSize.Y * 0.22f), BackpackSize);

		DrawText(Geometry, OutDrawElements, LayerId + 14, TEXT("Rucksack ausgelegt"), PlanePosition + FVector2D(34.0f, 28.0f), FLinearColor(0.86f, 0.84f, 0.74f, 0.82f), 14, true, 220.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 14, TEXT("Tab / Esc"), PlanePosition + FVector2D(PlaneSize.X - 96.0f, 28.0f), FLinearColor(0.62f, 0.59f, 0.48f, 0.72f), 11, false, 90.0f);

		DrawInventoryCluster(Geometry, OutDrawElements, LayerId + 18, ItemHitBoxes, HoveredItemId, TEXT("Rohstoffe"), RawStacks, Crafting, PlanePosition + FVector2D(PlaneSize.X * 0.29f, PlaneSize.Y * 0.37f), FVector2D(PlaneSize.X * 0.14f, PlaneSize.Y * 0.18f), TimeSeconds);
		DrawInventoryCluster(Geometry, OutDrawElements, LayerId + 60, ItemHitBoxes, HoveredItemId, TEXT("Naturmaterial"), NaturalStacks, Crafting, PlanePosition + FVector2D(PlaneSize.X * 0.51f, PlaneSize.Y * 0.31f), FVector2D(PlaneSize.X * 0.13f, PlaneSize.Y * 0.16f), TimeSeconds);
		DrawInventoryCluster(Geometry, OutDrawElements, LayerId + 102, ItemHitBoxes, HoveredItemId, TEXT("Werkzeug"), ToolStacks, Crafting, PlanePosition + FVector2D(PlaneSize.X * 0.75f, PlaneSize.Y * 0.36f), FVector2D(PlaneSize.X * 0.15f, PlaneSize.Y * 0.19f), TimeSeconds);
		DrawInventoryCluster(Geometry, OutDrawElements, LayerId + 150, ItemHitBoxes, HoveredItemId, TEXT("Materialien"), ProcessedStacks, Crafting, PlanePosition + FVector2D(PlaneSize.X * 0.31f, PlaneSize.Y * 0.72f), FVector2D(PlaneSize.X * 0.16f, PlaneSize.Y * 0.17f), TimeSeconds);
		DrawInventoryCluster(Geometry, OutDrawElements, LayerId + 198, ItemHitBoxes, HoveredItemId, TEXT("Nahrung"), FoodStacks, Crafting, PlanePosition + FVector2D(PlaneSize.X * 0.76f, PlaneSize.Y * 0.72f), FVector2D(PlaneSize.X * 0.12f, PlaneSize.Y * 0.15f), TimeSeconds);
		DrawInventoryCluster(Geometry, OutDrawElements, LayerId + 230, ItemHitBoxes, HoveredItemId, TEXT("Ausrustung"), UtilityStacks, Crafting, PlanePosition + FVector2D(PlaneSize.X * 0.52f, PlaneSize.Y * 0.74f), FVector2D(PlaneSize.X * 0.09f, PlaneSize.Y * 0.12f), TimeSeconds);

		DrawCraftingArea(Geometry, OutDrawElements, LayerId + 270, RecipeHitBoxes, Crafting, Inventory, PlanePosition + FVector2D(PlaneSize.X * 0.40f, PlaneSize.Y * 0.43f), FVector2D(PlaneSize.X * 0.34f, PlaneSize.Y * 0.27f));
		DrawInventoryTooltip(Geometry, OutDrawElements, LayerId + 340, ItemHitBoxes, HoveredItemId);
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
	InventoryItemHitBoxes.Reset();

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const ASurvivalPlayerController* Controller = GetSurvivalController(this);
	const ASurvivalCharacter* SurvivalCharacter = GetSurvivalCharacter(this);

	if (!Controller || !SurvivalCharacter)
	{
		return CurrentLayer;
	}

	if (Controller->IsInventoryOpen())
	{
		const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		if (!bWasInventoryOpen)
		{
			InventoryOpenedAtSeconds = CurrentTime;
			bWasInventoryOpen = true;
		}

		const float OpenAlpha = FMath::Clamp((CurrentTime - InventoryOpenedAtSeconds) / 0.32f, 0.0f, 1.0f);
		DrawInventoryOverlay(AllottedGeometry, OutDrawElements, CurrentLayer + 1, RecipeHitBoxes, InventoryItemHitBoxes, HoveredItemId, this, SurvivalCharacter, ViewSize, OpenAlpha);
		return CurrentLayer + 360;
	}

	bWasInventoryOpen = false;
	HoveredItemId = NAME_None;

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

	for (const FInventoryItemHitBox& HitBox : InventoryItemHitBoxes)
	{
		if (LocalMousePosition.X >= HitBox.Bounds.Left && LocalMousePosition.X <= HitBox.Bounds.Right
			&& LocalMousePosition.Y >= HitBox.Bounds.Top && LocalMousePosition.Y <= HitBox.Bounds.Bottom)
		{
			if (SurvivalCharacter && SurvivalCharacter->ConsumeInventoryItem(HitBox.ItemId))
			{
				return FReply::Handled();
			}
		}
	}

	return FReply::Handled();
}

FReply USurvivalHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const ASurvivalPlayerController* Controller = GetSurvivalController(this);
	if (!Controller || !Controller->IsInventoryOpen())
	{
		HoveredItemId = NAME_None;
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	const FVector2D LocalMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	FName NewHoveredItemId = NAME_None;
	for (const FInventoryItemHitBox& HitBox : InventoryItemHitBoxes)
	{
		if (LocalMousePosition.X >= HitBox.Bounds.Left && LocalMousePosition.X <= HitBox.Bounds.Right
			&& LocalMousePosition.Y >= HitBox.Bounds.Top && LocalMousePosition.Y <= HitBox.Bounds.Bottom)
		{
			NewHoveredItemId = HitBox.ItemId;
			break;
		}
	}

	HoveredItemId = NewHoveredItemId;
	return FReply::Handled();
}

void USurvivalHUDWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	HoveredItemId = NAME_None;
	Super::NativeOnMouseLeave(InMouseEvent);
}
