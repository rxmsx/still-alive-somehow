#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CampfireComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCampfireChanged);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UCampfireComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCampfireComponent();

	UPROPERTY(BlueprintAssignable, Category = "Campfire")
	FOnCampfireChanged OnCampfireChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Campfire, Category = "Campfire", meta = (ClampMin = "0.0", Units = "s"))
	float FuelSecondsRemaining = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "50.0", Units = "cm"))
	float HeatRadius = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0.0"))
	float WarmthSecondsPerPulse = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0.0"))
	float FuelUseMultiplier = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Campfire")
	bool IsLit() const { return FuelSecondsRemaining > 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	void AddFuelSeconds(float FuelSeconds);

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	bool TryAddBestFuelFromActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	bool TryCookFoodFromActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Campfire")
	bool InteractWithActor(AActor* Actor);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Campfire();

private:
	void WarmNearbyActors();
	void BroadcastChanged();
};
