#pragma once

#include "CoreMinimal.h"
#include "Building/BuildableActor.h"
#include "CampfireActor.generated.h"

class UAudioComponent;
class UInventoryComponent;
class UParticleSystemComponent;
class UPointLightComponent;

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FCampfireCookRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName InputItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OutputItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float CookSeconds = 5.0f;
};

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API ACampfireActor : public ABuildableActor
{
	GENERATED_BODY()

public:
	ACampfireActor();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire")
	TObjectPtr<UInventoryComponent> CampfireInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Effects")
	TObjectPtr<UPointLightComponent> FireLightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Effects")
	TObjectPtr<UParticleSystemComponent> FireParticleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campfire|Effects")
	TObjectPtr<UAudioComponent> FireAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CampfireState, Category = "Campfire")
	bool bIsLit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CampfireState, Category = "Campfire", meta = (ClampMin = "0.0"))
	float FuelSecondsRemaining = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Campfire", meta = (ClampMin = "0.0"))
	float CurrentCookSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire")
	TArray<FCampfireCookRecipe> CookRecipes;

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	bool Ignite();

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void Extinguish();

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void SetCampfireState(bool bNewIsLit, float NewFuelSecondsRemaining, float NewCookSeconds);

	virtual FText GetInteractionPrompt_Implementation(const AActor* InteractingActor) const override;
	virtual bool CanInteract_Implementation(const AActor* InteractingActor) const override;
	virtual bool Interact_Implementation(AActor* InteractingActor) override;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CampfireState();

	bool TryConsumeFuelItem();
	bool TryCookOneItem(float DeltaSeconds);
	void RefreshEffects();
};
