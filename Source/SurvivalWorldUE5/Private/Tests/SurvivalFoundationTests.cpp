#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Crafting/CraftingComponent.h"
#include "GameFramework/Actor.h"
#include "Items/InventoryComponent.h"
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

	TestEqual(TEXT("Fallback recipes are available"), Crafting->GetKnownRecipes().Num(), 2);
	TestFalse(TEXT("Stone blade cannot craft without stone"), Crafting->CanCraft(TEXT("StoneBlade")));

	Inventory->AddItem(TEXT("Stone"), 2);
	TestTrue(TEXT("Stone blade becomes craftable with two stone"), Crafting->CanCraft(TEXT("StoneBlade")));
	TestTrue(TEXT("Crafting stone blade succeeds"), Crafting->CraftRecipe(TEXT("StoneBlade")));
	TestEqual(TEXT("Stone is consumed"), Inventory->GetItemCount(TEXT("Stone")), 0);
	TestEqual(TEXT("Stone blade is added"), Inventory->GetItemCount(TEXT("StoneBlade")), 1);

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
