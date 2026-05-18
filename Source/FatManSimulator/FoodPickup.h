#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FoodPickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EFoodType : uint8
{
	Kebapi,
	Burger,
	Gyro,
	Burek,
	Baklava,
	Candy
};

UCLASS()
class FATMANSIMULATOR_API AFoodPickup : public AActor
{
	GENERATED_BODY()

public:
	AFoodPickup();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food") EFoodType FoodType = EFoodType::Kebapi;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food") float HungerRestore = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food") float HealthRestore = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food") bool bRequireInteractKey = false;

	// If true (default), pickup is stashed in Johnny's throwable hotbar instead of
	// being eaten on contact. Falls back to eating when his hunger is below
	// EmergencyEatThreshold or when the hotbar is full.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food") bool bGoesToHotbar = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food") float EmergencyEatHungerFraction = 0.25f;

	UPROPERTY(VisibleAnywhere) USphereComponent* Trigger = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Mesh = nullptr;

protected:
	UFUNCTION() void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other,
	                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                                bool bFromSweep, const FHitResult& SweepResult);
};
