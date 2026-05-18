#include "FoodPickup.h"
#include "JohnnyCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AFoodPickup::AFoodPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(80.f);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = Trigger;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AFoodPickup::OnBeginOverlap);
}

void AFoodPickup::OnBeginOverlap(UPrimitiveComponent*, AActor* Other,
                                 UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (bRequireInteractKey) return;
	AJohnnyCharacter* Johnny = Cast<AJohnnyCharacter>(Other);
	if (!Johnny) return;

	const bool bIsStarving = Johnny->MaxHunger > 0.f &&
	                         Johnny->Hunger / Johnny->MaxHunger <= EmergencyEatHungerFraction;

	bool bHandled = false;
	if (bGoesToHotbar && !bIsStarving)
	{
		bHandled = Johnny->AddFoodToHotbar(FoodType, 1);
	}
	if (!bHandled)
	{
		Johnny->EatFood(HungerRestore, HealthRestore);
	}
	Destroy();
}
