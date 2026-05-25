#include "Survival/SurvivalGameMode.h"
#include "Items/ItemPickupActor.h"
#include "Player/SurvivalCharacter.h"
#include "Player/SurvivalPlayerController.h"
#include "Resources/ResourceNodeActor.h"
#include "Resources/ResourceNodeComponent.h"
#include "UI/SurvivalHUD.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	FResourceLootEntry TestLoot(FName ItemId, int32 MinCount, int32 MaxCount, float Chance = 1.0f)
	{
		FResourceLootEntry Entry;
		Entry.ItemId = ItemId;
		Entry.MinCount = MinCount;
		Entry.MaxCount = MaxCount;
		Entry.Chance = Chance;
		return Entry;
	}

	FResourceNodeDef TestResourceDef(FName NodeId)
	{
		FResourceNodeDef Def;
		Def.ResourceNodeId = NodeId;
		Def.BaseHarvestDamage = 1.0f;
		Def.ToolDurabilityCost = 1.0f;

		if (NodeId == TEXT("Rock"))
		{
			Def.DisplayName = NSLOCTEXT("SurvivalWorld", "MilestoneRock", "Felsen");
			Def.OutputItemId = TEXT("Stone");
			Def.RequiredToolType = ESurvivalToolType::Pickaxe;
			Def.bAllowBareHands = false;
			Def.MaxHealth = 32.0f;
			Def.MaxHarvests = 4;
			Def.Loot = { TestLoot(TEXT("Stone"), 1, 3), TestLoot(TEXT("Flint"), 0, 1, 0.45f) };
		}
		else if (NodeId == TEXT("Bush"))
		{
			Def.DisplayName = NSLOCTEXT("SurvivalWorld", "MilestoneBush", "Busch");
			Def.OutputItemId = TEXT("PlantFiber");
			Def.RequiredToolType = ESurvivalToolType::Hand;
			Def.AlternativeToolTypes = { ESurvivalToolType::Knife };
			Def.bAllowBareHands = true;
			Def.MaxHealth = 8.0f;
			Def.MaxHarvests = 2;
			Def.Loot = { TestLoot(TEXT("PlantFiber"), 1, 3), TestLoot(TEXT("Berries"), 0, 2, 0.35f), TestLoot(TEXT("Herbs"), 0, 1, 0.25f) };
		}
		else
		{
			Def.ResourceNodeId = TEXT("Tree");
			Def.DisplayName = NSLOCTEXT("SurvivalWorld", "MilestoneTree", "Baum");
			Def.OutputItemId = TEXT("Wood");
			Def.RequiredToolType = ESurvivalToolType::Axe;
			Def.bAllowBareHands = true;
			Def.WrongToolDamageMultiplier = 0.10f;
			Def.MaxHealth = 44.0f;
			Def.MaxHarvests = 5;
			Def.Loot = { TestLoot(TEXT("Wood"), 1, 3), TestLoot(TEXT("Stick"), 1, 2, 0.70f), TestLoot(TEXT("Bark"), 0, 1, 0.55f) };
		}

		return Def;
	}
}

ASurvivalGameMode::ASurvivalGameMode()
{
	DefaultPawnClass = ASurvivalCharacter::StaticClass();
	PlayerControllerClass = ASurvivalPlayerController::StaticClass();
	HUDClass = ASurvivalHUD::StaticClass();
}

void ASurvivalGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!bSpawnMilestoneTestContent || !GetWorld())
	{
		return;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	const FVector Origin = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
	const FRotator Rotation = PlayerPawn ? PlayerPawn->GetActorRotation() : FRotator::ZeroRotator;
	const FVector Forward = Rotation.Vector();
	const FVector Right = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);

	auto SpawnPickup = [this, Origin, Forward, Right](FName ItemId, int32 Count, FVector Offset)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AItemPickupActor* Pickup = GetWorld()->SpawnActor<AItemPickupActor>(AItemPickupActor::StaticClass(), Origin + Forward * Offset.X + Right * Offset.Y + FVector(0.0f, 0.0f, Offset.Z), FRotator::ZeroRotator, SpawnParams);
		if (Pickup)
		{
			FInventoryStack Stack;
			Stack.ItemId = ItemId;
			Stack.Count = Count;
			Stack.Freshness = 1.0f;
			Pickup->InitializePickup(Stack, nullptr);
		}
	};

	SpawnPickup(TEXT("Stick"), 2, FVector(240.0f, -120.0f, 45.0f));
	SpawnPickup(TEXT("Stone"), 2, FVector(280.0f, 20.0f, 45.0f));
	SpawnPickup(TEXT("Wood"), 1, FVector(220.0f, 150.0f, 45.0f));

	auto SpawnResourceNode = [this, Origin, Forward, Right](FName NodeId, FVector Offset, FVector Scale)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AResourceNodeActor* Node = GetWorld()->SpawnActor<AResourceNodeActor>(AResourceNodeActor::StaticClass(), Origin + Forward * Offset.X + Right * Offset.Y + FVector(0.0f, 0.0f, Offset.Z), FRotator::ZeroRotator, SpawnParams);
		if (Node && Node->ResourceNodeComponent)
		{
			Node->ResourceNodeComponent->ConfigureFromDefinition(TestResourceDef(NodeId));
			if (Node->MeshComponent)
			{
				Node->MeshComponent->SetRelativeScale3D(Scale);
			}
		}
	};

	SpawnResourceNode(TEXT("Tree"), FVector(620.0f, -180.0f, 65.0f), FVector(0.85f, 0.85f, 3.6f));
	SpawnResourceNode(TEXT("Rock"), FVector(560.0f, 130.0f, 45.0f), FVector(1.45f, 1.10f, 0.70f));
	SpawnResourceNode(TEXT("Bush"), FVector(420.0f, 260.0f, 35.0f), FVector(0.95f, 0.95f, 0.55f));
}
