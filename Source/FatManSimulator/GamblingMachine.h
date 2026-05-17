#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GamblingMachine.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AJohnnyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamblingResult, int32, Winnings);

UCLASS()
class FATMANSIMULATOR_API AGamblingMachine : public AActor
{
	GENERATED_BODY()

public:
	AGamblingMachine();

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Mesh = nullptr;
	UPROPERTY(VisibleAnywhere) UBoxComponent* InteractVolume = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Gambling") int32 RiggedFirstPullPayout = 1000000;
	UPROPERTY(EditDefaultsOnly, Category = "Gambling") int32 CostPerPull = 1;

	UPROPERTY(BlueprintAssignable) FOnGamblingResult OnPullResolved;

	UFUNCTION(BlueprintCallable, Category = "Gambling")
	int32 Pull(AJohnnyCharacter* Johnny);

protected:
	bool bFirstPullDone = false;
};
