#include "Resources/ResourceNodeActor.h"
#include "Map/MapMarkerComponent.h"
#include "Resources/ResourceNodeComponent.h"
#include "Components/StaticMeshComponent.h"

AResourceNodeActor::AResourceNodeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	ResourceNodeComponent = CreateDefaultSubobject<UResourceNodeComponent>(TEXT("ResourceNode"));

	MapMarkerComponent = CreateDefaultSubobject<UMapMarkerComponent>(TEXT("MapMarker"));
	MapMarkerComponent->MarkerType = ESurvivalMapMarkerType::Resource;
	MapMarkerComponent->MarkerColor = FLinearColor(0.82f, 0.82f, 0.72f, 1.0f);
}

void AResourceNodeActor::BeginPlay()
{
	Super::BeginPlay();

	if (ResourceNodeComponent)
	{
		ResourceNodeComponent->OnResourceHarvested.AddDynamic(this, &AResourceNodeActor::HandleResourceHarvested);
	}

	RefreshMapMarker();
	RefreshDepletedState();
}

FText AResourceNodeActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	if (!ResourceNodeComponent || ResourceNodeComponent->IsDepleted())
	{
		return NSLOCTEXT("SurvivalWorld", "ResourceDepleted", "Depleted");
	}

	if (ResourceNodeComponent->RequiredToolType != ESurvivalToolType::None)
	{
		return FText::Format(
			NSLOCTEXT("SurvivalWorld", "HarvestResourceWithTool", "E Abbauen {0} (Werkzeug noetig)"),
			FText::FromName(ResourceNodeComponent->OutputItemId));
	}

	return FText::Format(
		NSLOCTEXT("SurvivalWorld", "HarvestResource", "Harvest {0}"),
		FText::FromName(ResourceNodeComponent->OutputItemId));
}

bool AResourceNodeActor::CanInteract_Implementation(const AActor* InteractingActor) const
{
	return ResourceNodeComponent && ResourceNodeComponent->CanHarvest(InteractingActor);
}

bool AResourceNodeActor::Interact_Implementation(AActor* InteractingActor)
{
	if (!ResourceNodeComponent)
	{
		return false;
	}

	FName HarvestedItemId;
	int32 HarvestedAmount = 0;
	return ResourceNodeComponent->Harvest(InteractingActor, HarvestedItemId, HarvestedAmount);
}

void AResourceNodeActor::HandleResourceHarvested(int32 RemainingHarvests)
{
	RefreshDepletedState();
}

void AResourceNodeActor::RefreshMapMarker()
{
	if (!MapMarkerComponent || !ResourceNodeComponent)
	{
		return;
	}

	MapMarkerComponent->MarkerId = ResourceNodeComponent->StableResourceId.IsEmpty()
		? FName(*GetName())
		: FName(*ResourceNodeComponent->StableResourceId);
	MapMarkerComponent->DisplayName = FText::FromName(ResourceNodeComponent->OutputItemId);

	if (ResourceNodeComponent->OutputItemId == TEXT("Wood"))
	{
		MapMarkerComponent->MarkerColor = FLinearColor(0.23f, 0.86f, 0.38f, 1.0f);
	}
	else if (ResourceNodeComponent->OutputItemId == TEXT("Stone"))
	{
		MapMarkerComponent->MarkerColor = FLinearColor(0.82f, 0.82f, 0.72f, 1.0f);
	}
	else
	{
		MapMarkerComponent->MarkerColor = FLinearColor(0.72f, 0.62f, 0.92f, 1.0f);
	}
}

void AResourceNodeActor::RefreshDepletedState()
{
	if (!MeshComponent || !ResourceNodeComponent)
	{
		return;
	}

	const bool bDepleted = ResourceNodeComponent->IsDepleted();
	MeshComponent->SetVisibility(!bDepleted, true);
	MeshComponent->SetCollisionEnabled(bDepleted ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);

	if (MapMarkerComponent)
	{
		MapMarkerComponent->bShowOnMinimap = !bDepleted;
	}
}
