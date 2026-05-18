#include "EnemyBase.h"
#include "JohnnyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "AIController.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI    = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead || bKnockedDown || bStaggered) return;

	TimeSinceLastAttack += DeltaSeconds;

	if (!Target.IsValid())
	{
		if (AJohnnyCharacter* Player = Cast<AJohnnyCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			if (FVector::Dist(Player->GetActorLocation(), GetActorLocation()) <= SightRange)
			{
				OnSeePlayer(Player);
			}
		}
		return;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI) return;

	const float Dist = FVector::Dist(Target->GetActorLocation(), GetActorLocation());
	if (Dist > AttackRange * 0.85f)
	{
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		AI->MoveToActor(Target.Get(), AttackRange * 0.7f, true, true, false);
	}
	else
	{
		TryAttack();
	}
}

void AEnemyBase::OnSeePlayer(AJohnnyCharacter* Player)
{
	Target = Player;
}

void AEnemyBase::TryAttack()
{
	if (TimeSinceLastAttack < AttackCooldown || !Target.IsValid()) return;
	TimeSinceLastAttack = 0.f;

	// Give Johnny a chance to parry this. If consumed, the parry will stagger us
	// and trigger a Riposte — so we eat the cost and skip damage.
	if (bAttacksAreParryable && Target->TryConsumeParry(this))
	{
		return;
	}

	FDamageEvent DamageEvent;
	Target->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                             AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead) return 0.f;
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);

	if (!Target.IsValid())
	{
		if (AJohnnyCharacter* J = Cast<AJohnnyCharacter>(DamageCauser)) Target = J;
	}

	if (Health <= 0.f) OnDeath();
	return Applied;
}

void AEnemyBase::ApplyKnockback(FVector Impulse, bool bKnockdown, float KnockdownDuration)
{
	if (bDead) return;
	LaunchCharacter(Impulse, true, true);
	if (bKnockdown && KnockdownDuration > 0.f)
	{
		bKnockedDown = true;
		bStaggered = false;
		GetCharacterMovement()->StopMovementImmediately();
		GetWorldTimerManager().SetTimer(KnockdownTimer, this, &AEnemyBase::EndKnockdown, KnockdownDuration, false);
	}
}

void AEnemyBase::EndKnockdown()
{
	bKnockedDown = false;
}

void AEnemyBase::Stagger(float Duration)
{
	if (bDead || bKnockedDown) return;
	bStaggered = true;
	GetCharacterMovement()->StopMovementImmediately();
	GetWorldTimerManager().SetTimer(StaggerTimer, this, &AEnemyBase::EndStagger, Duration, false);
}

void AEnemyBase::EndStagger()
{
	bStaggered = false;
}

bool AEnemyBase::IsFinishable(float HpFraction) const
{
	if (bDead) return false;
	if (bKnockedDown) return true;
	if (MaxHealth > 0.f && Health / MaxHealth <= HpFraction) return true;
	return false;
}

void AEnemyBase::OnDeath()
{
	bDead = true;
	bKnockedDown = false;
	bStaggered = false;
	GetWorldTimerManager().ClearAllTimersForObject(this);
	GetCharacterMovement()->DisableMovement();
	SetLifeSpan(4.f);
}
