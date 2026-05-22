#include "World/WorldTimeWeatherSubsystem.h"

#include "Stats/Stats.h"

void UWorldTimeWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RecalculateWeatherDerivedValues();
	ScheduleNextWeatherChange();
}

void UWorldTimeWeatherSubsystem::Tick(float DeltaTime)
{
	if (DeltaTime <= 0.0f || TimeScale <= 0.0f)
	{
		return;
	}

	AdvanceTime(DeltaTime);
	AdvanceWeather();
	RecalculateWeatherDerivedValues();
}

TStatId UWorldTimeWeatherSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWorldTimeWeatherSubsystem, STATGROUP_Tickables);
}

bool UWorldTimeWeatherSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject);
}

void UWorldTimeWeatherSubsystem::RestoreState(const FWorldTimeWeatherState& NewState)
{
	State = NewState;
	SetTime(FMath::Max(1, State.Day), State.TimeOfDayHours);
	SetWeather(State.WeatherState);
	ScheduleNextWeatherChange();
}

void UWorldTimeWeatherSubsystem::SetTime(int32 NewDay, float NewTimeOfDayHours)
{
	State.Day = FMath::Max(1, NewDay);
	State.TimeOfDayHours = FMath::Fmod(FMath::Max(0.0f, NewTimeOfDayHours), 24.0f);
	RecalculateWeatherDerivedValues();
}

void UWorldTimeWeatherSubsystem::SetWeather(ESurvivalWeatherState NewWeatherState)
{
	State.WeatherState = NewWeatherState;
	RecalculateWeatherDerivedValues();
}

bool UWorldTimeWeatherSubsystem::IsNight() const
{
	return State.TimeOfDayHours < 5.5f || State.TimeOfDayHours > 21.0f;
}

float UWorldTimeWeatherSubsystem::GetDaylightAlpha() const
{
	const float Sunrise = 5.5f;
	const float Noon = 13.0f;
	const float Sunset = 21.0f;

	if (State.TimeOfDayHours < Sunrise || State.TimeOfDayHours > Sunset)
	{
		return 0.0f;
	}

	if (State.TimeOfDayHours <= Noon)
	{
		return FMath::Clamp((State.TimeOfDayHours - Sunrise) / (Noon - Sunrise), 0.0f, 1.0f);
	}

	return FMath::Clamp((Sunset - State.TimeOfDayHours) / (Sunset - Noon), 0.0f, 1.0f);
}

void UWorldTimeWeatherSubsystem::AdvanceTime(float DeltaTime)
{
	const float PreviousTime = State.TimeOfDayHours;
	State.TimeOfDayHours += (DeltaTime * TimeScale) / 3600.0f;

	while (State.TimeOfDayHours >= 24.0f)
	{
		State.TimeOfDayHours -= 24.0f;
		State.Day += 1;
	}

	if (State.TimeOfDayHours < PreviousTime)
	{
		ScheduleNextWeatherChange();
	}
}

void UWorldTimeWeatherSubsystem::AdvanceWeather()
{
	const float CurrentWorldHour = static_cast<float>((State.Day - 1) * 24) + State.TimeOfDayHours;
	if (CurrentWorldHour < NextWeatherChangeWorldHour)
	{
		return;
	}

	const float Roll = FMath::FRand();
	if (Roll < 0.34f)
	{
		SetWeather(ESurvivalWeatherState::Clear);
	}
	else if (Roll < 0.58f)
	{
		SetWeather(ESurvivalWeatherState::Cloudy);
	}
	else if (Roll < 0.80f)
	{
		SetWeather(ESurvivalWeatherState::Rain);
	}
	else if (Roll < 0.92f)
	{
		SetWeather(ESurvivalWeatherState::Fog);
	}
	else
	{
		SetWeather(ESurvivalWeatherState::Storm);
	}

	ScheduleNextWeatherChange();
}

void UWorldTimeWeatherSubsystem::RecalculateWeatherDerivedValues()
{
	const float Daylight = GetDaylightAlpha();
	float WeatherTemperatureOffset = 0.0f;
	State.RainIntensity = 0.0f;
	State.WindIntensity = 0.15f;

	switch (State.WeatherState)
	{
	case ESurvivalWeatherState::Clear:
		State.WindIntensity = 0.12f;
		break;
	case ESurvivalWeatherState::Cloudy:
		WeatherTemperatureOffset = -1.5f;
		State.WindIntensity = 0.25f;
		break;
	case ESurvivalWeatherState::Rain:
		WeatherTemperatureOffset = -3.5f;
		State.RainIntensity = 0.65f;
		State.WindIntensity = 0.45f;
		break;
	case ESurvivalWeatherState::Storm:
		WeatherTemperatureOffset = -5.0f;
		State.RainIntensity = 1.0f;
		State.WindIntensity = 0.90f;
		break;
	case ESurvivalWeatherState::Fog:
		WeatherTemperatureOffset = -2.0f;
		State.RainIntensity = 0.15f;
		State.WindIntensity = 0.18f;
		break;
	default:
		break;
	}

	const float DayTemperatureOffset = FMath::Lerp(-6.0f, 7.0f, Daylight);
	State.AmbientTemperatureC = BaseTemperatureC + DayTemperatureOffset + WeatherTemperatureOffset;
}

void UWorldTimeWeatherSubsystem::ScheduleNextWeatherChange()
{
	const float CurrentWorldHour = static_cast<float>((State.Day - 1) * 24) + State.TimeOfDayHours;
	NextWeatherChangeWorldHour = CurrentWorldHour + FMath::FRandRange(3.0f, 7.0f);
}
