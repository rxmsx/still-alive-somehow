#include "Survival/SurvivalStatsComponent.h"
#include "Net/UnrealNetwork.h"

USurvivalStatsComponent::USurvivalStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USurvivalStatsComponent::TickSurvival(float DeltaSeconds, bool bIsSprinting)
{
	const float EncumbranceDrainBonus = FMath::Clamp(EncumbranceRatio - 0.55f, 0.0f, 1.0f);
	const float HungerMultiplier = (bIsSprinting ? SprintHungerMultiplier : 1.0f) + EncumbranceDrainBonus * 0.65f + Disease * 0.0025f;
	const float ThirstMultiplier = (bIsSprinting ? SprintThirstMultiplier : 1.0f) + EncumbranceDrainBonus * 0.85f + Disease * 0.0035f;
	Hunger -= HungerDrainPerSecond * HungerMultiplier * DeltaSeconds;
	Thirst -= ThirstDrainPerSecond * ThirstMultiplier * DeltaSeconds;
	Fatigue += (0.002f + EncumbranceDrainBonus * 0.010f + (bIsSprinting ? 0.018f : 0.0f)) * DeltaSeconds;

	if (bIsSprinting)
	{
		Stamina -= SprintStaminaDrainPerSecond * (1.0f + EncumbranceDrainBonus * 0.9f + Bleeding * 0.003f) * DeltaSeconds;
	}
	else
	{
		const float ScarcityPenalty = (Hunger < 15.0f || Thirst < 15.0f) ? 0.35f : 0.0f;
		const float RecoveryPenalty = FMath::Clamp(1.0f - EncumbranceDrainBonus * 0.65f - Fatigue * 0.004f - ScarcityPenalty, 0.18f, 1.0f);
		Stamina += StaminaRecoveryPerSecond * RecoveryPenalty * DeltaSeconds;
	}

	if (Hunger <= 0.0f || Thirst <= 0.0f)
	{
		Health -= DeltaSeconds;
	}

	if (Bleeding > 0.0f)
	{
		Health -= (0.12f + Bleeding * 0.018f) * DeltaSeconds;
		Bleeding -= 0.08f * DeltaSeconds;
	}

	if (Disease > 0.0f)
	{
		Health -= Disease * 0.002f * DeltaSeconds;
		Disease -= 0.02f * DeltaSeconds;
	}

	if (Poison > 0.0f)
	{
		Health -= Poison * 0.004f * DeltaSeconds;
		Poison -= 0.05f * DeltaSeconds;
	}

	ClampAndBroadcast();
}

bool USurvivalStatsComponent::ConsumeStamina(float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (Stamina < Amount)
	{
		return false;
	}

	Stamina -= Amount;
	ClampAndBroadcast();
	return true;
}

void USurvivalStatsComponent::ApplyHealthDelta(float Delta)
{
	Health += Delta;
	ClampAndBroadcast();
}

void USurvivalStatsComponent::ApplyItemEffects(const TArray<FSurvivalItemEffect>& Effects)
{
	for (const FSurvivalItemEffect& Effect : Effects)
	{
		switch (Effect.EffectType)
		{
		case ESurvivalItemEffectType::Health:
			Health += Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Hunger:
			Hunger += Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Thirst:
			Thirst += Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Stamina:
			Stamina += Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Temperature:
			TemperatureCelsius += Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Fatigue:
			Fatigue = Effect.bRemovesNegativeState ? 0.0f : Fatigue + Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Bleeding:
			Bleeding = Effect.bRemovesNegativeState ? 0.0f : Bleeding + Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Disease:
			Disease = Effect.bRemovesNegativeState ? FMath::Max(0.0f, Disease + Effect.Magnitude) : Disease + Effect.Magnitude;
			break;
		case ESurvivalItemEffectType::Poison:
			Poison = Effect.bRemovesNegativeState ? FMath::Max(0.0f, Poison + Effect.Magnitude) : Poison + Effect.Magnitude;
			break;
		default:
			break;
		}
	}

	ClampAndBroadcast();
}

void USurvivalStatsComponent::SetEncumbranceRatio(float NewEncumbranceRatio)
{
	EncumbranceRatio = FMath::Clamp(NewEncumbranceRatio, 0.0f, 2.0f);
	ClampAndBroadcast();
}

void USurvivalStatsComponent::ApplyNutrition(float NutritionAmount, float HydrationAmount)
{
	Hunger += NutritionAmount;
	Thirst += HydrationAmount;
	ClampAndBroadcast();
}

void USurvivalStatsComponent::SetSurvivalStats(float NewHealth, float NewHunger, float NewThirst, float NewStamina)
{
	Health = NewHealth;
	Hunger = NewHunger;
	Thirst = NewThirst;
	Stamina = NewStamina;
	ClampAndBroadcast();
}

void USurvivalStatsComponent::OnRep_Stats()
{
	OnStatsChanged.Broadcast();
}

void USurvivalStatsComponent::ClampAndBroadcast()
{
	Health = FMath::Clamp(Health, 0.0f, 100.0f);
	Hunger = FMath::Clamp(Hunger, 0.0f, 100.0f);
	Thirst = FMath::Clamp(Thirst, 0.0f, 100.0f);
	Stamina = FMath::Clamp(Stamina, 0.0f, 100.0f);
	TemperatureCelsius = FMath::Clamp(TemperatureCelsius, -60.0f, 60.0f);
	Fatigue = FMath::Clamp(Fatigue, 0.0f, 100.0f);
	Disease = FMath::Clamp(Disease, 0.0f, 100.0f);
	Bleeding = FMath::Clamp(Bleeding, 0.0f, 100.0f);
	Poison = FMath::Clamp(Poison, 0.0f, 100.0f);
	EncumbranceRatio = FMath::Clamp(EncumbranceRatio, 0.0f, 2.0f);
	OnStatsChanged.Broadcast();
}

void USurvivalStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USurvivalStatsComponent, Health);
	DOREPLIFETIME(USurvivalStatsComponent, Hunger);
	DOREPLIFETIME(USurvivalStatsComponent, Thirst);
	DOREPLIFETIME(USurvivalStatsComponent, Stamina);
	DOREPLIFETIME(USurvivalStatsComponent, TemperatureCelsius);
	DOREPLIFETIME(USurvivalStatsComponent, Fatigue);
	DOREPLIFETIME(USurvivalStatsComponent, Disease);
	DOREPLIFETIME(USurvivalStatsComponent, Bleeding);
	DOREPLIFETIME(USurvivalStatsComponent, Poison);
	DOREPLIFETIME(USurvivalStatsComponent, EncumbranceRatio);
}
