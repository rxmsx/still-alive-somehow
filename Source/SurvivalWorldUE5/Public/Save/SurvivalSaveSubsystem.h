#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SurvivalSaveSubsystem.generated.h"

class USurvivalSaveGame;

UCLASS(BlueprintType)
class SURVIVALWORLDUE5_API USurvivalSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveCurrentWorld(const FString& SlotName, int32 UserIndex);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadCurrentWorld(const FString& SlotName, int32 UserIndex);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveCurrentWorldToDefaultSlot();

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadCurrentWorldFromDefaultSlot();

	UFUNCTION(BlueprintPure, Category = "Save")
	USurvivalSaveGame* GetLastLoadedSave() const { return LastLoadedSave.Get(); }

private:
	UPROPERTY()
	TObjectPtr<USurvivalSaveGame> LastLoadedSave;
};
