#include "Survival/CampfireActor.h"

#include "Survival/CampfireComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

ACampfireActor::ACampfireActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	FireLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLightComponent->SetupAttachment(SceneRoot);
	FireLightComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	FireLightComponent->SetLightColor(FLinearColor(1.0f, 0.48f, 0.20f));
	FireLightComponent->SetIntensity(0.0f);
	FireLightComponent->SetAttenuationRadius(650.0f);

	CampfireComponent = CreateDefaultSubobject<UCampfireComponent>(TEXT("Campfire"));
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();

	if (CampfireComponent)
	{
		CampfireComponent->OnCampfireChanged.AddDynamic(this, &ACampfireActor::HandleCampfireChanged);
	}

	HandleCampfireChanged();
}

FText ACampfireActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	if (!CampfireComponent)
	{
		return NSLOCTEXT("SurvivalWorld", "CampfireUnavailable", "Campfire");
	}

	return CampfireComponent->IsLit()
		? NSLOCTEXT("SurvivalWorld", "CampfireCookOrFuel", "Cook food / add fuel")
		: NSLOCTEXT("SurvivalWorld", "CampfireAddFuel", "Add fuel");
}

bool ACampfireActor::CanInteract_Implementation(const AActor* InteractingActor) const
{
	return CampfireComponent && InteractingActor;
}

bool ACampfireActor::Interact_Implementation(AActor* InteractingActor)
{
	return CampfireComponent && CampfireComponent->InteractWithActor(InteractingActor);
}

void ACampfireActor::HandleCampfireChanged()
{
	if (!CampfireComponent || !FireLightComponent)
	{
		return;
	}

	const bool bLit = CampfireComponent->IsLit();
	FireLightComponent->SetIntensity(bLit ? 4200.0f : 0.0f);
}
