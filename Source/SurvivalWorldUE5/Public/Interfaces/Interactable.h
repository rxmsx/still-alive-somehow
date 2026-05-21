#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(BlueprintType)
class SURVIVALWORLDUE5_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

class SURVIVALWORLDUE5_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt(const AActor* InteractingActor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(const AActor* InteractingActor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool Interact(AActor* InteractingActor);
};
