#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JohnnyCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UJohnnyVoiceComponent;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EJohnnyState : uint8
{
	Walking,
	Sprinting,
	Dodging,
	Attacking,
	Staggered,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeterChanged, float, NewNormalizedValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJohnnyDied);

UCLASS()
class FATMANSIMULATOR_API AJohnnyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AJohnnyCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	                         AController* EventInstigator, AActor* DamageCauser) override;

	// --- Meters (0..Max) ---
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float MaxHealth   = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float MaxStamina  = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float MaxHunger   = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny|Meters") float Health  = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny|Meters") float Stamina = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny|Meters") float Hunger  = 100.f;

	// Rates (per second)
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float HungerDecayPerSecond     = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float StaminaDrainSprintPerSec = 18.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float StaminaRegenPerSec       = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float StarvationDamagePerSec   = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float HealthRegenPerSec        = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float HealthRegenMinHunger     = 40.f;

	// Speeds
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Movement") float WalkSpeed   = 320.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Movement") float SprintSpeed = 520.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Movement") float ExhaustedSpeed = 220.f;

	// Dodge
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Dodge") float DodgeImpulse     = 900.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Dodge") float DodgeDuration    = 0.45f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Dodge") float DodgeCooldown    = 1.2f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Dodge") float DodgeStaminaCost = 25.f;

	// Melee
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Melee") float MeleeDamage     = 18.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Melee") float MeleeRange      = 160.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Melee") float MeleeRadius     = 70.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Melee") float MeleeCooldown   = 0.55f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Melee") float MeleeStaminaCost = 8.f;

	// --- Enhanced Input (assigned in the Johnny Blueprint asset) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputMappingContext* DefaultMappingContext = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* MoveAction   = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* LookAction   = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* JumpAction   = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* SprintAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* DodgeAction  = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* AttackAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* InteractAction = nullptr;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny") USpringArmComponent* CameraBoom = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny") UCameraComponent* FollowCamera   = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny") UJohnnyVoiceComponent* Voice     = nullptr;

	// State
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") EJohnnyState State = EJohnnyState::Walking;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") bool bIsInvulnerable = false;

	// Events for UI
	UPROPERTY(BlueprintAssignable) FOnMeterChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable) FOnMeterChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable) FOnMeterChanged OnHungerChanged;
	UPROPERTY(BlueprintAssignable) FOnJohnnyDied OnDied;

	UFUNCTION(BlueprintCallable, Category = "Johnny") void EatFood(float HungerRestore, float HealthRestore);
	UFUNCTION(BlueprintCallable, Category = "Johnny") void Stagger(float Duration);

protected:
	// Input handlers
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSprintStart(const FInputActionValue& Value);
	void OnSprintStop(const FInputActionValue& Value);
	void OnDodge(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);

	void TickMeters(float DeltaSeconds);
	void UpdateSpeedFromState();
	void PerformMeleeHit();
	void EndDodge();
	void EndStagger();
	void Die();

	bool bSprintInput = false;
	float TimeSinceLastDodge = 999.f;
	float TimeSinceLastAttack = 999.f;
	bool bExhausted = false;

	FTimerHandle DodgeTimer;
	FTimerHandle StaggerTimer;
};
