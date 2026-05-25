#include "Player/SurvivalPlayerController.h"
#include "Building/BuildingComponent.h"
#include "Crafting/CraftingComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Items/InventoryComponent.h"
#include "Resources/ResourceNodeActor.h"
#include "Resources/ResourceNodeComponent.h"
#include "Save/SurvivalSaveSubsystem.h"

namespace
{
	FResourceLootEntry Loot(FName ItemId, int32 MinCount, int32 MaxCount, float Chance = 1.0f, bool bOnlyWhenDepleted = false)
	{
		FResourceLootEntry Entry;
		Entry.ItemId = ItemId;
		Entry.MinCount = MinCount;
		Entry.MaxCount = MaxCount;
		Entry.Chance = Chance;
		Entry.bOnlyWhenDepleted = bOnlyWhenDepleted;
		return Entry;
	}

	FResourceNodeDef MakeDebugResourceDef(FName ResourceNodeId)
	{
		FResourceNodeDef Def;
		Def.ResourceNodeId = ResourceNodeId.IsNone() ? FName(TEXT("Tree")) : ResourceNodeId;
		Def.AmountPerHarvest = 1;
		Def.MaxHarvests = 5;
		Def.BaseHarvestDamage = 1.0f;
		Def.ToolDurabilityCost = 1.0f;
		Def.WrongToolDamageMultiplier = 0.15f;

		if (Def.ResourceNodeId == TEXT("Rock"))
		{
			Def.DisplayName = NSLOCTEXT("SurvivalWorld", "DebugRockNode", "Felsen");
			Def.OutputItemId = TEXT("Stone");
			Def.RequiredToolType = ESurvivalToolType::Pickaxe;
			Def.bAllowBareHands = false;
			Def.MaxHealth = 32.0f;
			Def.Loot = { Loot(TEXT("Stone"), 1, 3), Loot(TEXT("Flint"), 0, 1, 0.45f) };
		}
		else if (Def.ResourceNodeId == TEXT("Bush"))
		{
			Def.DisplayName = NSLOCTEXT("SurvivalWorld", "DebugBushNode", "Busch");
			Def.OutputItemId = TEXT("PlantFiber");
			Def.RequiredToolType = ESurvivalToolType::Hand;
			Def.AlternativeToolTypes = { ESurvivalToolType::Knife };
			Def.bAllowBareHands = true;
			Def.MaxHealth = 8.0f;
			Def.Loot = { Loot(TEXT("PlantFiber"), 1, 3), Loot(TEXT("Berries"), 0, 2, 0.35f), Loot(TEXT("Herbs"), 0, 1, 0.25f) };
		}
		else
		{
			Def.ResourceNodeId = TEXT("Tree");
			Def.DisplayName = NSLOCTEXT("SurvivalWorld", "DebugTreeNode", "Baum");
			Def.OutputItemId = TEXT("Wood");
			Def.RequiredToolType = ESurvivalToolType::Axe;
			Def.bAllowBareHands = true;
			Def.MaxHealth = 44.0f;
			Def.Loot = { Loot(TEXT("Wood"), 1, 3), Loot(TEXT("Stick"), 1, 2, 0.70f), Loot(TEXT("Bark"), 0, 1, 0.55f) };
		}

		return Def;
	}
}

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &ASurvivalPlayerController::ToggleInventory);
		InputComponent->BindAction(TEXT("ToggleMap"), IE_Pressed, this, &ASurvivalPlayerController::ToggleMap);
		InputComponent->BindAction(TEXT("CloseUI"), IE_Pressed, this, &ASurvivalPlayerController::CloseOpenUI);
	}
}

void ASurvivalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, MappingPriority);
		}
	}
}

void ASurvivalPlayerController::ToggleInventory()
{
	if (IsInventoryOpen())
	{
		CloseOpenUI();
	}
	else
	{
		SetUIState(ESurvivalUIState::Inventory);
	}
}

void ASurvivalPlayerController::ToggleMap()
{
	SetUIState(IsMapOpen() ? ESurvivalUIState::None : ESurvivalUIState::Map);
}

void ASurvivalPlayerController::CloseOpenUI()
{
	SetUIState(ESurvivalUIState::None);
	ClearWorldInteractionContext();
}

void ASurvivalPlayerController::OpenWorldInventory(AActor* InventoryOwner, UInventoryComponent* WorldInventory, ECraftingStationType StationType)
{
	OpenedWorldInventoryOwner = InventoryOwner;
	OpenedWorldInventory = WorldInventory;
	OpenedCraftingStation = StationType;
	SetUIState(ESurvivalUIState::Inventory);

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UCraftingComponent* Crafting = ControlledPawn->FindComponentByClass<UCraftingComponent>())
		{
			Crafting->SetActiveCraftingStation(StationType);
		}
	}
}

void ASurvivalPlayerController::OpenCraftingStation(ECraftingStationType StationType, AActor* StationActor)
{
	OpenedWorldInventoryOwner = StationActor;
	OpenedWorldInventory = nullptr;
	OpenedCraftingStation = StationType;
	SetUIState(ESurvivalUIState::Inventory);

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UCraftingComponent* Crafting = ControlledPawn->FindComponentByClass<UCraftingComponent>())
		{
			Crafting->SetActiveCraftingStation(StationType);
		}
	}
}

void ASurvivalPlayerController::GiveItem(FName ItemId, int32 Count)
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UInventoryComponent* Inventory = ControlledPawn->FindComponentByClass<UInventoryComponent>())
		{
			Inventory->AddItem(ItemId, Count);
		}
	}
}

void ASurvivalPlayerController::SpawnResource(FName ResourceNodeId)
{
	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	if (!World || !ControlledPawn)
	{
		return;
	}

	const FVector SpawnLocation = ControlledPawn->GetActorLocation() + ControlledPawn->GetActorForwardVector() * 420.0f;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = ControlledPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AResourceNodeActor* ResourceNode = World->SpawnActor<AResourceNodeActor>(AResourceNodeActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (ResourceNode && ResourceNode->ResourceNodeComponent)
	{
		ResourceNode->ResourceNodeComponent->ConfigureFromDefinition(MakeDebugResourceDef(ResourceNodeId));
	}
}

void ASurvivalPlayerController::SaveSurvival()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USurvivalSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USurvivalSaveSubsystem>())
		{
			SaveSubsystem->SaveCurrentWorldToDefaultSlot();
		}
	}
}

void ASurvivalPlayerController::LoadSurvival()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USurvivalSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USurvivalSaveSubsystem>())
		{
			SaveSubsystem->LoadCurrentWorldFromDefaultSlot();
		}
	}
}

void ASurvivalPlayerController::SelectBuildPart(FName PartId)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UBuildingComponent* Building = ControlledPawn->FindComponentByClass<UBuildingComponent>())
		{
			Building->SelectBuildPart(PartId);
		}
	}
}

void ASurvivalPlayerController::SetUIState(ESurvivalUIState NewUIState)
{
	UIState = NewUIState;
	bShowMouseCursor = UIState != ESurvivalUIState::None;

	if (UIState != ESurvivalUIState::None)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void ASurvivalPlayerController::ClearWorldInteractionContext()
{
	OpenedWorldInventory = nullptr;
	OpenedWorldInventoryOwner = nullptr;
	OpenedCraftingStation = ECraftingStationType::None;

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UCraftingComponent* Crafting = ControlledPawn->FindComponentByClass<UCraftingComponent>())
		{
			Crafting->SetActiveCraftingStation(ECraftingStationType::None);
		}
	}
}
