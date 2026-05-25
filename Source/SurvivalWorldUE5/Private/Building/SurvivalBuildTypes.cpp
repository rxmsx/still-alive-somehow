#include "Building/SurvivalBuildTypes.h"

#include "Building/BuildableActor.h"
#include "Building/CampfireActor.h"
#include "Building/StorageContainerActor.h"

const FSurvivalBuildPartDef* USurvivalBuildCatalog::FindBuildPart(FName PartId) const
{
	return BuildParts.FindByPredicate([PartId](const FSurvivalBuildPartDef& Part)
	{
		return Part.PartId == PartId;
	});
}

const TArray<FSurvivalBuildPartDef>& USurvivalBuildCatalog::GetDefaultBuildParts()
{
	static const TArray<FSurvivalBuildPartDef> DefaultBuildParts = []
	{
		TArray<FSurvivalBuildPartDef> Parts;

		auto Cost = [](FName ItemId, int32 Count)
		{
			FCraftingIngredient Ingredient;
			Ingredient.ItemId = ItemId;
			Ingredient.Count = Count;
			return Ingredient;
		};

		auto AddPart = [&Parts, &Cost](
			FName PartId,
			const FText& DisplayName,
			const FText& Description,
			ESurvivalBuildPartKind Kind,
			FVector MeshScale,
			FVector PlacementBounds,
			TSubclassOf<ABuildableActor> ActorClass,
			std::initializer_list<FCraftingIngredient> Costs,
			bool bRequiresSupport = false,
			std::initializer_list<FName> SnapToPartIds = {},
			ECraftingStationType StationType = ECraftingStationType::None)
		{
			FSurvivalBuildPartDef Part;
			Part.PartId = PartId;
			Part.DisplayName = DisplayName;
			Part.Description = Description;
			Part.Kind = Kind;
			Part.MeshScale = MeshScale;
			Part.PlacementBounds = PlacementBounds;
			Part.BuildActorClass = ActorClass;
			Part.bRequiresSupport = bRequiresSupport;
			Part.CraftingStationType = StationType;
			for (const FCraftingIngredient& Ingredient : Costs)
			{
				Part.Costs.Add(Ingredient);
			}
			for (FName SnapPartId : SnapToPartIds)
			{
				Part.SnapToPartIds.Add(SnapPartId);
			}
			Parts.Add(Part);
		};

		AddPart(
			TEXT("Foundation"),
			NSLOCTEXT("SurvivalWorld", "BuildFoundation", "Fundament"),
			NSLOCTEXT("SurvivalWorld", "BuildFoundationDescription", "Flaches Holzfundament als tragfaehige Basis."),
			ESurvivalBuildPartKind::Foundation,
			FVector(3.0f, 3.0f, 0.18f),
			FVector(155.0f, 155.0f, 18.0f),
			ABuildableActor::StaticClass(),
			{ Cost(TEXT("Wood"), 4), Cost(TEXT("Stone"), 2) });

		AddPart(
			TEXT("Wall"),
			NSLOCTEXT("SurvivalWorld", "BuildWall", "Wand"),
			NSLOCTEXT("SurvivalWorld", "BuildWallDescription", "Gerade Wand, die an Fundamentkanten einrastet."),
			ESurvivalBuildPartKind::Wall,
			FVector(3.0f, 0.18f, 1.85f),
			FVector(155.0f, 14.0f, 92.0f),
			ABuildableActor::StaticClass(),
			{ Cost(TEXT("Wood"), 3), Cost(TEXT("PlantFiber"), 1) },
			true,
			{ TEXT("Foundation") });

		AddPart(
			TEXT("Door"),
			NSLOCTEXT("SurvivalWorld", "BuildDoor", "Tuer"),
			NSLOCTEXT("SurvivalWorld", "BuildDoorDescription", "Primitive Tueroeffnung fuer einfache Huetten."),
			ESurvivalBuildPartKind::Door,
			FVector(2.0f, 0.18f, 1.85f),
			FVector(105.0f, 14.0f, 92.0f),
			ABuildableActor::StaticClass(),
			{ Cost(TEXT("Wood"), 3), Cost(TEXT("PlantFiber"), 2) },
			true,
			{ TEXT("Foundation") });

		AddPart(
			TEXT("Roof"),
			NSLOCTEXT("SurvivalWorld", "BuildRoof", "Dach"),
			NSLOCTEXT("SurvivalWorld", "BuildRoofDescription", "Einfaches Dachteil fuer trockene Innenraeume."),
			ESurvivalBuildPartKind::Roof,
			FVector(3.0f, 3.0f, 0.16f),
			FVector(155.0f, 155.0f, 16.0f),
			ABuildableActor::StaticClass(),
			{ Cost(TEXT("Wood"), 4), Cost(TEXT("PlantFiber"), 2) },
			true,
			{ TEXT("Wall"), TEXT("Door") });

		AddPart(
			TEXT("StorageChest"),
			NSLOCTEXT("SurvivalWorld", "BuildStorageChest", "Lagerkiste"),
			NSLOCTEXT("SurvivalWorld", "BuildStorageChestDescription", "Kleine Kiste mit eigenem Inventar."),
			ESurvivalBuildPartKind::Storage,
			FVector(1.1f, 0.7f, 0.55f),
			FVector(60.0f, 42.0f, 35.0f),
			AStorageContainerActor::StaticClass(),
			{ Cost(TEXT("Wood"), 4), Cost(TEXT("Stone"), 2) });

		AddPart(
			TEXT("Campfire"),
			NSLOCTEXT("SurvivalWorld", "BuildCampfire", "Lagerfeuer"),
			NSLOCTEXT("SurvivalWorld", "BuildCampfireDescription", "Feuerstelle mit Brennstoff- und Kochinventar."),
			ESurvivalBuildPartKind::Campfire,
			FVector(0.9f, 0.9f, 0.22f),
			FVector(52.0f, 52.0f, 20.0f),
			ACampfireActor::StaticClass(),
			{ Cost(TEXT("Wood"), 2), Cost(TEXT("Stone"), 4) });

		AddPart(
			TEXT("Workbench"),
			NSLOCTEXT("SurvivalWorld", "BuildWorkbench", "Werkbank"),
			NSLOCTEXT("SurvivalWorld", "BuildWorkbenchDescription", "Primitive Werkbank fuer stationaeres Crafting."),
			ESurvivalBuildPartKind::Workbench,
			FVector(1.8f, 0.8f, 0.8f),
			FVector(100.0f, 50.0f, 48.0f),
			ABuildableActor::StaticClass(),
			{ Cost(TEXT("Wood"), 4), Cost(TEXT("PlantFiber"), 3) },
			false,
			{},
			ECraftingStationType::Workbench);

		AddPart(
			TEXT("Bedroll"),
			NSLOCTEXT("SurvivalWorld", "BuildBedroll", "Schlafsack"),
			NSLOCTEXT("SurvivalWorld", "BuildBedrollDescription", "Einfacher Schlafplatz aus Stoff und Pflanzenfaser."),
			ESurvivalBuildPartKind::Bedroll,
			FVector(1.8f, 0.75f, 0.12f),
			FVector(98.0f, 42.0f, 12.0f),
			ABuildableActor::StaticClass(),
			{ Cost(TEXT("Cloth"), 2), Cost(TEXT("PlantFiber"), 2) });

		return Parts;
	}();

	return DefaultBuildParts;
}

const FSurvivalBuildPartDef* USurvivalBuildCatalog::FindDefaultBuildPart(FName PartId)
{
	return GetDefaultBuildParts().FindByPredicate([PartId](const FSurvivalBuildPartDef& Part)
	{
		return Part.PartId == PartId;
	});
}
