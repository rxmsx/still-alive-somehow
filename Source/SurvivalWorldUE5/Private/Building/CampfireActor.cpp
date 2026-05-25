#include "Building/CampfireActor.h"

#include "Items/InventoryComponent.h"
#include "Player/SurvivalPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"

ACampfireActor::ACampfireActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PartId = TEXT("Campfire");

	CampfireInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("CampfireInventory"));
	CampfireInventory->InventorySlotCount = 8;
	CampfireInventory->HotbarSlotCount = 1;
	CampfireInventory->MaxWeightKg = 40.0f;

	FireLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLightComponent->SetupAttachment(SceneRoot);
	FireLightComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
	FireLightComponent->Intensity = 2600.0f;
	FireLightComponent->AttenuationRadius = 640.0f;
	FireLightComponent->LightColor = FColor(255, 147, 74);
	FireLightComponent->SetVisibility(false);

	FireParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireParticles"));
	FireParticleComponent->SetupAttachment(SceneRoot);
	FireParticleComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 26.0f));
	FireParticleComponent->bAutoActivate = false;

	FireAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudioComponent->SetupAttachment(SceneRoot);
	FireAudioComponent->bAutoActivate = false;

	FCampfireCookRecipe MeatRecipe;
	MeatRecipe.InputItemId = TEXT("RawMeat");
	MeatRecipe.OutputItemId = TEXT("CookedMeat");
	MeatRecipe.CookSeconds = 8.0f;
	CookRecipes.Add(MeatRecipe);

	FCampfireCookRecipe WaterRecipe;
	WaterRecipe.InputItemId = TEXT("DirtyWater");
	WaterRecipe.OutputItemId = TEXT("CleanWater");
	WaterRecipe.CookSeconds = 6.0f;
	CookRecipes.Add(WaterRecipe);
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshEffects();
}

void ACampfireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (bIsLit)
	{
		FuelSecondsRemaining = FMath::Max(0.0f, FuelSecondsRemaining - DeltaSeconds);
		if (FuelSecondsRemaining <= 0.0f && !TryConsumeFuelItem())
		{
			Extinguish();
			return;
		}

		TryCookOneItem(DeltaSeconds);
	}
	else if (FuelSecondsRemaining > 0.0f)
	{
		Ignite();
	}

	RefreshEffects();
}

bool ACampfireActor::Ignite()
{
	if (FuelSecondsRemaining <= 0.0f && !TryConsumeFuelItem())
	{
		bIsLit = false;
		RefreshEffects();
		return false;
	}

	bIsLit = FuelSecondsRemaining > 0.0f;
	RefreshEffects();
	return bIsLit;
}

void ACampfireActor::Extinguish()
{
	bIsLit = false;
	CurrentCookSeconds = 0.0f;
	RefreshEffects();
}

void ACampfireActor::SetCampfireState(bool bNewIsLit, float NewFuelSecondsRemaining, float NewCookSeconds)
{
	bIsLit = bNewIsLit;
	FuelSecondsRemaining = FMath::Max(0.0f, NewFuelSecondsRemaining);
	CurrentCookSeconds = FMath::Max(0.0f, NewCookSeconds);
	RefreshEffects();
}

FText ACampfireActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	return bIsLit
		? NSLOCTEXT("SurvivalWorld", "OpenLitCampfirePrompt", "Lagerfeuer oeffnen")
		: NSLOCTEXT("SurvivalWorld", "OpenCampfirePrompt", "Lagerfeuer oeffnen");
}

bool ACampfireActor::CanInteract_Implementation(const AActor* InteractingActor) const
{
	return !bIsPreviewActor && CampfireInventory != nullptr;
}

bool ACampfireActor::Interact_Implementation(AActor* InteractingActor)
{
	if (!CanInteract_Implementation(InteractingActor))
	{
		return false;
	}

	if (APawn* Pawn = Cast<APawn>(InteractingActor))
	{
		if (ASurvivalPlayerController* Controller = Cast<ASurvivalPlayerController>(Pawn->GetController()))
		{
			Controller->OpenWorldInventory(this, CampfireInventory, ECraftingStationType::Campfire);
			Ignite();
			return true;
		}
	}

	return false;
}

void ACampfireActor::OnRep_CampfireState()
{
	RefreshEffects();
}

bool ACampfireActor::TryConsumeFuelItem()
{
	if (!CampfireInventory)
	{
		return false;
	}

	for (const FInventoryStack& Slot : CampfireInventory->GetSlots())
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		FItemDef ItemDef;
		if (!CampfireInventory->GetItemDefinition(Slot.ItemId, ItemDef) || !ItemDef.bCanUseAsFuel || ItemDef.FuelSeconds <= 0.0f)
		{
			continue;
		}

		if (CampfireInventory->RemoveFromSlot(Slot.SlotIndex, 1))
		{
			FuelSecondsRemaining += ItemDef.FuelSeconds;
			return true;
		}
	}

	return false;
}

bool ACampfireActor::TryCookOneItem(float DeltaSeconds)
{
	if (!CampfireInventory || CookRecipes.Num() <= 0)
	{
		return false;
	}

	for (const FCampfireCookRecipe& Recipe : CookRecipes)
	{
		if (Recipe.InputItemId.IsNone() || Recipe.OutputItemId.IsNone() || !CampfireInventory->HasItem(Recipe.InputItemId, 1))
		{
			continue;
		}

		CurrentCookSeconds += DeltaSeconds;
		if (CurrentCookSeconds < Recipe.CookSeconds)
		{
			return false;
		}

		if (!CampfireInventory->RemoveItem(Recipe.InputItemId, 1))
		{
			CurrentCookSeconds = 0.0f;
			return false;
		}

		if (!CampfireInventory->AddItem(Recipe.OutputItemId, 1))
		{
			CampfireInventory->AddItem(Recipe.InputItemId, 1);
			CurrentCookSeconds = 0.0f;
			return false;
		}

		CurrentCookSeconds = 0.0f;
		return true;
	}

	CurrentCookSeconds = 0.0f;
	return false;
}

void ACampfireActor::RefreshEffects()
{
	if (FireLightComponent)
	{
		FireLightComponent->SetVisibility(bIsLit);
	}
	if (FireParticleComponent)
	{
		if (bIsLit && !FireParticleComponent->IsActive())
		{
			FireParticleComponent->Activate(true);
		}
		else if (!bIsLit && FireParticleComponent->IsActive())
		{
			FireParticleComponent->Deactivate();
		}
	}
	if (FireAudioComponent)
	{
		if (bIsLit && !FireAudioComponent->IsPlaying())
		{
			FireAudioComponent->Play();
		}
		else if (!bIsLit && FireAudioComponent->IsPlaying())
		{
			FireAudioComponent->Stop();
		}
	}
}

void ACampfireActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACampfireActor, bIsLit);
	DOREPLIFETIME(ACampfireActor, FuelSecondsRemaining);
	DOREPLIFETIME(ACampfireActor, CurrentCookSeconds);
}
