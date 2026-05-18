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

	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float MaxHealth      = 60.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float AttackDamage   = 12.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float AttackRange    = 160.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float AttackCooldown = 1.4f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float SightRange     = 1400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float ChaseSpeed     = 440.f;
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") float PatrolSpeed    = 180.f;

	// If true, this enemy's attacks can be parried by Johnny. Bosses and
	// charging-strike enemies may want this false.
	UPROPERTY(EditDefaultsOnly, Category = "Enemy") bool bAttacksAreParryable = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Health = 60.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bDead = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bKnockedDown = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bStaggered   = false;

	UFUNCTION() virtual void OnSeePlayer(AJohnnyCharacter* Player);

	// Johnny's heavy attacks call these to apply impact effects.
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ApplyKnockback(FVector Impulse, bool bKnockdown, float KnockdownDuration);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Stagger(float Duration);

	// Eligible for Johnny's finisher? Knocked-down OR below HP threshold.
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsFinishable(float HpFraction) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool IsKnockedDown() const { return bKnockedDown; }

protected:
	virtual void OnDeath();
	virtual void TryAttack();

	void EndKnockdown();
	void EndStagger();

	TWeakObjectPtr<AJohnnyCharacter> Target;
	float TimeSinceLastAttack = 999.f;

	FTimerHandle KnockdownTimer;
	FTimerHandle StaggerTimer;
};
