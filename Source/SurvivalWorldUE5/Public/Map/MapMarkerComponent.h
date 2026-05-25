#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MapMarkerComponent.generated.h"

UENUM(BlueprintType)
enum class ESurvivalMapMarkerType : uint8
{
	Resource,
	Camp,
	Cave,
	Player,
	Custom
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FMapMarkerSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName MarkerId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	ESurvivalMapMarkerType MarkerType = ESurvivalMapMarkerType::Custom;

	UPROPERTY(BlueprintReadOnly)
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor MarkerColor = FLinearColor::White;
};

UCLASS(ClassGroup = (Survival), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SURVIVALWORLDUE5_API UMapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMapMarkerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName MarkerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	ESurvivalMapMarkerType MarkerType = ESurvivalMapMarkerType::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FLinearColor MarkerColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	bool bShowOnMinimap = true;

	UFUNCTION(BlueprintPure, Category = "Map")
	FMapMarkerSnapshot GetMarkerSnapshot() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
