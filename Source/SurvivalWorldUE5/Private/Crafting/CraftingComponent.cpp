#include "Crafting/CraftingComponent.h"

#include "World/OpenWorldPrototypeSettings.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogSurvivalCrafting, Log, All);

namespace
{
	FText CraftingFailureText(ECraftingFailureReason Reason)
	{
		switch (Reason)
		{
		case ECraftingFailureReason::UnknownRecipe:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureUnknownRecipe", "Rezept nicht gefunden.");
		case ECraftingFailureReason::RecipeLocked:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureRecipeLocked", "Rezept ist noch unbekannt.");
		case ECraftingFailureReason::MissingMaterials:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureMissingMaterials", "Material fehlt oder liegt nicht passend auf der Werkflaeche.");
		case ECraftingFailureReason::WrongStation:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureWrongStation", "Falsche Werkbank oder Feuerstelle.");
		case ECraftingFailureReason::InventoryFull:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureInventoryFull", "Inventar ist voll oder zu schwer.");
		case ECraftingFailureReason::InvalidInput:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureInvalidInput", "Ungueltige Crafting-Eingabe.");
		case ECraftingFailureReason::AlreadyCrafting:
			return NSLOCTEXT("SurvivalWorld", "CraftingFailureAlreadyCrafting", "Crafting laeuft bereits.");
		default:
			return FText::GetEmpty();
		}
	}

	bool RecipeMatchesCategory(const FCraftingRecipe& Recipe, ECraftingRecipeCategory Category)
	{
		return Category == ECraftingRecipeCategory::Unknown || Recipe.Category == Category;
	}
}

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ItemCatalog)
	{
		if (const UOpenWorldPrototypeSettings* Settings = GetDefault<UOpenWorldPrototypeSettings>())
		{
			ItemCatalog = Settings->ItemCatalog.LoadSynchronous();
		}
	}

	EnsureCraftingSlots();
}

void UCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveRecipeId.IsNone())
	{
		if (IsRegistered())
		{
			SetComponentTickEnabled(false);
		}
		return;
	}

	ActiveRecipeElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (ActiveRecipeElapsedSeconds >= ActiveRecipeTotalSeconds)
	{
		FinishActiveRecipe();
	}
	else
	{
		OnCraftingChanged.Broadcast();
	}
}

void UCraftingComponent::SetItemCatalog(USurvivalItemCatalog* NewItemCatalog)
{
	ItemCatalog = NewItemCatalog;
	OnCraftingChanged.Broadcast();
}

TArray<FCraftingRecipe> UCraftingComponent::GetKnownRecipes() const
{
	TArray<FCraftingRecipe> Recipes;
	for (const FCraftingRecipe& Recipe : GetAllRecipes())
	{
		if (IsRecipeKnown(Recipe.RecipeId))
		{
			Recipes.Add(Recipe);
		}
	}
	return Recipes;
}

TArray<FCraftingRecipe> UCraftingComponent::GetVisibleRecipes(bool bIncludeLocked) const
{
	TArray<FCraftingRecipe> Recipes;
	for (const FCraftingRecipe& Recipe : GetAllRecipes())
	{
		if (IsRecipeKnown(Recipe.RecipeId) || (bIncludeLocked && Recipe.bShowWhenLocked))
		{
			Recipes.Add(Recipe);
		}
	}
	return Recipes;
}

TArray<FCraftingRecipe> UCraftingComponent::GetRecipesFiltered(const FString& SearchText, ECraftingRecipeCategory Category, bool bIncludeLocked) const
{
	TArray<FCraftingRecipe> Recipes;
	const FString NormalizedSearch = SearchText.TrimStartAndEnd().ToLower();
	for (const FCraftingRecipe& Recipe : GetVisibleRecipes(bIncludeLocked))
	{
		if (!RecipeMatchesCategory(Recipe, Category))
		{
			continue;
		}

		if (!NormalizedSearch.IsEmpty())
		{
			const FString Haystack = FString::Printf(TEXT("%s %s %s"), *Recipe.RecipeId.ToString(), *Recipe.DisplayName.ToString(), *Recipe.Description.ToString()).ToLower();
			if (!Haystack.Contains(NormalizedSearch))
			{
				continue;
			}
		}

		Recipes.Add(Recipe);
	}
	return Recipes;
}

TArray<FCraftingRecipe> UCraftingComponent::GetCraftableRecipes() const
{
	TArray<FCraftingRecipe> CraftableRecipes;
	for (const FCraftingRecipe& Recipe : GetKnownRecipes())
	{
		if (CanCraft(Recipe.RecipeId))
		{
			CraftableRecipes.Add(Recipe);
		}
	}
	return CraftableRecipes;
}

bool UCraftingComponent::CanCraft(FName RecipeId) const
{
	return ValidateCraftRecipe(RecipeId).bCanCraft;
}

FCraftingValidationResult UCraftingComponent::ValidateCraftRecipe(FName RecipeId) const
{
	FCraftingValidationResult Result;
	const FCraftingRecipe* Recipe = FindRecipe(RecipeId);
	if (!Recipe || Recipe->OutputItemId.IsNone() || Recipe->OutputCount <= 0)
	{
		Result.FailureReason = ECraftingFailureReason::UnknownRecipe;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	if (!IsRecipeKnown(RecipeId))
	{
		Result.FailureReason = ECraftingFailureReason::RecipeLocked;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	if (!ActiveRecipeId.IsNone())
	{
		Result.FailureReason = ECraftingFailureReason::AlreadyCrafting;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	if (Recipe->RequiredStation != ECraftingStationType::None && Recipe->RequiredStation != ActiveCraftingStation)
	{
		Result.FailureReason = ECraftingFailureReason::WrongStation;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	const UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		Result.FailureReason = ECraftingFailureReason::InvalidInput;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	const bool bUseSurface = HasAnyCraftingInput();
	const bool bHasIngredients = bUseSurface ? HasRecipeIngredientsOnSurface(*Recipe) : HasRecipeIngredientsInInventory(*Recipe);
	if (!bHasIngredients)
	{
		Result.FailureReason = ECraftingFailureReason::MissingMaterials;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	if (!CanInventoryAcceptRecipeOutput(*Recipe, bUseSurface))
	{
		Result.FailureReason = ECraftingFailureReason::InventoryFull;
		Result.Message = CraftingFailureText(Result.FailureReason);
		return Result;
	}

	Result.bCanCraft = true;
	Result.FailureReason = ECraftingFailureReason::None;
	return Result;
}

bool UCraftingComponent::CraftRecipe(FName RecipeId)
{
	const FCraftingValidationResult Validation = ValidateCraftRecipe(RecipeId);
	if (!Validation.bCanCraft)
	{
		FailCrafting(RecipeId, Validation.FailureReason, Validation.Message);
		return false;
	}

	const FCraftingRecipe* Recipe = FindRecipe(RecipeId);
	if (!Recipe)
	{
		FailCrafting(RecipeId, ECraftingFailureReason::UnknownRecipe, CraftingFailureText(ECraftingFailureReason::UnknownRecipe));
		return false;
	}

	const bool bConsumed = HasAnyCraftingInput() ? ConsumeRecipeIngredientsFromSurface(*Recipe) : ConsumeRecipeIngredientsFromInventory(*Recipe);
	if (!bConsumed)
	{
		FailCrafting(RecipeId, ECraftingFailureReason::MissingMaterials, CraftingFailureText(ECraftingFailureReason::MissingMaterials));
		return false;
	}

	ActiveRecipeId = RecipeId;
	ActiveRecipeElapsedSeconds = 0.0f;
	ActiveRecipeTotalSeconds = FMath::Max(0.0f, Recipe->CraftTimeSeconds);
	SetCraftingMessage(FText::GetEmpty(), false);

	if (ActiveRecipeTotalSeconds <= 0.0f)
	{
		FinishActiveRecipe();
	}
	else
	{
		if (IsRegistered())
		{
			SetComponentTickEnabled(true);
		}
		OnCraftingChanged.Broadcast();
	}

	return true;
}

bool UCraftingComponent::UnlockRecipe(FName RecipeId)
{
	if (!FindRecipe(RecipeId) || IsRecipeKnown(RecipeId))
	{
		return false;
	}

	UnlockedRecipeIds.Add(RecipeId);
	OnCraftingChanged.Broadcast();
	return true;
}

bool UCraftingComponent::IsRecipeKnown(FName RecipeId) const
{
	const FCraftingRecipe* Recipe = FindRecipe(RecipeId);
	return Recipe && (Recipe->bUnlockedByDefault || UnlockedRecipeIds.Contains(RecipeId));
}

void UCraftingComponent::SetActiveCraftingStation(ECraftingStationType NewStation)
{
	ActiveCraftingStation = NewStation;
	SetCraftingMessage(FText::GetEmpty(), false);
	OnCraftingChanged.Broadcast();
}

bool UCraftingComponent::AddInventorySlotToCrafting(int32 InventorySlotIndex, int32 Count)
{
	UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		return false;
	}

	EnsureCraftingSlots();
	const FInventoryStack SourceSlot = Inventory->GetSlot(InventorySlotIndex);
	if (SourceSlot.IsEmpty())
	{
		return false;
	}

	const int32 MoveCount = Count <= 0 ? SourceSlot.Count : FMath::Min(SourceSlot.Count, Count);
	TArray<FInventoryStack> CandidateCraftingSlots = CraftingInputSlots;
	if (!AddItemToCraftingSlots(SourceSlot.ItemId, MoveCount, SourceSlot.Durability, SourceSlot.Freshness, CandidateCraftingSlots))
	{
		return false;
	}

	if (!Inventory->RemoveFromSlot(InventorySlotIndex, MoveCount))
	{
		return false;
	}

	CraftingInputSlots = CandidateCraftingSlots;
	EnsureCraftingSlots();
	OnCraftingChanged.Broadcast();
	return true;
}

bool UCraftingComponent::RemoveCraftingInput(int32 CraftingSlotIndex)
{
	if (!CraftingInputSlots.IsValidIndex(CraftingSlotIndex) || CraftingInputSlots[CraftingSlotIndex].IsEmpty())
	{
		return false;
	}

	UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		return false;
	}

	const FInventoryStack Slot = CraftingInputSlots[CraftingSlotIndex];
	if (!Inventory->AddItemWithState(Slot.ItemId, Slot.Count, Slot.Durability, Slot.Freshness))
	{
		return false;
	}

	CraftingInputSlots[CraftingSlotIndex] = FInventoryStack();
	EnsureCraftingSlots();
	OnCraftingChanged.Broadcast();
	return true;
}

void UCraftingComponent::ClearCraftingInputs()
{
	for (int32 SlotIndex = CraftingInputSlots.Num() - 1; SlotIndex >= 0; --SlotIndex)
	{
		RemoveCraftingInput(SlotIndex);
	}
}

TMap<FName, int32> UCraftingComponent::GetCraftingInputCounts() const
{
	TMap<FName, int32> Counts;
	for (const FInventoryStack& Slot : CraftingInputSlots)
	{
		if (!Slot.IsEmpty())
		{
			Counts.FindOrAdd(Slot.ItemId) += Slot.Count;
		}
	}
	return Counts;
}

bool UCraftingComponent::FindMatchingRecipeFromInputs(FCraftingRecipe& OutRecipe) const
{
	for (const FCraftingRecipe& Recipe : GetKnownRecipes())
	{
		if (HasRecipeIngredientsOnSurface(Recipe) && (Recipe.RequiredStation == ECraftingStationType::None || Recipe.RequiredStation == ActiveCraftingStation))
		{
			OutRecipe = Recipe;
			return true;
		}
	}
	return false;
}

float UCraftingComponent::GetCraftingProgress() const
{
	if (ActiveRecipeId.IsNone() || ActiveRecipeTotalSeconds <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(ActiveRecipeElapsedSeconds / ActiveRecipeTotalSeconds, 0.0f, 1.0f);
}

FText UCraftingComponent::GetItemDisplayName(FName ItemId) const
{
	if (const FItemDef* Item = ResolveItemDefinition(ItemId))
	{
		if (!Item->DisplayName.IsEmpty())
		{
			return Item->DisplayName;
		}
	}

	return FText::FromName(ItemId);
}

ESurvivalItemCategory UCraftingComponent::GetItemCategory(FName ItemId) const
{
	if (const FItemDef* Item = ResolveItemDefinition(ItemId))
	{
		return Item->Category;
	}

	return ESurvivalItemCategory::Misc;
}

bool UCraftingComponent::GetItemDefinition(FName ItemId, FItemDef& OutItem) const
{
	if (const FItemDef* Item = ResolveItemDefinition(ItemId))
	{
		OutItem = *Item;
		return true;
	}

	return false;
}

void UCraftingComponent::OnRep_CraftingState()
{
	EnsureCraftingSlots();
	OnCraftingChanged.Broadcast();
}

const FCraftingRecipe* UCraftingComponent::FindRecipe(FName RecipeId) const
{
	if (ItemCatalog)
	{
		if (const FCraftingRecipe* Recipe = ItemCatalog->FindRecipe(RecipeId))
		{
			return Recipe;
		}
	}

	return USurvivalItemCatalog::FindDefaultRecipe(RecipeId);
}

UInventoryComponent* UCraftingComponent::GetOwnerInventory() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

TArray<FCraftingRecipe> UCraftingComponent::GetAllRecipes() const
{
	if (ItemCatalog && ItemCatalog->Recipes.Num() > 0)
	{
		return ItemCatalog->Recipes;
	}

	return USurvivalItemCatalog::GetDefaultRecipes();
}

bool UCraftingComponent::HasRecipeIngredientsInInventory(const FCraftingRecipe& Recipe) const
{
	const UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		return false;
	}

	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Ingredient.ItemId.IsNone() || Ingredient.Count <= 0 || !Inventory->HasItem(Ingredient.ItemId, Ingredient.Count))
		{
			return false;
		}
	}

	return true;
}

bool UCraftingComponent::HasRecipeIngredientsOnSurface(const FCraftingRecipe& Recipe) const
{
	const TMap<FName, int32> InputCounts = GetCraftingInputCounts();
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Ingredient.ItemId.IsNone() || Ingredient.Count <= 0 || InputCounts.FindRef(Ingredient.ItemId) < Ingredient.Count)
		{
			return false;
		}
	}

	return true;
}

bool UCraftingComponent::ConsumeRecipeIngredientsFromInventory(const FCraftingRecipe& Recipe)
{
	UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory || !HasRecipeIngredientsInInventory(Recipe))
	{
		return false;
	}

	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		if (!Inventory->RemoveItem(Ingredient.ItemId, Ingredient.Count))
		{
			return false;
		}
	}

	return true;
}

bool UCraftingComponent::ConsumeRecipeIngredientsFromSurface(const FCraftingRecipe& Recipe)
{
	if (!HasRecipeIngredientsOnSurface(Recipe))
	{
		return false;
	}

	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 Remaining = Ingredient.Count;
		for (FInventoryStack& Slot : CraftingInputSlots)
		{
			if (Remaining <= 0)
			{
				break;
			}

			if (Slot.ItemId != Ingredient.ItemId || Slot.Count <= 0)
			{
				continue;
			}

			const int32 Consumed = FMath::Min(Slot.Count, Remaining);
			Slot.Count -= Consumed;
			Remaining -= Consumed;
			if (Slot.Count <= 0)
			{
				Slot = FInventoryStack();
			}
		}
	}

	EnsureCraftingSlots();
	return true;
}

bool UCraftingComponent::ConsumeRecipeIngredientsFromSlots(const FCraftingRecipe& Recipe, TArray<FInventoryStack>& Slots) const
{
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 Available = 0;
		for (const FInventoryStack& Slot : Slots)
		{
			if (Slot.ItemId == Ingredient.ItemId)
			{
				Available += Slot.Count;
			}
		}

		if (Available < Ingredient.Count)
		{
			return false;
		}
	}

	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 Remaining = Ingredient.Count;
		for (FInventoryStack& Slot : Slots)
		{
			if (Remaining <= 0)
			{
				break;
			}

			if (Slot.ItemId != Ingredient.ItemId || Slot.Count <= 0)
			{
				continue;
			}

			const int32 Consumed = FMath::Min(Slot.Count, Remaining);
			Slot.Count -= Consumed;
			Remaining -= Consumed;
			if (Slot.Count <= 0)
			{
				Slot = FInventoryStack();
			}
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		Slots[SlotIndex].SlotIndex = SlotIndex;
	}
	return true;
}

bool UCraftingComponent::CanInventoryAcceptRecipeOutput(const FCraftingRecipe& Recipe, bool bUseSurface) const
{
	const UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Inventory)
	{
		return false;
	}

	if (bUseSurface)
	{
		return Inventory->CanAddItem(Recipe.OutputItemId, Recipe.OutputCount);
	}

	TArray<FInventoryStack> ProjectedSlots = Inventory->GetSlots();
	if (!ConsumeRecipeIngredientsFromSlots(Recipe, ProjectedSlots))
	{
		return false;
	}

	return Inventory->CanAddItemToSlotSnapshot(Recipe.OutputItemId, Recipe.OutputCount, ProjectedSlots);
}

bool UCraftingComponent::HasAnyCraftingInput() const
{
	for (const FInventoryStack& Slot : CraftingInputSlots)
	{
		if (!Slot.IsEmpty())
		{
			return true;
		}
	}
	return false;
}

bool UCraftingComponent::AddItemToCraftingSlots(FName ItemId, int32 Count, float Durability, float Freshness, TArray<FInventoryStack>& Slots) const
{
	if (ItemId.IsNone() || Count <= 0)
	{
		return false;
	}

	const FItemDef* ItemDef = ResolveItemDefinition(ItemId);
	const int32 MaxStack = ItemDef ? ItemDef->GetEffectiveMaxStack() : 99;
	int32 Remaining = Count;
	for (FInventoryStack& Slot : Slots)
	{
		if (Slot.ItemId == ItemId && Slot.Count > 0 && Slot.Count < MaxStack)
		{
			const int32 Added = FMath::Min(MaxStack - Slot.Count, Remaining);
			Slot.Count += Added;
			Slot.Durability = Durability;
			Slot.Freshness = FMath::Clamp(Freshness, 0.0f, 1.0f);
			Remaining -= Added;
			if (Remaining <= 0)
			{
				return true;
			}
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && Remaining > 0; ++SlotIndex)
	{
		if (!Slots[SlotIndex].IsEmpty())
		{
			continue;
		}

		const int32 Added = FMath::Min(MaxStack, Remaining);
		FInventoryStack NewSlot;
		NewSlot.ItemId = ItemId;
		NewSlot.Count = Added;
		NewSlot.Durability = Durability;
		NewSlot.Freshness = FMath::Clamp(Freshness, 0.0f, 1.0f);
		NewSlot.SlotIndex = SlotIndex;
		Slots[SlotIndex] = NewSlot;
		Remaining -= Added;
	}

	return Remaining <= 0;
}

void UCraftingComponent::EnsureCraftingSlots()
{
	CraftingInputSlots.SetNum(FMath::Max(1, CraftingInputSlotCount));
	for (int32 SlotIndex = 0; SlotIndex < CraftingInputSlots.Num(); ++SlotIndex)
	{
		if (CraftingInputSlots[SlotIndex].IsEmpty())
		{
			CraftingInputSlots[SlotIndex] = FInventoryStack();
		}
		CraftingInputSlots[SlotIndex].SlotIndex = SlotIndex;
	}
}

void UCraftingComponent::FinishActiveRecipe()
{
	const FCraftingRecipe* Recipe = FindRecipe(ActiveRecipeId);
	UInventoryComponent* Inventory = GetOwnerInventory();
	if (!Recipe || !Inventory || !Inventory->AddItem(Recipe->OutputItemId, Recipe->OutputCount))
	{
		FailCrafting(ActiveRecipeId, ECraftingFailureReason::InventoryFull, CraftingFailureText(ECraftingFailureReason::InventoryFull));
		ActiveRecipeId = NAME_None;
		ActiveRecipeElapsedSeconds = 0.0f;
		ActiveRecipeTotalSeconds = 0.0f;
		if (IsRegistered())
		{
			SetComponentTickEnabled(false);
		}
		OnCraftingChanged.Broadcast();
		return;
	}

	const FName FinishedRecipeId = ActiveRecipeId;
	const int32 OutputCount = Recipe->OutputCount;
	ActiveRecipeId = NAME_None;
	ActiveRecipeElapsedSeconds = 0.0f;
	ActiveRecipeTotalSeconds = 0.0f;
	if (IsRegistered())
	{
		SetComponentTickEnabled(false);
	}
	OnRecipeCrafted.Broadcast(FinishedRecipeId, OutputCount);
	SetCraftingMessage(FText::Format(NSLOCTEXT("SurvivalWorld", "CraftingSuccessMessage", "Crafted {0} x{1}"), Recipe->DisplayName.IsEmpty() ? FText::FromName(FinishedRecipeId) : Recipe->DisplayName, OutputCount), false);
	OnCraftingChanged.Broadcast();
}

void UCraftingComponent::SetCraftingMessage(const FText& Message, bool bIsError)
{
	LastCraftingMessage = Message;
	bLastCraftingMessageIsError = bIsError;
}

void UCraftingComponent::FailCrafting(FName RecipeId, ECraftingFailureReason Reason, const FText& Message)
{
	const FText FailureMessage = Message.IsEmpty() ? CraftingFailureText(Reason) : Message;
	SetCraftingMessage(FailureMessage, true);
	UE_LOG(LogSurvivalCrafting, Warning, TEXT("Crafting failed for %s: %s"), *RecipeId.ToString(), *FailureMessage.ToString());
	OnCraftingFailed.Broadcast(RecipeId, FailureMessage);
	OnCraftingChanged.Broadcast();
}

const FItemDef* UCraftingComponent::ResolveItemDefinition(FName ItemId) const
{
	if (ItemCatalog)
	{
		if (const FItemDef* Item = ItemCatalog->FindItem(ItemId))
		{
			return Item;
		}
	}

	return USurvivalItemCatalog::FindDefaultItem(ItemId);
}

void UCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCraftingComponent, CraftingInputSlots);
	DOREPLIFETIME(UCraftingComponent, UnlockedRecipeIds);
	DOREPLIFETIME(UCraftingComponent, ActiveCraftingStation);
	DOREPLIFETIME(UCraftingComponent, ActiveRecipeId);
	DOREPLIFETIME(UCraftingComponent, ActiveRecipeElapsedSeconds);
	DOREPLIFETIME(UCraftingComponent, ActiveRecipeTotalSeconds);
	DOREPLIFETIME(UCraftingComponent, LastCraftingMessage);
	DOREPLIFETIME(UCraftingComponent, bLastCraftingMessageIsError);
}
