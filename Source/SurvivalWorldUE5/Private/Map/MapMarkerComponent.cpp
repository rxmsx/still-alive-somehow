#include "Map/MapMarkerComponent.h"

#include "Map/WorldMapSubsystem.h"

UMapMarkerComponent::UMapMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UWorldMapSubsystem* MapSubsystem = GameInstance->GetSubsystem<UWorldMapSubsystem>())
			{
				MapSubsystem->RegisterMarker(this);
			}
		}
	}
}

void UMapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UWorldMapSubsystem* MapSubsystem = GameInstance->GetSubsystem<UWorldMapSubsystem>())
			{
				MapSubsystem->UnregisterMarker(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

FMapMarkerSnapshot UMapMarkerComponent::GetMarkerSnapshot() const
{
	FMapMarkerSnapshot Snapshot;
	Snapshot.MarkerId = MarkerId;
	Snapshot.DisplayName = DisplayName;
	Snapshot.MarkerType = MarkerType;
	Snapshot.MarkerColor = MarkerColor;

	if (const AActor* Owner = GetOwner())
	{
		Snapshot.WorldLocation = Owner->GetActorLocation();
	}

	return Snapshot;
}
