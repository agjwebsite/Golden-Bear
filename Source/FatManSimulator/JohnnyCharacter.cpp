#include "JohnnyCharacter.h"
#include "JohnnyVoiceComponent.h"
#include "EnemyBase.h"
#include "ThrownFood.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

AJohnnyCharacter::AJohnnyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 480.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 1800.f;

	GetCapsuleComponent()->InitCapsuleSize(48.f, 96.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 360.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 60.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Voice = CreateDefaultSubobject<UJohnnyVoiceComponent>(TEXT("Voice"));
}

void AJohnnyCharacter::BeginPlay()
{
	Super::BeginPlay();

	Health  = MaxHealth;
	Stamina = MaxStamina;
	Hunger  = MaxHunger;

	HotbarFood.Init(EFoodType::Kebapi, HotbarSize);
	HotbarCount.Init(0, HotbarSize);
	ActiveHotbarSlot = 0;
	OnHotbarChanged.Broadcast(ActiveHotbarSlot);

	UpdateSpeedFromState();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AJohnnyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == EJohnnyState::Dead) return;

	TimeSinceLastAttack += DeltaSeconds;
	TimeSinceLastThrow  += DeltaSeconds;
	TimeSinceComboHit   += DeltaSeconds;
	TimeSinceParry      += DeltaSeconds;

	TickMeters(DeltaSeconds);
	TickCombo(DeltaSeconds);
	TickFinisherPrompt(DeltaSeconds);
	UpdateSpeedFromState();
}

void AJohnnyCharacter::TickMeters(float DeltaSeconds)
{
	const float OldHunger = Hunger;
	Hunger = FMath::Max(0.f, Hunger - HungerDecayPerSecond * DeltaSeconds);
	if (!FMath::IsNearlyEqual(OldHunger, Hunger)) OnHungerChanged.Broadcast(Hunger / MaxHunger);

	const bool bWantsSprint = bSprintInput && GetVelocity().SizeSquared2D() > 100.f && Stamina > 0.f && !bExhausted;
	const float OldStamina = Stamina;
	if (bWantsSprint)
	{
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainSprintPerSec * DeltaSeconds);
		if (Stamina <= 0.f && !bExhausted)
		{
			bExhausted = true;
			if (State == EJohnnyState::Sprinting) State = EJohnnyState::Walking;
			if (Voice) Voice->Say(EJohnnyLine::Exhausted);
		}
		else if (State == EJohnnyState::Walking)
		{
			State = EJohnnyState::Sprinting;
			if (Voice) Voice->MaybeSayWhileSprinting(DeltaSeconds);
		}
	}
	else
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSec * DeltaSeconds);
		if (bExhausted && Stamina > MaxStamina * 0.4f) bExhausted = false;
		if (State == EJohnnyState::Sprinting) State = EJohnnyState::Walking;
	}
	if (!FMath::IsNearlyEqual(OldStamina, Stamina)) OnStaminaChanged.Broadcast(Stamina / MaxStamina);

	const float OldHealth = Health;
	if (Hunger <= 0.f)
	{
		Health = FMath::Max(0.f, Health - StarvationDamagePerSec * DeltaSeconds);
		if (Voice) Voice->MaybeSayWhileStarving(DeltaSeconds);
	}
	else if (Hunger >= HealthRegenMinHunger && Health < MaxHealth)
	{
		Health = FMath::Min(MaxHealth, Health + HealthRegenPerSec * DeltaSeconds);
	}
	if (!FMath::IsNearlyEqual(OldHealth, Health)) OnHealthChanged.Broadcast(Health / MaxHealth);

	if (Health <= 0.f && State != EJohnnyState::Dead) Die();
}

void AJohnnyCharacter::TickCombo(float DeltaSeconds)
{
	if (ComboCount > 0 && TimeSinceComboHit > ComboResetWindow)
	{
		ComboCount = 0;
		OnComboChanged.Broadcast(ComboCount);
	}
}

void AJohnnyCharacter::TickFinisherPrompt(float DeltaSeconds)
{
	if (State == EJohnnyState::Dead || State == EJohnnyState::Finishing)
	{
		if (bFinisherPromptVisible)
		{
			bFinisherPromptVisible = false;
			FinisherPromptKind = EFinisherKind::None;
			CachedFinisherTarget = nullptr;
			OnFinisherPromptChanged.Broadcast(false, EFinisherKind::None);
		}
		return;
	}

	EFinisherKind Kind = EFinisherKind::None;
	AEnemyBase* Target = FindFinisherTarget(Kind);
	const bool bVisible = Target != nullptr;

	if (bVisible != bFinisherPromptVisible || Kind != FinisherPromptKind)
	{
		bFinisherPromptVisible = bVisible;
		FinisherPromptKind = Kind;
		CachedFinisherTarget = Target;
		OnFinisherPromptChanged.Broadcast(bVisible, Kind);
	}
	else
	{
		CachedFinisherTarget = Target;
	}
}

void AJohnnyCharacter::UpdateSpeedFromState()
{
	float Target = WalkSpeed;
	switch (State)
	{
		case EJohnnyState::Sprinting:      Target = SprintSpeed; break;
		case EJohnnyState::WindingUpHeavy: Target = WalkSpeed * 0.25f; break;
		case EJohnnyState::HeavyRecovery:  Target = WalkSpeed * 0.5f;  break;
		case EJohnnyState::Parrying:       Target = WalkSpeed * 0.4f;  break;
		case EJohnnyState::Riposting:
		case EJohnnyState::Attacking:      Target = WalkSpeed * 0.6f;  break;
		case EJohnnyState::Finishing:
		case EJohnnyState::Staggered:
		case EJohnnyState::Dead:           Target = 0.f; break;
		default: break;
	}
	if (bExhausted) Target = FMath::Min(Target, ExhaustedSpeed);
	GetCharacterMovement()->MaxWalkSpeed = Target;
}

void AJohnnyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (MoveAction)     EIC->BindAction(MoveAction,     ETriggerEvent::Triggered, this, &AJohnnyCharacter::OnMove);
	if (LookAction)     EIC->BindAction(LookAction,     ETriggerEvent::Triggered, this, &AJohnnyCharacter::OnLook);
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (SprintAction)
	{
		EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &AJohnnyCharacter::OnSprintStart);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AJohnnyCharacter::OnSprintStop);
	}
	if (AttackAction)    EIC->BindAction(AttackAction,    ETriggerEvent::Started, this, &AJohnnyCharacter::OnAttack);
	if (ParryAction)     EIC->BindAction(ParryAction,     ETriggerEvent::Started, this, &AJohnnyCharacter::OnParry);
	if (BellySlamAction) EIC->BindAction(BellySlamAction, ETriggerEvent::Started, this, &AJohnnyCharacter::OnBellySlam);
	if (HaymakerAction)  EIC->BindAction(HaymakerAction,  ETriggerEvent::Started, this, &AJohnnyCharacter::OnHaymaker);
	if (ThrowFoodAction) EIC->BindAction(ThrowFoodAction, ETriggerEvent::Started, this, &AJohnnyCharacter::OnThrowFood);
	if (InteractAction)  EIC->BindAction(InteractAction,  ETriggerEvent::Started, this, &AJohnnyCharacter::OnInteract);

	if (HotbarSlot1Action) EIC->BindAction(HotbarSlot1Action, ETriggerEvent::Started, this, &AJohnnyCharacter::OnSelectSlot1);
	if (HotbarSlot2Action) EIC->BindAction(HotbarSlot2Action, ETriggerEvent::Started, this, &AJohnnyCharacter::OnSelectSlot2);
	if (HotbarSlot3Action) EIC->BindAction(HotbarSlot3Action, ETriggerEvent::Started, this, &AJohnnyCharacter::OnSelectSlot3);
	if (HotbarSlot4Action) EIC->BindAction(HotbarSlot4Action, ETriggerEvent::Started, this, &AJohnnyCharacter::OnSelectSlot4);
	if (HotbarSlot5Action) EIC->BindAction(HotbarSlot5Action, ETriggerEvent::Started, this, &AJohnnyCharacter::OnSelectSlot5);
}

void AJohnnyCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero()) return;
	if (State == EJohnnyState::Staggered || State == EJohnnyState::Dead ||
	    State == EJohnnyState::Finishing) return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right,   Axis.X);
}

void AJohnnyCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller) return;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AJohnnyCharacter::OnSprintStart(const FInputActionValue&) { bSprintInput = true; }
void AJohnnyCharacter::OnSprintStop(const FInputActionValue&)  { bSprintInput = false; }

void AJohnnyCharacter::OnAttack(const FInputActionValue&)
{
	StartLightAttack();
}

void AJohnnyCharacter::OnParry(const FInputActionValue&)
{
	if (State == EJohnnyState::Dead || State == EJohnnyState::Staggered ||
	    State == EJohnnyState::Finishing) return;
	if (IsBusyInHeavy()) return;
	if (TimeSinceParry < ParryWindow + ParryWhiffCooldown) return;

	State = EJohnnyState::Parrying;
	bInParryWindow = true;
	TimeSinceParry = 0.f;
	GetWorldTimerManager().SetTimer(ParryTimer, this, &AJohnnyCharacter::EndParryWindow, ParryWindow, false);
	if (Voice) Voice->Say(EJohnnyLine::Parry);
}

void AJohnnyCharacter::OnBellySlam(const FInputActionValue&)
{
	const bool bComboCancel = ComboCount >= 2 && TimeSinceComboHit <= ComboResetWindow;
	StartBellySlam(bComboCancel);
}

void AJohnnyCharacter::OnHaymaker(const FInputActionValue&)
{
	const bool bComboCancel = ComboCount >= 2 && TimeSinceComboHit <= ComboResetWindow;
	StartHaymaker(bComboCancel);
}

void AJohnnyCharacter::OnThrowFood(const FInputActionValue&)
{
	if (State == EJohnnyState::Dead || State == EJohnnyState::Staggered ||
	    State == EJohnnyState::Finishing) return;
	if (IsBusyInHeavy()) return;
	if (TimeSinceLastThrow < ThrowCooldown) return;
	if (!ThrownFoodClass) return;

	EFoodType FoodType;
	if (!ConsumeActiveHotbarFood(FoodType)) return;

	TimeSinceLastThrow = 0.f;

	const FVector Eye   = GetActorLocation() + FVector(0, 0, 40.f);
	FRotator AimRot     = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FVector Dir   = AimRot.Vector();
	const FVector Spawn = Eye + Dir * 60.f;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AThrownFood* Projectile = GetWorld()->SpawnActor<AThrownFood>(ThrownFoodClass, Spawn, AimRot, Params);
	if (Projectile)
	{
		Projectile->Launch(Dir * ThrowSpeed, FoodType);
	}

	if (Voice) Voice->Say(EJohnnyLine::ThrowFood);
}

void AJohnnyCharacter::OnInteract(const FInputActionValue&)
{
	if (bFinisherPromptVisible && CachedFinisherTarget.IsValid())
	{
		StartFinisher(FinisherPromptKind, CachedFinisherTarget.Get());
		return;
	}
	// Other interactables (FoodPickup, GamblingMachine, level triggers) react to overlap.
}

void AJohnnyCharacter::OnSelectSlot1(const FInputActionValue&) { SelectHotbarSlot(0); }
void AJohnnyCharacter::OnSelectSlot2(const FInputActionValue&) { SelectHotbarSlot(1); }
void AJohnnyCharacter::OnSelectSlot3(const FInputActionValue&) { SelectHotbarSlot(2); }
void AJohnnyCharacter::OnSelectSlot4(const FInputActionValue&) { SelectHotbarSlot(3); }
void AJohnnyCharacter::OnSelectSlot5(const FInputActionValue&) { SelectHotbarSlot(4); }

void AJohnnyCharacter::SelectHotbarSlot(int32 Index)
{
	if (Index < 0 || Index >= HotbarCount.Num()) return;
	ActiveHotbarSlot = Index;
	OnHotbarChanged.Broadcast(ActiveHotbarSlot);
}

bool AJohnnyCharacter::ConsumeActiveHotbarFood(EFoodType& OutType)
{
	if (!HotbarCount.IsValidIndex(ActiveHotbarSlot)) return false;
	if (HotbarCount[ActiveHotbarSlot] <= 0)
	{
		// Active slot empty — fall back to the first non-empty slot.
		for (int32 i = 0; i < HotbarCount.Num(); ++i)
		{
			if (HotbarCount[i] > 0) { ActiveHotbarSlot = i; break; }
		}
		if (HotbarCount[ActiveHotbarSlot] <= 0) return false;
	}
	OutType = HotbarFood[ActiveHotbarSlot];
	HotbarCount[ActiveHotbarSlot]--;
	if (HotbarCount[ActiveHotbarSlot] <= 0) HotbarCount[ActiveHotbarSlot] = 0;
	OnHotbarChanged.Broadcast(ActiveHotbarSlot);
	return true;
}

bool AJohnnyCharacter::AddFoodToHotbar(EFoodType Food, int32 Count)
{
	if (Count <= 0) return false;
	if (HotbarCount.Num() == 0)
	{
		HotbarFood.Init(EFoodType::Kebapi, HotbarSize);
		HotbarCount.Init(0, HotbarSize);
	}

	// Stack onto an existing slot of the same food.
	for (int32 i = 0; i < HotbarCount.Num(); ++i)
	{
		if (HotbarCount[i] > 0 && HotbarFood[i] == Food)
		{
			HotbarCount[i] += Count;
			OnHotbarChanged.Broadcast(ActiveHotbarSlot);
			return true;
		}
	}
	// Otherwise drop into the first empty slot.
	for (int32 i = 0; i < HotbarCount.Num(); ++i)
	{
		if (HotbarCount[i] <= 0)
		{
			HotbarFood[i] = Food;
			HotbarCount[i] = Count;
			OnHotbarChanged.Broadcast(ActiveHotbarSlot);
			return true;
		}
	}
	return false;
}

// ----- Light attack / combo -----------------------------------------------

void AJohnnyCharacter::StartLightAttack()
{
	if (State == EJohnnyState::Dead || State == EJohnnyState::Staggered ||
	    State == EJohnnyState::Finishing || State == EJohnnyState::Parrying) return;
	if (IsBusyInHeavy()) return;
	if (TimeSinceLastAttack < LightCooldown) return;
	if (Stamina < LightStaminaCost) return;

	TimeSinceLastAttack = 0.f;
	Stamina -= LightStaminaCost;
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);

	if (TimeSinceComboHit > ComboResetWindow) ComboCount = 0;
	ComboCount = FMath::Min(ComboCount + 1, 3);
	TimeSinceComboHit = 0.f;
	State = EJohnnyState::Attacking;
	OnComboChanged.Broadcast(ComboCount);

	// Light attack is fast: hit immediately, then resolve back to Walking on next tick via cooldown.
	const bool bIsHook = (ComboCount == 3);
	const float Damage = bIsHook ? LightHookDamage : LightDamage;

	const FVector Start = GetActorLocation() + GetActorForwardVector() * 50.f;
	const FVector End   = Start + GetActorForwardVector() * LightRange;

	TArray<FHitResult> Hits;
	FCollisionShape Shape = FCollisionShape::MakeSphere(LightRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JohnnyLight), false, this);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if !UE_BUILD_SHIPPING
	DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f, LightRadius,
	                 FQuat::FindBetweenNormals(FVector::UpVector, (End - Start).GetSafeNormal()),
	                 bIsHook ? FColor::Red : FColor::Yellow, false, 0.2f);
#endif

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this) continue;

		FDamageEvent DamageEvent;
		HitActor->TakeDamage(Damage, DamageEvent, GetController(), this);

		if (bIsHook)
		{
			if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
			{
				const FVector Away = (Enemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
				Enemy->ApplyKnockback(Away * HookKnockback + FVector(0, 0, 200.f), false, 0.f);
			}
		}
	}

	if (Voice) Voice->Say(EJohnnyLine::Attack);

	if (bIsHook)
	{
		ComboCount = 0;
		OnComboChanged.Broadcast(ComboCount);
	}

	// Return to Walking quickly so the next combo input is responsive.
	State = EJohnnyState::Attacking;
	GetWorldTimerManager().SetTimer(RecoveryTimer, this, &AJohnnyCharacter::EndHeavyRecovery, LightCooldown * 0.5f, false);
}

// ----- Heavy attacks ------------------------------------------------------

void AJohnnyCharacter::StartBellySlam(bool bComboCancel)
{
	if (State == EJohnnyState::Dead || State == EJohnnyState::Staggered ||
	    State == EJohnnyState::Finishing || State == EJohnnyState::WindingUpHeavy ||
	    State == EJohnnyState::HeavyRecovery) return;

	const float Cost = FMath::Max(0.f, SlamStaminaCost - (bComboCancel ? SlamCancelStaminaDiscount : 0.f));
	if (Stamina < Cost) return;

	Stamina -= Cost;
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
	PendingHeavy = EHeavyAttack::BellySlam;
	PendingHeavyDamageMult = 1.f;

	if (Voice) Voice->Say(EJohnnyLine::Heavy);

	if (bComboCancel)
	{
		// Skip the wind-up — combo-cancel reward.
		ComboCount = 0;
		OnComboChanged.Broadcast(ComboCount);
		DoBellySlamHit();
	}
	else
	{
		State = EJohnnyState::WindingUpHeavy;
		bSuperArmor = ShouldHaveSuperArmor();
		GetWorldTimerManager().SetTimer(WindupTimer, this, &AJohnnyCharacter::DoBellySlamHit, SlamWindup, false);
	}
}

void AJohnnyCharacter::StartHaymaker(bool bComboCancel)
{
	if (State == EJohnnyState::Dead || State == EJohnnyState::Staggered ||
	    State == EJohnnyState::Finishing || State == EJohnnyState::WindingUpHeavy ||
	    State == EJohnnyState::HeavyRecovery) return;

	const float Cost = FMath::Max(0.f, HaymakerStaminaCost - (bComboCancel ? HaymakerCancelStaminaDiscount : 0.f));
	if (Stamina < Cost) return;

	Stamina -= Cost;
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
	PendingHeavy = EHeavyAttack::Haymaker;
	PendingHeavyDamageMult = 1.f;

	if (Voice) Voice->Say(EJohnnyLine::Heavy);

	if (bComboCancel)
	{
		ComboCount = 0;
		OnComboChanged.Broadcast(ComboCount);
		OnHaymakerWindupComplete();
	}
	else
	{
		State = EJohnnyState::WindingUpHeavy;
		bSuperArmor = ShouldHaveSuperArmor();
		GetWorldTimerManager().SetTimer(WindupTimer, this, &AJohnnyCharacter::OnHaymakerWindupComplete, HaymakerWindup, false);
	}
}

void AJohnnyCharacter::OnHaymakerWindupComplete()
{
	DoHaymakerHit(PendingHeavyDamageMult);
	GetWorldTimerManager().SetTimer(RecoveryTimer, this, &AJohnnyCharacter::EndHeavyRecovery, HaymakerRecovery, false);
}

void AJohnnyCharacter::DoBellySlamHit()
{
	bSuperArmor = false;
	State = EJohnnyState::HeavyRecovery;
	PendingHeavy = EHeavyAttack::None;

	const FVector Origin = GetActorLocation();
	TArray<FHitResult> Hits;
	FCollisionShape Shape = FCollisionShape::MakeSphere(SlamRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JohnnyBellySlam), false, this);
	GetWorld()->SweepMultiByChannel(Hits, Origin, Origin + FVector(0, 0, 1.f), FQuat::Identity,
	                                ECC_Pawn, Shape, Params);

#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), Origin, SlamRadius, 16, FColor::Orange, false, 0.4f);
#endif

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this) continue;

		FDamageEvent DamageEvent;
		HitActor->TakeDamage(SlamDamage, DamageEvent, GetController(), this);

		if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
		{
			const FVector Away = (Enemy->GetActorLocation() - Origin).GetSafeNormal2D();
			Enemy->ApplyKnockback(Away * SlamKnockback + FVector(0, 0, 350.f),
			                      /*bKnockdown=*/true, SlamKnockdownTime);
		}
	}

	GetWorldTimerManager().SetTimer(RecoveryTimer, this, &AJohnnyCharacter::EndHeavyRecovery, SlamRecovery, false);
}

void AJohnnyCharacter::DoHaymakerHit(float DamageMultiplier)
{
	bSuperArmor = false;
	State = EJohnnyState::HeavyRecovery;
	PendingHeavy = EHeavyAttack::None;

	const FVector Start = GetActorLocation() + GetActorForwardVector() * 50.f;
	const FVector End   = Start + GetActorForwardVector() * HaymakerRange;

	TArray<FHitResult> Hits;
	FCollisionShape Shape = FCollisionShape::MakeSphere(HaymakerRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JohnnyHaymaker), false, this);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if !UE_BUILD_SHIPPING
	DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f, HaymakerRadius,
	                 FQuat::FindBetweenNormals(FVector::UpVector, (End - Start).GetSafeNormal()),
	                 FColor::Red, false, 0.4f);
#endif

	// Haymaker hits one target — closest in cone.
	AActor* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == this) continue;
		const float D = FVector::DistSquared(HitActor->GetActorLocation(), GetActorLocation());
		if (D < BestDist) { Best = HitActor; BestDist = D; }
	}

	if (Best)
	{
		FDamageEvent DamageEvent;
		Best->TakeDamage(HaymakerDamage * DamageMultiplier, DamageEvent, GetController(), this);
		if (AEnemyBase* Enemy = Cast<AEnemyBase>(Best))
		{
			Enemy->Stagger(HaymakerStaggerTime);
		}
	}
}

void AJohnnyCharacter::EndHeavyRecovery()
{
	bSuperArmor = false;
	if (State == EJohnnyState::Attacking || State == EJohnnyState::HeavyRecovery)
	{
		State = EJohnnyState::Walking;
	}
}

// ----- Parry & Riposte ----------------------------------------------------

void AJohnnyCharacter::EndParryWindow()
{
	bInParryWindow = false;
	if (State == EJohnnyState::Parrying) State = EJohnnyState::Walking;
}

bool AJohnnyCharacter::TryConsumeParry(AActor* Attacker)
{
	if (!bInParryWindow) return false;
	bInParryWindow = false;
	GetWorldTimerManager().ClearTimer(ParryTimer);

	if (AEnemyBase* Enemy = Cast<AEnemyBase>(Attacker))
	{
		Enemy->Stagger(ParryStaggerDuration);
	}

	StartRiposte(Attacker);
	return true;
}

void AJohnnyCharacter::StartRiposte(AActor* AgainstAttacker)
{
	State = EJohnnyState::Riposting;
	PendingHeavyDamageMult = RiposteDamageMult;

	if (AgainstAttacker)
	{
		const FVector ToAttacker = (AgainstAttacker->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToAttacker.IsNearlyZero())
		{
			SetActorRotation(ToAttacker.Rotation());
		}
	}

	if (Voice) Voice->Say(EJohnnyLine::Heavy);

	// Riposte is a free Haymaker with no wind-up.
	DoHaymakerHit(PendingHeavyDamageMult);
	GetWorldTimerManager().SetTimer(RecoveryTimer, this, &AJohnnyCharacter::EndRiposte, HaymakerRecovery * 0.5f, false);
}

void AJohnnyCharacter::EndRiposte()
{
	if (State == EJohnnyState::Riposting || State == EJohnnyState::HeavyRecovery)
	{
		State = EJohnnyState::Walking;
	}
}

// ----- Finisher -----------------------------------------------------------

AEnemyBase* AJohnnyCharacter::FindFinisherTarget(EFinisherKind& OutKind) const
{
	OutKind = EFinisherKind::None;

	const FVector Origin  = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(FinisherRange);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JohnnyFinisherScan), false, this);
	GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Shape, Params);

	AEnemyBase* Best = nullptr;
	float BestScore = -1.f;
	for (const FOverlapResult& O : Overlaps)
	{
		AEnemyBase* Enemy = Cast<AEnemyBase>(O.GetActor());
		if (!Enemy || !Enemy->IsFinishable(FinisherHpFraction)) continue;

		const FVector ToEnemy = Enemy->GetActorLocation() - Origin;
		const float Dist = ToEnemy.Size2D();
		if (Dist > FinisherRange) continue;
		const FVector ToEnemyN = ToEnemy.GetSafeNormal2D();
		const float Dot = FVector::DotProduct(Forward, ToEnemyN);
		if (Dot < FinisherConeCos) continue;

		// Prefer knocked-down enemies, then closer ones.
		const float Score = (Enemy->IsKnockedDown() ? 1000.f : 0.f) + (FinisherRange - Dist);
		if (Score > BestScore) { Best = Enemy; BestScore = Score; }
	}

	if (Best)
	{
		OutKind = (Hunger < EatHungerThreshold) ? EFinisherKind::Eat : EFinisherKind::Stomp;
	}
	return Best;
}

void AJohnnyCharacter::StartFinisher(EFinisherKind Kind, AEnemyBase* TargetEnemy)
{
	if (!TargetEnemy || Kind == EFinisherKind::None) return;
	if (State == EJohnnyState::Finishing || State == EJohnnyState::Dead) return;

	GetWorldTimerManager().ClearTimer(WindupTimer);
	GetWorldTimerManager().ClearTimer(RecoveryTimer);

	State = EJohnnyState::Finishing;
	bIsInvulnerable = true;
	bSuperArmor = true;

	const FVector ToEnemy = (TargetEnemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToEnemy.IsNearlyZero()) SetActorRotation(ToEnemy.Rotation());

	if (Kind == EFinisherKind::Eat)
	{
		EatFood(EatHungerRestore, 0.f);
	}

	FDamageEvent DamageEvent;
	TargetEnemy->TakeDamage(StompDamage, DamageEvent, GetController(), this);

	if (Voice) Voice->Say(Kind == EFinisherKind::Eat ? EJohnnyLine::Eating : EJohnnyLine::Finisher);

	bFinisherPromptVisible = false;
	FinisherPromptKind = EFinisherKind::None;
	OnFinisherPromptChanged.Broadcast(false, EFinisherKind::None);
	CachedFinisherTarget = nullptr;

	GetWorldTimerManager().SetTimer(FinisherTimer, this, &AJohnnyCharacter::EndFinisher, FinisherDuration, false);
}

void AJohnnyCharacter::EndFinisher()
{
	bIsInvulnerable = false;
	bSuperArmor = false;
	if (State == EJohnnyState::Finishing) State = EJohnnyState::Walking;
}

// ----- Damage / armor -----------------------------------------------------

float AJohnnyCharacter::CurrentIncomingDamageMult() const
{
	const float HungerFrac = (MaxHunger > 0.f) ? Hunger / MaxHunger : 0.f;
	if (HungerFrac >= ArmorFedThreshold)    return ArmorFedDamageMult;
	if (HungerFrac <= ArmorHungryThreshold) return ArmorHungryDamageMult;
	return 1.f;
}

bool AJohnnyCharacter::ShouldHaveSuperArmor() const
{
	const float HungerFrac = (MaxHunger > 0.f) ? Hunger / MaxHunger : 0.f;
	return HungerFrac >= ArmorFedThreshold;
}

bool AJohnnyCharacter::IsBusyInHeavy() const
{
	return State == EJohnnyState::WindingUpHeavy ||
	       State == EJohnnyState::HeavyRecovery  ||
	       State == EJohnnyState::Riposting      ||
	       State == EJohnnyState::Finishing;
}

float AJohnnyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                   AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsInvulnerable || State == EJohnnyState::Dead) return 0.f;

	const float Scaled = DamageAmount * CurrentIncomingDamageMult();
	const float Applied = Super::TakeDamage(Scaled, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);
	OnHealthChanged.Broadcast(Health / MaxHealth);

	if (Voice) Voice->Say(EJohnnyLine::Hurt);

	// Super-armor: no flinch, the heavy wind-up keeps rolling.
	if (!bSuperArmor)
	{
		const float HungerFrac = (MaxHunger > 0.f) ? Hunger / MaxHunger : 0.f;
		if (HungerFrac <= ArmorHungryThreshold)
		{
			Stagger(0.4f);
		}
	}

	if (Health <= 0.f) Die();
	return Applied;
}

void AJohnnyCharacter::Stagger(float Duration)
{
	if (State == EJohnnyState::Dead) return;
	GetWorldTimerManager().ClearTimer(WindupTimer);
	GetWorldTimerManager().ClearTimer(RecoveryTimer);
	bSuperArmor = false;
	bInParryWindow = false;
	PendingHeavy = EHeavyAttack::None;
	State = EJohnnyState::Staggered;
	GetWorldTimerManager().SetTimer(StaggerTimer, this, &AJohnnyCharacter::EndStagger, Duration, false);
}

void AJohnnyCharacter::EndStagger()
{
	if (State == EJohnnyState::Staggered) State = EJohnnyState::Walking;
}

void AJohnnyCharacter::EatFood(float HungerRestore, float HealthRestore)
{
	Hunger = FMath::Min(MaxHunger, Hunger + HungerRestore);
	Health = FMath::Min(MaxHealth, Health + HealthRestore);
	OnHungerChanged.Broadcast(Hunger / MaxHunger);
	OnHealthChanged.Broadcast(Health / MaxHealth);
	if (Voice) Voice->Say(EJohnnyLine::Eating);
}

void AJohnnyCharacter::Die()
{
	State = EJohnnyState::Dead;
	bSprintInput = false;
	bSuperArmor = false;
	bIsInvulnerable = false;
	GetCharacterMovement()->DisableMovement();
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (Voice) Voice->Say(EJohnnyLine::Death);
	OnDied.Broadcast();
}
