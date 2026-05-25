#include "Resources/ResourceNodeActor.h"
#include "Map/MapMarkerComponent.h"
#include "Resources/ResourceNodeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FText ToolTypeLabel(ESurvivalToolType ToolType)
	{
		switch (ToolType)
		{
		case ESurvivalToolType::Axe:
			return NSLOCTEXT("SurvivalWorld", "ToolTypeAxe", "Axt");
		case ESurvivalToolType::Pickaxe:
			return NSLOCTEXT("SurvivalWorld", "ToolTypePickaxe", "Spitzhacke");
		case ESurvivalToolType::Knife:
			return NSLOCTEXT("SurvivalWorld", "ToolTypeKnife", "Messer");
		case ESurvivalToolType::Hammer:
			return NSLOCTEXT("SurvivalWorld", "ToolTypeHammer", "Hammer");
		case ESurvivalToolType::FireStarter:
			return NSLOCTEXT("SurvivalWorld", "ToolTypeFireStarter", "Feuerzeug");
		case ESurvivalToolType::Hand:
			return NSLOCTEXT("SurvivalWorld", "ToolTypeHand", "Hand");
		default:
			return NSLOCTEXT("SurvivalWorld", "ToolTypeNone", "Werkzeug");
		}
	}
}

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultResourceMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DefaultResourceMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultResourceMesh.Object);
		MeshComponent->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.7f));
	}

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
		return NSLOCTEXT("SurvivalWorld", "ResourceDepleted", "Erschoepft");
	}

	const FText NodeName = !ResourceNodeComponent->DisplayName.IsEmpty()
		? ResourceNodeComponent->DisplayName
		: FText::FromName(ResourceNodeComponent->OutputItemId);
	if (!ResourceNodeComponent->CanHarvest(InteractingActor))
	{
		return FText::Format(
			NSLOCTEXT("SurvivalWorld", "HarvestResourceMissingTool", "{0} benoetigt {1}"),
			NodeName,
			ToolTypeLabel(ResourceNodeComponent->RequiredToolType));
	}

	return FText::Format(
		NSLOCTEXT("SurvivalWorld", "HarvestResource", "Abbauen {0}"),
		NodeName);
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
	MapMarkerComponent->DisplayName = !ResourceNodeComponent->DisplayName.IsEmpty()
		? ResourceNodeComponent->DisplayName
		: FText::FromName(ResourceNodeComponent->OutputItemId);

	if (ResourceNodeComponent->ResourceNodeId == TEXT("Tree"))
	{
		MapMarkerComponent->MarkerColor = FLinearColor(0.23f, 0.86f, 0.38f, 1.0f);
	}
	else if (ResourceNodeComponent->ResourceNodeId == TEXT("Rock"))
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
