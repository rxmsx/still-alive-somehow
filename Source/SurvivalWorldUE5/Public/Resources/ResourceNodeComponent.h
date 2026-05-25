#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/SurvivalItemTypes.h"
#include "ResourceNodeComponent.generated.h"

class AItemPickupActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceHarvested, int32, RemainingHarvests);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceDamaged, float, CurrentHealth, float, MaxHealth);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UResourceNodeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceNodeComponent();

	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnResourceHarvested OnResourceHarvested;

	UPROPERTY(BlueprintAssignable, Category = "Resource")
	FOnResourceDamaged OnResourceDamaged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FString StableResourceId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FName ResourceNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FName OutputItemId = FName(TEXT("Stone"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	FName RequiredToolItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	ESurvivalToolType RequiredToolType = ESurvivalToolType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	TArray<ESurvivalToolType> AlternativeToolTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	bool bAllowBareHands = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "1"))
	int32 AmountPerHarvest = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "1"))
	int32 MaxHarvests = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "1.0"))
	float MaxHealth = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Resource")
	float CurrentHealth = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "0.1"))
	float BaseHarvestDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "0.0"))
	float WrongToolDamageMultiplier = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource", meta = (ClampMin = "0.0"))
	float ToolDurabilityCost = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Resource")
	TArray<FResourceLootEntry> Loot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|World")
	TSubclassOf<AItemPickupActor> LootPickupActorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_RemainingHarvests, Category = "Resource")
	int32 RemainingHarvests = 3;

	UFUNCTION(BlueprintCallable, Category = "Resource")
	void ConfigureFromDefinition(const FResourceNodeDef& ResourceDef);

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

	UFUNCTION()
	void OnRep_Health();

	bool ResolveHarvestTool(const AActor* HarvestingActor, int32& OutToolSlotIndex, FItemDef& OutToolDef, bool& bOutHasTool, bool& bOutCorrectTool) const;
	bool IsToolTypeAccepted(ESurvivalToolType ToolType) const;
	bool GrantOrDropLoot(AActor* HarvestingActor, const FResourceLootEntry& LootEntry, bool bIsDepletionLoot, FName& OutFirstItemId, int32& OutFirstAmount) const;
	bool SpawnLootPickup(AActor* HarvestingActor, FName ItemId, int32 Count) const;
	void RefreshRemainingHarvestsFromHealth();
};
