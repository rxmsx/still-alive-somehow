#include "Player/SurvivalCharacter.h"
#include "Crafting/CraftingComponent.h"
#include "EnhancedInputComponent.h"
#include "Interfaces/Interactable.h"
#include "Items/InventoryComponent.h"
#include "Player/SurvivalPlayerController.h"
#include "Survival/BodyConditionComponent.h"
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
	SurvivalStatsComponent = CreateDefaultSubobject<USurvivalStatsComponent>(TEXT("SurvivalStats"));
	BodyConditionComponent = CreateDefaultSubobject<UBodyConditionComponent>(TEXT("BodyCondition"));

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
}

bool ASurvivalCharacter::UseInteract()
{
	if (!FirstPersonCamera)
	{
		return false;
	}

	const FVector TraceStart = FirstPersonCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + FirstPersonCamera->GetForwardVector() * InteractionRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalInteractTrace), false, this);
	FHitResult HitResult;
	if (!GetWorld() || !GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || !HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return false;
	}

	if (!IInteractable::Execute_CanInteract(HitActor, this))
	{
		return false;
	}

	return IInteractable::Execute_Interact(HitActor, this);
}

bool ASurvivalCharacter::ConsumeInventoryItem(FName ItemId)
{
	if (!InventoryComponent || !CraftingComponent || !SurvivalStatsComponent || ItemId.IsNone())
	{
		return false;
	}

	FItemDef Item;
	if (!CraftingComponent->GetItemDefinition(ItemId, Item) || !Item.bIsEdible)
	{
		return false;
	}

	if (!InventoryComponent->RemoveItem(ItemId, 1))
	{
		return false;
	}

	SurvivalStatsComponent->ApplyNutrition(static_cast<float>(Item.NutritionValue), static_cast<float>(Item.HydrationValue));
	return true;
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

	UseInteract();
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
