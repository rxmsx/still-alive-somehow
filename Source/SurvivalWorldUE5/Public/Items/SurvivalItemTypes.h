#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurvivalItemTypes.generated.h"

UENUM(BlueprintType)
enum class ESurvivalItemCategory : uint8
{
	Resource,
	Tool,
	Food,
	Building,
	Ore,
	Misc
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FItemDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESurvivalItemCategory Category = ESurvivalItemCategory::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MaxStack = 99;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsTool = false;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FCraftingIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 Count = 1;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FCraftingRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCraftingIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OutputItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 OutputCount = 1;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FResourceNodeDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ResourceNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OutputItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RequiredToolItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 AmountPerHarvest = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MaxHarvests = 1;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FBiomeDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BiomeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float TreeDensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float RockDensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float OreDensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor DebugColor = FLinearColor::Green;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FOreVeinDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OreVeinId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OreItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PreferredBiomeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 NodesPerCluster = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MinDepthMeters = 8.0f;
};

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API USurvivalItemCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemDef> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCraftingRecipe> Recipes;
};

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API UOpenWorldBiomeCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FBiomeDef> Biomes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FResourceNodeDef> ResourceNodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FOreVeinDef> OreVeins;
};
