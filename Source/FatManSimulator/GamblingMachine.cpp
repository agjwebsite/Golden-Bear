#include "GamblingMachine.h"
#include "JohnnyCharacter.h"
#include "JohnnyVoiceComponent.h"
#include "StoryFlowSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AGamblingMachine::AGamblingMachine()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	InteractVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractVolume"));
	InteractVolume->SetupAttachment(Mesh);
	InteractVolume->SetBoxExtent(FVector(120.f, 120.f, 120.f));
	InteractVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

int32 AGamblingMachine::Pull(AJohnnyCharacter* Johnny)
{
	int32 Winnings = 0;

	if (!bFirstPullDone)
	{
		// Story beat: the rigged jackpot.
		bFirstPullDone = true;
		Winnings = RiggedFirstPullPayout;
		if (Johnny && Johnny->Voice) Johnny->Voice->Say(EJohnnyLine::Jackpot);

		if (UWorld* W = GetWorld())
		{
			if (UStoryFlowSubsystem* Flow = W->GetGameInstance()
			    ? W->GetGameInstance()->GetSubsystem<UStoryFlowSubsystem>() : nullptr)
			{
				Flow->AdvanceToEnding(Winnings);
			}
		}
	}
	else
	{
		// Subsequent pulls are punishingly bad — this is a Fat Man Simulator, not a casino sim.
		const float Roll = FMath::FRand();
		if      (Roll < 0.02f) Winnings = 100;
		else if (Roll < 0.10f) Winnings = 10;
		else                   Winnings = 0;
	}

	OnPullResolved.Broadcast(Winnings);
	return Winnings;
}
