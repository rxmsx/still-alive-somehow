#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "CampfireActor.generated.h"

class UCampfireComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API ACampfireActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ACampfireActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UPointLightComponent> FireLightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UCampfireComponent> CampfireComponent;

	virtual FText GetInteractionPrompt_Implementation(const AActor* InteractingActor) const override;
	virtual bool CanInteract_Implementation(const AActor* InteractingActor) const override;
	virtual bool Interact_Implementation(AActor* InteractingActor) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCampfireChanged();
};
