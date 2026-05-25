#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldTimeWeatherSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESurvivalWeatherState : uint8
{
	Clear,
	Cloudy,
	Rain,
	Storm,
	Fog
};

USTRUCT(BlueprintType)
struct SURVIVALWORLDUE5_API FWorldTimeWeatherState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 Day = 1;

	UPROPERTY(BlueprintReadWrite)
	float TimeOfDayHours = 8.0f;

	UPROPERTY(BlueprintReadWrite)
	ESurvivalWeatherState WeatherState = ESurvivalWeatherState::Clear;

	UPROPERTY(BlueprintReadWrite)
	float AmbientTemperatureC = 12.0f;

	UPROPERTY(BlueprintReadWrite)
	float RainIntensity = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float WindIntensity = 0.15f;
};

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API UWorldTimeWeatherSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Time", meta = (ClampMin = "0.0"))
	float TimeScale = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "-40.0", ClampMax = "50.0"))
	float BaseTemperatureC = 12.0f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintPure, Category = "World Time")
	FWorldTimeWeatherState GetState() const { return State; }

	UFUNCTION(BlueprintCallable, Category = "World Time")
	void RestoreState(const FWorldTimeWeatherState& NewState);

	UFUNCTION(BlueprintCallable, Category = "World Time")
	void SetTime(int32 NewDay, float NewTimeOfDayHours);

	UFUNCTION(BlueprintCallable, Category = "Weather")
	void SetWeather(ESurvivalWeatherState NewWeatherState);

	UFUNCTION(BlueprintPure, Category = "World Time")
	bool IsNight() const;

	UFUNCTION(BlueprintPure, Category = "World Time")
	float GetDaylightAlpha() const;

private:
	UPROPERTY()
	FWorldTimeWeatherState State;

	float NextWeatherChangeWorldHour = 11.0f;

	void AdvanceTime(float DeltaTime);
	void AdvanceWeather();
	void RecalculateWeatherDerivedValues();
	void ScheduleNextWeatherChange();
};
