#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "JohnnyCar.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UCameraComponent;
class USpringArmComponent;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class UParticleSystemComponent;
class UAudioComponent;
class AJohnnyCharacter;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCarBrokeDown);

UCLASS()
class FATMANSIMULATOR_API AJohnnyCar : public APawn
{
	GENERATED_BODY()

public:
	AJohnnyCar();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Components
	UPROPERTY(VisibleAnywhere) UBoxComponent* Body = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Mesh = nullptr;
	UPROPERTY(VisibleAnywhere) USpringArmComponent* CameraBoom = nullptr;
	UPROPERTY(VisibleAnywhere) UCameraComponent* FollowCamera = nullptr;
	UPROPERTY(VisibleAnywhere) UFloatingPawnMovement* Movement = nullptr;
	UPROPERTY(VisibleAnywhere) UParticleSystemComponent* ExhaustSmoke = nullptr;
	UPROPERTY(VisibleAnywhere) UAudioComponent* EngineAudio = nullptr;

	// Driving tunables
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float MaxForwardSpeed  = 1400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float MaxReverseSpeed  =  500.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float Acceleration     = 1200.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float BrakeDeceleration= 2400.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float CoastDeceleration=  600.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float SteerRateDeg     =   60.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Driving") float MinSpeedToSteer  =   50.f;

	// Breakdown choreography
	UPROPERTY(EditDefaultsOnly, Category = "Car|Breakdown") float TimeUntilSputter = 22.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Breakdown") float SputterDuration  = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Breakdown") float SputterCoughInterval = 1.8f;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Breakdown") float SputterSpeedMultiplier = 0.45f;

	// The Johnny pawn the player switches to when the car dies.
	// Place a JohnnyCharacter in the level (hidden if you like), assign here.
	UPROPERTY(EditInstanceOnly, Category = "Car|Handoff") AJohnnyCharacter* DriverOnExit = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Car|Handoff") FVector ExitOffset = FVector(0.f, -180.f, 0.f);

	// Enhanced Input (assigned in BP_JohnnyCar)
	UPROPERTY(EditAnywhere, Category = "Car|Input") UInputMappingContext* DrivingMappingContext = nullptr;
	UPROPERTY(EditAnywhere, Category = "Car|Input") UInputAction* IA_Throttle = nullptr;  // Axis1D
	UPROPERTY(EditAnywhere, Category = "Car|Input") UInputAction* IA_Steer    = nullptr;  // Axis1D
	UPROPERTY(EditAnywhere, Category = "Car|Input") UInputAction* IA_Look     = nullptr;  // Axis2D
	UPROPERTY(EditAnywhere, Category = "Car|Input") UInputAction* IA_Horn     = nullptr;

	UPROPERTY(BlueprintAssignable) FOnCarBrokeDown OnBrokeDown;

	UFUNCTION(BlueprintCallable, Category = "Car") void ForceBreakdown();

protected:
	void OnThrottle(const FInputActionValue& Value);
	void OnSteer(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnHorn(const FInputActionValue& Value);

	void TickDriving(float DeltaSeconds);
	void TickBreakdown(float DeltaSeconds);
	void EnterSputter();
	void FinishBreakdown();
	void HandoffToJohnny();

	float ThrottleInput = 0.f;   // -1..1
	float SteerInput    = 0.f;   // -1..1
	float CurrentSpeed  = 0.f;
	float TimeAlive     = 0.f;
	float SputterTimer  = 0.f;
	float TimeSinceCough = 0.f;

	enum class ECarState : uint8 { Driving, Sputtering, Dead } State = ECarState::Driving;
};
