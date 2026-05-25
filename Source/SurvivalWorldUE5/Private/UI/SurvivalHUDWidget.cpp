#include "UI/SurvivalHUDWidget.h"

#include "Building/BuildingComponent.h"
#include "Building/CampfireActor.h"
#include "Building/StorageContainerActor.h"
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
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr float HudMargin = 28.0f;
	constexpr float MinimapRadius = 74.0f;
	constexpr float MinimapWorldRadius = 4500.0f;
	constexpr float InventorySlotSize = 74.0f;

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

	bool ContainsPoint(const FSlateRect& Rect, const FVector2D& Point)
	{
		return Point.X >= Rect.Left && Point.X <= Rect.Right && Point.Y >= Rect.Top && Point.Y <= Rect.Bottom;
	}

	UTexture2D* LoadItemTextureFromPath(FName AssetIconPath)
	{
		if (AssetIconPath.IsNone())
		{
			return nullptr;
		}

		static TMap<FName, TWeakObjectPtr<UTexture2D>> CachedTextures;
		if (const TWeakObjectPtr<UTexture2D>* CachedTexture = CachedTextures.Find(AssetIconPath))
		{
			if (CachedTexture->IsValid())
			{
				return CachedTexture->Get();
			}
		}

		FString TexturePath = AssetIconPath.ToString();
		if (!TexturePath.Contains(TEXT(".")))
		{
			FString AssetName;
			TexturePath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (!AssetName.IsEmpty())
			{
				TexturePath = FString::Printf(TEXT("%s.%s"), *TexturePath, *AssetName);
			}
		}

		UTexture2D* LoadedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));
		if (LoadedTexture)
		{
			CachedTextures.Add(AssetIconPath, LoadedTexture);
		}
		return LoadedTexture;
	}

	void DrawBox(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& Color)
	{
		if (Size.X <= 0.0f || Size.Y <= 0.0f)
		{
			return;
		}

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
		if (Text.IsEmpty() || Width <= 0.0f)
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
			DrawBox(Geometry, OutDrawElements, LayerId, FVector2D(Center.X - HalfWidth, Center.Y + Y), FVector2D(HalfWidth * 2.0f, 2.0f), Color);
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

	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	FLinearColor ScaleColor(const FLinearColor& Color, float Scale, float Alpha)
	{
		return FLinearColor(Color.R * Scale, Color.G * Scale, Color.B * Scale, Alpha);
	}

	FLinearColor StatColor(float Value, const FLinearColor& Normal)
	{
		if (Value <= 18.0f)
		{
			return FLinearColor(0.82f, 0.10f, 0.08f, 1.0f);
		}
		if (Value <= 38.0f)
		{
			return FLinearColor(0.86f, 0.52f, 0.16f, 1.0f);
		}
		return Normal;
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
			return TEXT("Ressource");
		case ESurvivalItemCategory::Food:
			return TEXT("Nahrung");
		case ESurvivalItemCategory::Drink:
			return TEXT("Getraenk");
		case ESurvivalItemCategory::Medicine:
			return TEXT("Medizin");
		case ESurvivalItemCategory::Tool:
			return TEXT("Werkzeug");
		case ESurvivalItemCategory::Weapon:
			return TEXT("Waffe");
		case ESurvivalItemCategory::Ammunition:
			return TEXT("Munition");
		case ESurvivalItemCategory::Armor:
			return TEXT("Kleidung");
		case ESurvivalItemCategory::Building:
			return TEXT("Bauen");
		case ESurvivalItemCategory::Ore:
			return TEXT("Erz");
		case ESurvivalItemCategory::Quest:
			return TEXT("Quest");
		case ESurvivalItemCategory::Special:
			return TEXT("Spezial");
		default:
			return TEXT("Sonstiges");
		}
	}

	FString RarityLabel(ESurvivalItemRarity Rarity)
	{
		switch (Rarity)
		{
		case ESurvivalItemRarity::Uncommon:
			return TEXT("Ungewoehnlich");
		case ESurvivalItemRarity::Rare:
			return TEXT("Selten");
		case ESurvivalItemRarity::Epic:
			return TEXT("Episch");
		case ESurvivalItemRarity::Legendary:
			return TEXT("Legendaer");
		case ESurvivalItemRarity::Quest:
			return TEXT("Quest");
		default:
			return TEXT("Gewoehnlich");
		}
	}

	FString StationLabel(ECraftingStationType Station)
	{
		switch (Station)
		{
		case ECraftingStationType::Workbench:
			return TEXT("Werkbank");
		case ECraftingStationType::Campfire:
			return TEXT("Lagerfeuer");
		case ECraftingStationType::Forge:
			return TEXT("Schmiede");
		case ECraftingStationType::CookingStation:
			return TEXT("Kochstelle");
		case ECraftingStationType::MedicalTable:
			return TEXT("Medizin-Tisch");
		default:
			return TEXT("Handwerk");
		}
	}

	FLinearColor ItemAccentColor(FName ItemId, ESurvivalItemCategory Category)
	{
		if (ItemId == TEXT("Wood") || ItemId == TEXT("Stick"))
		{
			return FLinearColor(0.46f, 0.32f, 0.18f, 1.0f);
		}
		if (ItemId == TEXT("Stone") || ItemId == TEXT("SharpenedStone") || ItemId == TEXT("PrimitiveTool"))
		{
			return FLinearColor(0.55f, 0.56f, 0.52f, 1.0f);
		}
		if (ItemId == TEXT("PlantFiber") || ItemId == TEXT("Rope"))
		{
			return FLinearColor(0.42f, 0.48f, 0.26f, 1.0f);
		}
		if (ItemId == TEXT("Cloth") || ItemId == TEXT("Bandage"))
		{
			return FLinearColor(0.66f, 0.62f, 0.50f, 1.0f);
		}
		if (ItemId == TEXT("WaterBottle") || ItemId == TEXT("DirtyWater"))
		{
			return FLinearColor(0.28f, 0.50f, 0.58f, 1.0f);
		}
		if (ItemId == TEXT("RawMeat") || ItemId == TEXT("CookedMeat"))
		{
			return FLinearColor(0.55f, 0.24f, 0.18f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Tool)
		{
			return FLinearColor(0.62f, 0.58f, 0.48f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Weapon)
		{
			return FLinearColor(0.52f, 0.44f, 0.32f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Medicine)
		{
			return FLinearColor(0.58f, 0.52f, 0.44f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Food)
		{
			return FLinearColor(0.48f, 0.36f, 0.20f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Armor)
		{
			return FLinearColor(0.40f, 0.36f, 0.28f, 1.0f);
		}
		if (Category == ESurvivalItemCategory::Ore)
		{
			return FLinearColor(0.43f, 0.48f, 0.52f, 1.0f);
		}
		return FLinearColor(0.36f, 0.42f, 0.40f, 1.0f);
	}

	void DrawPanel(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& Accent = FLinearColor(0.48f, 0.42f, 0.30f, 1.0f))
	{
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(8.0f, 10.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.030f, 0.033f, 0.032f, 0.96f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(2.0f, 2.0f), Size - FVector2D(4.0f, 4.0f), FLinearColor(0.070f, 0.072f, 0.066f, 0.82f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(8.0f, 8.0f), Size - FVector2D(16.0f, 16.0f), FLinearColor(0.018f, 0.020f, 0.020f, 0.62f));
		DrawBox(Geometry, OutDrawElements, LayerId + 4, Position, FVector2D(Size.X, 2.0f), WithAlpha(Accent, 0.42f));
		DrawBox(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(0.0f, Size.Y - 2.0f), FVector2D(Size.X, 2.0f), WithAlpha(Accent, 0.24f));

		for (int32 ScratchIndex = 0; ScratchIndex < 5; ++ScratchIndex)
		{
			const float X = Position.X + 28.0f + ScratchIndex * (Size.X - 64.0f) / 5.0f;
			const float Y = Position.Y + 18.0f + (ScratchIndex % 3) * (Size.Y - 50.0f) / 3.0f;
			DrawLine(Geometry, OutDrawElements, LayerId + 5, FVector2D(X, Y), FVector2D(X + 54.0f, Y - 4.0f), FLinearColor(0.82f, 0.78f, 0.62f, 0.055f), 1.0f);
		}
	}

	void DrawSlotFrame(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& Accent,
		bool bHovered,
		bool bSelected,
		bool bEquipped = false,
		bool bDisabled = false)
	{
		const FLinearColor Base = bDisabled ? FLinearColor(0.020f, 0.020f, 0.020f, 0.55f) : FLinearColor(0.036f, 0.038f, 0.036f, 0.94f);
		const FLinearColor Edge = bSelected ? FLinearColor(0.82f, 0.70f, 0.42f, 0.94f) : bHovered ? FLinearColor(0.70f, 0.66f, 0.52f, 0.74f) : FLinearColor(0.20f, 0.19f, 0.16f, 0.86f);
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(3.0f, 4.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, Edge);
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(3.0f, 3.0f), Size - FVector2D(6.0f, 6.0f), Base);
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(8.0f, 8.0f), Size - FVector2D(16.0f, 16.0f), FLinearColor(0.010f, 0.012f, 0.012f, bDisabled ? 0.38f : 0.72f));
		DrawBox(Geometry, OutDrawElements, LayerId + 4, Position + FVector2D(5.0f, 5.0f), FVector2D(Size.X - 10.0f, 2.0f), WithAlpha(Accent, bHovered || bSelected ? 0.36f : 0.14f));
		if (bEquipped)
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 5, Position + FVector2D(Size.X - 7.0f, 7.0f), FVector2D(3.0f, Size.Y - 14.0f), FLinearColor(0.42f, 0.72f, 0.48f, 0.76f));
		}
	}

	void DrawItemGlyph(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Scale,
		FName ItemId,
		ESurvivalItemCategory Category,
		bool bHighlighted)
	{
		const FLinearColor Accent = ItemAccentColor(ItemId, Category);
		const FLinearColor Body = ScaleColor(Accent, bHighlighted ? 1.16f : 0.86f, 0.96f);
		const FLinearColor Dark = ScaleColor(Accent, 0.38f, 0.96f);
		const FLinearColor Metal(0.62f, 0.62f, 0.56f, 0.94f);

		DrawBox(Geometry, OutDrawElements, LayerId, Center + FVector2D(-24.0f, 16.0f) * Scale, FVector2D(50.0f, 7.0f) * Scale, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f));

		if (ItemId == TEXT("Axe"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-20.0f, 20.0f) * Scale, Center + FVector2D(16.0f, -18.0f) * Scale, Body, 5.0f * Scale);
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(8.0f, -24.0f) * Scale, FVector2D(26.0f, 20.0f) * Scale, Metal);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(12.0f, -20.0f) * Scale, Center + FVector2D(30.0f, -8.0f) * Scale, FLinearColor(0.86f, 0.84f, 0.72f, 0.46f), 1.0f * Scale);
		}
		else if (ItemId == TEXT("Pickaxe"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-18.0f, 21.0f) * Scale, Center + FVector2D(12.0f, -18.0f) * Scale, Body, 5.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-18.0f, -18.0f) * Scale, Center + FVector2D(30.0f, -10.0f) * Scale, Metal, 5.0f * Scale);
		}
		else if (ItemId == TEXT("SharpenedStone") || ItemId == TEXT("PrimitiveTool"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-24.0f, 14.0f) * Scale, Center + FVector2D(24.0f, -13.0f) * Scale, Metal, 7.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-26.0f, 16.0f) * Scale, Center + FVector2D(-8.0f, 6.0f) * Scale, Dark, 5.0f * Scale);
			if (ItemId == TEXT("PrimitiveTool"))
			{
				DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-18.0f, -10.0f) * Scale, Center + FVector2D(18.0f, 12.0f) * Scale, FLinearColor(0.45f, 0.28f, 0.15f, 0.94f), 4.0f * Scale);
			}
		}
		else if (ItemId == TEXT("Stick") || ItemId == TEXT("Wood"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-28.0f, -7.0f) * Scale, FVector2D(56.0f, 15.0f) * Scale, Body);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-22.0f, -1.0f) * Scale, Center + FVector2D(22.0f, -5.0f) * Scale, FLinearColor(0.80f, 0.66f, 0.42f, 0.42f), 1.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-18.0f, 7.0f) * Scale, Center + FVector2D(18.0f, 4.0f) * Scale, Dark, 1.0f * Scale);
		}
		else if (ItemId == TEXT("Stone"))
		{
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-9.0f, 3.0f) * Scale, 18.0f * Scale, Body);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(11.0f, -4.0f) * Scale, 15.0f * Scale, Dark);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-18.0f, -2.0f) * Scale, Center + FVector2D(-4.0f, -12.0f) * Scale, FLinearColor(0.86f, 0.84f, 0.72f, 0.40f), 1.0f * Scale);
		}
		else if (ItemId == TEXT("PlantFiber") || ItemId == TEXT("Rope"))
		{
			for (int32 Strand = 0; Strand < 4; ++Strand)
			{
				const float Offset = static_cast<float>(Strand - 2) * 4.0f;
				DrawLine(Geometry, OutDrawElements, LayerId + 1 + Strand, Center + FVector2D(-26.0f, Offset) * Scale, Center + FVector2D(26.0f, -Offset * 0.4f) * Scale, Body, (ItemId == TEXT("Rope") ? 3.0f : 1.6f) * Scale);
			}
		}
		else if (ItemId == TEXT("Cloth") || ItemId == TEXT("Bandage"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-24.0f, -14.0f) * Scale, FVector2D(48.0f, 30.0f) * Scale, Body);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-22.0f, -4.0f) * Scale, Center + FVector2D(22.0f, -8.0f) * Scale, Dark, 1.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-20.0f, 7.0f) * Scale, Center + FVector2D(18.0f, 4.0f) * Scale, FLinearColor(0.88f, 0.84f, 0.72f, 0.35f), 1.0f * Scale);
			if (ItemId == TEXT("Bandage"))
			{
				DrawBox(Geometry, OutDrawElements, LayerId + 4, Center + FVector2D(-3.0f, -12.0f) * Scale, FVector2D(6.0f, 26.0f) * Scale, FLinearColor(0.70f, 0.16f, 0.14f, 0.72f));
				DrawBox(Geometry, OutDrawElements, LayerId + 4, Center + FVector2D(-14.0f, -1.0f) * Scale, FVector2D(28.0f, 6.0f) * Scale, FLinearColor(0.70f, 0.16f, 0.14f, 0.72f));
			}
		}
		else if (ItemId == TEXT("WaterBottle") || ItemId == TEXT("DirtyWater") || ItemId == TEXT("Alcohol"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-12.0f, -24.0f) * Scale, FVector2D(24.0f, 46.0f) * Scale, FLinearColor(0.18f, 0.22f, 0.24f, 0.86f));
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-8.0f, -18.0f) * Scale, FVector2D(16.0f, 34.0f) * Scale, Body);
			DrawBox(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-7.0f, -30.0f) * Scale, FVector2D(14.0f, 8.0f) * Scale, Dark);
		}
		else if (ItemId == TEXT("RawMeat") || ItemId == TEXT("CookedMeat"))
		{
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-8.0f, 0.0f) * Scale, 18.0f * Scale, Body);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(12.0f, -3.0f) * Scale, 14.0f * Scale, Dark);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-18.0f, -6.0f) * Scale, Center + FVector2D(14.0f, -12.0f) * Scale, FLinearColor(0.88f, 0.66f, 0.48f, 0.30f), 1.0f * Scale);
		}
		else if (ItemId == TEXT("Spear"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-28.0f, 20.0f) * Scale, Center + FVector2D(28.0f, -22.0f) * Scale, Body, 4.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(20.0f, -16.0f) * Scale, Center + FVector2D(31.0f, -27.0f) * Scale, Metal, 4.0f * Scale);
		}
		else if (ItemId == TEXT("StoneAxe"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-20.0f, 20.0f) * Scale, Center + FVector2D(16.0f, -18.0f) * Scale, FLinearColor(0.42f, 0.26f, 0.14f, 0.96f), 5.0f * Scale);
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(6.0f, -24.0f) * Scale, FVector2D(28.0f, 18.0f) * Scale, Metal);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(1.0f, -9.0f) * Scale, Center + FVector2D(22.0f, -5.0f) * Scale, FLinearColor(0.44f, 0.52f, 0.30f, 0.88f), 2.0f * Scale);
		}
		else if (ItemId == TEXT("Torch"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-18.0f, 22.0f) * Scale, Center + FVector2D(12.0f, -18.0f) * Scale, Body, 5.0f * Scale);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(16.0f, -22.0f) * Scale, 11.0f * Scale, FLinearColor(0.86f, 0.45f, 0.16f, 0.90f));
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(18.0f, -27.0f) * Scale, 6.0f * Scale, FLinearColor(0.95f, 0.78f, 0.34f, 0.80f));
		}
		else if (ItemId == TEXT("Campfire"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-26.0f, 16.0f) * Scale, Center + FVector2D(22.0f, 2.0f) * Scale, Body, 5.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-18.0f, 0.0f) * Scale, Center + FVector2D(26.0f, 18.0f) * Scale, Dark, 5.0f * Scale);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(0.0f, -6.0f) * Scale, 13.0f * Scale, FLinearColor(0.84f, 0.34f, 0.16f, 0.80f));
		}
		else if (ItemId == TEXT("SimpleBackpack") || ItemId == TEXT("PrimitiveClothing") || ItemId == TEXT("Hide"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-22.0f, -20.0f) * Scale, FVector2D(44.0f, 42.0f) * Scale, Body);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-16.0f, -14.0f) * Scale, Center + FVector2D(16.0f, -11.0f) * Scale, Dark, 2.0f * Scale);
			DrawLine(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-20.0f, 14.0f) * Scale, Center + FVector2D(18.0f, 16.0f) * Scale, FLinearColor(0.86f, 0.74f, 0.48f, 0.25f), 1.0f * Scale);
		}
		else if (Category == ESurvivalItemCategory::Tool)
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-24.0f, 17.0f) * Scale, Center + FVector2D(24.0f, -14.0f) * Scale, Body, 5.0f * Scale);
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(7.0f, -21.0f) * Scale, FVector2D(24.0f, 16.0f) * Scale, Metal);
		}
		else
		{
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center, 18.0f * Scale, Body);
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 2, Center, 18.0f * Scale, Dark, 2.0f * Scale);
			DrawFilledCircle(Geometry, OutDrawElements, LayerId + 3, Center + FVector2D(-6.0f, -7.0f) * Scale, 4.0f * Scale, FLinearColor(0.88f, 0.84f, 0.70f, 0.36f));
		}
	}

	FString ItemDisplayName(const UCraftingComponent* Crafting, FName ItemId)
	{
		return Crafting ? Crafting->GetItemDisplayName(ItemId).ToString() : ItemId.ToString();
	}

	ESurvivalItemCategory ItemCategory(const UCraftingComponent* Crafting, FName ItemId)
	{
		return Crafting ? Crafting->GetItemCategory(ItemId) : ESurvivalItemCategory::Misc;
	}

	FString ItemDescription(const UCraftingComponent* Crafting, FName ItemId)
	{
		if (!Crafting)
		{
			return FString();
		}

		FItemDef Item;
		return Crafting->GetItemDefinition(ItemId, Item) ? Item.Description.ToString() : FString();
	}

	void AddInventoryHitBox(
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		const FSlateRect& Bounds,
		const FInventoryStack& Stack,
		const UCraftingComponent* Crafting,
		bool bCraftingInput = false,
		bool bWorldInventory = false)
	{
		USurvivalHUDWidget::FInventoryItemHitBox HitBox;
		HitBox.ItemId = Stack.ItemId;
		HitBox.SlotIndex = Stack.SlotIndex;
		HitBox.bCraftingInput = bCraftingInput;
		HitBox.bWorldInventory = bWorldInventory;
		HitBox.DisplayName = ItemDisplayName(Crafting, Stack.ItemId);
		HitBox.Category = CategoryLabel(ItemCategory(Crafting, Stack.ItemId));
		HitBox.Description = ItemDescription(Crafting, Stack.ItemId);
		HitBox.Count = Stack.Count;
		HitBox.Durability = Stack.Durability;
		FItemDef ItemDef;
		HitBox.MaxDurability = Crafting && Crafting->GetItemDefinition(Stack.ItemId, ItemDef) && ItemDef.bHasDurability ? ItemDef.MaxDurability : 0.0f;
		HitBox.Bounds = Bounds;
		ItemHitBoxes.Add(HitBox);
	}

	void AddSlotHitBox(
		TArray<USurvivalHUDWidget::FSlotHitBox>& SlotHitBoxes,
		const FSlateRect& Bounds,
		int32 SlotIndex,
		bool bCraftingInput = false,
		bool bHotbarSlot = false,
		bool bWorldInventory = false,
		int32 HotbarIndex = INDEX_NONE)
	{
		USurvivalHUDWidget::FSlotHitBox HitBox;
		HitBox.SlotIndex = SlotIndex;
		HitBox.bCraftingInput = bCraftingInput;
		HitBox.bHotbarSlot = bHotbarSlot;
		HitBox.bWorldInventory = bWorldInventory;
		HitBox.HotbarIndex = HotbarIndex;
		HitBox.Bounds = Bounds;
		SlotHitBoxes.Add(HitBox);
	}

	void DrawInventoryItemInSlot(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		TArray<USurvivalHUDWidget::FSlotHitBox>& SlotHitBoxes,
		const FVector2D& SlotPosition,
		const FVector2D& SlotSize,
		const FInventoryStack& Stack,
		const UCraftingComponent* Crafting,
		FName HoveredItemId,
		FName SelectedItemId,
		bool bEquipped = false,
		bool bCraftingInput = false,
		bool bHotbarSlot = false,
		bool bWorldInventory = false,
		int32 HotbarIndex = INDEX_NONE)
	{
		const ESurvivalItemCategory Category = ItemCategory(Crafting, Stack.ItemId);
		const FLinearColor Accent = ItemAccentColor(Stack.ItemId, Category);
		const bool bHovered = Stack.ItemId == HoveredItemId;
		const bool bSelected = Stack.ItemId == SelectedItemId;
		DrawSlotFrame(Geometry, OutDrawElements, LayerId, SlotPosition, SlotSize, Accent, bHovered, bSelected, bEquipped);

		FItemDef ItemDef;
		UTexture2D* ItemTexture = nullptr;
		if (Crafting && Crafting->GetItemDefinition(Stack.ItemId, ItemDef))
		{
			ItemTexture = ItemDef.InventoryImage ? ItemDef.InventoryImage.Get() : ItemDef.Icon.Get();
			if (!ItemTexture)
			{
				ItemTexture = LoadItemTextureFromPath(ItemDef.AssetIconPath);
			}
		}

		if (ItemTexture)
		{
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(ItemTexture);
			IconBrush.ImageSize = FVector2D(SlotSize.X - 14.0f, SlotSize.Y - 14.0f);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 6,
				Geometry.ToPaintGeometry(FVector2f(SlotSize - FVector2D(14.0f, 14.0f)), FSlateLayoutTransform(FVector2f(SlotPosition + FVector2D(7.0f, 7.0f)))),
				&IconBrush,
				ESlateDrawEffect::None,
				bHovered || bSelected ? FLinearColor(1.08f, 1.05f, 0.94f, 1.0f) : FLinearColor::White);
		}
		else
		{
			DrawItemGlyph(Geometry, OutDrawElements, LayerId + 6, SlotPosition + SlotSize * 0.5f + FVector2D(0.0f, -2.0f), SlotSize.X / 76.0f, Stack.ItemId, Category, bHovered || bSelected);
		}

		if (Stack.Count > 1)
		{
			const FVector2D CountTagPosition = SlotPosition + FVector2D(SlotSize.X - 34.0f, SlotSize.Y - 22.0f);
			DrawBox(Geometry, OutDrawElements, LayerId + 12, CountTagPosition + FVector2D(2.0f, 2.0f), FVector2D(30.0f, 18.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
			DrawBox(Geometry, OutDrawElements, LayerId + 13, CountTagPosition, FVector2D(30.0f, 18.0f), FLinearColor(0.055f, 0.052f, 0.046f, 0.90f));
			DrawText(Geometry, OutDrawElements, LayerId + 14, FString::Printf(TEXT("%d"), Stack.Count), CountTagPosition + FVector2D(6.0f, 1.0f), FLinearColor(0.92f, 0.88f, 0.74f, 1.0f), 10, true, 26.0f);
		}

		if (ItemDef.bHasDurability && ItemDef.MaxDurability > 0.0f)
		{
			const float DurabilityAlpha = FMath::Clamp(Stack.Durability / ItemDef.MaxDurability, 0.0f, 1.0f);
			const FVector2D BarPosition = SlotPosition + FVector2D(8.0f, SlotSize.Y - 9.0f);
			const FVector2D BarSize(SlotSize.X - 16.0f, 3.0f);
			DrawBox(Geometry, OutDrawElements, LayerId + 15, BarPosition, BarSize, FLinearColor(0.010f, 0.010f, 0.010f, 0.86f));
			DrawBox(Geometry, OutDrawElements, LayerId + 16, BarPosition, FVector2D(BarSize.X * DurabilityAlpha, BarSize.Y), DurabilityAlpha <= 0.0f ? FLinearColor(0.70f, 0.12f, 0.10f, 0.92f) : FLinearColor(0.44f, 0.72f, 0.38f, 0.86f));
		}

		const FSlateRect Bounds(SlotPosition.X, SlotPosition.Y, SlotPosition.X + SlotSize.X, SlotPosition.Y + SlotSize.Y);
		AddSlotHitBox(SlotHitBoxes, Bounds, Stack.SlotIndex, bCraftingInput, bHotbarSlot, bWorldInventory, HotbarIndex);
		AddInventoryHitBox(ItemHitBoxes, Bounds, Stack, Crafting, bCraftingInput, bWorldInventory);
	}

	void DrawMapBackground(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Radius)
	{
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center, Radius, FLinearColor(0.080f, 0.095f, 0.086f, 0.98f));
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(-18.0f, 12.0f), Radius * 0.42f, FLinearColor(0.13f, 0.16f, 0.12f, 0.70f));
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, Center + FVector2D(22.0f, -18.0f), Radius * 0.36f, FLinearColor(0.09f, 0.13f, 0.13f, 0.78f));
		DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-58.0f, -5.0f), Center + FVector2D(54.0f, -20.0f), FLinearColor(0.52f, 0.48f, 0.36f, 0.42f), 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-20.0f, 22.0f), Center + FVector2D(35.0f, 48.0f), FLinearColor(0.52f, 0.48f, 0.36f, 0.34f), 2.0f);
		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 3, Center, Radius, FLinearColor(0.010f, 0.012f, 0.012f, 0.98f), 3.0f);
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
		const FVector2D Tip = Center + Forward * 13.0f;
		const FVector2D Left = Center - Forward * 10.0f - Right * 7.0f;
		const FVector2D RightPoint = Center - Forward * 10.0f + Right * 7.0f;

		DrawLine(Geometry, OutDrawElements, LayerId, Tip, Left, FLinearColor(0.92f, 0.90f, 0.78f, 1.0f), 3.0f);
		DrawLine(Geometry, OutDrawElements, LayerId, Tip, RightPoint, FLinearColor(0.92f, 0.90f, 0.78f, 1.0f), 3.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 1, Left, RightPoint, FLinearColor(0.30f, 0.50f, 0.58f, 0.92f), 3.0f);
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
		const TArray<FMapMarkerSnapshot> Markers = bLargeMap ? MapSubsystem->GetAllMarkers() : MapSubsystem->GetMarkersAround(PlayerLocation, WorldRadius);
		for (const FMapMarkerSnapshot& Marker : Markers)
		{
			const FVector2D MarkerPosition = WorldToMap(Marker.WorldLocation, PlayerLocation, Center, Radius, WorldRadius);
			if (FVector2D::Distance(MarkerPosition, Center) > Radius - 8.0f)
			{
				continue;
			}

			DrawFilledCircle(Geometry, OutDrawElements, LayerId, MarkerPosition, bLargeMap ? 6.0f : 4.0f, WithAlpha(Marker.MarkerColor, 0.82f));
			DrawCircleOutline(Geometry, OutDrawElements, LayerId + 1, MarkerPosition, bLargeMap ? 6.0f : 4.0f, FLinearColor(0.02f, 0.02f, 0.02f, 1.0f), 1.0f);
		}
	}

	void DrawStatusGlyph(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		const FString& Kind,
		const FLinearColor& Color)
	{
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, Center, 13.0f, FLinearColor(0.018f, 0.020f, 0.020f, 0.95f));
		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 1, Center, 13.0f, WithAlpha(Color, 0.70f), 1.5f);

		if (Kind == TEXT("Health"))
		{
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-2.0f, -8.0f), FVector2D(4.0f, 16.0f), Color);
			DrawBox(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-8.0f, -2.0f), FVector2D(16.0f, 4.0f), Color);
		}
		else if (Kind == TEXT("Stamina"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-7.0f, 5.0f), Center + FVector2D(-1.0f, -6.0f), Color, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-1.0f, -6.0f), Center + FVector2D(7.0f, 5.0f), Color, 2.0f);
		}
		else if (Kind == TEXT("Thirst"))
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(0.0f, -8.0f), Center + FVector2D(-6.0f, 2.0f), Color, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(0.0f, -8.0f), Center + FVector2D(6.0f, 2.0f), Color, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-6.0f, 2.0f), Center + FVector2D(0.0f, 8.0f), Color, 2.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(6.0f, 2.0f), Center + FVector2D(0.0f, 8.0f), Color, 2.0f);
		}
		else
		{
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-6.0f, -8.0f), Center + FVector2D(-6.0f, 8.0f), Color, 1.8f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-9.0f, -8.0f), Center + FVector2D(-9.0f, -1.0f), Color, 1.2f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(-3.0f, -8.0f), Center + FVector2D(-3.0f, -1.0f), Color, 1.2f);
			DrawLine(Geometry, OutDrawElements, LayerId + 2, Center + FVector2D(5.0f, -8.0f), Center + FVector2D(8.0f, 8.0f), Color, 1.8f);
		}
	}

	void DrawStatusMeter(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Position,
		const FString& Kind,
		float Value,
		const FLinearColor& NormalColor)
	{
		const FVector2D Size(178.0f, 30.0f);
		const FLinearColor Color = StatColor(Value, NormalColor);
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(3.0f, 4.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.24f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.024f, 0.026f, 0.026f, 0.82f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(32.0f, 10.0f), FVector2D(132.0f, 8.0f), FLinearColor(0.008f, 0.010f, 0.010f, 0.92f));
		DrawBox(Geometry, OutDrawElements, LayerId + 3, Position + FVector2D(32.0f, 10.0f), FVector2D(132.0f * FMath::Clamp(Value, 0.0f, 100.0f) / 100.0f, 8.0f), WithAlpha(Color, 0.88f));
		DrawStatusGlyph(Geometry, OutDrawElements, LayerId + 5, Position + FVector2D(15.0f, 15.0f), Kind, Color);
		DrawText(Geometry, OutDrawElements, LayerId + 8, FString::Printf(TEXT("%02.0f"), Value), Position + FVector2D(137.0f, 5.0f), FLinearColor(0.78f, 0.76f, 0.66f, 0.92f), 10, true, 32.0f);
	}

	void DrawStaminaMeter(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& ViewSize,
		float Stamina)
	{
		const FVector2D Size(300.0f, 18.0f);
		const FVector2D Position((ViewSize.X - Size.X) * 0.5f, ViewSize.Y - 106.0f);
		const FLinearColor Color = StatColor(Stamina, FLinearColor(0.62f, 0.68f, 0.58f, 1.0f));
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(3.0f, 4.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.18f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, FLinearColor(0.018f, 0.020f, 0.020f, 0.62f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(4.0f, 5.0f), FVector2D((Size.X - 8.0f) * FMath::Clamp(Stamina, 0.0f, 100.0f) / 100.0f, 8.0f), WithAlpha(Color, 0.70f));
	}

	void DrawHotbar(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& ViewSize)
	{
		if (!SurvivalCharacter || !SurvivalCharacter->InventoryComponent)
		{
			return;
		}

		const UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
		const UCraftingComponent* Crafting = SurvivalCharacter->CraftingComponent;
		const TArray<FInventoryStack> Stacks = Inventory->GetHotbarStacks();
		const int32 SlotCount = 6;
		const FVector2D SlotSize(56.0f, 56.0f);
		const float Gap = 8.0f;
		const float TotalWidth = SlotCount * SlotSize.X + (SlotCount - 1) * Gap;
		const FVector2D Start((ViewSize.X - TotalWidth) * 0.5f, ViewSize.Y - 82.0f);

		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			const FVector2D Position = Start + FVector2D(Index * (SlotSize.X + Gap), 0.0f);
			const bool bHasItem = Stacks.IsValidIndex(Index) && !Stacks[Index].IsEmpty();
			const FLinearColor Accent = bHasItem ? ItemAccentColor(Stacks[Index].ItemId, ItemCategory(Crafting, Stacks[Index].ItemId)) : FLinearColor(0.26f, 0.24f, 0.20f, 1.0f);
			const bool bEquipped = bHasItem && Stacks[Index].SlotIndex == Inventory->GetEquippedSlotIndex();
			DrawSlotFrame(Geometry, OutDrawElements, LayerId + Index * 12, Position, SlotSize, Accent, false, bEquipped, bEquipped, !bHasItem);
			DrawText(Geometry, OutDrawElements, LayerId + Index * 12 + 6, FString::FromInt(Index + 1), Position + FVector2D(5.0f, 3.0f), FLinearColor(0.72f, 0.70f, 0.62f, 0.86f), 10, true, 18.0f);
			if (bHasItem)
			{
				DrawItemGlyph(Geometry, OutDrawElements, LayerId + Index * 12 + 7, Position + SlotSize * 0.5f + FVector2D(0.0f, 1.0f), 0.64f, Stacks[Index].ItemId, ItemCategory(Crafting, Stacks[Index].ItemId), Index == 0);
			}
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
		const FVector2D MinimapCenter(ViewSize.X - HudMargin - MinimapRadius - 14.0f, HudMargin + MinimapRadius + 14.0f);
		DrawFilledCircle(Geometry, OutDrawElements, LayerId, MinimapCenter, MinimapRadius + 17.0f, FLinearColor(0.012f, 0.014f, 0.014f, 0.80f));
		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 1, MinimapCenter, MinimapRadius + 14.0f, FLinearColor(0.26f, 0.24f, 0.18f, 0.78f), 4.0f);
		DrawMapBackground(Geometry, OutDrawElements, LayerId + 2, MinimapCenter, MinimapRadius);
		DrawMapMarkers(Geometry, OutDrawElements, LayerId + 7, Widget, SurvivalCharacter, MinimapCenter, MinimapRadius);
		DrawPlayerArrow(Geometry, OutDrawElements, LayerId + 10, MinimapCenter, SurvivalCharacter);
		DrawText(Geometry, OutDrawElements, LayerId + 11, TEXT("N"), MinimapCenter + FVector2D(-4.0f, -MinimapRadius - 10.0f), FLinearColor(0.82f, 0.80f, 0.70f, 0.86f), 11, true, 20.0f);

		const FVector2D StatusStart(HudMargin, ViewSize.Y - 150.0f);
		DrawStatusMeter(Geometry, OutDrawElements, LayerId + 20, StatusStart, TEXT("Health"), Stats->Health, FLinearColor(0.62f, 0.18f, 0.16f, 1.0f));
		DrawStatusMeter(Geometry, OutDrawElements, LayerId + 32, StatusStart + FVector2D(0.0f, 36.0f), TEXT("Hunger"), Stats->Hunger, FLinearColor(0.58f, 0.50f, 0.30f, 1.0f));
		DrawStatusMeter(Geometry, OutDrawElements, LayerId + 44, StatusStart + FVector2D(0.0f, 72.0f), TEXT("Thirst"), Stats->Thirst, FLinearColor(0.30f, 0.52f, 0.62f, 1.0f));
		DrawStaminaMeter(Geometry, OutDrawElements, LayerId + 58, ViewSize, Stats->Stamina);
		DrawHotbar(Geometry, OutDrawElements, LayerId + 70, SurvivalCharacter, ViewSize);

		if (SurvivalCharacter->FirstPersonCamera)
		{
			const FVector2D CrosshairCenter(ViewSize.X * 0.5f, ViewSize.Y * 0.5f);
			DrawLine(Geometry, OutDrawElements, LayerId + 150, CrosshairCenter + FVector2D(-6.0f, 0.0f), CrosshairCenter + FVector2D(6.0f, 0.0f), FLinearColor(0.86f, 0.86f, 0.78f, 0.72f), 1.0f);
			DrawLine(Geometry, OutDrawElements, LayerId + 150, CrosshairCenter + FVector2D(0.0f, -6.0f), CrosshairCenter + FVector2D(0.0f, 6.0f), FLinearColor(0.86f, 0.86f, 0.78f, 0.72f), 1.0f);

			bool bCanInteract = false;
			const FText Prompt = SurvivalCharacter->GetCurrentInteractionPrompt(bCanInteract);
			if (!Prompt.IsEmpty())
			{
				const FString PromptText = FString::Printf(TEXT("E  %s"), *Prompt.ToString());
				const FVector2D PromptSize(260.0f, 34.0f);
				const FVector2D PromptPosition(CrosshairCenter.X - PromptSize.X * 0.5f, CrosshairCenter.Y + 26.0f);
				DrawBox(Geometry, OutDrawElements, LayerId + 151, PromptPosition, PromptSize, FLinearColor(0.014f, 0.016f, 0.016f, 0.68f));
				DrawText(Geometry, OutDrawElements, LayerId + 152, PromptText, PromptPosition + FVector2D(16.0f, 7.0f), bCanInteract ? FLinearColor(0.92f, 0.90f, 0.80f, 1.0f) : FLinearColor(0.88f, 0.42f, 0.28f, 1.0f), 13, true, PromptSize.X - 28.0f);
			}

			const FText Feedback = SurvivalCharacter->GetInteractionFeedback();
			if (!Feedback.IsEmpty())
			{
				const FVector2D FeedbackSize(360.0f, 30.0f);
				const FVector2D FeedbackPosition(CrosshairCenter.X - FeedbackSize.X * 0.5f, CrosshairCenter.Y + 66.0f);
				DrawBox(Geometry, OutDrawElements, LayerId + 153, FeedbackPosition, FeedbackSize, FLinearColor(0.025f, 0.020f, 0.016f, 0.72f));
				DrawText(Geometry, OutDrawElements, LayerId + 154, Feedback.ToString(), FeedbackPosition + FVector2D(14.0f, 6.0f), FLinearColor(0.92f, 0.70f, 0.48f, 0.96f), 11, true, FeedbackSize.X - 28.0f);
			}

			if (SurvivalCharacter->BuildingComponent && SurvivalCharacter->BuildingComponent->IsBuildModeActive())
			{
				FSurvivalBuildPartDef BuildPart;
				const FString PartName = SurvivalCharacter->BuildingComponent->GetSelectedBuildPartDefinition(BuildPart)
					? BuildPart.DisplayName.ToString()
					: SurvivalCharacter->BuildingComponent->GetSelectedBuildPartId().ToString();
				const FVector2D BuildPanelSize(360.0f, 74.0f);
				const FVector2D BuildPanelPosition(CrosshairCenter.X - BuildPanelSize.X * 0.5f, CrosshairCenter.Y - 134.0f);
				DrawBox(Geometry, OutDrawElements, LayerId + 160, BuildPanelPosition, BuildPanelSize, FLinearColor(0.018f, 0.020f, 0.020f, 0.78f));
				DrawBox(Geometry, OutDrawElements, LayerId + 161, BuildPanelPosition, FVector2D(4.0f, BuildPanelSize.Y), SurvivalCharacter->BuildingComponent->IsPlacementValid() ? FLinearColor(0.28f, 0.78f, 0.34f, 0.90f) : FLinearColor(0.88f, 0.22f, 0.16f, 0.90f));
				DrawText(Geometry, OutDrawElements, LayerId + 162, FString::Printf(TEXT("BAUEN  %s"), *PartName), BuildPanelPosition + FVector2D(16.0f, 12.0f), FLinearColor(0.92f, 0.90f, 0.78f, 0.96f), 13, true, BuildPanelSize.X - 32.0f);
				DrawText(Geometry, OutDrawElements, LayerId + 162, SurvivalCharacter->BuildingComponent->GetPlacementMessage().ToString(), BuildPanelPosition + FVector2D(16.0f, 36.0f), SurvivalCharacter->BuildingComponent->IsPlacementValid() ? FLinearColor(0.66f, 0.86f, 0.58f, 0.92f) : FLinearColor(0.94f, 0.50f, 0.36f, 0.92f), 11, false, BuildPanelSize.X - 32.0f);
			}
		}
	}

	FString IngredientListText(const FCraftingRecipe& Recipe, const UCraftingComponent* Crafting, const UInventoryComponent* Inventory)
	{
		TArray<FString> Parts;
		for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
		{
			const FString Name = ItemDisplayName(Crafting, Ingredient.ItemId);
			const int32 Have = Inventory ? Inventory->GetItemCount(Ingredient.ItemId) : 0;
			Parts.Add(FString::Printf(TEXT("%s %d/%d"), *Name, Have, Ingredient.Count));
		}
		return FString::Join(Parts, TEXT("   "));
	}

	bool RecipeExists(const TArray<FCraftingRecipe>& Recipes, FName RecipeId)
	{
		return Recipes.ContainsByPredicate([RecipeId](const FCraftingRecipe& Recipe)
		{
			return Recipe.RecipeId == RecipeId;
		});
	}

	const FCraftingRecipe* FindRecipe(const TArray<FCraftingRecipe>& Recipes, FName RecipeId)
	{
		return Recipes.FindByPredicate([RecipeId](const FCraftingRecipe& Recipe)
		{
			return Recipe.RecipeId == RecipeId;
		});
	}

	void DrawRecipeRow(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		const FVector2D& Position,
		const FVector2D& Size,
		const FCraftingRecipe& Recipe,
		const UCraftingComponent* Crafting,
		bool bSelected)
	{
		const bool bKnown = Crafting && Crafting->IsRecipeKnown(Recipe.RecipeId);
		const bool bCanCraft = bKnown && Crafting && Crafting->CanCraft(Recipe.RecipeId);
		const FLinearColor Accent = !bKnown ? FLinearColor(0.30f, 0.30f, 0.28f, 1.0f) : bCanCraft ? FLinearColor(0.38f, 0.60f, 0.38f, 1.0f) : FLinearColor(0.58f, 0.46f, 0.28f, 1.0f);
		DrawBox(Geometry, OutDrawElements, LayerId, Position + FVector2D(2.0f, 3.0f), Size, FLinearColor(0.0f, 0.0f, 0.0f, 0.20f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position, Size, bSelected ? FLinearColor(0.105f, 0.102f, 0.086f, 0.96f) : FLinearColor(0.040f, 0.042f, 0.040f, 0.86f));
		DrawBox(Geometry, OutDrawElements, LayerId + 2, Position, FVector2D(4.0f, Size.Y), WithAlpha(Accent, bSelected ? 0.90f : 0.55f));
		DrawText(Geometry, OutDrawElements, LayerId + 3, bKnown ? (Recipe.DisplayName.IsEmpty() ? Recipe.RecipeId.ToString() : Recipe.DisplayName.ToString()) : TEXT("Unbekanntes Rezept"), Position + FVector2D(14.0f, 9.0f), bKnown ? FLinearColor(0.92f, 0.90f, 0.80f, 1.0f) : FLinearColor(0.54f, 0.53f, 0.47f, 0.86f), 12, true, Size.X - 24.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 3, !bKnown ? TEXT("noch nicht gelernt") : bCanCraft ? TEXT("bereit") : TEXT("nicht bereit"), Position + FVector2D(14.0f, 30.0f), bCanCraft ? FLinearColor(0.58f, 0.76f, 0.52f, 0.90f) : FLinearColor(0.76f, 0.62f, 0.40f, 0.82f), 10, false, Size.X - 24.0f);

		USurvivalHUDWidget::FRecipeHitBox HitBox;
		HitBox.RecipeId = Recipe.RecipeId;
		HitBox.bCraftButton = false;
		HitBox.Bounds = FSlateRect(Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y);
		RecipeHitBoxes.Add(HitBox);
	}

	void DrawCraftingSurface(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		TArray<USurvivalHUDWidget::FSlotHitBox>& SlotHitBoxes,
		const FVector2D& Position,
		const FVector2D& Size,
		const UCraftingComponent* Crafting,
		FName HoveredItemId,
		FName SelectedItemId)
	{
		DrawBox(Geometry, OutDrawElements, LayerId, Position, Size, FLinearColor(0.045f, 0.040f, 0.032f, 0.92f));
		DrawBox(Geometry, OutDrawElements, LayerId + 1, Position + FVector2D(8.0f, 8.0f), Size - FVector2D(16.0f, 16.0f), FLinearColor(0.078f, 0.066f, 0.045f, 0.80f));
		DrawLine(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(16.0f, Size.Y * 0.38f), Position + FVector2D(Size.X - 18.0f, Size.Y * 0.34f), FLinearColor(0.20f, 0.15f, 0.09f, 0.42f), 2.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 2, Position + FVector2D(18.0f, Size.Y * 0.70f), Position + FVector2D(Size.X - 24.0f, Size.Y * 0.64f), FLinearColor(0.22f, 0.17f, 0.10f, 0.36f), 1.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 3, TEXT("Werkflaeche"), Position + FVector2D(14.0f, 8.0f), FLinearColor(0.72f, 0.68f, 0.56f, 0.84f), 11, true, Size.X - 28.0f);

		if (!Crafting)
		{
			return;
		}

		const TArray<FInventoryStack> InputSlots = Crafting->GetCraftingInputSlots();
		const FVector2D SlotSize(54.0f, 54.0f);
		const float Gap = 8.0f;
		const int32 Columns = FMath::Max(1, FMath::FloorToInt((Size.X - 28.0f + Gap) / (SlotSize.X + Gap)));
		const FVector2D Start = Position + FVector2D(14.0f, 34.0f);
		for (int32 SlotIndex = 0; SlotIndex < InputSlots.Num(); ++SlotIndex)
		{
			const int32 Column = SlotIndex % Columns;
			const int32 Row = SlotIndex / Columns;
			const FVector2D SlotPosition = Start + FVector2D(Column * (SlotSize.X + Gap), Row * (SlotSize.Y + Gap));
			if (SlotPosition.Y + SlotSize.Y > Position.Y + Size.Y - 10.0f)
			{
				break;
			}

			const FSlateRect Bounds(SlotPosition.X, SlotPosition.Y, SlotPosition.X + SlotSize.X, SlotPosition.Y + SlotSize.Y);
			if (InputSlots[SlotIndex].IsEmpty())
			{
				DrawSlotFrame(Geometry, OutDrawElements, LayerId + 10 + SlotIndex * 14, SlotPosition, SlotSize, FLinearColor(0.36f, 0.30f, 0.20f, 1.0f), false, false, false, true);
				AddSlotHitBox(SlotHitBoxes, Bounds, SlotIndex, true);
			}
			else
			{
				DrawInventoryItemInSlot(Geometry, OutDrawElements, LayerId + 10 + SlotIndex * 14, ItemHitBoxes, SlotHitBoxes, SlotPosition, SlotSize, InputSlots[SlotIndex], Crafting, HoveredItemId, SelectedItemId, false, true);
			}
		}
	}

	void DrawCraftingDetails(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		TArray<USurvivalHUDWidget::FSlotHitBox>& SlotHitBoxes,
		const FVector2D& Position,
		const FVector2D& Size,
		const FCraftingRecipe& Recipe,
		const UCraftingComponent* Crafting,
		const UInventoryComponent* Inventory,
		FName HoveredItemId,
		FName SelectedItemId)
	{
		const bool bKnown = Crafting && Crafting->IsRecipeKnown(Recipe.RecipeId);
		const bool bCanCraft = bKnown && Crafting && Crafting->CanCraft(Recipe.RecipeId);
		const FString OutputName = ItemDisplayName(Crafting, Recipe.OutputItemId);
		const ESurvivalItemCategory OutputCategory = ItemCategory(Crafting, Recipe.OutputItemId);
		const FLinearColor Accent = bCanCraft ? FLinearColor(0.40f, 0.68f, 0.42f, 1.0f) : FLinearColor(0.64f, 0.48f, 0.28f, 1.0f);

		DrawPanel(Geometry, OutDrawElements, LayerId, Position, Size, Accent);
		DrawText(Geometry, OutDrawElements, LayerId + 8, Recipe.DisplayName.IsEmpty() ? Recipe.RecipeId.ToString() : Recipe.DisplayName.ToString(), Position + FVector2D(22.0f, 18.0f), FLinearColor(0.94f, 0.92f, 0.82f, 1.0f), 18, true, Size.X - 44.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 8, Recipe.Description.ToString(), Position + FVector2D(22.0f, 50.0f), FLinearColor(0.70f, 0.68f, 0.58f, 0.86f), 11, false, Size.X - 44.0f);

		const FVector2D ResultSlot(Position.X + 24.0f, Position.Y + 91.0f);
		DrawSlotFrame(Geometry, OutDrawElements, LayerId + 12, ResultSlot, FVector2D(78.0f, 78.0f), ItemAccentColor(Recipe.OutputItemId, OutputCategory), false, true, false, Recipe.OutputItemId.IsNone());
		DrawItemGlyph(Geometry, OutDrawElements, LayerId + 18, ResultSlot + FVector2D(39.0f, 38.0f), 0.90f, Recipe.OutputItemId, OutputCategory, true);
		DrawText(Geometry, OutDrawElements, LayerId + 22, FString::Printf(TEXT("%s x%d"), *OutputName, Recipe.OutputCount), Position + FVector2D(116.0f, 103.0f), FLinearColor(0.88f, 0.84f, 0.70f, 1.0f), 14, true, Size.X - 138.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 22, CategoryLabel(OutputCategory), Position + FVector2D(116.0f, 128.0f), FLinearColor(0.62f, 0.60f, 0.52f, 0.86f), 11, false, Size.X - 138.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 22, FString::Printf(TEXT("%.0fs   %s"), Recipe.CraftTimeSeconds, *StationLabel(Recipe.RequiredStation)), Position + FVector2D(116.0f, 149.0f), FLinearColor(0.62f, 0.60f, 0.52f, 0.86f), 10, false, Size.X - 138.0f);

		DrawText(Geometry, OutDrawElements, LayerId + 24, TEXT("Zutaten"), Position + FVector2D(24.0f, 192.0f), FLinearColor(0.78f, 0.74f, 0.62f, 0.92f), 12, true, Size.X - 48.0f);
		for (int32 Index = 0; Index < Recipe.Ingredients.Num(); ++Index)
		{
			const FCraftingIngredient& Ingredient = Recipe.Ingredients[Index];
			const int32 Have = Inventory ? Inventory->GetItemCount(Ingredient.ItemId) : 0;
			const bool bHasEnough = Have >= Ingredient.Count;
			const FVector2D RowPosition = Position + FVector2D(24.0f, 220.0f + Index * 34.0f);
			const FVector2D RowSize(Size.X - 48.0f, 28.0f);
			DrawBox(Geometry, OutDrawElements, LayerId + 25 + Index * 4, RowPosition, RowSize, bHasEnough ? FLinearColor(0.032f, 0.044f, 0.034f, 0.78f) : FLinearColor(0.060f, 0.042f, 0.034f, 0.78f));
			DrawBox(Geometry, OutDrawElements, LayerId + 26 + Index * 4, RowPosition, FVector2D(4.0f, RowSize.Y), bHasEnough ? FLinearColor(0.36f, 0.62f, 0.36f, 0.80f) : FLinearColor(0.72f, 0.38f, 0.24f, 0.80f));
			DrawText(Geometry, OutDrawElements, LayerId + 27 + Index * 4, ItemDisplayName(Crafting, Ingredient.ItemId), RowPosition + FVector2D(12.0f, 5.0f), FLinearColor(0.88f, 0.86f, 0.76f, 0.94f), 11, false, RowSize.X - 80.0f);
			DrawText(Geometry, OutDrawElements, LayerId + 27 + Index * 4, FString::Printf(TEXT("%d / %d"), Have, Ingredient.Count), RowPosition + FVector2D(RowSize.X - 56.0f, 5.0f), bHasEnough ? FLinearColor(0.74f, 0.86f, 0.68f, 0.94f) : FLinearColor(0.92f, 0.58f, 0.42f, 0.94f), 11, true, 52.0f);
		}

		const FVector2D SurfacePosition(Position.X + 24.0f, Position.Y + Size.Y - 184.0f);
		const FVector2D SurfaceSize(Size.X - 196.0f, 136.0f);
		DrawCraftingSurface(Geometry, OutDrawElements, LayerId + 48, ItemHitBoxes, SlotHitBoxes, SurfacePosition, SurfaceSize, Crafting, HoveredItemId, SelectedItemId);

		if (Crafting && Crafting->IsCrafting())
		{
			const FVector2D ProgressPosition(Position.X + Size.X - 172.0f, Position.Y + Size.Y - 88.0f);
			const FVector2D ProgressSize(148.0f, 9.0f);
			DrawBox(Geometry, OutDrawElements, LayerId + 58, ProgressPosition, ProgressSize, FLinearColor(0.010f, 0.012f, 0.012f, 0.88f));
			DrawBox(Geometry, OutDrawElements, LayerId + 59, ProgressPosition, FVector2D(ProgressSize.X * Crafting->GetCraftingProgress(), ProgressSize.Y), FLinearColor(0.62f, 0.54f, 0.34f, 0.86f));
		}

		const FVector2D ButtonSize(148.0f, 38.0f);
		const FVector2D ButtonPosition(Position.X + Size.X - ButtonSize.X - 24.0f, Position.Y + Size.Y - ButtonSize.Y - 24.0f);
		const FVector2D FillButtonPosition(ButtonPosition - FVector2D(0.0f, 46.0f));
		DrawBox(Geometry, OutDrawElements, LayerId + 56, FillButtonPosition + FVector2D(3.0f, 4.0f), ButtonSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.22f));
		DrawBox(Geometry, OutDrawElements, LayerId + 57, FillButtonPosition, ButtonSize, FLinearColor(0.060f, 0.058f, 0.050f, 0.90f));
		DrawText(Geometry, OutDrawElements, LayerId + 58, TEXT("Material legen"), FillButtonPosition + FVector2D(22.0f, 10.0f), FLinearColor(0.78f, 0.74f, 0.62f, 0.90f), 12, true, ButtonSize.X - 32.0f);
		if (bKnown)
		{
			USurvivalHUDWidget::FRecipeHitBox FillHitBox;
			FillHitBox.RecipeId = Recipe.RecipeId;
			FillHitBox.bAutoFillButton = true;
			FillHitBox.Bounds = FSlateRect(FillButtonPosition.X, FillButtonPosition.Y, FillButtonPosition.X + ButtonSize.X, FillButtonPosition.Y + ButtonSize.Y);
			RecipeHitBoxes.Add(FillHitBox);
		}

		DrawBox(Geometry, OutDrawElements, LayerId + 60, ButtonPosition + FVector2D(3.0f, 4.0f), ButtonSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.28f));
		DrawBox(Geometry, OutDrawElements, LayerId + 61, ButtonPosition, ButtonSize, bCanCraft ? FLinearColor(0.18f, 0.30f, 0.20f, 0.96f) : FLinearColor(0.050f, 0.048f, 0.044f, 0.88f));
		DrawBox(Geometry, OutDrawElements, LayerId + 62, ButtonPosition, FVector2D(ButtonSize.X, 2.0f), WithAlpha(Accent, bCanCraft ? 0.72f : 0.26f));
		DrawText(Geometry, OutDrawElements, LayerId + 63, bCanCraft ? TEXT("Craften") : TEXT("Blockiert"), ButtonPosition + FVector2D(38.0f, 10.0f), bCanCraft ? FLinearColor(0.90f, 0.88f, 0.76f, 1.0f) : FLinearColor(0.52f, 0.50f, 0.44f, 1.0f), 12, true, ButtonSize.X - 44.0f);

		if (bCanCraft)
		{
			USurvivalHUDWidget::FRecipeHitBox HitBox;
			HitBox.RecipeId = Recipe.RecipeId;
			HitBox.bCraftButton = true;
			HitBox.Bounds = FSlateRect(ButtonPosition.X, ButtonPosition.Y, ButtonPosition.X + ButtonSize.X, ButtonPosition.Y + ButtonSize.Y);
			RecipeHitBoxes.Add(HitBox);
		}
	}

	void DrawInventoryGridSection(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		TArray<USurvivalHUDWidget::FSlotHitBox>& SlotHitBoxes,
		const FVector2D& Position,
		const FVector2D& Size,
		const FString& Title,
		const TArray<FInventoryStack>& Stacks,
		const UCraftingComponent* Crafting,
		FName HoveredItemId,
		FName SelectedItemId,
		bool bTreatAsEquipped,
		bool bTreatAsHotbar = false,
		bool bWorldInventory = false,
		int32 EquippedSlotIndex = INDEX_NONE)
	{
		DrawPanel(Geometry, OutDrawElements, LayerId, Position, Size);
		DrawText(Geometry, OutDrawElements, LayerId + 8, Title, Position + FVector2D(18.0f, 14.0f), FLinearColor(0.82f, 0.78f, 0.66f, 0.92f), 13, true, Size.X - 36.0f);
		DrawLine(Geometry, OutDrawElements, LayerId + 8, Position + FVector2D(18.0f, 40.0f), Position + FVector2D(Size.X - 18.0f, 40.0f), FLinearColor(0.54f, 0.48f, 0.32f, 0.34f), 1.0f);

		const float Gap = 10.0f;
		const FVector2D SlotSize(InventorySlotSize, InventorySlotSize);
		const int32 Columns = FMath::Max(1, FMath::FloorToInt((Size.X - 36.0f + Gap) / (SlotSize.X + Gap)));
		const int32 Rows = FMath::Max(1, FMath::FloorToInt((Size.Y - 56.0f + Gap) / (SlotSize.Y + Gap)));
		const int32 VisibleSlots = Columns * Rows;
		const FVector2D GridStart = Position + FVector2D(18.0f, 52.0f);

		for (int32 SlotIndex = 0; SlotIndex < VisibleSlots; ++SlotIndex)
		{
			const int32 Column = SlotIndex % Columns;
			const int32 Row = SlotIndex / Columns;
			const FVector2D SlotPosition = GridStart + FVector2D(Column * (SlotSize.X + Gap), Row * (SlotSize.Y + Gap));
			const FSlateRect SlotBounds(SlotPosition.X, SlotPosition.Y, SlotPosition.X + SlotSize.X, SlotPosition.Y + SlotSize.Y);
			if (Stacks.IsValidIndex(SlotIndex) && !Stacks[SlotIndex].IsEmpty())
			{
				const bool bEquipped = bTreatAsEquipped && Stacks[SlotIndex].SlotIndex == EquippedSlotIndex;
				DrawInventoryItemInSlot(Geometry, OutDrawElements, LayerId + 12 + SlotIndex * 18, ItemHitBoxes, SlotHitBoxes, SlotPosition, SlotSize, Stacks[SlotIndex], Crafting, HoveredItemId, SelectedItemId, bEquipped, false, bTreatAsHotbar, bWorldInventory, bTreatAsHotbar ? SlotIndex : INDEX_NONE);
			}
			else
			{
				DrawSlotFrame(Geometry, OutDrawElements, LayerId + 12 + SlotIndex * 18, SlotPosition, SlotSize, FLinearColor(0.26f, 0.24f, 0.20f, 1.0f), false, false, false, true);
				if (Stacks.IsValidIndex(SlotIndex) || bTreatAsHotbar)
				{
					const int32 HitSlotIndex = Stacks.IsValidIndex(SlotIndex) ? Stacks[SlotIndex].SlotIndex : INDEX_NONE;
					AddSlotHitBox(SlotHitBoxes, SlotBounds, HitSlotIndex, false, bTreatAsHotbar, bWorldInventory, bTreatAsHotbar ? SlotIndex : INDEX_NONE);
				}
			}
		}
	}

	void DrawInventoryTooltip(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		const UCraftingComponent* Crafting,
		FName TooltipItemId,
		const FVector2D& ViewSize)
	{
		if (TooltipItemId.IsNone())
		{
			return;
		}

		for (const USurvivalHUDWidget::FInventoryItemHitBox& HitBox : ItemHitBoxes)
		{
			if (HitBox.ItemId != TooltipItemId)
			{
				continue;
			}

			const FVector2D Size(292.0f, HitBox.Description.IsEmpty() ? 104.0f : 138.0f);
			FVector2D Position(HitBox.Bounds.Right + 14.0f, HitBox.Bounds.Top - 6.0f);
			if (Position.X + Size.X > ViewSize.X - 24.0f)
			{
				Position.X = HitBox.Bounds.Left - Size.X - 14.0f;
			}
			if (Position.Y + Size.Y > ViewSize.Y - 24.0f)
			{
				Position.Y = ViewSize.Y - Size.Y - 24.0f;
			}
			Position.Y = FMath::Max(24.0f, Position.Y);

			DrawPanel(Geometry, OutDrawElements, LayerId, Position, Size, FLinearColor(0.58f, 0.48f, 0.30f, 1.0f));
			DrawText(Geometry, OutDrawElements, LayerId + 8, HitBox.DisplayName, Position + FVector2D(16.0f, 14.0f), FLinearColor(0.94f, 0.92f, 0.82f, 1.0f), 15, true, Size.X - 32.0f);
			DrawText(Geometry, OutDrawElements, LayerId + 8, FString::Printf(TEXT("%s   Bestand: %d"), *HitBox.Category, HitBox.Count), Position + FVector2D(16.0f, 42.0f), FLinearColor(0.70f, 0.68f, 0.58f, 0.92f), 11, false, Size.X - 32.0f);
			if (!HitBox.Description.IsEmpty())
			{
				DrawText(Geometry, OutDrawElements, LayerId + 8, HitBox.Description, Position + FVector2D(16.0f, 70.0f), FLinearColor(0.82f, 0.80f, 0.70f, 0.90f), 11, false, Size.X - 32.0f);
			}
			FString MetaText = TEXT("Gewicht/Zustand: keine Itemdaten");
			FItemDef ItemDef;
			if (Crafting && Crafting->GetItemDefinition(HitBox.ItemId, ItemDef))
			{
				MetaText = FString::Printf(TEXT("%s   %.2f kg   Stack %d"), *RarityLabel(ItemDef.Rarity), ItemDef.WeightKg, ItemDef.GetEffectiveMaxStack());
				if (ItemDef.bHasDurability)
				{
					MetaText += FString::Printf(TEXT("   Haltbarkeit %.0f / %.0f"), HitBox.Durability, ItemDef.MaxDurability);
				}
			}
			DrawText(Geometry, OutDrawElements, LayerId + 8, MetaText, Position + FVector2D(16.0f, Size.Y - 30.0f), FLinearColor(0.50f, 0.49f, 0.43f, 0.82f), 10, false, Size.X - 32.0f);
			return;
		}
	}

	void DrawContextMenu(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FContextActionHitBox>& ContextActionHitBoxes,
		int32 ContextSlotIndex,
		bool bContextWorldInventory,
		const FVector2D& DesiredPosition,
		const UInventoryComponent* Inventory,
		const UInventoryComponent* WorldInventory,
		const UCraftingComponent* Crafting,
		const FVector2D& ViewSize)
	{
		const UInventoryComponent* SourceInventory = bContextWorldInventory ? WorldInventory : Inventory;
		if (!SourceInventory || ContextSlotIndex == INDEX_NONE)
		{
			return;
		}

		const FInventoryStack Slot = SourceInventory->GetSlot(ContextSlotIndex);
		if (Slot.IsEmpty())
		{
			return;
		}

		const FVector2D Size(172.0f, bContextWorldInventory ? 104.0f : 204.0f);
		FVector2D Position = DesiredPosition + FVector2D(10.0f, 10.0f);
		Position.X = FMath::Clamp(Position.X, 16.0f, FMath::Max(16.0f, ViewSize.X - Size.X - 16.0f));
		Position.Y = FMath::Clamp(Position.Y, 16.0f, FMath::Max(16.0f, ViewSize.Y - Size.Y - 16.0f));
		DrawPanel(Geometry, OutDrawElements, LayerId, Position, Size, FLinearColor(0.58f, 0.48f, 0.30f, 1.0f));
		DrawText(Geometry, OutDrawElements, LayerId + 8, Crafting ? Crafting->GetItemDisplayName(Slot.ItemId).ToString() : Slot.ItemId.ToString(), Position + FVector2D(14.0f, 12.0f), FLinearColor(0.92f, 0.90f, 0.80f, 1.0f), 12, true, Size.X - 28.0f);

		TArray<TPair<FName, FString>> Actions;
		if (bContextWorldInventory)
		{
			Actions = {
				TPair<FName, FString>(TEXT("Take"), TEXT("Nehmen")),
				TPair<FName, FString>(TEXT("Inspect"), TEXT("Untersuchen"))
			};
		}
		else
		{
			Actions = {
				TPair<FName, FString>(TEXT("Use"), TEXT("Benutzen")),
				TPair<FName, FString>(TEXT("Equip"), TEXT("Ausruesten")),
				TPair<FName, FString>(TEXT("Split"), TEXT("Teilen")),
				TPair<FName, FString>(TEXT("Drop"), TEXT("Fallenlassen")),
				TPair<FName, FString>(TEXT("Store"), TEXT("Einlagern")),
				TPair<FName, FString>(TEXT("Inspect"), TEXT("Untersuchen"))
			};
		}

		for (int32 Index = 0; Index < Actions.Num(); ++Index)
		{
			const FVector2D RowPosition = Position + FVector2D(10.0f, 40.0f + Index * 26.0f);
			const FVector2D RowSize(Size.X - 20.0f, 22.0f);
			DrawBox(Geometry, OutDrawElements, LayerId + 10 + Index * 3, RowPosition, RowSize, FLinearColor(0.036f, 0.038f, 0.036f, 0.92f));
			DrawText(Geometry, OutDrawElements, LayerId + 11 + Index * 3, Actions[Index].Value, RowPosition + FVector2D(10.0f, 4.0f), FLinearColor(0.78f, 0.76f, 0.66f, 0.92f), 10, false, RowSize.X - 20.0f);

			USurvivalHUDWidget::FContextActionHitBox HitBox;
			HitBox.ActionId = Actions[Index].Key;
			HitBox.SlotIndex = ContextSlotIndex;
			HitBox.bWorldInventory = bContextWorldInventory;
			HitBox.Bounds = FSlateRect(RowPosition.X, RowPosition.Y, RowPosition.X + RowSize.X, RowPosition.Y + RowSize.Y);
			ContextActionHitBoxes.Add(HitBox);
		}
	}

	void DrawInventoryOverlay(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		TArray<USurvivalHUDWidget::FRecipeHitBox>& RecipeHitBoxes,
		TArray<USurvivalHUDWidget::FInventoryItemHitBox>& ItemHitBoxes,
		TArray<USurvivalHUDWidget::FSlotHitBox>& SlotHitBoxes,
		TArray<USurvivalHUDWidget::FContextActionHitBox>& ContextActionHitBoxes,
		FName HoveredItemId,
		FName SelectedItemId,
		FName& SelectedRecipeId,
		int32 ContextSlotIndex,
		bool bContextWorldInventory,
		const FVector2D& ContextMenuPosition,
		const ASurvivalCharacter* SurvivalCharacter,
		const FVector2D& ViewSize)
	{
		RecipeHitBoxes.Reset();
		ItemHitBoxes.Reset();
		SlotHitBoxes.Reset();
		ContextActionHitBoxes.Reset();
		if (!SurvivalCharacter || !SurvivalCharacter->InventoryComponent)
		{
			return;
		}

		const UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
		const UCraftingComponent* Crafting = SurvivalCharacter->CraftingComponent;
		const ASurvivalPlayerController* Controller = Cast<ASurvivalPlayerController>(SurvivalCharacter->GetController());
		const UInventoryComponent* WorldInventory = Controller ? Controller->GetOpenedWorldInventory() : nullptr;
		const float PanelMarginX = FMath::Clamp(ViewSize.X * 0.045f, 38.0f, 72.0f);
		const float PanelMarginY = FMath::Clamp(ViewSize.Y * 0.060f, 40.0f, 68.0f);
		const FVector2D PanelPosition(PanelMarginX, PanelMarginY);
		const FVector2D PanelSize(FMath::Max(860.0f, ViewSize.X - PanelMarginX * 2.0f), FMath::Max(580.0f, ViewSize.Y - PanelMarginY * 2.0f));

		DrawBox(Geometry, OutDrawElements, LayerId, FVector2D::ZeroVector, ViewSize, FLinearColor(0.002f, 0.003f, 0.003f, 0.84f));
		DrawPanel(Geometry, OutDrawElements, LayerId + 1, PanelPosition, PanelSize, FLinearColor(0.55f, 0.48f, 0.33f, 1.0f));
		DrawText(Geometry, OutDrawElements, LayerId + 9, TEXT("INVENTAR"), PanelPosition + FVector2D(30.0f, 24.0f), FLinearColor(0.90f, 0.88f, 0.76f, 0.96f), 20, true, 260.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 9, TEXT("Tab / Esc"), PanelPosition + FVector2D(PanelSize.X - 104.0f, 31.0f), FLinearColor(0.62f, 0.60f, 0.52f, 0.82f), 11, false, 90.0f);

		TArray<FInventoryStack> AllSlots = Inventory->GetSlots();
		TArray<FInventoryStack> HotbarStacks = Inventory->GetHotbarStacks();
		TArray<FInventoryStack> ToolStacks;
		for (const FInventoryStack& Stack : AllSlots)
		{
			if (Stack.IsEmpty())
			{
				continue;
			}
			const ESurvivalItemCategory Category = ItemCategory(Crafting, Stack.ItemId);
			if (Category == ESurvivalItemCategory::Tool || Category == ESurvivalItemCategory::Weapon || Category == ESurvivalItemCategory::Armor)
			{
				ToolStacks.Add(Stack);
			}
		}

		const float HeaderHeight = 70.0f;
		const float BottomHeight = 106.0f;
		const float ContentTop = PanelPosition.Y + HeaderHeight;
		const float ContentHeight = PanelSize.Y - HeaderHeight - BottomHeight - 26.0f;
		const float Gap = 18.0f;
		const float LeftWidth = FMath::Clamp(PanelSize.X * 0.34f, 300.0f, 430.0f);
		const float CraftWidth = FMath::Clamp(PanelSize.X * 0.42f, 430.0f, 600.0f);
		const float ToolWidth = PanelSize.X - LeftWidth - CraftWidth - Gap * 4.0f;

		const FVector2D PackPosition(PanelPosition.X + Gap, ContentTop);
		const FVector2D CraftListPosition(PackPosition.X + LeftWidth + Gap, ContentTop);
		const FVector2D ToolsPosition(PanelPosition.X + PanelSize.X - Gap - ToolWidth, ContentTop);
		const FVector2D BottomPosition(PanelPosition.X + Gap, PanelPosition.Y + PanelSize.Y - BottomHeight - Gap);

		DrawInventoryGridSection(Geometry, OutDrawElements, LayerId + 20, ItemHitBoxes, SlotHitBoxes, PackPosition, FVector2D(LeftWidth, ContentHeight), TEXT("Rucksack"), AllSlots, Crafting, HoveredItemId, SelectedItemId, false, false, false, Inventory->GetEquippedSlotIndex());
		if (WorldInventory)
		{
			DrawInventoryGridSection(Geometry, OutDrawElements, LayerId + 130, ItemHitBoxes, SlotHitBoxes, ToolsPosition, FVector2D(ToolWidth, ContentHeight), TEXT("Container"), WorldInventory->GetSlots(), Crafting, HoveredItemId, SelectedItemId, false, false, true, INDEX_NONE);

			if (const ACampfireActor* Campfire = Cast<ACampfireActor>(Controller ? Controller->GetOpenedWorldInventoryOwner() : nullptr))
			{
				const FString FireState = Campfire->bIsLit
					? FString::Printf(TEXT("Feuer %.0fs"), Campfire->FuelSecondsRemaining)
					: TEXT("Feuer aus");
				DrawText(Geometry, OutDrawElements, LayerId + 140, FireState, ToolsPosition + FVector2D(18.0f, ContentHeight - 34.0f), Campfire->bIsLit ? FLinearColor(0.92f, 0.56f, 0.30f, 0.96f) : FLinearColor(0.62f, 0.60f, 0.52f, 0.84f), 11, true, ToolWidth - 36.0f);
			}
		}
		else
		{
			DrawInventoryGridSection(Geometry, OutDrawElements, LayerId + 130, ItemHitBoxes, SlotHitBoxes, ToolsPosition, FVector2D(ToolWidth, ContentHeight), TEXT("Ausrustung"), ToolStacks, Crafting, HoveredItemId, SelectedItemId, true, false, false, Inventory->GetEquippedSlotIndex());
		}
		DrawInventoryGridSection(Geometry, OutDrawElements, LayerId + 220, ItemHitBoxes, SlotHitBoxes, BottomPosition, FVector2D(PanelSize.X - Gap * 2.0f, BottomHeight), TEXT("Quickslots"), HotbarStacks, Crafting, HoveredItemId, SelectedItemId, true, true, false, Inventory->GetEquippedSlotIndex());

		DrawText(Geometry, OutDrawElements, LayerId + 222, FString::Printf(TEXT("%.1f / %.1f kg"), Inventory->GetCurrentWeightKg(), Inventory->GetMaxWeightKg()), BottomPosition + FVector2D(PanelSize.X - Gap * 2.0f - 116.0f, 14.0f), FLinearColor(0.70f, 0.68f, 0.58f, 0.86f), 11, true, 104.0f);

		DrawPanel(Geometry, OutDrawElements, LayerId + 320, CraftListPosition, FVector2D(CraftWidth, ContentHeight), FLinearColor(0.46f, 0.50f, 0.42f, 1.0f));
		DrawText(Geometry, OutDrawElements, LayerId + 328, TEXT("CRAFTING"), CraftListPosition + FVector2D(20.0f, 14.0f), FLinearColor(0.82f, 0.78f, 0.66f, 0.92f), 13, true, CraftWidth - 40.0f);

		if (Crafting)
		{
			const FText CraftingMessage = Crafting->GetLastCraftingMessage();
			if (!CraftingMessage.IsEmpty())
			{
				const FLinearColor MessageColor = Crafting->IsLastCraftingMessageError()
					? FLinearColor(0.88f, 0.48f, 0.34f, 0.94f)
					: FLinearColor(0.58f, 0.76f, 0.52f, 0.94f);
				DrawText(Geometry, OutDrawElements, LayerId + 329, CraftingMessage.ToString(), CraftListPosition + FVector2D(124.0f, 14.0f), MessageColor, 11, true, CraftWidth - 146.0f);
			}

			const TArray<FCraftingRecipe> Recipes = Crafting->GetVisibleRecipes(true);
			if ((SelectedRecipeId.IsNone() || !RecipeExists(Recipes, SelectedRecipeId)) && Recipes.Num() > 0)
			{
				SelectedRecipeId = Recipes[0].RecipeId;
			}

			const FVector2D RecipeListPosition = CraftListPosition + FVector2D(18.0f, 50.0f);
			const FVector2D RecipeListSize(FMath::Clamp(CraftWidth * 0.36f, 150.0f, 210.0f), ContentHeight - 70.0f);
			const FVector2D RowSize(RecipeListSize.X, 54.0f);
			for (int32 Index = 0; Index < Recipes.Num(); ++Index)
			{
				const FVector2D RowPosition = RecipeListPosition + FVector2D(0.0f, Index * 62.0f);
				if (RowPosition.Y + RowSize.Y > CraftListPosition.Y + ContentHeight - 16.0f)
				{
					break;
				}
				DrawRecipeRow(Geometry, OutDrawElements, LayerId + 332 + Index * 8, RecipeHitBoxes, RowPosition, RowSize, Recipes[Index], Crafting, Recipes[Index].RecipeId == SelectedRecipeId);
			}

			const FVector2D DetailPosition = RecipeListPosition + FVector2D(RecipeListSize.X + 16.0f, 0.0f);
			const FVector2D DetailSize(CraftListPosition.X + CraftWidth - DetailPosition.X - 18.0f, RecipeListSize.Y);
			if (const FCraftingRecipe* SelectedRecipe = FindRecipe(Recipes, SelectedRecipeId))
			{
				DrawCraftingDetails(Geometry, OutDrawElements, LayerId + 410, RecipeHitBoxes, ItemHitBoxes, SlotHitBoxes, DetailPosition, DetailSize, *SelectedRecipe, Crafting, Inventory, HoveredItemId, SelectedItemId);
			}
		}

		const FName TooltipItemId = !HoveredItemId.IsNone() ? HoveredItemId : SelectedItemId;
		DrawInventoryTooltip(Geometry, OutDrawElements, LayerId + 820, ItemHitBoxes, Crafting, TooltipItemId, ViewSize);
		DrawContextMenu(Geometry, OutDrawElements, LayerId + 850, ContextActionHitBoxes, ContextSlotIndex, bContextWorldInventory, ContextMenuPosition, Inventory, WorldInventory, Crafting, ViewSize);
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
		DrawFilledCircle(Geometry, OutDrawElements, LayerId + 1, ScreenCenter, Radius + 28.0f, FLinearColor(0.014f, 0.017f, 0.017f, 0.96f));
		DrawCircleOutline(Geometry, OutDrawElements, LayerId + 2, ScreenCenter, Radius + 22.0f, FLinearColor(0.30f, 0.27f, 0.20f, 0.80f), 4.0f);
		DrawMapBackground(Geometry, OutDrawElements, LayerId + 3, ScreenCenter, Radius);
		DrawMapMarkers(Geometry, OutDrawElements, LayerId + 8, Widget, SurvivalCharacter, ScreenCenter, Radius, MinimapWorldRadius * 3.5f, true);
		DrawPlayerArrow(Geometry, OutDrawElements, LayerId + 11, ScreenCenter, SurvivalCharacter);
		DrawText(Geometry, OutDrawElements, LayerId + 12, TEXT("WELTKARTE"), ScreenCenter + FVector2D(-58.0f, -Radius - 76.0f), FLinearColor(0.88f, 0.86f, 0.76f, 1.0f), 20, true, 160.0f);
		DrawText(Geometry, OutDrawElements, LayerId + 12, TEXT("M / Esc schliessen"), ScreenCenter + FVector2D(-63.0f, Radius + 44.0f), FLinearColor(0.70f, 0.68f, 0.58f, 1.0f), 13, false, 160.0f);
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
	SlotHitBoxes.Reset();
	ContextActionHitBoxes.Reset();

	const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
	const ASurvivalPlayerController* Controller = GetSurvivalController(this);
	const ASurvivalCharacter* SurvivalCharacter = GetSurvivalCharacter(this);

	if (!Controller || !SurvivalCharacter)
	{
		return CurrentLayer;
	}

	if (Controller->IsInventoryOpen())
	{
		DrawInventoryOverlay(AllottedGeometry, OutDrawElements, CurrentLayer + 1, RecipeHitBoxes, InventoryItemHitBoxes, SlotHitBoxes, ContextActionHitBoxes, HoveredItemId, SelectedItemId, SelectedRecipeId, ContextSlotIndex, bContextFromWorldInventory, ContextMenuPosition, SurvivalCharacter, ViewSize);
		return CurrentLayer + 900;
	}

	HoveredItemId = NAME_None;
	ContextSlotIndex = INDEX_NONE;
	bContextFromWorldInventory = false;

	if (Controller->IsMapOpen())
	{
		DrawLargeMapOverlay(AllottedGeometry, OutDrawElements, CurrentLayer + 1, this, SurvivalCharacter, ViewSize);
		return CurrentLayer + 80;
	}

	DrawGameplayHud(AllottedGeometry, OutDrawElements, CurrentLayer + 1, this, SurvivalCharacter, ViewSize);
	return CurrentLayer + 180;
}

FReply USurvivalHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ASurvivalPlayerController* Controller = GetSurvivalController(this);
	ASurvivalCharacter* SurvivalCharacter = GetSurvivalCharacter(this);
	if (!Controller || !Controller->IsInventoryOpen())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D LocalMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	LastMousePosition = LocalMousePosition;

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ContextSlotIndex = INDEX_NONE;
		bContextFromWorldInventory = false;
		for (const FInventoryItemHitBox& HitBox : InventoryItemHitBoxes)
		{
			if (!HitBox.bCraftingInput && ContainsPoint(HitBox.Bounds, LocalMousePosition))
			{
				ContextSlotIndex = HitBox.SlotIndex;
				bContextFromWorldInventory = HitBox.bWorldInventory;
				ContextMenuPosition = LocalMousePosition;
				SelectedSlotIndex = HitBox.SlotIndex;
				SelectedItemId = HitBox.ItemId;
				return FReply::Handled();
			}
		}
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	for (const FContextActionHitBox& HitBox : ContextActionHitBoxes)
	{
		if (!ContainsPoint(HitBox.Bounds, LocalMousePosition) || !SurvivalCharacter || !SurvivalCharacter->InventoryComponent)
		{
			continue;
		}

		UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
		UInventoryComponent* WorldInventory = Controller ? Controller->GetOpenedWorldInventory() : nullptr;
		UInventoryComponent* SourceInventory = HitBox.bWorldInventory ? WorldInventory : Inventory;
		if (HitBox.ActionId == TEXT("Use"))
		{
			Inventory->UseSlot(HitBox.SlotIndex);
		}
		else if (HitBox.ActionId == TEXT("Equip"))
		{
			Inventory->EquipSlot(HitBox.SlotIndex);
		}
		else if (HitBox.ActionId == TEXT("Split"))
		{
			Inventory->SplitStackHalf(HitBox.SlotIndex);
		}
		else if (HitBox.ActionId == TEXT("Drop"))
		{
			Inventory->DropSlot(HitBox.SlotIndex);
		}
		else if (HitBox.ActionId == TEXT("Store") && WorldInventory)
		{
			Inventory->TransferSlotTo(WorldInventory, HitBox.SlotIndex, -1);
		}
		else if (HitBox.ActionId == TEXT("Take") && WorldInventory)
		{
			WorldInventory->TransferSlotTo(Inventory, HitBox.SlotIndex, -1);
		}
		else if (HitBox.ActionId == TEXT("Inspect"))
		{
			SelectedSlotIndex = HitBox.SlotIndex;
			SelectedItemId = SourceInventory ? SourceInventory->GetSlot(HitBox.SlotIndex).ItemId : NAME_None;
		}

		ContextSlotIndex = INDEX_NONE;
		bContextFromWorldInventory = false;
		return FReply::Handled();
	}

	for (const FRecipeHitBox& HitBox : RecipeHitBoxes)
	{
		if (!ContainsPoint(HitBox.Bounds, LocalMousePosition))
		{
			continue;
		}

		SelectedRecipeId = HitBox.RecipeId;
		if (HitBox.bAutoFillButton && SurvivalCharacter && SurvivalCharacter->CraftingComponent && SurvivalCharacter->InventoryComponent)
		{
			UCraftingComponent* Crafting = SurvivalCharacter->CraftingComponent;
			UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
			const TArray<FCraftingRecipe> Recipes = Crafting->GetVisibleRecipes(true);
			if (const FCraftingRecipe* Recipe = FindRecipe(Recipes, HitBox.RecipeId))
			{
				Crafting->ClearCraftingInputs();
				for (const FCraftingIngredient& Ingredient : Recipe->Ingredients)
				{
					int32 Remaining = Ingredient.Count;
					for (const FInventoryStack& Slot : Inventory->GetSlots())
					{
						if (Remaining <= 0)
						{
							break;
						}
						if (Slot.ItemId == Ingredient.ItemId && Slot.Count > 0)
						{
							const int32 MoveCount = FMath::Min(Remaining, Slot.Count);
							if (Crafting->AddInventorySlotToCrafting(Slot.SlotIndex, MoveCount))
							{
								Remaining -= MoveCount;
							}
						}
					}
				}
			}
		}
		else if (HitBox.bCraftButton && SurvivalCharacter && SurvivalCharacter->CraftingComponent)
		{
			SurvivalCharacter->CraftingComponent->CraftRecipe(HitBox.RecipeId);
		}
		return FReply::Handled();
	}

	for (const FInventoryItemHitBox& HitBox : InventoryItemHitBoxes)
	{
		if (ContainsPoint(HitBox.Bounds, LocalMousePosition))
		{
			SelectedItemId = HitBox.ItemId;
			SelectedSlotIndex = HitBox.SlotIndex;
			ContextSlotIndex = INDEX_NONE;
			bContextFromWorldInventory = false;
			DraggedSlotIndex = HitBox.SlotIndex;
			bDraggedFromCraftingInput = HitBox.bCraftingInput;
			bDraggedFromWorldInventory = HitBox.bWorldInventory;
			return FReply::Handled();
		}
	}

	ContextSlotIndex = INDEX_NONE;
	bContextFromWorldInventory = false;
	return FReply::Handled();
}

FReply USurvivalHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ASurvivalPlayerController* Controller = GetSurvivalController(this);
	ASurvivalCharacter* SurvivalCharacter = GetSurvivalCharacter(this);
	if (!Controller || !Controller->IsInventoryOpen() || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	const int32 ReleasedSlotIndex = DraggedSlotIndex;
	const bool bReleasedFromCraftingInput = bDraggedFromCraftingInput;
	const bool bReleasedFromWorldInventory = bDraggedFromWorldInventory;
	DraggedSlotIndex = INDEX_NONE;
	bDraggedFromCraftingInput = false;
	bDraggedFromWorldInventory = false;

	if (ReleasedSlotIndex == INDEX_NONE || !SurvivalCharacter)
	{
		return FReply::Handled();
	}

	const FVector2D LocalMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	for (const FSlotHitBox& HitBox : SlotHitBoxes)
	{
		if (!ContainsPoint(HitBox.Bounds, LocalMousePosition))
		{
			continue;
		}

		if (bReleasedFromCraftingInput)
		{
			if (!HitBox.bCraftingInput && SurvivalCharacter->CraftingComponent)
			{
				SurvivalCharacter->CraftingComponent->RemoveCraftingInput(ReleasedSlotIndex);
			}
		}
		else if (SurvivalCharacter->InventoryComponent)
		{
			UInventoryComponent* PlayerInventory = SurvivalCharacter->InventoryComponent;
			UInventoryComponent* WorldInventory = Controller ? Controller->GetOpenedWorldInventory() : nullptr;
			UInventoryComponent* SourceInventory = bReleasedFromWorldInventory ? WorldInventory : PlayerInventory;
			UInventoryComponent* TargetInventory = HitBox.bWorldInventory ? WorldInventory : PlayerInventory;
			if (SourceInventory && TargetInventory && SourceInventory != TargetInventory && !HitBox.bCraftingInput && !HitBox.bHotbarSlot)
			{
				SourceInventory->TransferSlotTo(TargetInventory, ReleasedSlotIndex, -1);
			}
			else if (!bReleasedFromWorldInventory && HitBox.bHotbarSlot)
			{
				PlayerInventory->AssignSlotToHotbar(ReleasedSlotIndex, HitBox.HotbarIndex);
			}
			else if (!bReleasedFromWorldInventory && HitBox.bCraftingInput && SurvivalCharacter->CraftingComponent)
			{
				SurvivalCharacter->CraftingComponent->AddInventorySlotToCrafting(ReleasedSlotIndex, -1);
			}
			else if (!bReleasedFromWorldInventory && HitBox.SlotIndex != INDEX_NONE && HitBox.SlotIndex != ReleasedSlotIndex)
			{
				PlayerInventory->MoveStack(ReleasedSlotIndex, HitBox.SlotIndex);
			}
		}

		return FReply::Handled();
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
	LastMousePosition = LocalMousePosition;
	HoveredItemId = NAME_None;
	for (const FInventoryItemHitBox& HitBox : InventoryItemHitBoxes)
	{
		if (ContainsPoint(HitBox.Bounds, LocalMousePosition))
		{
			HoveredItemId = HitBox.ItemId;
			break;
		}
	}

	return FReply::Handled();
}

void USurvivalHUDWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	HoveredItemId = NAME_None;
	DraggedSlotIndex = INDEX_NONE;
	bDraggedFromCraftingInput = false;
	bDraggedFromWorldInventory = false;
	Super::NativeOnMouseLeave(InMouseEvent);
}
