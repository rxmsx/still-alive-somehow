#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/SurvivalItemTypes.h"
#include "SurvivalBuildTypes.generated.h"

class ABuildableActor;
class UStaticMesh;

UENUM(BlueprintType)
enum class ESurvivalBuildPartKind : uint8
{
	Foundation,
	Wall,
	Door,
	Roof,
	Storage,
	Campfire,
	Workbench,
	Bedroll
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FSurvivalBuildPartDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESurvivalBuildPartKind Kind = ESurvivalBuildPartKind::Foundation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCraftingIngredient> Costs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ABuildableActor> BuildActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> PlacedMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector MeshScale = FVector(1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector PlacementBounds = FVector(150.0f, 150.0f, 20.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GroundOffsetZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRequiresGroundHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRequiresSupport = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> SnapToPartIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECraftingStationType CraftingStationType = ECraftingStationType::None;
};

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API USurvivalBuildCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSurvivalBuildPartDef> BuildParts;

	const FSurvivalBuildPartDef* FindBuildPart(FName PartId) const;

	static const TArray<FSurvivalBuildPartDef>& GetDefaultBuildParts();
	static const FSurvivalBuildPartDef* FindDefaultBuildPart(FName PartId);
};
