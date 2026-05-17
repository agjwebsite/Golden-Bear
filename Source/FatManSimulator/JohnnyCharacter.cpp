#include "JohnnyCharacter.h"
#include "JohnnyVoiceComponent.h"
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

	// Johnny is a big lad — wider capsule.
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

	Health = MaxHealth;
	Stamina = MaxStamina;
	Hunger = MaxHunger;
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

	TimeSinceLastDodge  += DeltaSeconds;
	TimeSinceLastAttack += DeltaSeconds;

	TickMeters(DeltaSeconds);
	UpdateSpeedFromState();
}

void AJohnnyCharacter::TickMeters(float DeltaSeconds)
{
	// Hunger always drains.
	const float OldHunger = Hunger;
	Hunger = FMath::Max(0.f, Hunger - HungerDecayPerSecond * DeltaSeconds);
	if (!FMath::IsNearlyEqual(OldHunger, Hunger)) OnHungerChanged.Broadcast(Hunger / MaxHunger);

	// Stamina: drain while sprinting, regen otherwise.
	const bool bWantsSprint = bSprintInput && GetVelocity().SizeSquared2D() > 100.f && Stamina > 0.f && !bExhausted;
	const float OldStamina = Stamina;
	if (bWantsSprint)
	{
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainSprintPerSec * DeltaSeconds);
		if (Stamina <= 0.f && !bExhausted)
		{
			bExhausted = true;
			State = EJohnnyState::Walking;
			if (Voice) Voice->Say(EJohnnyLine::Exhausted);
		}
		else
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

	// Health: starvation damage if hungry, slow regen if fed.
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

void AJohnnyCharacter::UpdateSpeedFromState()
{
	float Target = WalkSpeed;
	switch (State)
	{
		case EJohnnyState::Sprinting: Target = SprintSpeed; break;
		case EJohnnyState::Staggered: Target = 0.f; break;
		case EJohnnyState::Dead:      Target = 0.f; break;
		default: break;
	}
	if (bExhausted) Target = FMath::Min(Target, ExhaustedSpeed);
	GetCharacterMovement()->MaxWalkSpeed = Target;
}

void AJohnnyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
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
		if (DodgeAction)    EIC->BindAction(DodgeAction,    ETriggerEvent::Started, this, &AJohnnyCharacter::OnDodge);
		if (AttackAction)   EIC->BindAction(AttackAction,   ETriggerEvent::Started, this, &AJohnnyCharacter::OnAttack);
		if (InteractAction) EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AJohnnyCharacter::OnInteract);
	}
}

void AJohnnyCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero()) return;
	if (State == EJohnnyState::Staggered || State == EJohnnyState::Dead) return;

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

void AJohnnyCharacter::OnDodge(const FInputActionValue&)
{
	if (TimeSinceLastDodge < DodgeCooldown) return;
	if (Stamina < DodgeStaminaCost) return;
	if (State == EJohnnyState::Dodging || State == EJohnnyState::Staggered || State == EJohnnyState::Dead) return;

	Stamina -= DodgeStaminaCost;
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
	TimeSinceLastDodge = 0.f;
	State = EJohnnyState::Dodging;
	bIsInvulnerable = true;

	FVector Dir = GetVelocity().GetSafeNormal2D();
	if (Dir.IsNearlyZero()) Dir = GetActorForwardVector();
	LaunchCharacter(Dir * DodgeImpulse + FVector(0,0,80.f), true, true);

	GetWorldTimerManager().SetTimer(DodgeTimer, this, &AJohnnyCharacter::EndDodge, DodgeDuration, false);
	if (Voice) Voice->Say(EJohnnyLine::Dodge);
}

void AJohnnyCharacter::EndDodge()
{
	bIsInvulnerable = false;
	if (State == EJohnnyState::Dodging) State = EJohnnyState::Walking;
}

void AJohnnyCharacter::OnAttack(const FInputActionValue&)
{
	if (TimeSinceLastAttack < MeleeCooldown) return;
	if (Stamina < MeleeStaminaCost) return;
	if (State == EJohnnyState::Dodging || State == EJohnnyState::Staggered || State == EJohnnyState::Dead) return;

	TimeSinceLastAttack = 0.f;
	Stamina -= MeleeStaminaCost;
	OnStaminaChanged.Broadcast(Stamina / MaxStamina);
	State = EJohnnyState::Attacking;
	PerformMeleeHit();
	if (Voice) Voice->Say(EJohnnyLine::Attack);
}

void AJohnnyCharacter::PerformMeleeHit()
{
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 50.f;
	const FVector End   = Start + GetActorForwardVector() * MeleeRange;

	TArray<FHitResult> Hits;
	FCollisionShape Shape = FCollisionShape::MakeSphere(MeleeRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JohnnyMelee), false, this);

	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Shape, Params);

#if !UE_BUILD_SHIPPING
	DrawDebugCapsule(GetWorld(), (Start+End)*0.5f, (End-Start).Size()*0.5f, MeleeRadius,
	                 FQuat::FindBetweenNormals(FVector::UpVector, (End-Start).GetSafeNormal()),
	                 FColor::Yellow, false, 0.25f);
#endif

	for (const FHitResult& Hit : Hits)
	{
		if (AActor* Target = Hit.GetActor())
		{
			if (Target == this) continue;
			FDamageEvent DamageEvent;
			Target->TakeDamage(MeleeDamage, DamageEvent, GetController(), this);
		}
	}

	State = EJohnnyState::Walking;
}

void AJohnnyCharacter::OnInteract(const FInputActionValue&)
{
	// Hand off to whatever interactable is in range — implemented by trigger volumes
	// (FoodPickup, GamblingMachine, LevelTransitionTrigger) reacting to overlap.
	// Players still press Interact via this binding for explicit confirmations.
}

float AJohnnyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                   AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsInvulnerable || State == EJohnnyState::Dead) return 0.f;

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);
	OnHealthChanged.Broadcast(Health / MaxHealth);

	if (Voice) Voice->Say(EJohnnyLine::Hurt);

	if (Health <= 0.f) Die();
	return Applied;
}

void AJohnnyCharacter::Stagger(float Duration)
{
	if (State == EJohnnyState::Dead) return;
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
	GetCharacterMovement()->DisableMovement();
	if (Voice) Voice->Say(EJohnnyLine::Death);
	OnDied.Broadcast();
}
