#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class AJohnnyCharacter;

UCLASS()
class FATMANSIMULATOR_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	                         AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float MaxHealth     = 60.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float AttackDamage  = 12.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float AttackRange   = 160.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float AttackCooldown = 1.4f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float SightRange    = 1400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float ChaseSpeed    = 440.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float PatrolSpeed   = 180.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Health = 60.f;

	UFUNCTION() virtual void OnSeePlayer(AJohnnyCharacter* Player);

protected:
	virtual void OnDeath();
	virtual void TryAttack();

	TWeakObjectPtr<AJohnnyCharacter> Target;
	float TimeSinceLastAttack = 999.f;
	bool bDead = false;
};
