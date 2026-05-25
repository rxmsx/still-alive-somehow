#include "Building/BuildingComponent.h"

#include "Building/BuildableActor.h"
#include "Camera/CameraComponent.h"
#include "Items/InventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UBuildingComponent::UBuildingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBuildingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (SelectedBuildPartId.IsNone())
	{
		SelectedBuildPartId = DefaultBuildPartId;
	}
}

void UBuildingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyPreviewActor();
	Super::EndPlay(EndPlayReason);
}

void UBuildingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePreview();
}

void UBuildingComponent::ToggleBuildMode()
{
	if (bBuildModeActive)
	{
		CancelBuildMode();
	}
	else
	{
		StartBuildMode(SelectedBuildPartId.IsNone() ? DefaultBuildPartId : SelectedBuildPartId);
	}
}

bool UBuildingComponent::StartBuildMode(FName PartId)
{
	if (!ResolveBuildPart(PartId))
	{
		return false;
	}

	SelectedBuildPartId = PartId;
	bBuildModeActive = true;
	SetComponentTickEnabled(true);
	SpawnPreviewActor();
	UpdatePreview();
	OnBuildModeChanged.Broadcast(true, SelectedBuildPartId);
	return true;
}

void UBuildingComponent::CancelBuildMode()
{
	bBuildModeActive = false;
	bPlacementValid = false;
	SetComponentTickEnabled(false);
	DestroyPreviewActor();
	OnBuildModeChanged.Broadcast(false, SelectedBuildPartId);
}

bool UBuildingComponent::SelectBuildPart(FName PartId)
{
	if (!ResolveBuildPart(PartId))
	{
		return false;
	}

	SelectedBuildPartId = PartId;
	if (bBuildModeActive)
	{
		SpawnPreviewActor();
		UpdatePreview();
	}
	OnBuildModeChanged.Broadcast(bBuildModeActive, SelectedBuildPartId);
	return true;
}

bool UBuildingComponent::CycleBuildPart(int32 Direction)
{
	const TArray<FSurvivalBuildPartDef>& Parts = BuildCatalog && BuildCatalog->BuildParts.Num() > 0
		? BuildCatalog->BuildParts
		: USurvivalBuildCatalog::GetDefaultBuildParts();
	if (Parts.Num() <= 0)
	{
		return false;
	}

	int32 CurrentIndex = Parts.IndexOfByPredicate([this](const FSurvivalBuildPartDef& Part)
	{
		return Part.PartId == SelectedBuildPartId;
	});
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = 0;
	}

	const int32 Delta = Direction >= 0 ? 1 : -1;
	const int32 NextIndex = (CurrentIndex + Delta + Parts.Num()) % Parts.Num();
	return SelectBuildPart(Parts[NextIndex].PartId);
}

void UBuildingComponent::RotatePreview(float Direction)
{
	PreviewYaw = FMath::UnwindDegrees(PreviewYaw + RotationStepDegrees * (Direction >= 0.0f ? 1.0f : -1.0f));
	UpdatePreview();
}

bool UBuildingComponent::ConfirmPlacement()
{
	if (!bBuildModeActive)
	{
		return false;
	}

	UpdatePreview();
	const FSurvivalBuildPartDef* BuildPart = ResolveBuildPart(SelectedBuildPartId);
	if (!BuildPart || !bPlacementValid || !ConsumeCosts(*BuildPart))
	{
		return false;
	}

	UInventoryComponent* Inventory = GetOwnerInventory();
	if (Inventory && RequiredBuildToolType != ESurvivalToolType::None)
	{
		FInventoryStack ToolStack;
		int32 ToolSlotIndex = INDEX_NONE;
		if (Inventory->FindUsableTool(RequiredBuildToolType, ToolStack, ToolSlotIndex))
		{
			FItemDef ToolDef;
			Inventory->GetItemDefinition(ToolStack.ItemId, ToolDef);
			Inventory->DamageItemDurability(ToolSlotIndex, FMath::Max(0.05f, ToolDef.DurabilityLossPerUse), true);
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const TSubclassOf<ABuildableActor> ActorClass = BuildPart->BuildActorClass ? BuildPart->BuildActorClass.Get() : ABuildableActor::StaticClass();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ABuildableActor* Buildable = World->SpawnActor<ABuildableActor>(ActorClass, LastPreviewTransform, SpawnParams);
	if (!Buildable)
	{
		return false;
	}

	Buildable->InitializeBuildable(BuildPart->PartId, BuildCatalog, true);
	Buildable->SetPreviewState(false, true);
	UpdatePreview();
	return true;
}

bool UBuildingComponent::GetSelectedBuildPartDefinition(FSurvivalBuildPartDef& OutBuildPart) const
{
	if (const FSurvivalBuildPartDef* BuildPart = ResolveBuildPart(SelectedBuildPartId))
	{
		OutBuildPart = *BuildPart;
		return true;
	}
	return false;
}

const FSurvivalBuildPartDef* UBuildingComponent::ResolveBuildPart(FName PartId) const
{
	if (BuildCatalog)
	{
		if (const FSurvivalBuildPartDef* BuildPart = BuildCatalog->FindBuildPart(PartId))
		{
			return BuildPart;
		}
	}

	return USurvivalBuildCatalog::FindDefaultBuildPart(PartId);
}

UInventoryComponent* UBuildingComponent::GetOwnerInventory() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

bool UBuildingComponent::HasRequiredTool() const
{
	if (RequiredBuildToolType == ESurvivalToolType::None)
	{
		return true;
	}

	const UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		return false;
	}

	FInventoryStack ToolStack;
	int32 ToolSlotIndex = INDEX_NONE;
	return Inventory->FindUsableTool(RequiredBuildToolType, ToolStack, ToolSlotIndex);
}

bool UBuildingComponent::HasCosts(const FSurvivalBuildPartDef& BuildPart) const
{
	const UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		return false;
	}

	for (const FCraftingIngredient& Cost : BuildPart.Costs)
	{
		if (!Inventory->HasItem(Cost.ItemId, Cost.Count))
		{
			return false;
		}
	}
	return true;
}

bool UBuildingComponent::ConsumeCosts(const FSurvivalBuildPartDef& BuildPart) const
{
	UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory || !HasCosts(BuildPart))
	{
		return false;
	}

	for (const FCraftingIngredient& Cost : BuildPart.Costs)
	{
		if (!Inventory->RemoveItem(Cost.ItemId, Cost.Count))
		{
			return false;
		}
	}
	return true;
}

bool UBuildingComponent::BuildPreviewTransform(const FSurvivalBuildPartDef& BuildPart, FTransform& OutTransform, FText& OutMessage) const
{
	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		OutMessage = NSLOCTEXT("SurvivalWorld", "BuildNoWorld", "Keine Welt.");
		return false;
	}

	const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	const FVector TraceStart = Camera ? Camera->GetComponentLocation() : Owner->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
	const FVector TraceForward = Camera ? Camera->GetForwardVector() : Owner->GetActorForwardVector();
	const FVector TraceEnd = TraceStart + TraceForward * PlacementRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalBuildTrace), false, Owner);
	if (PreviewActor)
	{
		QueryParams.AddIgnoredActor(PreviewActor);
	}

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		OutMessage = NSLOCTEXT("SurvivalWorld", "BuildNoSurface", "Keine Flaeche.");
		return false;
	}

	if (TryGetSnapTransform(BuildPart, HitResult, OutTransform))
	{
		return true;
	}

	if (BuildPart.bRequiresSupport)
	{
		OutMessage = NSLOCTEXT("SurvivalWorld", "BuildNeedsSupport", "Braucht Snap-Punkt.");
		return false;
	}

	const FVector Location = HitResult.ImpactPoint + FVector(0.0f, 0.0f, BuildPart.PlacementBounds.Z + BuildPart.GroundOffsetZ);
	const FRotator Rotation(0.0f, PreviewYaw, 0.0f);
	OutTransform = FTransform(Rotation, Location);
	return true;
}

bool UBuildingComponent::TryGetSnapTransform(const FSurvivalBuildPartDef& BuildPart, const FHitResult& HitResult, FTransform& OutTransform) const
{
	const ABuildableActor* SupportActor = Cast<ABuildableActor>(HitResult.GetActor());
	if (!SupportActor || !BuildPart.SnapToPartIds.Contains(SupportActor->PartId))
	{
		return false;
	}

	const FSurvivalBuildPartDef& SupportPart = SupportActor->GetResolvedBuildPartDefinition();
	const FTransform SupportTransform = SupportActor->GetActorTransform();
	const FVector LocalImpact = SupportTransform.InverseTransformPosition(HitResult.ImpactPoint);

	FVector LocalLocation = FVector::ZeroVector;
	float YawOffset = 0.0f;
	if (FMath::Abs(LocalImpact.X) > FMath::Abs(LocalImpact.Y))
	{
		LocalLocation.X = FMath::Sign(LocalImpact.X) * (SupportPart.PlacementBounds.X + BuildPart.PlacementBounds.Y);
		YawOffset = 90.0f;
	}
	else
	{
		LocalLocation.Y = FMath::Sign(LocalImpact.Y) * (SupportPart.PlacementBounds.Y + BuildPart.PlacementBounds.Y);
		YawOffset = 0.0f;
	}
	LocalLocation.Z = SupportPart.PlacementBounds.Z + BuildPart.PlacementBounds.Z;

	const FVector WorldLocation = SupportTransform.TransformPosition(LocalLocation);
	const FRotator WorldRotation(0.0f, SupportActor->GetActorRotation().Yaw + YawOffset + PreviewYaw, 0.0f);
	OutTransform = FTransform(WorldRotation, WorldLocation);
	return true;
}

bool UBuildingComponent::IsLocationFree(const FSurvivalBuildPartDef& BuildPart, const FTransform& Transform) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalBuildOverlap), false, GetOwner());
	if (PreviewActor)
	{
		QueryParams.AddIgnoredActor(PreviewActor);
	}

	const FVector Extents = BuildPart.PlacementBounds + FVector(PlacementCollisionPadding);
	return !World->OverlapBlockingTestByChannel(Transform.GetLocation(), Transform.GetRotation(), ECC_WorldStatic, FCollisionShape::MakeBox(Extents), QueryParams);
}

void UBuildingComponent::UpdatePreview()
{
	if (!bBuildModeActive)
	{
		return;
	}

	const FSurvivalBuildPartDef* BuildPart = ResolveBuildPart(SelectedBuildPartId);
	if (!BuildPart)
	{
		bPlacementValid = false;
		PlacementMessage = NSLOCTEXT("SurvivalWorld", "BuildUnknownPart", "Bauteil unbekannt.");
		return;
	}

	FTransform PreviewTransform;
	FText Message;
	bool bValid = BuildPreviewTransform(*BuildPart, PreviewTransform, Message);
	if (bValid && !HasRequiredTool())
	{
		bValid = false;
		Message = NSLOCTEXT("SurvivalWorld", "BuildMissingHammer", "Bauhammer fehlt oder ist kaputt.");
	}
	if (bValid && !HasCosts(*BuildPart))
	{
		bValid = false;
		Message = NSLOCTEXT("SurvivalWorld", "BuildMissingMaterials", "Material fehlt.");
	}
	if (bValid && !IsLocationFree(*BuildPart, PreviewTransform))
	{
		bValid = false;
		Message = NSLOCTEXT("SurvivalWorld", "BuildBlocked", "Platz blockiert.");
	}

	bPlacementValid = bValid;
	PlacementMessage = bValid ? NSLOCTEXT("SurvivalWorld", "BuildCanPlace", "Platzierung gueltig.") : Message;
	LastPreviewTransform = PreviewTransform;

	if (!PreviewActor)
	{
		SpawnPreviewActor();
	}
	if (PreviewActor)
	{
		PreviewActor->SetActorTransform(PreviewTransform);
		PreviewActor->InitializeBuildable(BuildPart->PartId, BuildCatalog, false);
		PreviewActor->SetPreviewState(true, bPlacementValid);
	}
	OnBuildPlacementChanged.Broadcast(bPlacementValid, PlacementMessage);
}

void UBuildingComponent::SpawnPreviewActor()
{
	DestroyPreviewActor();

	const FSurvivalBuildPartDef* BuildPart = ResolveBuildPart(SelectedBuildPartId);
	UWorld* World = GetWorld();
	if (!BuildPart || !World)
	{
		return;
	}

	const TSubclassOf<ABuildableActor> ActorClass = PreviewActorClass ? PreviewActorClass.Get() : ABuildableActor::StaticClass();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PreviewActor = World->SpawnActor<ABuildableActor>(ActorClass, FTransform::Identity, SpawnParams);
	if (PreviewActor)
	{
		PreviewActor->InitializeBuildable(BuildPart->PartId, BuildCatalog, false);
		PreviewActor->SetPreviewState(true, false);
	}
}

void UBuildingComponent::DestroyPreviewActor()
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
}
