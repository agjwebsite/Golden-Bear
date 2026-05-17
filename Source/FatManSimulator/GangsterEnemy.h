#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "GangsterEnemy.generated.h"

UCLASS()
class FATMANSIMULATOR_API AGangsterEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	AGangsterEnemy();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnSeePlayer(AJohnnyCharacter* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "Gangster") float TauntInterval = 4.f;
	UPROPERTY(EditDefaultsOnly, Category = "Gangster") bool  bCanGiveUp    = false;
	UPROPERTY(EditDefaultsOnly, Category = "Gangster") float GiveUpDistance = 4000.f;

protected:
	float TimeSinceLastTaunt = 0.f;
};
