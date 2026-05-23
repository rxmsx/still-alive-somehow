#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/SurvivalItemTypes.h"
#include "SurvivalStatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSurvivalStatsChanged);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API USurvivalStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalStatsComponent();

	UPROPERTY(BlueprintAssignable, Category = "Survival")
	FOnSurvivalStatsChanged OnStatsChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Hunger = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Thirst = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "-60.0", ClampMax = "60.0"))
	float TemperatureCelsius = 37.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Fatigue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Disease = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Bleeding = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Poison = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stats, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float EncumbranceRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
	float HungerDrainPerSecond = 0.015f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
	float ThirstDrainPerSecond = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
	float SprintHungerMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
	float SprintThirstMultiplier = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
	float SprintStaminaDrainPerSecond = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
	float StaminaRecoveryPerSecond = 9.0f;

	UFUNCTION(BlueprintCallable, Category = "Survival")
	void TickSurvival(float DeltaSeconds, bool bIsSprinting);

	UFUNCTION(BlueprintCallable, Category = "Survival")
	bool ConsumeStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Survival")
	void ApplyHealthDelta(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Survival")
	void ApplyItemEffects(const TArray<FSurvivalItemEffect>& Effects);

	UFUNCTION(BlueprintCallable, Category = "Survival")
	void SetEncumbranceRatio(float NewEncumbranceRatio);

	UFUNCTION(BlueprintPure, Category = "Survival")
	bool IsAlive() const { return Health > 0.0f; }

protected:
	UFUNCTION()
	void OnRep_Stats();

	void ClampAndBroadcast();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
