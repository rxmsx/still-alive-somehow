#include "UI/SurvivalHUD.h"

#include "Interfaces/Interactable.h"
#include "Items/InventoryComponent.h"
#include "Player/SurvivalCharacter.h"
#include "Resources/ResourceNodeActor.h"
#include "Resources/ResourceNodeComponent.h"
#include "Survival/SurvivalStatsComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

namespace
{
	constexpr float HudMargin = 28.0f;
	constexpr float MinimapOuterRadius = 108.0f;
	constexpr float MinimapRadius = 76.0f;
	constexpr float MinimapWorldRadius = 4500.0f;
}

void ASurvivalHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !PlayerOwner)
	{
		return;
	}

	const ASurvivalCharacter* SurvivalCharacter = Cast<ASurvivalCharacter>(PlayerOwner->GetPawn());
	if (!SurvivalCharacter)
	{
		return;
	}

	DrawSurvivalMinimap(SurvivalCharacter);
	DrawInventorySummary(SurvivalCharacter);
	DrawCrosshairAndPrompt(SurvivalCharacter);
}

void ASurvivalHUD::DrawSurvivalMinimap(const ASurvivalCharacter* SurvivalCharacter)
{
	if (!SurvivalCharacter || !SurvivalCharacter->SurvivalStatsComponent)
	{
		return;
	}

	const USurvivalStatsComponent* Stats = SurvivalCharacter->SurvivalStatsComponent;

	const FVector2D Center(HudMargin + MinimapOuterRadius, HudMargin + MinimapOuterRadius);
	DrawFilledCircle(Center, MinimapOuterRadius + 9.0f, FLinearColor(0.015f, 0.018f, 0.022f, 0.76f));
	DrawMapBackground(Center, MinimapRadius);
	DrawMapResources(Center, MinimapRadius, SurvivalCharacter);
	DrawPlayerArrow(Center, SurvivalCharacter);
	DrawHealthAndStaminaRings(Center, MinimapRadius + 16.0f, Stats->Health, Stats->Stamina);

	const FVector2D HungerCenter = Center + FVector2D(44.0f, -110.0f);
	const FVector2D ThirstCenter = Center + FVector2D(84.0f, -84.0f);
	DrawStatusIcon(HungerCenter, Stats->Hunger, FLinearColor(0.96f, 0.62f, 0.15f, 0.95f), false);
	DrawStatusIcon(ThirstCenter, Stats->Thirst, FLinearColor(0.12f, 0.62f, 0.95f, 0.95f), true);
}

void ASurvivalHUD::DrawInventorySummary(const ASurvivalCharacter* SurvivalCharacter)
{
	if (!SurvivalCharacter || !SurvivalCharacter->InventoryComponent)
	{
		return;
	}

	const UInventoryComponent* Inventory = SurvivalCharacter->InventoryComponent;
	const FString InventoryText = FString::Printf(
		TEXT("Inventory  Axe:%d  Pickaxe:%d  Wood:%d  Stone:%d"),
		Inventory->GetItemCount(FName(TEXT("Axe"))),
		Inventory->GetItemCount(FName(TEXT("Pickaxe"))),
		Inventory->GetItemCount(FName(TEXT("Wood"))),
		Inventory->GetItemCount(FName(TEXT("Stone"))));

	DrawText(InventoryText, FLinearColor::White, HudMargin, HudMargin + (MinimapOuterRadius * 2.0f) + 26.0f, nullptr, 0.82f);
}

void ASurvivalHUD::DrawCrosshairAndPrompt(const ASurvivalCharacter* SurvivalCharacter)
{
	if (!SurvivalCharacter || !SurvivalCharacter->FirstPersonCamera || !Canvas)
	{
		return;
	}

	const float CenterX = Canvas->SizeX * 0.5f;
	const float CenterY = Canvas->SizeY * 0.5f;

	DrawLine(CenterX - 6.0f, CenterY, CenterX + 6.0f, CenterY, FLinearColor::White, 1.0f);
	DrawLine(CenterX, CenterY - 6.0f, CenterX, CenterY + 6.0f, FLinearColor::White, 1.0f);

	const FVector TraceStart = SurvivalCharacter->FirstPersonCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + SurvivalCharacter->FirstPersonCamera->GetForwardVector() * SurvivalCharacter->InteractionRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalHudInteractTrace), false, SurvivalCharacter);
	FHitResult HitResult;
	if (!GetWorld() || !GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || !HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return;
	}

	const bool bCanInteract = IInteractable::Execute_CanInteract(HitActor, SurvivalCharacter);
	const FText Prompt = IInteractable::Execute_GetInteractionPrompt(HitActor, SurvivalCharacter);
	const FString PromptText = FString::Printf(TEXT("F - %s"), *Prompt.ToString());
	const FLinearColor PromptColor = bCanInteract ? FLinearColor::White : FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);

	DrawText(PromptText, PromptColor, CenterX - 80.0f, CenterY + 24.0f, nullptr, 1.0f);
}

void ASurvivalHUD::DrawMapBackground(const FVector2D& Center, float Radius)
{
	DrawFilledCircle(Center, Radius, FLinearColor(0.34f, 0.48f, 0.35f, 1.0f));
	DrawFilledCircle(Center + FVector2D(-20.0f, 12.0f), Radius * 0.46f, FLinearColor(0.43f, 0.56f, 0.38f, 0.8f));
	DrawFilledCircle(Center + FVector2D(26.0f, -22.0f), Radius * 0.38f, FLinearColor(0.27f, 0.43f, 0.34f, 0.72f));

	const FLinearColor RoadColor(0.78f, 0.73f, 0.56f, 0.72f);
	DrawLine(Center.X - 62.0f, Center.Y - 8.0f, Center.X - 26.0f, Center.Y + 8.0f, RoadColor, 2.0f);
	DrawLine(Center.X - 26.0f, Center.Y + 8.0f, Center.X + 10.0f, Center.Y - 4.0f, RoadColor, 2.0f);
	DrawLine(Center.X + 10.0f, Center.Y - 4.0f, Center.X + 58.0f, Center.Y - 22.0f, RoadColor, 2.0f);
	DrawLine(Center.X - 18.0f, Center.Y + 18.0f, Center.X + 38.0f, Center.Y + 52.0f, RoadColor, 2.0f);

	DrawCircleOutline(Center, Radius, FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 3.0f);
	DrawText(TEXT("N"), FLinearColor::White, Center.X - 4.0f, Center.Y - Radius + 8.0f, nullptr, 0.75f);
}

void ASurvivalHUD::DrawMapResources(const FVector2D& Center, float Radius, const ASurvivalCharacter* SurvivalCharacter)
{
	if (!SurvivalCharacter || !GetWorld())
	{
		return;
	}

	const FVector PlayerLocation = SurvivalCharacter->GetActorLocation();
	for (TActorIterator<AResourceNodeActor> It(GetWorld()); It; ++It)
	{
		const AResourceNodeActor* NodeActor = *It;
		const UResourceNodeComponent* ResourceNode = NodeActor ? NodeActor->ResourceNodeComponent : nullptr;
		if (!NodeActor || !ResourceNode || ResourceNode->IsDepleted())
		{
			continue;
		}

		const FVector2D MarkerPosition = WorldToMap(NodeActor->GetActorLocation(), PlayerLocation, Center, Radius);
		if (FVector2D::Distance(MarkerPosition, Center) > Radius - 8.0f)
		{
			continue;
		}

		const FName OutputItem = ResourceNode->OutputItemId;
		const FLinearColor MarkerColor = OutputItem == FName(TEXT("Wood"))
			? FLinearColor(0.23f, 0.86f, 0.38f, 1.0f)
			: FLinearColor(0.82f, 0.82f, 0.72f, 1.0f);

		DrawFilledCircle(MarkerPosition, 4.0f, MarkerColor);
		DrawCircleOutline(MarkerPosition, 4.0f, FLinearColor(0.02f, 0.02f, 0.02f, 1.0f), 1.0f);
	}
}

void ASurvivalHUD::DrawPlayerArrow(const FVector2D& Center, const ASurvivalCharacter* SurvivalCharacter)
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

	DrawLine(Tip.X, Tip.Y, Left.X, Left.Y, FLinearColor::White, 3.0f);
	DrawLine(Tip.X, Tip.Y, RightPoint.X, RightPoint.Y, FLinearColor::White, 3.0f);
	DrawLine(Left.X, Left.Y, RightPoint.X, RightPoint.Y, FLinearColor(0.1f, 0.25f, 1.0f, 1.0f), 4.0f);
}

void ASurvivalHUD::DrawStatusIcon(const FVector2D& Center, float Value, const FLinearColor& StatusColor, bool bDrawThirstIcon)
{
	DrawFilledCircle(Center, 17.0f, FLinearColor(0.06f, 0.08f, 0.09f, 0.94f));
	DrawCircleOutline(Center, 17.0f, FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 2.0f);
	DrawRingArc(Center, 20.0f, -90.0f, -90.0f + (360.0f * FMath::Clamp(Value, 0.0f, 100.0f) / 100.0f), StatusColor, 3.0f);

	if (bDrawThirstIcon)
	{
		DrawLine(Center.X, Center.Y - 8.0f, Center.X - 6.0f, Center.Y + 2.0f, FLinearColor::White, 2.0f);
		DrawLine(Center.X, Center.Y - 8.0f, Center.X + 6.0f, Center.Y + 2.0f, FLinearColor::White, 2.0f);
		DrawLine(Center.X - 6.0f, Center.Y + 2.0f, Center.X, Center.Y + 9.0f, FLinearColor::White, 2.0f);
		DrawLine(Center.X + 6.0f, Center.Y + 2.0f, Center.X, Center.Y + 9.0f, FLinearColor::White, 2.0f);
	}
	else
	{
		DrawLine(Center.X - 5.0f, Center.Y - 8.0f, Center.X - 5.0f, Center.Y + 8.0f, FLinearColor::White, 2.0f);
		DrawLine(Center.X - 9.0f, Center.Y - 8.0f, Center.X - 9.0f, Center.Y - 2.0f, FLinearColor::White, 1.5f);
		DrawLine(Center.X - 1.0f, Center.Y - 8.0f, Center.X - 1.0f, Center.Y - 2.0f, FLinearColor::White, 1.5f);
		DrawLine(Center.X + 6.0f, Center.Y - 8.0f, Center.X + 9.0f, Center.Y + 8.0f, FLinearColor::White, 2.0f);
	}
}

void ASurvivalHUD::DrawFilledCircle(const FVector2D& Center, float Radius, const FLinearColor& Color)
{
	const int32 Rows = FMath::Max(8, FMath::RoundToInt(Radius * 1.4f));
	for (int32 Row = -Rows; Row <= Rows; ++Row)
	{
		const float NormalizedY = static_cast<float>(Row) / static_cast<float>(Rows);
		const float Y = NormalizedY * Radius;
		const float HalfWidth = FMath::Sqrt(FMath::Max(0.0f, (Radius * Radius) - (Y * Y)));
		DrawRect(Color, Center.X - HalfWidth, Center.Y + Y, HalfWidth * 2.0f, 2.0f);
	}
}

void ASurvivalHUD::DrawCircleOutline(const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness)
{
	DrawRingArc(Center, Radius, 0.0f, 360.0f, Color, Thickness);
}

void ASurvivalHUD::DrawRingArc(const FVector2D& Center, float Radius, float StartDegrees, float EndDegrees, const FLinearColor& Color, float Thickness)
{
	const float ArcLength = EndDegrees - StartDegrees;
	const int32 Segments = FMath::Max(3, FMath::CeilToInt(FMath::Abs(ArcLength) / 5.0f));
	FVector2D PreviousPoint(
		Center.X + FMath::Cos(FMath::DegreesToRadians(StartDegrees)) * Radius,
		Center.Y + FMath::Sin(FMath::DegreesToRadians(StartDegrees)) * Radius);

	for (int32 SegmentIndex = 1; SegmentIndex <= Segments; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(Segments);
		const float Degrees = FMath::Lerp(StartDegrees, EndDegrees, Alpha);
		const FVector2D NextPoint(
			Center.X + FMath::Cos(FMath::DegreesToRadians(Degrees)) * Radius,
			Center.Y + FMath::Sin(FMath::DegreesToRadians(Degrees)) * Radius);
		DrawLine(PreviousPoint.X, PreviousPoint.Y, NextPoint.X, NextPoint.Y, Color, Thickness);
		PreviousPoint = NextPoint;
	}
}

void ASurvivalHUD::DrawHealthAndStaminaRings(const FVector2D& Center, float Radius, float Health, float Stamina)
{
	const FLinearColor RingBackground(0.025f, 0.03f, 0.035f, 0.95f);
	DrawRingArc(Center, Radius, 104.0f, 256.0f, RingBackground, 14.0f);
	DrawRingArc(Center, Radius, -76.0f, 76.0f, RingBackground, 14.0f);

	DrawRingArc(Center, Radius, 104.0f, 104.0f + (152.0f * FMath::Clamp(Health, 0.0f, 100.0f) / 100.0f), FLinearColor(0.92f, 0.08f, 0.12f, 1.0f), 12.0f);
	DrawRingArc(Center, Radius, -76.0f, -76.0f + (152.0f * FMath::Clamp(Stamina, 0.0f, 100.0f) / 100.0f), FLinearColor(0.0f, 0.72f, 0.82f, 1.0f), 12.0f);

	DrawText(TEXT("Leben"), FLinearColor::White, Center.X - Radius - 34.0f, Center.Y + Radius - 8.0f, nullptr, 0.65f);
	DrawText(TEXT("Ausdauer"), FLinearColor::White, Center.X + Radius - 24.0f, Center.Y + Radius - 8.0f, nullptr, 0.65f);
}

FVector2D ASurvivalHUD::WorldToMap(const FVector& WorldLocation, const FVector& PlayerLocation, const FVector2D& Center, float Radius) const
{
	const FVector Delta = WorldLocation - PlayerLocation;
	return FVector2D(
		Center.X + FMath::Clamp(Delta.Y / MinimapWorldRadius, -1.0f, 1.0f) * Radius,
		Center.Y - FMath::Clamp(Delta.X / MinimapWorldRadius, -1.0f, 1.0f) * Radius);
}
