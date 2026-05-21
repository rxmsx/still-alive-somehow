#include "Resources/ResourceNodeActor.h"
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
}

void AResourceNodeActor::BeginPlay()
{
	Super::BeginPlay();

	if (ResourceNodeComponent)
	{
		ResourceNodeComponent->OnResourceHarvested.AddDynamic(this, &AResourceNodeActor::HandleResourceHarvested);
	}

	RefreshDepletedState();
}

FText AResourceNodeActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	if (!ResourceNodeComponent || ResourceNodeComponent->IsDepleted())
	{
		return NSLOCTEXT("SurvivalWorld", "ResourceDepleted", "Depleted");
	}

	if (!ResourceNodeComponent->RequiredToolItemId.IsNone())
	{
		return FText::Format(
			NSLOCTEXT("SurvivalWorld", "HarvestResourceWithTool", "Harvest {0} with {1}"),
			FText::FromName(ResourceNodeComponent->OutputItemId),
			FText::FromName(ResourceNodeComponent->RequiredToolItemId));
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

void AResourceNodeActor::RefreshDepletedState()
{
	if (!MeshComponent || !ResourceNodeComponent)
	{
		return;
	}

	const bool bDepleted = ResourceNodeComponent->IsDepleted();
	MeshComponent->SetVisibility(!bDepleted, true);
	MeshComponent->SetCollisionEnabled(bDepleted ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
}
