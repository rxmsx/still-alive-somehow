#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Items/InventoryComponent.h"
#include "ItemPickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USurvivalItemCatalog;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API AItemPickupActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AItemPickupActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemState, Category = "Pickup")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemState, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemState, Category = "Pickup", meta = (ClampMin = "0.0"))
	float Durability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemState, Category = "Pickup", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Freshness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USurvivalItemCatalog> ItemCatalog;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void InitializePickup(const FInventoryStack& Stack, USurvivalItemCatalog* NewItemCatalog);

	virtual FText GetInteractionPrompt_Implementation(const AActor* InteractingActor) const override;
	virtual bool CanInteract_Implementation(const AActor* InteractingActor) const override;
	virtual bool Interact_Implementation(AActor* InteractingActor) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ItemState();

	const FItemDef* ResolveItemDefinition() const;
	void RefreshVisuals();
};
