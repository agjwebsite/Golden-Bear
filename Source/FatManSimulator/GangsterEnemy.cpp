#include "GangsterEnemy.h"
#include "JohnnyCharacter.h"
#include "JohnnyVoiceComponent.h"

AGangsterEnemy::AGangsterEnemy()
{
	MaxHealth     = 80.f;
	AttackDamage  = 16.f;
	ChaseSpeed    = 500.f;
	SightRange    = 2500.f;
}

void AGangsterEnemy::OnSeePlayer(AJohnnyCharacter* Player)
{
	Super::OnSeePlayer(Player);
	if (Player && Player->Voice) Player->Voice->Say(EJohnnyLine::GangsterTaunt);
}

void AGangsterEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Target.IsValid()) return;

	TimeSinceLastTaunt += DeltaSeconds;
	if (TimeSinceLastTaunt >= TauntInterval)
	{
		TimeSinceLastTaunt = 0.f;
		if (Target->Voice) Target->Voice->Say(EJohnnyLine::GangsterTaunt);
	}

	if (bCanGiveUp)
	{
		const float Dist = FVector::Dist(Target->GetActorLocation(), GetActorLocation());
		if (Dist > GiveUpDistance) Target.Reset();
	}
}
