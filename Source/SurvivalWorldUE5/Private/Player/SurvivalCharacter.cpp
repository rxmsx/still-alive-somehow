#include "Player/SurvivalCharacter.h"
#include "Building/BuildingComponent.h"
#include "Crafting/CraftingComponent.h"
#include "EnhancedInputComponent.h"
#include "Interfaces/Interactable.h"
#include "Items/InventoryComponent.h"
#include "Player/SurvivalPlayerController.h"
#include "Survival/SurvivalStatsComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

ASurvivalCharacter::ASurvivalCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 74.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("Crafting"));
	BuildingComponent = CreateDefaultSubobject<UBuildingComponent>(TEXT("Building"));
	SurvivalStatsComponent = CreateDefaultSubobject<USurvivalStatsComponent>(TEXT("SurvivalStats"));

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ASurvivalCharacter::BeginPlay()
{
	Super::BeginPlay();
	AddStarterItems();
	RefreshMovementSpeed();
}

void ASurvivalCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsGameplayInputBlocked())
	{
		bWantsToSprint = false;
	}

	if (SurvivalStatsComponent)
	{
		const bool bCanSprint = bWantsToSprint && SurvivalStatsComponent->Stamina > 1.0f;
		SurvivalStatsComponent->TickSurvival(DeltaSeconds, bCanSprint);
		if (bWantsToSprint && !bCanSprint)
		{
			bWantsToSprint = false;
		}
	}

	RefreshMovementSpeed();
}

void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::Move);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::Look);
		}
		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ASurvivalCharacter::StartJump);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASurvivalCharacter::StopJump);
		}
		if (SprintAction)
		{
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &ASurvivalCharacter::StartSprint);
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASurvivalCharacter::StopSprint);
		}
		if (InteractAction)
		{
			EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ASurvivalCharacter::HandleInteractInput);
		}
	}

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASurvivalCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASurvivalCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASurvivalCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASurvivalCharacter::LookUp);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ASurvivalCharacter::StartJump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ASurvivalCharacter::StopJump);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ASurvivalCharacter::StartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ASurvivalCharacter::StopSprint);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ASurvivalCharacter::HandleInteractInput);
	PlayerInputComponent->BindAction(TEXT("BuildMode"), IE_Pressed, this, &ASurvivalCharacter::HandleBuildModeInput);
	PlayerInputComponent->BindAction(TEXT("PlaceBuildPart"), IE_Pressed, this, &ASurvivalCharacter::HandleBuildConfirmInput);
	PlayerInputComponent->BindAction(TEXT("RotateBuildPreview"), IE_Pressed, this, &ASurvivalCharacter::HandleBuildRotateInput);
	PlayerInputComponent->BindAction(TEXT("NextBuildPart"), IE_Pressed, this, &ASurvivalCharacter::HandleBuildNextInput);
	PlayerInputComponent->BindAction(TEXT("PreviousBuildPart"), IE_Pressed, this, &ASurvivalCharacter::HandleBuildPreviousInput);
}

bool ASurvivalCharacter::UseInteract()
{
	AActor* HitActor = nullptr;
	FHitResult HitResult;
	if (!GetFocusedInteractable(HitActor, HitResult))
	{
		return false;
	}

	if (!IInteractable::Execute_CanInteract(HitActor, this))
	{
		SetInteractionFeedback(IInteractable::Execute_GetInteractionPrompt(HitActor, this));
		return false;
	}

	const bool bInteracted = IInteractable::Execute_Interact(HitActor, this);
	if (!bInteracted)
	{
		SetInteractionFeedback(NSLOCTEXT("SurvivalWorld", "InteractionFailed", "Aktion nicht moeglich."));
	}
	return bInteracted;
}

bool ASurvivalCharacter::GetFocusedInteractable(AActor*& OutInteractableActor, FHitResult& OutHitResult) const
{
	OutInteractableActor = nullptr;
	if (!FirstPersonCamera || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = FirstPersonCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + FirstPersonCamera->GetForwardVector() * InteractionRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalInteractTrace), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	AActor* HitActor = OutHitResult.GetActor();
	if (!HitActor || !HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return false;
	}

	OutInteractableActor = HitActor;
	return true;
}

FText ASurvivalCharacter::GetCurrentInteractionPrompt(bool& bCanInteract) const
{
	bCanInteract = false;
	AActor* HitActor = nullptr;
	FHitResult HitResult;
	if (!GetFocusedInteractable(HitActor, HitResult))
	{
		return FText::GetEmpty();
	}

	bCanInteract = IInteractable::Execute_CanInteract(HitActor, this);
	return IInteractable::Execute_GetInteractionPrompt(HitActor, this);
}

void ASurvivalCharacter::SetInteractionFeedback(const FText& Message, float DurationSeconds)
{
	InteractionFeedbackMessage = Message;
	InteractionFeedbackExpiresAt = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, DurationSeconds) : 0.0f;
}

FText ASurvivalCharacter::GetInteractionFeedback() const
{
	if (!GetWorld() || InteractionFeedbackMessage.IsEmpty() || GetWorld()->GetTimeSeconds() > InteractionFeedbackExpiresAt)
	{
		return FText::GetEmpty();
	}
	return InteractionFeedbackMessage;
}

void ASurvivalCharacter::Move(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	MoveForward(MovementVector.Y);
	MoveRight(MovementVector.X);
}

void ASurvivalCharacter::Look(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void ASurvivalCharacter::StartSprint()
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	bWantsToSprint = true;
	RefreshMovementSpeed();
}

void ASurvivalCharacter::StopSprint()
{
	bWantsToSprint = false;
	RefreshMovementSpeed();
}

void ASurvivalCharacter::StartJump()
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	Jump();
}

void ASurvivalCharacter::StopJump()
{
	StopJumping();
}

void ASurvivalCharacter::HandleInteractInput()
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	if (BuildingComponent && BuildingComponent->IsBuildModeActive())
	{
		HandleBuildConfirmInput();
		return;
	}

	UseInteract();
}

void ASurvivalCharacter::HandleBuildModeInput()
{
	if (IsGameplayInputBlocked() || !BuildingComponent)
	{
		return;
	}

	BuildingComponent->ToggleBuildMode();
}

void ASurvivalCharacter::HandleBuildConfirmInput()
{
	if (IsGameplayInputBlocked() || !BuildingComponent || !BuildingComponent->IsBuildModeActive())
	{
		return;
	}

	if (!BuildingComponent->ConfirmPlacement())
	{
		SetInteractionFeedback(BuildingComponent->GetPlacementMessage());
	}
}

void ASurvivalCharacter::HandleBuildRotateInput()
{
	if (IsGameplayInputBlocked() || !BuildingComponent || !BuildingComponent->IsBuildModeActive())
	{
		return;
	}

	BuildingComponent->RotatePreview(1.0f);
}

void ASurvivalCharacter::HandleBuildNextInput()
{
	if (IsGameplayInputBlocked() || !BuildingComponent || !BuildingComponent->IsBuildModeActive())
	{
		return;
	}

	BuildingComponent->CycleBuildPart(1);
}

void ASurvivalCharacter::HandleBuildPreviousInput()
{
	if (IsGameplayInputBlocked() || !BuildingComponent || !BuildingComponent->IsBuildModeActive())
	{
		return;
	}

	BuildingComponent->CycleBuildPart(-1);
}

void ASurvivalCharacter::MoveForward(float Value)
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ASurvivalCharacter::MoveRight(float Value)
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ASurvivalCharacter::Turn(float Value)
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	AddControllerYawInput(Value);
}

void ASurvivalCharacter::LookUp(float Value)
{
	if (IsGameplayInputBlocked())
	{
		return;
	}

	AddControllerPitchInput(Value);
}

void ASurvivalCharacter::AddStarterItems()
{
	if (!InventoryComponent || !HasAuthority())
	{
		return;
	}

	InventoryComponent->AddItem(FName(TEXT("Axe")), 1);
	InventoryComponent->AddItem(FName(TEXT("Pickaxe")), 1);
	InventoryComponent->AddItem(FName(TEXT("Wood")), 3);
	InventoryComponent->AddItem(FName(TEXT("Stone")), 6);
	InventoryComponent->AddItem(FName(TEXT("Stick")), 4);
	InventoryComponent->AddItem(FName(TEXT("PlantFiber")), 8);
	InventoryComponent->AddItem(FName(TEXT("Rope")), 2);
	InventoryComponent->AddItem(FName(TEXT("Cloth")), 3);
	InventoryComponent->AddItem(FName(TEXT("RawMeat")), 2);
	InventoryComponent->AddItem(FName(TEXT("DirtyWater")), 2);
	InventoryComponent->AddItem(FName(TEXT("Alcohol")), 1);
	InventoryComponent->AddItem(FName(TEXT("Hide")), 2);
}

void ASurvivalCharacter::RefreshMovementSpeed()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	const bool bCanSprint = bWantsToSprint && (!SurvivalStatsComponent || SurvivalStatsComponent->Stamina > 1.0f);
	GetCharacterMovement()->MaxWalkSpeed = bCanSprint ? SprintSpeed : WalkSpeed;
}

bool ASurvivalCharacter::IsGameplayInputBlocked() const
{
	const ASurvivalPlayerController* SurvivalController = Cast<ASurvivalPlayerController>(GetController());
	return SurvivalController && SurvivalController->IsGameplayInputBlocked();
}

void ASurvivalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASurvivalCharacter, bWantsToSprint);
}
