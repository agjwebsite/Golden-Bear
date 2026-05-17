#include "FatManSimulatorPlayerController.h"

AFatManSimulatorPlayerController::AFatManSimulatorPlayerController()
{
	bShowMouseCursor = false;
}

void AFatManSimulatorPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
}
