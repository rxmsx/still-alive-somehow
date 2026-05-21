#include "Survival/SurvivalGameMode.h"
#include "Player/SurvivalCharacter.h"
#include "Player/SurvivalPlayerController.h"

ASurvivalGameMode::ASurvivalGameMode()
{
	DefaultPawnClass = ASurvivalCharacter::StaticClass();
	PlayerControllerClass = ASurvivalPlayerController::StaticClass();
}
