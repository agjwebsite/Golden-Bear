#include "JohnnyCar.h"
#include "JohnnyCharacter.h"
#include "JohnnyVoiceComponent.h"
#include "StoryFlowSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Particles/ParticleSystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AJohnnyCar::AJohnnyCar()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
	Body->InitBoxExtent(FVector(220.f, 95.f, 70.f));
	Body->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = Body;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Body);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ExhaustSmoke = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ExhaustSmoke"));
	ExhaustSmoke->SetupAttachment(Body);
	ExhaustSmoke->SetRelativeLocation(FVector(-220.f, 0.f, 0.f));
	ExhaustSmoke->bAutoActivate = false;

	EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
	EngineAudio->SetupAttachment(Body);
	EngineAudio->bAutoActivate = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Body);
	CameraBoom->TargetArmLength = 700.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 220.f);
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed     = MaxForwardSpeed;
	Movement->Acceleration = Acceleration;
	Movement->Deceleration = CoastDeceleration;

	bUseControllerRotationYaw = false;
}

void AJohnnyCar::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DrivingMappingContext)
			{
				Subsystem->ClearAllMappings();
				Subsystem->AddMappingContext(DrivingMappingContext, 0);
			}
		}
	}

	if (EngineAudio && EngineAudio->Sound) EngineAudio->Play();

	if (UWorld* W = GetWorld())
	{
		if (UStoryFlowSubsystem* Flow = W->GetGameInstance()
		    ? W->GetGameInstance()->GetSubsystem<UStoryFlowSubsystem>() : nullptr)
		{
			Flow->SetBeat(EStoryBeat::Driving);
		}
	}
}

void AJohnnyCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Throttle) EIC->BindAction(IA_Throttle, ETriggerEvent::Triggered, this, &AJohnnyCar::OnThrottle);
		if (IA_Throttle) EIC->BindAction(IA_Throttle, ETriggerEvent::Completed, this, &AJohnnyCar::OnThrottle);
		if (IA_Steer)    EIC->BindAction(IA_Steer,    ETriggerEvent::Triggered, this, &AJohnnyCar::OnSteer);
		if (IA_Steer)    EIC->BindAction(IA_Steer,    ETriggerEvent::Completed, this, &AJohnnyCar::OnSteer);
		if (IA_Look)     EIC->BindAction(IA_Look,     ETriggerEvent::Triggered, this, &AJohnnyCar::OnLook);
		if (IA_Horn)     EIC->BindAction(IA_Horn,     ETriggerEvent::Started,   this, &AJohnnyCar::OnHorn);
	}
}

void AJohnnyCar::OnThrottle(const FInputActionValue& Value) { ThrottleInput = FMath::Clamp(Value.Get<float>(), -1.f, 1.f); }
void AJohnnyCar::OnSteer(const FInputActionValue& Value)    { SteerInput    = FMath::Clamp(Value.Get<float>(), -1.f, 1.f); }
void AJohnnyCar::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}
void AJohnnyCar::OnHorn(const FInputActionValue&)
{
	// Placeholder for a horn sound trigger — designer wires it up via the BP.
}

void AJohnnyCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TimeAlive += DeltaSeconds;

	switch (State)
	{
		case ECarState::Driving:
			TickDriving(DeltaSeconds);
			if (TimeAlive >= TimeUntilSputter) EnterSputter();
			break;

		case ECarState::Sputtering:
			TickBreakdown(DeltaSeconds);
			break;

		case ECarState::Dead:
			// Coast to a stop.
			CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, 0.f, DeltaSeconds, BrakeDeceleration);
			AddActorLocalOffset(FVector(CurrentSpeed * DeltaSeconds, 0.f, 0.f), true);
			break;
	}
}

void AJohnnyCar::TickDriving(float DeltaSeconds)
{
	// Throttle / brake
	const float MaxSpeedForDir = (ThrottleInput >= 0.f) ? MaxForwardSpeed : -MaxReverseSpeed;
	const float TargetSpeed = ThrottleInput * FMath::Abs(MaxSpeedForDir);
	const float Rate = FMath::IsNearlyZero(ThrottleInput) ? CoastDeceleration
	                  : (FMath::Sign(TargetSpeed) != FMath::Sign(CurrentSpeed) && !FMath::IsNearlyZero(CurrentSpeed)
	                     ? BrakeDeceleration : Acceleration);
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, TargetSpeed, DeltaSeconds, Rate);

	// Steering scales with speed so the car feels planted at low speed.
	if (FMath::Abs(CurrentSpeed) > MinSpeedToSteer)
	{
		const float SpeedScale = FMath::Clamp(FMath::Abs(CurrentSpeed) / MaxForwardSpeed, 0.2f, 1.f);
		const float YawDelta = SteerInput * SteerRateDeg * SpeedScale * DeltaSeconds * FMath::Sign(CurrentSpeed);
		AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
	}

	AddActorLocalOffset(FVector(CurrentSpeed * DeltaSeconds, 0.f, 0.f), true);
}

void AJohnnyCar::TickBreakdown(float DeltaSeconds)
{
	SputterTimer    += DeltaSeconds;
	TimeSinceCough  += DeltaSeconds;

	// Same driving physics but throttle is forcibly reduced and randomly cut.
	const bool bMisfire = TimeSinceCough >= SputterCoughInterval;
	if (bMisfire)
	{
		TimeSinceCough = 0.f;
		SputterCoughInterval = FMath::FRandRange(0.6f, 2.0f);
		CurrentSpeed *= 0.55f;

		if (AJohnnyCharacter* J = DriverOnExit) if (J->Voice) J->Voice->Say(EJohnnyLine::CarSputter);
	}

	const float Effective = ThrottleInput * MaxForwardSpeed * SputterSpeedMultiplier;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, Effective, DeltaSeconds, Acceleration * 0.5f);

	if (FMath::Abs(CurrentSpeed) > MinSpeedToSteer)
	{
		const float SpeedScale = FMath::Clamp(FMath::Abs(CurrentSpeed) / MaxForwardSpeed, 0.2f, 1.f);
		const float YawDelta = SteerInput * SteerRateDeg * SpeedScale * DeltaSeconds * FMath::Sign(CurrentSpeed);
		AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
	}

	AddActorLocalOffset(FVector(CurrentSpeed * DeltaSeconds, 0.f, 0.f), true);

	if (SputterTimer >= SputterDuration) FinishBreakdown();
}

void AJohnnyCar::EnterSputter()
{
	State = ECarState::Sputtering;
	if (ExhaustSmoke) ExhaustSmoke->Activate(true);
	if (AJohnnyCharacter* J = DriverOnExit) if (J->Voice) J->Voice->Say(EJohnnyLine::CarDying);
}

void AJohnnyCar::ForceBreakdown()
{
	if (State == ECarState::Driving) EnterSputter();
	FinishBreakdown();
}

void AJohnnyCar::FinishBreakdown()
{
	if (State == ECarState::Dead) return;
	State = ECarState::Dead;

	if (EngineAudio) EngineAudio->Stop();
	if (AJohnnyCharacter* J = DriverOnExit) if (J->Voice) J->Voice->Say(EJohnnyLine::CarDeadResignation);

	OnBrokeDown.Broadcast();

	if (UWorld* W = GetWorld())
	{
		if (UStoryFlowSubsystem* Flow = W->GetGameInstance()
		    ? W->GetGameInstance()->GetSubsystem<UStoryFlowSubsystem>() : nullptr)
		{
			Flow->SetBeat(EStoryBeat::CarBreakdown);
		}
	}

	// Small pause so the player sees the car die before they're yanked out.
	FTimerHandle T;
	GetWorldTimerManager().SetTimer(T,
		FTimerDelegate::CreateLambda([this]() { HandoffToJohnny(); }),
		1.8f, false);
}

void AJohnnyCar::HandoffToJohnny()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !DriverOnExit) return;

	const FVector ExitWorld = GetActorTransform().TransformPosition(ExitOffset);
	DriverOnExit->SetActorLocation(ExitWorld + FVector(0.f, 0.f, 50.f), false, nullptr, ETeleportType::TeleportPhysics);
	DriverOnExit->SetActorRotation(FRotator(0.f, GetActorRotation().Yaw + 90.f, 0.f));

	PC->Possess(DriverOnExit);

	if (UWorld* W = GetWorld())
	{
		if (UStoryFlowSubsystem* Flow = W->GetGameInstance()
		    ? W->GetGameInstance()->GetSubsystem<UStoryFlowSubsystem>() : nullptr)
		{
			Flow->SetBeat(EStoryBeat::OnTheRoad);
		}
	}
}
