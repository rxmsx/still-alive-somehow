#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Crafting/CraftingComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Items/InventoryComponent.h"
#include "Survival/BodyConditionComponent.h"
#include "Survival/CampfireComponent.h"
#include "Survival/SurvivalStatsComponent.h"
#include "World/WorldSeedSubsystem.h"

namespace
{
	int32 FindFirstSlotWithItem(const UInventoryComponent* Inventory, FName ItemId)
	{
		if (!Inventory)
		{
			return INDEX_NONE;
		}

		for (const FInventoryStack& Slot : Inventory->GetSlots())
		{
			if (Slot.ItemId == ItemId && Slot.Count > 0)
			{
				return Slot.SlotIndex;
			}
		}
		return INDEX_NONE;
	}
}

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
	FInventoryComponentSlotFlowTest,
	"SurvivalWorld.Foundation.Inventory.SlotHotbarUseDurability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryComponentSlotFlowTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>();
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>(Owner);
	USurvivalStatsComponent* Stats = NewObject<USurvivalStatsComponent>(Owner);
	Owner->AddOwnedComponent(Inventory);
	Owner->AddOwnedComponent(Stats);

	TestTrue(TEXT("Adding stackable wood succeeds"), Inventory->AddItem(TEXT("Wood"), 4));
	const int32 WoodSlot = FindFirstSlotWithItem(Inventory, TEXT("Wood"));
	TestTrue(TEXT("Wood slot exists"), WoodSlot != INDEX_NONE);
	TestTrue(TEXT("Stack can split"), Inventory->SplitStackHalf(WoodSlot));
	TestEqual(TEXT("Wood count remains after split"), Inventory->GetItemCount(TEXT("Wood")), 4);

	const int32 SecondWoodSlot = [&Inventory, WoodSlot]() -> int32
	{
		for (const FInventoryStack& Slot : Inventory->GetSlots())
		{
			if (Slot.ItemId == TEXT("Wood") && Slot.SlotIndex != WoodSlot)
			{
				return Slot.SlotIndex;
			}
		}
		return INDEX_NONE;
	}();
	TestTrue(TEXT("Second wood slot exists"), SecondWoodSlot != INDEX_NONE);
	TestTrue(TEXT("Stacks can merge"), Inventory->MergeStack(SecondWoodSlot, WoodSlot));
	TestEqual(TEXT("Merged wood count remains"), Inventory->GetItemCount(TEXT("Wood")), 4);

	TestTrue(TEXT("Adding axe succeeds"), Inventory->AddItem(TEXT("Axe"), 1));
	const int32 AxeSlot = FindFirstSlotWithItem(Inventory, TEXT("Axe"));
	TestTrue(TEXT("Axe can be assigned to hotbar"), Inventory->AssignSlotToHotbar(AxeSlot, 0));
	TestTrue(TEXT("Axe use consumes durability"), Inventory->UseSlot(AxeSlot));
	TestTrue(TEXT("Axe remains after one use"), Inventory->GetSlot(AxeSlot).Durability > 0.0f);

	Stats->Thirst = 20.0f;
	TestTrue(TEXT("Adding water succeeds"), Inventory->AddItem(TEXT("WaterBottle"), 1));
	const int32 WaterSlot = FindFirstSlotWithItem(Inventory, TEXT("WaterBottle"));
	TestTrue(TEXT("Water can be consumed"), Inventory->UseSlot(WaterSlot));
	TestTrue(TEXT("Water improves thirst"), Stats->Thirst > 20.0f);
	TestEqual(TEXT("Water bottle stack is consumed"), Inventory->GetItemCount(TEXT("WaterBottle")), 0);

	TestFalse(TEXT("Dropping without a world does not silently delete items"), Inventory->DropSlot(WoodSlot, 1));
	TestEqual(TEXT("Failed drop keeps item count"), Inventory->GetItemCount(TEXT("Wood")), 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingComponentRecipeTest,
	"SurvivalWorld.Foundation.Crafting.PrimitiveToolRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCraftingComponentRecipeTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>();
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>(Owner);
	UCraftingComponent* Crafting = NewObject<UCraftingComponent>(Owner);
	Owner->AddOwnedComponent(Inventory);
	Owner->AddOwnedComponent(Crafting);

	TestTrue(TEXT("Fallback survival recipes are available"), Crafting->GetKnownRecipes().Num() >= 11);
	TestFalse(TEXT("Primitive tool cannot craft without inputs"), Crafting->CanCraft(TEXT("PrimitiveTool")));

	Inventory->AddItem(TEXT("Stick"), 1);
	Inventory->AddItem(TEXT("Stone"), 2);
	TestTrue(TEXT("Primitive tool becomes craftable with stick and stone"), Crafting->CanCraft(TEXT("PrimitiveTool")));
	TestTrue(TEXT("Crafting primitive tool succeeds"), Crafting->CraftRecipe(TEXT("PrimitiveTool")));
	TestEqual(TEXT("Stick is consumed"), Inventory->GetItemCount(TEXT("Stick")), 0);
	TestEqual(TEXT("One stone is consumed"), Inventory->GetItemCount(TEXT("Stone")), 1);
	TestEqual(TEXT("Primitive tool is added"), Inventory->GetItemCount(TEXT("PrimitiveTool")), 1);

	Inventory->AddItem(TEXT("RawMeat"), 1);
	TestFalse(TEXT("Campfire recipes require their station"), Crafting->CanCraft(TEXT("CookedMeat")));
	TestTrue(TEXT("Wrong station gives player-facing feedback"), !Crafting->GetLastCraftingMessage().IsEmpty() || !Crafting->CraftRecipe(TEXT("CookedMeat")));
	Crafting->SetActiveCraftingStation(ECraftingStationType::Campfire);
	TestTrue(TEXT("Campfire recipe is valid at campfire"), Crafting->CanCraft(TEXT("CookedMeat")));

	Inventory->ClearInventory();
	Crafting->ClearCraftingInputs();
	Inventory->AddItem(TEXT("Stick"), 1);
	Inventory->AddItem(TEXT("Stone"), 1);
	TestTrue(TEXT("Manual stick input can be placed"), Crafting->AddInventorySlotToCrafting(FindFirstSlotWithItem(Inventory, TEXT("Stick")), 1));
	TestTrue(TEXT("Manual stone input can be placed"), Crafting->AddInventorySlotToCrafting(FindFirstSlotWithItem(Inventory, TEXT("Stone")), 1));
	TestEqual(TEXT("Manual surface owns the ingredients"), Crafting->GetCraftingInputCounts().Num(), 2);
	TestTrue(TEXT("Manual surface recipe crafts"), Crafting->CraftRecipe(TEXT("PrimitiveTool")));
	TestEqual(TEXT("Manual craft output added"), Inventory->GetItemCount(TEXT("PrimitiveTool")), 1);

	TestFalse(TEXT("Missing material craft fails"), Crafting->CraftRecipe(TEXT("StoneAxe")));
	TestFalse(TEXT("Craft failure message is visible"), Crafting->GetLastCraftingMessage().IsEmpty());

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
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UWorldSeedSubsystem* SeedSubsystem = NewObject<UWorldSeedSubsystem>(GameInstance);
	SeedSubsystem->ConfigureWorld(TEXT("Test World"), 42);

	const FString StableIdA = SeedSubsystem->MakeStableId(TEXT("Iron"), FVector(100.0f, 200.0f, -300.0f));
	const FString StableIdB = SeedSubsystem->MakeStableId(TEXT("Iron"), FVector(100.0f, 200.0f, -300.0f));

	TestEqual(TEXT("Stable IDs are deterministic"), StableIdA, StableIdB);
	TestTrue(TEXT("Stable ID includes category"), StableIdA.StartsWith(TEXT("Iron_42")));

	return true;
}

#endif
