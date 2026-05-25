#include "Map/WorldMapSubsystem.h"

void UWorldMapSubsystem::RegisterMarker(UMapMarkerComponent* Marker)
{
	if (Marker && !Markers.Contains(Marker))
	{
		Markers.Add(Marker);
	}
}

void UWorldMapSubsystem::UnregisterMarker(UMapMarkerComponent* Marker)
{
	Markers.Remove(Marker);
}

TArray<FMapMarkerSnapshot> UWorldMapSubsystem::GetAllMarkers() const
{
	TArray<FMapMarkerSnapshot> Snapshots;
	for (const UMapMarkerComponent* Marker : Markers)
	{
		if (Marker && Marker->bShowOnMinimap)
		{
			Snapshots.Add(Marker->GetMarkerSnapshot());
		}
	}
	return Snapshots;
}

TArray<FMapMarkerSnapshot> UWorldMapSubsystem::GetMarkersAround(FVector WorldLocation, float Radius) const
{
	TArray<FMapMarkerSnapshot> Snapshots;
	const float RadiusSq = FMath::Square(Radius);
	for (const UMapMarkerComponent* Marker : Markers)
	{
		if (!Marker || !Marker->bShowOnMinimap)
		{
			continue;
		}

		const FMapMarkerSnapshot Snapshot = Marker->GetMarkerSnapshot();
		if (FVector::DistSquared2D(Snapshot.WorldLocation, WorldLocation) <= RadiusSq)
		{
			Snapshots.Add(Snapshot);
		}
	}
	return Snapshots;
}
