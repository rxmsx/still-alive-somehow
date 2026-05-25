#include "Survival/BodyConditionComponent.h"

#include "Survival/SurvivalStatsComponent.h"
#include "World/WorldTimeWeatherSubsystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UBodyConditionComponent::UBodyConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(true);
}

void UBodyConditionComponent::BeginPlay()
{
	Super::BeginPlay();
	ClampAndBroadcast();
}

void UBodyConditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime <= 0.0f)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	const UGameInstance* GameInstance = Owner && Owner->GetWorld() ? Owner->GetWorld()->GetGameInstance() : nullptr;
	const UWorldTimeWeatherSubsystem* WeatherSubsystem = GameInstance ? GameInstance->GetSubsystem<UWorldTimeWeatherSubsystem>() : nullptr;
	const FWorldTimeWeatherState Weather = WeatherSubsystem ? WeatherSubsystem->GetState() : FWorldTimeWeatherState();

	const bool bWarmed = WarmthBufferSeconds > 0.0f;
	const float RainWetness = Weather.RainIntensity * WetnessGainPerRainSecond * DeltaTime;
	Wetness += RainWetness;
	Wetness -= DryingPerSecond * (bWarmed ? 3.0f : 1.0f) * DeltaTime;

	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const bool bMoving = OwnerPawn && OwnerPawn->GetVelocity().SizeSquared2D() > FMath::Square(120.0f);
	Fatigue += FatigueGainPerSecond * (bMoving ? MovingFatigueMultiplier : 1.0f) * DeltaTime;

	const float WetColdPenalty = Wetness * 0.045f;
	const float WindColdPenalty = Weather.WindIntensity * 1.8f;
	const float WarmthBonus = bWarmed ? 14.0f : 0.0f;
	const float TargetCoreTemperature = 37.0f + ((Weather.AmbientTemperatureC + WarmthBonus - WetColdPenalty - WindColdPenalty) - 12.0f) * 0.035f;
	CoreTemperatureC = FMath::FInterpTo(CoreTemperatureC, TargetCoreTemperature, DeltaTime, 0.05f);

	if (WarmthBufferSeconds > 0.0f)
	{
		WarmthBufferSeconds = FMath::Max(0.0f, WarmthBufferSeconds - DeltaTime);
	}

	if (USurvivalStatsComponent* Stats = Owner ? Owner->FindComponentByClass<USurvivalStatsComponent>() : nullptr)
	{
		if (CoreTemperatureC < 35.0f)
		{
			Stats->ApplyHealthDelta(-(35.0f - CoreTemperatureC) * 0.08f * DeltaTime);
		}

		if (BleedingSeverity > 0.0f)
		{
			Stats->ApplyHealthDelta(-BleedingSeverity * 0.02f * DeltaTime);
		}
	}

	ClampAndBroadcast();
}

void UBodyConditionComponent::AddWarmth(float WarmthSeconds)
{
	WarmthBufferSeconds = FMath::Max(WarmthBufferSeconds, WarmthSeconds);
	CoreTemperatureC += FMath::Clamp(WarmthSeconds * 0.01f, 0.0f, 0.25f);
	ClampAndBroadcast();
}

void UBodyConditionComponent::ApplyBleeding(float SeverityDelta)
{
	BleedingSeverity += SeverityDelta;
	ClampAndBroadcast();
}

void UBodyConditionComponent::RestoreBodyCondition(const FBodyConditionState& NewState)
{
	CoreTemperatureC = NewState.CoreTemperatureC;
	Wetness = NewState.Wetness;
	Fatigue = NewState.Fatigue;
	BleedingSeverity = NewState.BleedingSeverity;
	ClampAndBroadcast();
}

FBodyConditionState UBodyConditionComponent::GetBodyConditionState() const
{
	FBodyConditionState State;
	State.CoreTemperatureC = CoreTemperatureC;
	State.Wetness = Wetness;
	State.Fatigue = Fatigue;
	State.BleedingSeverity = BleedingSeverity;
	return State;
}

void UBodyConditionComponent::OnRep_BodyCondition()
{
	OnBodyConditionChanged.Broadcast();
}

void UBodyConditionComponent::ClampAndBroadcast()
{
	CoreTemperatureC = FMath::Clamp(CoreTemperatureC, 30.0f, 42.0f);
	Wetness = FMath::Clamp(Wetness, 0.0f, 100.0f);
	Fatigue = FMath::Clamp(Fatigue, 0.0f, 100.0f);
	BleedingSeverity = FMath::Clamp(BleedingSeverity, 0.0f, 100.0f);
	OnBodyConditionChanged.Broadcast();
}

void UBodyConditionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBodyConditionComponent, CoreTemperatureC);
	DOREPLIFETIME(UBodyConditionComponent, Wetness);
	DOREPLIFETIME(UBodyConditionComponent, Fatigue);
	DOREPLIFETIME(UBodyConditionComponent, BleedingSeverity);
}
