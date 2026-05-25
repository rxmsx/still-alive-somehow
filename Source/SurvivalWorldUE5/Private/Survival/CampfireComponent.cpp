#include "Survival/CampfireComponent.h"

#include "Crafting/CraftingComponent.h"
#include "Items/InventoryComponent.h"
#include "Survival/BodyConditionComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UCampfireComponent::UCampfireComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	SetIsReplicatedByDefault(true);
}

void UCampfireComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLit())
	{
		return;
	}

	FuelSecondsRemaining = FMath::Max(0.0f, FuelSecondsRemaining - DeltaTime * FuelUseMultiplier);
	WarmNearbyActors();
	BroadcastChanged();
}

void UCampfireComponent::AddFuelSeconds(float FuelSeconds)
{
	if (FuelSeconds <= 0.0f)
	{
		return;
	}

	FuelSecondsRemaining += FuelSeconds;
	BroadcastChanged();
}

bool UCampfireComponent::TryAddBestFuelFromActor(AActor* Actor)
{
	UInventoryComponent* Inventory = Actor ? Actor->FindComponentByClass<UInventoryComponent>() : nullptr;
	UCraftingComponent* Crafting = Actor ? Actor->FindComponentByClass<UCraftingComponent>() : nullptr;
	if (!Inventory || !Crafting)
	{
		return false;
	}

	FName BestFuelItemId = NAME_None;
	float BestBurnDuration = 0.0f;
	for (const TPair<FName, int32>& Pair : Inventory->GetSnapshot())
	{
		if (Pair.Value <= 0)
		{
			continue;
		}

		FItemDef Item;
		if (Crafting->GetItemDefinition(Pair.Key, Item) && Item.bIsFuel && Item.BurnDurationSeconds > BestBurnDuration)
		{
			BestFuelItemId = Pair.Key;
			BestBurnDuration = Item.BurnDurationSeconds;
		}
	}

	if (BestFuelItemId.IsNone() || BestBurnDuration <= 0.0f || !Inventory->RemoveItem(BestFuelItemId, 1))
	{
		return false;
	}

	AddFuelSeconds(BestBurnDuration);
	return true;
}

bool UCampfireComponent::TryCookFoodFromActor(AActor* Actor)
{
	if (!IsLit())
	{
		return false;
	}

	UInventoryComponent* Inventory = Actor ? Actor->FindComponentByClass<UInventoryComponent>() : nullptr;
	UCraftingComponent* Crafting = Actor ? Actor->FindComponentByClass<UCraftingComponent>() : nullptr;
	if (!Inventory || !Crafting)
	{
		return false;
	}

	FName RawFoodItemId = NAME_None;
	FName CookedFoodItemId = NAME_None;
	int32 CookedFoodCount = 1;
	for (const TPair<FName, int32>& Pair : Inventory->GetSnapshot())
	{
		if (Pair.Value <= 0)
		{
			continue;
		}

		FItemDef Item;
		if (!Crafting->GetItemDefinition(Pair.Key, Item)
			|| Item.Category != ESurvivalItemCategory::Food
			|| !Item.bCanBeProcessed
			|| Item.ProcessingOutputItemId.IsNone())
		{
			continue;
		}

		RawFoodItemId = Pair.Key;
		CookedFoodItemId = Item.ProcessingOutputItemId;
		CookedFoodCount = FMath::Max(1, Item.ProcessingOutputCount);
		break;
	}

	if (RawFoodItemId.IsNone() || CookedFoodItemId.IsNone() || !Inventory->RemoveItem(RawFoodItemId, 1))
	{
		return false;
	}

	Inventory->AddItem(CookedFoodItemId, CookedFoodCount);
	BroadcastChanged();
	return true;
}

bool UCampfireComponent::InteractWithActor(AActor* Actor)
{
	if (TryCookFoodFromActor(Actor))
	{
		return true;
	}

	return TryAddBestFuelFromActor(Actor);
}

void UCampfireComponent::OnRep_Campfire()
{
	BroadcastChanged();
}

void UCampfireComponent::WarmNearbyActors()
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	const FVector FireLocation = Owner->GetActorLocation();
	const float HeatRadiusSquared = FMath::Square(HeatRadius);
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn || FVector::DistSquared(Pawn->GetActorLocation(), FireLocation) > HeatRadiusSquared)
		{
			continue;
		}

		if (UBodyConditionComponent* BodyCondition = Pawn->FindComponentByClass<UBodyConditionComponent>())
		{
			BodyCondition->AddWarmth(WarmthSecondsPerPulse);
		}
	}
}

void UCampfireComponent::BroadcastChanged()
{
	OnCampfireChanged.Broadcast();
}

void UCampfireComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCampfireComponent, FuelSecondsRemaining);
}
