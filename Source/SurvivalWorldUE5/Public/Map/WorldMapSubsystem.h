#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Map/MapMarkerComponent.h"
#include "WorldMapSubsystem.generated.h"

UCLASS()
class SURVIVALWORLDUE5_API UWorldMapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void RegisterMarker(UMapMarkerComponent* Marker);
	void UnregisterMarker(UMapMarkerComponent* Marker);

	UFUNCTION(BlueprintPure, Category = "Map")
	TArray<FMapMarkerSnapshot> GetAllMarkers() const;

	UFUNCTION(BlueprintPure, Category = "Map")
	TArray<FMapMarkerSnapshot> GetMarkersAround(FVector WorldLocation, float Radius) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<UMapMarkerComponent>> Markers;
};
