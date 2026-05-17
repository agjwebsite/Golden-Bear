#include "StoryFlowSubsystem.h"
#include "JohnnyCharacter.h"
#include "JohnnyVoiceComponent.h"
#include "Kismet/GameplayStatics.h"

void UStoryFlowSubsystem::SetBeat(EStoryBeat NewBeat)
{
	if (CurrentBeat == NewBeat) return;
	CurrentBeat = NewBeat;
	OnBeatChanged.Broadcast(NewBeat);
}

void UStoryFlowSubsystem::AdvanceToEnding(int32 GamblingWinnings)
{
	Wallet += GamblingWinnings;
	SetBeat(EStoryBeat::Jackpot);

	// The rest of the ending is a chain of timed transitions the designer can pace
	// with WidgetBlueprints / level sequences. The subsystem just tells them when
	// to fire.
	if (UWorld* W = GetWorld())
	{
		FTimerHandle T1, T2, T3;
		W->GetTimerManager().SetTimer(T1,
			FTimerDelegate::CreateLambda([this]() { PayOffDebt(); }),
			2.5f, false);

		W->GetTimerManager().SetTimer(T2,
			FTimerDelegate::CreateLambda([this]() { SetBeat(EStoryBeat::Luxury); }),
			5.0f, false);

		W->GetTimerManager().SetTimer(T3,
			FTimerDelegate::CreateLambda([this]()
			{
				// All the luxury money goes to food. Of course.
				SpendOnLuxury(Wallet);
				SetBeat(EStoryBeat::BrokeAgain);
				if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
				{
					if (AJohnnyCharacter* J = Cast<AJohnnyCharacter>(P))
					{
						if (J->Voice) J->Voice->Say(EJohnnyLine::BrokeAgain);
					}
				}
			}),
			15.0f, false);
	}
}

void UStoryFlowSubsystem::PayOffDebt()
{
	const int32 Pay = FMath::Min(Wallet, DebtOwed);
	Wallet   -= Pay;
	DebtOwed -= Pay;
	SetBeat(EStoryBeat::DebtPaid);
}

void UStoryFlowSubsystem::SpendOnLuxury(int32 Amount)
{
	Wallet = FMath::Max(0, Wallet - Amount);
}
