#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Crafting/CraftingComponent.h"
#include "GameFramework/Actor.h"
#include "Items/InventoryComponent.h"
#include "Survival/BodyConditionComponent.h"
#include "Survival/CampfireComponent.h"
#include "Survival/SurvivalStatsComponent.h"
#include "World/WorldSeedSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryComponentBasicTest,
	"SurvivalWorld.Foundation.Inventory.AddRemoveHas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryComponentBasicTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>();

	TestTrue(TEXT("Adding wood succeeds"), Inventory->AddItem(TEXT("Wood"), 3));
	TestEqual(TEXT("Wood count is tracked"), Inventory->GetItemCount(TEXT("Wood")), 3);
	TestTrue(TEXT("HasItem accepts exact count"), Inventory->HasItem(TEXT("Wood"), 3));
	TestTrue(TEXT("Removing one wood succeeds"), Inventory->RemoveItem(TEXT("Wood"), 1));
	TestEqual(TEXT("Wood count is reduced"), Inventory->GetItemCount(TEXT("Wood")), 2);
	TestFalse(TEXT("Cannot remove more than available"), Inventory->RemoveItem(TEXT("Wood"), 99));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingComponentRecipeTest,
	"SurvivalWorld.Foundation.Crafting.StoneBladeRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCraftingComponentRecipeTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>();
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>(Owner);
	UCraftingComponent* Crafting = NewObject<UCraftingComponent>(Owner);
	Owner->AddOwnedComponent(Inventory);
	Owner->AddOwnedComponent(Crafting);

	TestTrue(TEXT("Fallback recipes include the survival crafting set"), Crafting->GetKnownRecipes().Num() >= 7);
	TestFalse(TEXT("Stone blade cannot craft without stone"), Crafting->CanCraft(TEXT("StoneBlade")));

	Inventory->AddItem(TEXT("Stone"), 2);
	TestTrue(TEXT("Stone blade becomes craftable with two stone"), Crafting->CanCraft(TEXT("StoneBlade")));
	TestTrue(TEXT("Crafting stone blade succeeds"), Crafting->CraftRecipe(TEXT("StoneBlade")));
	TestEqual(TEXT("Stone is consumed"), Inventory->GetItemCount(TEXT("Stone")), 0);
	TestEqual(TEXT("Stone blade is added"), Inventory->GetItemCount(TEXT("StoneBlade")), 1);

	Inventory->AddItem(TEXT("Stick"), 2);
	Inventory->AddItem(TEXT("Rope"), 1);
	Inventory->AddItem(TEXT("AnimalSinew"), 2);
	TestTrue(TEXT("Bow recipe becomes craftable with sticks, rope, and sinew"), Crafting->CanCraft(TEXT("Bow")));
	TestTrue(TEXT("Crafting bow succeeds"), Crafting->CraftRecipe(TEXT("Bow")));
	TestEqual(TEXT("Bow is added"), Inventory->GetItemCount(TEXT("Bow")), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurvivalStatsConsumptionTest,
	"SurvivalWorld.Foundation.Survival.Consumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurvivalStatsConsumptionTest::RunTest(const FString& Parameters)
{
	USurvivalStatsComponent* Stats = NewObject<USurvivalStatsComponent>();
	Stats->SetSurvivalStats(80.0f, 25.0f, 20.0f, 60.0f);
	Stats->ApplyNutrition(35.0f, 45.0f);

	TestEqual(TEXT("Food increases hunger"), Stats->Hunger, 60.0f);
	TestEqual(TEXT("Water increases thirst"), Stats->Thirst, 65.0f);
	TestEqual(TEXT("Health is preserved"), Stats->Health, 80.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBodyConditionRestoreTest,
	"SurvivalWorld.Foundation.Survival.BodyConditionRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBodyConditionRestoreTest::RunTest(const FString& Parameters)
{
	UBodyConditionComponent* BodyCondition = NewObject<UBodyConditionComponent>();
	FBodyConditionState State;
	State.CoreTemperatureC = 35.5f;
	State.Wetness = 42.0f;
	State.Fatigue = 18.0f;
	State.BleedingSeverity = 4.0f;

	BodyCondition->RestoreBodyCondition(State);
	const FBodyConditionState Restored = BodyCondition->GetBodyConditionState();

	TestEqual(TEXT("Temperature restores"), Restored.CoreTemperatureC, 35.5f);
	TestEqual(TEXT("Wetness restores"), Restored.Wetness, 42.0f);
	TestEqual(TEXT("Fatigue restores"), Restored.Fatigue, 18.0f);
	TestEqual(TEXT("Bleeding restores"), Restored.BleedingSeverity, 4.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCampfireComponentFuelAndCookingTest,
	"SurvivalWorld.Foundation.Survival.CampfireFuelAndCooking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCampfireComponentFuelAndCookingTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>();
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>(Owner);
	UCraftingComponent* Crafting = NewObject<UCraftingComponent>(Owner);
	UCampfireComponent* Campfire = NewObject<UCampfireComponent>(Owner);
	Owner->AddOwnedComponent(Inventory);
	Owner->AddOwnedComponent(Crafting);
	Owner->AddOwnedComponent(Campfire);

	Inventory->AddItem(TEXT("Wood"), 1);
	TestTrue(TEXT("Campfire accepts wood as fuel"), Campfire->TryAddBestFuelFromActor(Owner));
	TestTrue(TEXT("Campfire is lit after fuel"), Campfire->IsLit());

	Inventory->AddItem(TEXT("RawFish"), 1);
	TestTrue(TEXT("Campfire cooks raw fish"), Campfire->TryCookFoodFromActor(Owner));
	TestEqual(TEXT("Raw fish is consumed"), Inventory->GetItemCount(TEXT("RawFish")), 0);
	TestEqual(TEXT("Cooked fish is added"), Inventory->GetItemCount(TEXT("CookedFish")), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldSeedStableIdFormatTest,
	"SurvivalWorld.Foundation.WorldSeed.StableIdFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSeedStableIdFormatTest::RunTest(const FString& Parameters)
{
	UWorldSeedSubsystem* SeedSubsystem = NewObject<UWorldSeedSubsystem>();
	SeedSubsystem->ConfigureWorld(TEXT("Test World"), 42);

	const FString StableIdA = SeedSubsystem->MakeStableId(TEXT("Iron"), FVector(100.0f, 200.0f, -300.0f));
	const FString StableIdB = SeedSubsystem->MakeStableId(TEXT("Iron"), FVector(100.0f, 200.0f, -300.0f));

	TestEqual(TEXT("Stable IDs are deterministic"), StableIdA, StableIdB);
	TestTrue(TEXT("Stable ID includes category"), StableIdA.StartsWith(TEXT("Iron_42")));

	return true;
}

#endif
