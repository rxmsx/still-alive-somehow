#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "ResourceNodeActor.generated.h"

class UResourceNodeComponent;
class UMapMarkerComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API AResourceNodeActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AResourceNodeActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	TObjectPtr<UResourceNodeComponent> ResourceNodeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map")
	TObjectPtr<UMapMarkerComponent> MapMarkerComponent;

	virtual FText GetInteractionPrompt_Implementation(const AActor* InteractingActor) const override;
	virtual bool CanInteract_Implementation(const AActor* InteractingActor) const override;
	virtual bool Interact_Implementation(AActor* InteractingActor) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleResourceHarvested(int32 RemainingHarvests);

	void RefreshMapMarker();
	void RefreshDepletedState();
};
