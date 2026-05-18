#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoodPickup.h"
#include "ThrownFood.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class FATMANSIMULATOR_API AThrownFood : public AActor
{
	GENERATED_BODY()

public:
	AThrownFood();

	// Per-food tuning. Designer can override in BP_ThrownFood.
	UPROPERTY(EditDefaultsOnly, Category = "Thrown Food") float GreasyImpactDamage = 4.f;
	UPROPERTY(EditDefaultsOnly, Category = "Thrown Food") float GreasySlipDuration = 1.6f;
	UPROPERTY(EditDefaultsOnly, Category = "Thrown Food") float SweetImpactDamage  = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "Thrown Food") float Lifespan           = 5.f;

	UPROPERTY(VisibleAnywhere) USphereComponent* Collision = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Mesh  = nullptr;
	UPROPERTY(VisibleAnywhere) UProjectileMovementComponent* Movement = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Thrown Food")
	void Launch(FVector Velocity, EFoodType InFoodType);

	UPROPERTY(BlueprintReadOnly) EFoodType FoodType = EFoodType::Kebapi;

protected:
	UFUNCTION() void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	                       UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool IsGreasy() const;
};
