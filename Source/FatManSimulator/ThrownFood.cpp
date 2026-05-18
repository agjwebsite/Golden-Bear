#include "ThrownFood.h"
#include "EnemyBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/DamageEvents.h"

AThrownFood::AThrownFood()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(18.f);
	Collision->SetCollisionProfileName(TEXT("PhysicsActor"));
	Collision->SetNotifyRigidBodyCollision(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 0.f;       // Launch() sets this
	Movement->MaxSpeed = 4000.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = true;
	Movement->Bounciness = 0.3f;
	Movement->ProjectileGravityScale = 0.7f;

	Collision->OnComponentHit.AddDynamic(this, &AThrownFood::OnHit);

	InitialLifeSpan = Lifespan;
}

void AThrownFood::Launch(FVector Velocity, EFoodType InFoodType)
{
	FoodType = InFoodType;
	if (Movement)
	{
		Movement->Velocity = Velocity;
	}
}

bool AThrownFood::IsGreasy() const
{
	switch (FoodType)
	{
		case EFoodType::Kebapi:
		case EFoodType::Burger:
		case EFoodType::Gyro:
		case EFoodType::Burek:
			return true;
		default:
			return false;
	}
}

void AThrownFood::OnHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult&)
{
	if (!OtherActor || OtherActor == GetInstigator() || OtherActor == this) return;

	AEnemyBase* Enemy = Cast<AEnemyBase>(OtherActor);
	if (Enemy)
	{
		const float Damage = IsGreasy() ? GreasyImpactDamage : SweetImpactDamage;
		FDamageEvent DamageEvent;
		Enemy->TakeDamage(Damage, DamageEvent, GetInstigatorController(), this);

		if (IsGreasy())
		{
			// Greasy food makes them slip — stagger is the closest existing verb.
			Enemy->Stagger(GreasySlipDuration);
		}
	}

	Destroy();
}
