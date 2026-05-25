#include "Survival/SurvivalGameMode.h"
#include "Player/SurvivalCharacter.h"
#include "Player/SurvivalPlayerController.h"
#include "UI/SurvivalHUD.h"

ASurvivalGameMode::ASurvivalGameMode()
{
	DefaultPawnClass = ASurvivalCharacter::StaticClass();
	PlayerControllerClass = ASurvivalPlayerController::StaticClass();
	HUDClass = ASurvivalHUD::StaticClass();
}
