#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ResourceNodeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceHarvested, int32, RemainingHarvests);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UResourceNodeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceNodeComponent();

	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnResourceHarvested OnResourceHarvested;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FString StableResourceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FName OutputItemId = FName(TEXT("Stone"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FName RequiredToolItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "1"))
	int32 AmountPerHarvest = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "1"))
	int32 MaxHarvests = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_RemainingHarvests, Category = "Resource")
	int32 RemainingHarvests = 3;

	UFUNCTION(BlueprintCallable, Category = "Resource")
	bool CanHarvest(const AActor* HarvestingActor) const;

	UFUNCTION(BlueprintCallable, Category = "Resource")
	bool Harvest(AActor* HarvestingActor, FName& OutItemId, int32& OutAmount);

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void SetRemainingHarvests(int32 NewRemainingHarvests);

	UFUNCTION(BlueprintPure, Category = "Resource")
	bool IsDepleted() const { return RemainingHarvests <= 0; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_RemainingHarvests();
};
