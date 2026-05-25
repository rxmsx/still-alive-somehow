#pragma once

#include "CoreMinimal.h"
#include "Building/SurvivalBuildTypes.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "BuildableActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API ABuildableActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABuildableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buildable")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buildable")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BuildableState, Category = "Buildable")
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Buildable")
	bool bWasPlacedByPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Buildable")
	bool bIsPreviewActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Buildable")
	FString StableBuildId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buildable")
	TObjectPtr<USurvivalBuildCatalog> BuildCatalog;

	UFUNCTION(BlueprintCallable, Category = "Buildable")
	virtual void InitializeBuildable(FName NewPartId, USurvivalBuildCatalog* NewBuildCatalog, bool bPlacedByPlayer);

	UFUNCTION(BlueprintCallable, Category = "Buildable")
	void SetPreviewState(bool bPreview, bool bPlacementValid);

	UFUNCTION(BlueprintPure, Category = "Buildable")
	const FSurvivalBuildPartDef& GetResolvedBuildPartDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Buildable")
	ECraftingStationType GetCraftingStationType() const;

	virtual FText GetInteractionPrompt_Implementation(const AActor* InteractingActor) const override;
	virtual bool CanInteract_Implementation(const AActor* InteractingActor) const override;
	virtual bool Interact_Implementation(AActor* InteractingActor) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_BuildableState();

	virtual void ApplyBuildPartDefinition();
	void ApplyTint(const FLinearColor& Tint);
};
