#include "Building/BuildableActor.h"

#include "Crafting/CraftingComponent.h"
#include "Player/SurvivalPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

namespace
{
	UStaticMesh* GetDefaultBuildMesh()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		return DefaultMesh.Succeeded() ? DefaultMesh.Object : nullptr;
	}

	UMaterialInterface* GetDefaultBuildMaterial()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return DefaultMaterial.Succeeded() ? DefaultMaterial.Object : nullptr;
	}
}

ABuildableActor::ABuildableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetStaticMesh(GetDefaultBuildMesh());
	if (UMaterialInterface* Material = GetDefaultBuildMaterial())
	{
		MeshComponent->SetMaterial(0, Material);
	}
}

void ABuildableActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyBuildPartDefinition();
}

void ABuildableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyBuildPartDefinition();
}

void ABuildableActor::InitializeBuildable(FName NewPartId, USurvivalBuildCatalog* NewBuildCatalog, bool bPlacedByPlayer)
{
	PartId = NewPartId;
	BuildCatalog = NewBuildCatalog;
	bWasPlacedByPlayer = bPlacedByPlayer;
	if (StableBuildId.IsEmpty())
	{
		StableBuildId = FString::Printf(TEXT("%s_%s_%d"), *PartId.ToString(), *GetActorLocation().ToCompactString(), FMath::Rand());
	}
	ApplyBuildPartDefinition();
}

void ABuildableActor::SetPreviewState(bool bPreview, bool bPlacementValid)
{
	bIsPreviewActor = bPreview;
	SetActorEnableCollision(!bPreview);
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetVisibility(true, true);
	}
	ApplyTint(bPreview
		? (bPlacementValid ? FLinearColor(0.18f, 0.95f, 0.32f, 0.64f) : FLinearColor(0.95f, 0.12f, 0.10f, 0.64f))
		: FLinearColor(0.62f, 0.48f, 0.32f, 1.0f));
}

const FSurvivalBuildPartDef& ABuildableActor::GetResolvedBuildPartDefinition() const
{
	if (BuildCatalog)
	{
		if (const FSurvivalBuildPartDef* Part = BuildCatalog->FindBuildPart(PartId))
		{
			return *Part;
		}
	}

	if (const FSurvivalBuildPartDef* DefaultPart = USurvivalBuildCatalog::FindDefaultBuildPart(PartId))
	{
		return *DefaultPart;
	}

	return USurvivalBuildCatalog::GetDefaultBuildParts()[0];
}

ECraftingStationType ABuildableActor::GetCraftingStationType() const
{
	return GetResolvedBuildPartDefinition().CraftingStationType;
}

FText ABuildableActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	if (GetCraftingStationType() != ECraftingStationType::None)
	{
		return FText::Format(NSLOCTEXT("SurvivalWorld", "UseBuildStationPrompt", "Benutzen {0}"), GetResolvedBuildPartDefinition().DisplayName);
	}
	return FText::GetEmpty();
}

bool ABuildableActor::CanInteract_Implementation(const AActor* InteractingActor) const
{
	return !bIsPreviewActor && GetCraftingStationType() != ECraftingStationType::None;
}

bool ABuildableActor::Interact_Implementation(AActor* InteractingActor)
{
	if (!CanInteract_Implementation(InteractingActor))
	{
		return false;
	}

	if (APawn* Pawn = Cast<APawn>(InteractingActor))
	{
		if (ASurvivalPlayerController* Controller = Cast<ASurvivalPlayerController>(Pawn->GetController()))
		{
			Controller->OpenCraftingStation(GetCraftingStationType(), this);
			return true;
		}
	}

	return false;
}

void ABuildableActor::OnRep_BuildableState()
{
	ApplyBuildPartDefinition();
}

void ABuildableActor::ApplyBuildPartDefinition()
{
	if (!MeshComponent)
	{
		return;
	}

	const FSurvivalBuildPartDef& Part = GetResolvedBuildPartDefinition();
	UStaticMesh* Mesh = Part.PlacedMesh ? Part.PlacedMesh.Get() : Part.PreviewMesh.Get();
	MeshComponent->SetStaticMesh(Mesh ? Mesh : GetDefaultBuildMesh());
	MeshComponent->SetRelativeScale3D(Part.MeshScale);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, Part.GroundOffsetZ));

	if (!bIsPreviewActor)
	{
		ApplyTint(FLinearColor(0.55f, 0.40f, 0.24f, 1.0f));
	}
}

void ABuildableActor::ApplyTint(const FLinearColor& Tint)
{
	if (!MeshComponent)
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Tint);
		DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
	}
}

void ABuildableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABuildableActor, PartId);
	DOREPLIFETIME(ABuildableActor, bWasPlacedByPlayer);
	DOREPLIFETIME(ABuildableActor, bIsPreviewActor);
	DOREPLIFETIME(ABuildableActor, StableBuildId);
}
