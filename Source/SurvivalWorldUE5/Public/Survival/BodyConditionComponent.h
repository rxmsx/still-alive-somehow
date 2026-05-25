#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BodyConditionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBodyConditionChanged);

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FBodyConditionState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float CoreTemperatureC = 37.0f;

	UPROPERTY(BlueprintReadWrite)
	float Wetness = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float Fatigue = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float BleedingSeverity = 0.0f;
};

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UBodyConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBodyConditionComponent();

	UPROPERTY(BlueprintAssignable, Category = "Body")
	FOnBodyConditionChanged OnBodyConditionChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BodyCondition, Category = "Body", meta = (ClampMin = "30.0", ClampMax = "42.0"))
	float CoreTemperatureC = 37.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BodyCondition, Category = "Body", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Wetness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BodyCondition, Category = "Body", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Fatigue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BodyCondition, Category = "Body", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float BleedingSeverity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float WetnessGainPerRainSecond = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float DryingPerSecond = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float FatigueGainPerSecond = 0.004f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.0"))
	float MovingFatigueMultiplier = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "Body")
	void AddWarmth(float WarmthSeconds);

	UFUNCTION(BlueprintCallable, Category = "Body")
	void ApplyBleeding(float SeverityDelta);

	UFUNCTION(BlueprintCallable, Category = "Body")
	void RestoreBodyCondition(const FBodyConditionState& NewState);

	UFUNCTION(BlueprintPure, Category = "Body")
	FBodyConditionState GetBodyConditionState() const;

	UFUNCTION(BlueprintPure, Category = "Body")
	bool IsHypothermic() const { return CoreTemperatureC < 35.0f; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_BodyCondition();

private:
	float WarmthBufferSeconds = 0.0f;

	void ClampAndBroadcast();
};
