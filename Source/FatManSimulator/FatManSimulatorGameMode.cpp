#include "FatManSimulatorGameMode.h"
#include "JohnnyCharacter.h"
#include "FatManSimulatorPlayerController.h"

AFatManSimulatorGameMode::AFatManSimulatorGameMode()
{
	DefaultPawnClass      = AJohnnyCharacter::StaticClass();
	PlayerControllerClass = AFatManSimulatorPlayerController::StaticClass();
}
