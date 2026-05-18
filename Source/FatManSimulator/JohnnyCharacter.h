#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FoodPickup.h"
#include "JohnnyCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UJohnnyVoiceComponent;
class AThrownFood;
class AEnemyBase;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EJohnnyState : uint8
{
	Walking,
	Sprinting,
	Parrying,         // inside the parry window
	Attacking,        // mid-light (very short)
	WindingUpHeavy,   // committed to Belly Slam or Haymaker
	HeavyRecovery,    // post-hit recovery on a heavy
	Riposting,        // free counter after a successful parry
	Finishing,        // playing Stomp or Eat
	Staggered,
	Dead
};

UENUM(BlueprintType)
enum class EHeavyAttack : uint8
{
	None,
	BellySlam,
	Haymaker
};

UENUM(BlueprintType)
enum class EFinisherKind : uint8
{
	None,
	Stomp,
	Eat
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeterChanged, float, NewNormalizedValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJohnnyDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHotbarChanged, int32, ActiveSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFinisherPromptChanged, bool, bVisible, EFinisherKind, Kind);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboChanged, int32, ComboCount);

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

	// ----- Meters (0..Max) -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float MaxHealth   = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float MaxStamina  = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float MaxHunger   = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny|Meters") float Health  = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny|Meters") float Stamina = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny|Meters") float Hunger  = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float HungerDecayPerSecond     = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float StaminaDrainSprintPerSec = 18.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float StaminaRegenPerSec       = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float StarvationDamagePerSec   = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float HealthRegenPerSec        = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Meters") float HealthRegenMinHunger     = 40.f;

	// ----- Speeds -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Movement") float WalkSpeed      = 320.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Movement") float SprintSpeed    = 520.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Movement") float ExhaustedSpeed = 220.f;

	// ----- Light attack / 3-hit combo -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float LightDamage       = 14.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float LightHookDamage   = 22.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float LightRange        = 160.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float LightRadius       = 70.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float LightCooldown     = 0.40f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float LightStaminaCost  = 6.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float HookKnockback     = 700.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Light") float ComboResetWindow  = 1.1f; // miss this, combo drops

	// ----- Belly Slam (heavy 1, AOE knockback + knockdown) -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamWindup        = 0.55f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamRecovery      = 0.55f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamDamage        = 30.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamRadius        = 280.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamKnockback     = 1100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamKnockdownTime = 2.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamStaminaCost   = 22.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|BellySlam") float SlamCancelStaminaDiscount = 8.f;

	// ----- Haymaker (heavy 2, single-target stagger) -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerWindup   = 0.70f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerRecovery = 0.70f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerDamage   = 55.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerRange    = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerRadius   = 90.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerStaggerTime = 1.8f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerStaminaCost = 28.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Haymaker") float HaymakerCancelStaminaDiscount = 10.f;

	// ----- Parry -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Parry") float ParryWindow          = 0.35f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Parry") float ParryWhiffCooldown   = 0.45f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Parry") float ParryStaggerDuration = 1.4f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Parry") float RiposteDamageMult    = 1.5f;

	// ----- Throwables -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Throwables") TSubclassOf<AThrownFood> ThrownFoodClass;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Throwables") float ThrowCooldown = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Throwables") float ThrowSpeed    = 1600.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Throwables") int32 HotbarSize    = 5;

	// ----- Finisher -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float FinisherRange       = 220.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float FinisherConeCos     = 0.5f;   // ~60° half-angle
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float FinisherHpFraction  = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float StompDamage         = 250.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float EatHungerRestore    = 35.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float EatHungerThreshold  = 60.f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|Finisher") float FinisherDuration    = 0.9f;

	// ----- Hunger-as-armor -----
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|HungerArmor") float ArmorFedThreshold      = 0.75f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|HungerArmor") float ArmorHungryThreshold   = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|HungerArmor") float ArmorFedDamageMult     = 0.75f; // -25%
	UPROPERTY(EditDefaultsOnly, Category = "Johnny|HungerArmor") float ArmorHungryDamageMult  = 1.5f;  // +50%

	// ----- Enhanced Input (assigned in the Johnny Blueprint asset) -----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputMappingContext* DefaultMappingContext = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* MoveAction        = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* LookAction        = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* JumpAction        = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* SprintAction      = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* AttackAction      = nullptr; // LMB - light
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* ParryAction       = nullptr; // RMB
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* BellySlamAction   = nullptr; // Q
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* HaymakerAction    = nullptr; // F
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* ThrowFoodAction   = nullptr; // G
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* InteractAction    = nullptr; // E (Finisher when prompt visible)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* HotbarSlot1Action = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* HotbarSlot2Action = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* HotbarSlot3Action = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* HotbarSlot4Action = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Johnny|Input") UInputAction* HotbarSlot5Action = nullptr;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny") USpringArmComponent* CameraBoom = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny") UCameraComponent* FollowCamera   = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Johnny") UJohnnyVoiceComponent* Voice     = nullptr;

	// State
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") EJohnnyState State = EJohnnyState::Walking;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") bool bIsInvulnerable = false;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") bool bSuperArmor = false;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") int32 ComboCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") int32 ActiveHotbarSlot = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") bool bFinisherPromptVisible = false;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|State") EFinisherKind FinisherPromptKind = EFinisherKind::None;

	// Throwable inventory: parallel arrays indexed by hotbar slot (0..HotbarSize-1).
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|Throwables") TArray<EFoodType> HotbarFood;
	UPROPERTY(BlueprintReadOnly, Category = "Johnny|Throwables") TArray<int32>     HotbarCount;

	// Events for UI
	UPROPERTY(BlueprintAssignable) FOnMeterChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable) FOnMeterChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable) FOnMeterChanged OnHungerChanged;
	UPROPERTY(BlueprintAssignable) FOnJohnnyDied OnDied;
	UPROPERTY(BlueprintAssignable) FOnHotbarChanged OnHotbarChanged;
	UPROPERTY(BlueprintAssignable) FOnFinisherPromptChanged OnFinisherPromptChanged;
	UPROPERTY(BlueprintAssignable) FOnComboChanged OnComboChanged;

	UFUNCTION(BlueprintCallable, Category = "Johnny") void EatFood(float HungerRestore, float HealthRestore);
	UFUNCTION(BlueprintCallable, Category = "Johnny") void Stagger(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Johnny|Throwables")
	bool AddFoodToHotbar(EFoodType Food, int32 Count = 1);

	// Returns true if the attack got eaten by an active parry. Enemies must call this
	// before applying damage so the parry can convert the hit into a Riposte setup.
	UFUNCTION(BlueprintCallable, Category = "Johnny|Parry")
	bool TryConsumeParry(AActor* Attacker);

protected:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSprintStart(const FInputActionValue& Value);
	void OnSprintStop(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	void OnParry(const FInputActionValue& Value);
	void OnBellySlam(const FInputActionValue& Value);
	void OnHaymaker(const FInputActionValue& Value);
	void OnThrowFood(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);
	void OnSelectSlot1(const FInputActionValue& Value);
	void OnSelectSlot2(const FInputActionValue& Value);
	void OnSelectSlot3(const FInputActionValue& Value);
	void OnSelectSlot4(const FInputActionValue& Value);
	void OnSelectSlot5(const FInputActionValue& Value);

	void TickMeters(float DeltaSeconds);
	void TickCombo(float DeltaSeconds);
	void TickFinisherPrompt(float DeltaSeconds);
	void UpdateSpeedFromState();

	void StartLightAttack();
	void StartBellySlam(bool bComboCancel);
	void StartHaymaker(bool bComboCancel);
	void StartRiposte(AActor* AgainstAttacker);
	void StartFinisher(EFinisherKind Kind, AEnemyBase* Target);

	void DoBellySlamHit();
	void DoHaymakerHit(float DamageMultiplier);
	void OnHaymakerWindupComplete();
	void EndHeavyRecovery();
	void EndParryWindow();
	void EndStagger();
	void EndRiposte();
	void EndFinisher();
	void Die();

	AEnemyBase* FindFinisherTarget(EFinisherKind& OutKind) const;
	void SelectHotbarSlot(int32 Index);
	bool ConsumeActiveHotbarFood(EFoodType& OutType);

	float CurrentIncomingDamageMult() const;
	bool ShouldHaveSuperArmor() const;
	bool IsBusyInHeavy() const;

	bool bSprintInput   = false;
	bool bExhausted     = false;
	bool bInParryWindow = false;
	float TimeSinceLastAttack = 999.f;
	float TimeSinceLastThrow  = 999.f;
	float TimeSinceComboHit   = 999.f;
	float TimeSinceParry      = 999.f;

	// Pending heavy attack the wind-up timer will fire.
	EHeavyAttack PendingHeavy = EHeavyAttack::None;
	// Damage multiplier captured at wind-up start (so a Riposte locks in its bonus).
	float PendingHeavyDamageMult = 1.f;

	TWeakObjectPtr<AEnemyBase> CachedFinisherTarget;

	FTimerHandle WindupTimer;
	FTimerHandle RecoveryTimer;
	FTimerHandle ParryTimer;
	FTimerHandle StaggerTimer;
	FTimerHandle FinisherTimer;
};
