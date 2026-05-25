#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivalCharacter.generated.h"

class UCameraComponent;
class UBuildingComponent;
class UCraftingComponent;
class UInputAction;
class UInputMappingContext;
class UInventoryComponent;
class USurvivalStatsComponent;
struct FInputActionValue;

UCLASS(BlueprintType, Blueprintable)
class SURVIVALWORLDUE5_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASurvivalCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UBuildingComponent> BuildingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
	TObjectPtr<USurvivalStatsComponent> SurvivalStatsComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "50.0"))
	float InteractionRange = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0"))
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0"))
	float SprintSpeed = 750.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool UseInteract();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool GetFocusedInteractable(AActor*& OutInteractableActor, FHitResult& OutHitResult) const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetCurrentInteractionPrompt(bool& bCanInteract) const;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionFeedback(const FText& Message, float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetInteractionFeedback() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	bool bWantsToSprint = false;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void StartJump();
	void StopJump();
	void HandleInteractInput();
	void HandleBuildModeInput();
	void HandleBuildConfirmInput();
	void HandleBuildRotateInput();
	void HandleBuildNextInput();
	void HandleBuildPreviousInput();

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void AddStarterItems();
	void RefreshMovementSpeed();
	bool IsGameplayInputBlocked() const;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText InteractionFeedbackMessage;

	float InteractionFeedbackExpiresAt = 0.0f;
};
