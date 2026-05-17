#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FatManSimulatorPlayerController.generated.h"

UCLASS()
class FATMANSIMULATOR_API AFatManSimulatorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFatManSimulatorPlayerController();

protected:
	virtual void BeginPlay() override;
};
