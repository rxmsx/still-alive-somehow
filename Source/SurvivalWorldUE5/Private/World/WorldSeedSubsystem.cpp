#include "World/WorldSeedSubsystem.h"
#include "Misc/Crc.h"

void UWorldSeedSubsystem::ConfigureWorld(const FString& InWorldName, int32 InWorldSeed)
{
	WorldName = InWorldName.IsEmpty() ? TEXT("Neue Welt") : InWorldName;
	WorldSeed = InWorldSeed;
}

void UWorldSeedSubsystem::GenerateRandomSeed()
{
	WorldSeed = FMath::Rand();
}

FRandomStream UWorldSeedSubsystem::MakeStream(FName Channel, int32 Salt) const
{
	const FString SeedKey = FString::Printf(TEXT("%d:%s:%d"), WorldSeed, *Channel.ToString(), Salt);
	return FRandomStream(static_cast<int32>(FCrc::StrCrc32(*SeedKey)));
}

FString UWorldSeedSubsystem::MakeStableId(FName Category, const FVector& WorldLocation, int32 Salt) const
{
	const FIntVector Quantized(
		FMath::RoundToInt(WorldLocation.X / 100.0f),
		FMath::RoundToInt(WorldLocation.Y / 100.0f),
		FMath::RoundToInt(WorldLocation.Z / 100.0f));

	return FString::Printf(
		TEXT("%s_%d_%d_%d_%d_%d"),
		*Category.ToString(),
		WorldSeed,
		Quantized.X,
		Quantized.Y,
		Quantized.Z,
		Salt);
}
