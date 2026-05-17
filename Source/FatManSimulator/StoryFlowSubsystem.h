#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryFlowSubsystem.generated.h"

UENUM(BlueprintType)
enum class EStoryBeat : uint8
{
	CarBreakdown,    // Opening: Johnny's car dies outside Skopje.
	OnTheRoad,       // Trekking toward Veles. Patrol enemies. Food pickups.
	GangsterChase,   // Mid-game: loan-shark goons chase him.
	ArrivedTooLate,  // Reaches Veles after the meeting.
	BossFires,       // Cutscene: boss fires him.
	FoundADollar,    // Wandering, finds a dollar on the ground.
	GamblingHall,    // Plays the rigged machine.
	Jackpot,         // Wins a million.
	DebtPaid,        // Pays off the gangsters.
	Luxury,          // Brief montage of the good life.
	BrokeAgain       // Ending: spent it all on food.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryBeatChanged, EStoryBeat, NewBeat);

UCLASS()
class FATMANSIMULATOR_API UStoryFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Story") EStoryBeat CurrentBeat = EStoryBeat::CarBreakdown;
	UPROPERTY(BlueprintReadOnly, Category = "Story") int32 Wallet = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Story") int32 DebtOwed = 50000;

	UPROPERTY(BlueprintAssignable) FOnStoryBeatChanged OnBeatChanged;

	UFUNCTION(BlueprintCallable) void SetBeat(EStoryBeat NewBeat);
	UFUNCTION(BlueprintCallable) void AdvanceToEnding(int32 GamblingWinnings);
	UFUNCTION(BlueprintCallable) void PayOffDebt();
	UFUNCTION(BlueprintCallable) void SpendOnLuxury(int32 Amount);
};
