#include "Player/SurvivalPlayerController.h"
#include "EnhancedInputSubsystems.h"

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
	SetUIState(IsInventoryOpen() ? ESurvivalUIState::None : ESurvivalUIState::Inventory);
}

void ASurvivalPlayerController::ToggleMap()
{
	SetUIState(IsMapOpen() ? ESurvivalUIState::None : ESurvivalUIState::Map);
}

void ASurvivalPlayerController::CloseOpenUI()
{
	SetUIState(ESurvivalUIState::None);
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
