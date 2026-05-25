#include "Items/ItemPickupActor.h"

#include "World/OpenWorldPrototypeSettings.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AItemPickupActor::AItemPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(42.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(CollisionComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultPickupMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultPickupMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(DefaultPickupMesh.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.08f));
	}
}

void AItemPickupActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshVisuals();
}

void AItemPickupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisuals();
}

void AItemPickupActor::InitializePickup(const FInventoryStack& Stack, USurvivalItemCatalog* NewItemCatalog)
{
	ItemId = Stack.ItemId;
	Count = FMath::Max(1, Stack.Count);
	Durability = Stack.Durability;
	Freshness = FMath::Clamp(Stack.Freshness, 0.0f, 1.0f);
	ItemCatalog = NewItemCatalog;
	RefreshVisuals();
}

FText AItemPickupActor::GetInteractionPrompt_Implementation(const AActor* InteractingActor) const
{
	const FItemDef* ItemDef = ResolveItemDefinition();
	const FText ItemName = ItemDef && !ItemDef->DisplayName.IsEmpty() ? ItemDef->DisplayName : FText::FromName(ItemId);
	return FText::Format(NSLOCTEXT("SurvivalWorld", "PickupItemPrompt", "Take {0} x{1}"), ItemName, Count);
}

bool AItemPickupActor::CanInteract_Implementation(const AActor* InteractingActor) const
{
	if (!InteractingActor || ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	const UInventoryComponent* Inventory = InteractingActor->FindComponentByClass<UInventoryComponent>();
	return Inventory && Inventory->CanAddItem(ItemId, Count);
}

bool AItemPickupActor::Interact_Implementation(AActor* InteractingActor)
{
	if (!InteractingActor || ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	UInventoryComponent* Inventory = InteractingActor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory || !Inventory->AddItemWithState(ItemId, Count, Durability, Freshness))
	{
		return false;
	}

	Destroy();
	return true;
}

void AItemPickupActor::OnRep_ItemState()
{
	RefreshVisuals();
}

const FItemDef* AItemPickupActor::ResolveItemDefinition() const
{
	if (ItemCatalog)
	{
		if (const FItemDef* ItemDef = ItemCatalog->FindItem(ItemId))
		{
			return ItemDef;
		}
	}

	if (const UOpenWorldPrototypeSettings* Settings = GetDefault<UOpenWorldPrototypeSettings>())
	{
		if (const USurvivalItemCatalog* LoadedCatalog = Settings->ItemCatalog.LoadSynchronous())
		{
			if (const FItemDef* ItemDef = LoadedCatalog->FindItem(ItemId))
			{
				return ItemDef;
			}
		}
	}

	return USurvivalItemCatalog::FindDefaultItem(ItemId);
}

void AItemPickupActor::RefreshVisuals()
{
	if (!MeshComponent)
	{
		return;
	}

	if (const FItemDef* ItemDef = ResolveItemDefinition())
	{
		bool bUsingItemMesh = false;
		if (ItemDef->WorldMesh)
		{
			MeshComponent->SetStaticMesh(ItemDef->WorldMesh);
			bUsingItemMesh = true;
		}
		else if (ItemDef->PreviewMesh)
		{
			MeshComponent->SetStaticMesh(ItemDef->PreviewMesh);
			bUsingItemMesh = true;
		}

		if (bUsingItemMesh)
		{
			MeshComponent->SetRelativeScale3D(ItemDef->PreviewScale.IsNearlyZero() ? FVector(0.8f) : ItemDef->PreviewScale);
			MeshComponent->SetRelativeRotation(ItemDef->PreviewRotation);
		}
	}

	MeshComponent->SetVisibility(MeshComponent->GetStaticMesh() != nullptr, true);
}

void AItemPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemPickupActor, ItemId);
	DOREPLIFETIME(AItemPickupActor, Count);
	DOREPLIFETIME(AItemPickupActor, Durability);
	DOREPLIFETIME(AItemPickupActor, Freshness);
}
