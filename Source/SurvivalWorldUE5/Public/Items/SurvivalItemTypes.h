#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurvivalItemTypes.generated.h"

class UStaticMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class ESurvivalItemCategory : uint8
{
	Resource,
	Food,
	Drink,
	Medicine,
	Tool,
	Weapon,
	Ammunition,
	Armor,
	Building,
	Ore,
	Quest,
	Special,
	Misc
};

UENUM(BlueprintType)
enum class ESurvivalItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary,
	Quest
};

UENUM(BlueprintType)
enum class ESurvivalItemEffectType : uint8
{
	Health,
	Hunger,
	Thirst,
	Stamina,
	Temperature,
	Fatigue,
	Bleeding,
	Disease,
	Poison
};

UENUM(BlueprintType)
enum class ESurvivalItemUseType : uint8
{
	None,
	Consume,
	Equip,
	UseTool,
	PlaceWorldObject,
	Read,
	Quest
};

UENUM(BlueprintType)
enum class ECraftingStationType : uint8
{
	None,
	Workbench,
	Campfire,
	Forge,
	CookingStation,
	MedicalTable
};

UENUM(BlueprintType)
enum class ECraftingRecipeCategory : uint8
{
	Survival,
	Tools,
	Weapons,
	Food,
	Medicine,
	Building,
	Clothing,
	Quest,
	Unknown
};

UENUM(BlueprintType)
enum class ECraftingFailureReason : uint8
{
	None,
	UnknownRecipe,
	RecipeLocked,
	MissingMaterials,
	WrongStation,
	InventoryFull,
	InvalidInput,
	AlreadyCrafting
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FSurvivalItemEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESurvivalItemEffectType EffectType = ESurvivalItemEffectType::Health;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRemovesNegativeState = false;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FItemDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESurvivalItemCategory Category = ESurvivalItemCategory::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESurvivalItemRarity Rarity = ESurvivalItemRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> InventoryImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MaxStack = 99;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float WeightKg = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MaxDurability = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHasDurability = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bPerishable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", EditCondition = "bPerishable"))
	float SpoilTimeHours = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESurvivalItemUseType UseType = ESurvivalItemUseType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bStackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDroppable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bEquippable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bHotbarAllowed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSurvivalItemEffect> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AssetIconPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PreviewMeshPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector PreviewScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator PreviewRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsTool = false;

	int32 GetEffectiveMaxStack() const { return bStackable ? FMath::Max(1, MaxStack) : 1; }
	float GetSpawnDurability() const { return bHasDurability ? FMath::Max(1.0f, MaxDurability) : 0.0f; }
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
struct SURVIVALWORLDUE5_API FCraftingValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bCanCraft = false;

	UPROPERTY(BlueprintReadOnly)
	ECraftingFailureReason FailureReason = ECraftingFailureReason::None;

	UPROPERTY(BlueprintReadOnly)
	FText Message;
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FCraftingRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUnlockedByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowWhenLocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECraftingRecipeCategory Category = ECraftingRecipeCategory::Survival;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECraftingStationType RequiredStation = ECraftingStationType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCraftingIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OutputItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 OutputCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float CraftTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequiredSkillLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UnlockTag = NAME_None;
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

	const FItemDef* FindItem(FName ItemId) const;
	const FCraftingRecipe* FindRecipe(FName RecipeId) const;

	static const TArray<FItemDef>& GetDefaultItems();
	static const TArray<FCraftingRecipe>& GetDefaultRecipes();
	static const FItemDef* FindDefaultItem(FName ItemId);
	static const FCraftingRecipe* FindDefaultRecipe(FName RecipeId);
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
