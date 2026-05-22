#include "Player/SurvivalPlayerController.h"
#include "EnhancedInputSubsystems.h"

void ASurvivalPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &ASurvivalPlayerController::ToggleInventory);
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
	SetInventoryOpen(!bInventoryOpen);
}

void ASurvivalPlayerController::SetInventoryOpen(bool bNewInventoryOpen)
{
	bInventoryOpen = bNewInventoryOpen;
	bShowMouseCursor = bInventoryOpen;

	if (bInventoryOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}
