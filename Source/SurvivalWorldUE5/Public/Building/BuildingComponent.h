#pragma once

#include "CoreMinimal.h"
#include "Building/SurvivalBuildTypes.h"
#include "Components/ActorComponent.h"
#include "BuildingComponent.generated.h"

class ABuildableActor;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildModeChanged, bool, bBuildModeActive, FName, SelectedPartId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildPlacementChanged, bool, bIsValid, FText, Message);

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UBuildingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Building")
	FOnBuildModeChanged OnBuildModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Building")
	FOnBuildPlacementChanged OnBuildPlacementChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<USurvivalBuildCatalog> BuildCatalog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FName DefaultBuildPartId = TEXT("Foundation");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	TSubclassOf<ABuildableActor> PreviewActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "100.0"))
	float PlacementRange = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "1.0"))
	float RotationStepDegrees = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building", meta = (ClampMin = "0.0"))
	float PlacementCollisionPadding = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	ESurvivalToolType RequiredBuildToolType = ESurvivalToolType::Hammer;

	UFUNCTION(BlueprintCallable, Category = "Building")
	void ToggleBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool StartBuildMode(FName PartId);

	UFUNCTION(BlueprintCallable, Category = "Building")
	void CancelBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool SelectBuildPart(FName PartId);

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool CycleBuildPart(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Building")
	void RotatePreview(float Direction = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool ConfirmPlacement();

	UFUNCTION(BlueprintPure, Category = "Building")
	bool IsBuildModeActive() const { return bBuildModeActive; }

	UFUNCTION(BlueprintPure, Category = "Building")
	FName GetSelectedBuildPartId() const { return SelectedBuildPartId; }

	UFUNCTION(BlueprintPure, Category = "Building")
	bool IsPlacementValid() const { return bPlacementValid; }

	UFUNCTION(BlueprintPure, Category = "Building")
	FText GetPlacementMessage() const { return PlacementMessage; }

	UFUNCTION(BlueprintPure, Category = "Building")
	bool GetSelectedBuildPartDefinition(FSurvivalBuildPartDef& OutBuildPart) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TObjectPtr<ABuildableActor> PreviewActor;

	bool bBuildModeActive = false;
	bool bPlacementValid = false;
	FName SelectedBuildPartId = NAME_None;
	float PreviewYaw = 0.0f;
	FTransform LastPreviewTransform;
	FText PlacementMessage;

	const FSurvivalBuildPartDef* ResolveBuildPart(FName PartId) const;
	UInventoryComponent* GetOwnerInventory() const;
	bool HasRequiredTool() const;
	bool HasCosts(const FSurvivalBuildPartDef& BuildPart) const;
	bool ConsumeCosts(const FSurvivalBuildPartDef& BuildPart) const;
	bool BuildPreviewTransform(const FSurvivalBuildPartDef& BuildPart, FTransform& OutTransform, FText& OutMessage) const;
	bool TryGetSnapTransform(const FSurvivalBuildPartDef& BuildPart, const FHitResult& HitResult, FTransform& OutTransform) const;
	bool IsLocationFree(const FSurvivalBuildPartDef& BuildPart, const FTransform& Transform) const;
	void UpdatePreview();
	void SpawnPreviewActor();
	void DestroyPreviewActor();
};
