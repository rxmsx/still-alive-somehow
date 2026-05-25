#include "Survival/SurvivalStatsComponent.h"
#include "Net/UnrealNetwork.h"

USurvivalStatsComponent::USurvivalStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USurvivalStatsComponent::TickSurvival(float DeltaSeconds, bool bIsSprinting)
{
	const float HungerMultiplier = bIsSprinting ? SprintHungerMultiplier : 1.0f;
	const float ThirstMultiplier = bIsSprinting ? SprintThirstMultiplier : 1.0f;
	Hunger -= HungerDrainPerSecond * HungerMultiplier * DeltaSeconds;
	Thirst -= ThirstDrainPerSecond * ThirstMultiplier * DeltaSeconds;

	if (bIsSprinting)
	{
		Stamina -= SprintStaminaDrainPerSecond * DeltaSeconds;
	}
	else
	{
		Stamina += StaminaRecoveryPerSecond * DeltaSeconds;
	}

	if (Hunger <= 0.0f || Thirst <= 0.0f)
	{
		Health -= DeltaSeconds;
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
	OnStatsChanged.Broadcast();
}

void USurvivalStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USurvivalStatsComponent, Health);
	DOREPLIFETIME(USurvivalStatsComponent, Hunger);
	DOREPLIFETIME(USurvivalStatsComponent, Thirst);
	DOREPLIFETIME(USurvivalStatsComponent, Stamina);
}
