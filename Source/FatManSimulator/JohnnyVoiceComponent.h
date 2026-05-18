#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JohnnyVoiceComponent.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EJohnnyLine : uint8
{
	SprintBelly,    // grumbling about his stomach while running
	FoodCraving,    // daydreaming about food
	Exhausted,      // out of stamina
	Starving,       // hunger at zero
	Eating,         // restoring hunger
	Dodge,          // legacy: kept for save-game compat, no longer used
	Parry,          // tight-window parry grunt
	Attack,         // throwing a jab
	Heavy,          // committing to a Belly Slam or Haymaker
	ThrowFood,      // lobbing a kebapi at the dog
	Finisher,       // stomping a downed enemy
	Hurt,           // took damage
	Death,          // health zero
	GangsterTaunt,  // when chased
	BossFires,      // story moment
	Jackpot,        // gambling win
	BrokeAgain,     // ending
	CarSputter,     // engine misfire during the driving scene
	CarDying,       // moment the sputtering starts
	CarDeadResignation // when the car finally quits
};

USTRUCT(BlueprintType)
struct FJohnnyLineSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FText>    Subtitles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<USoundBase*> Sounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinIntervalSeconds = 6.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceLine, EJohnnyLine, Kind, const FText&, Subtitle);

UCLASS(ClassGroup = (Johnny), meta = (BlueprintSpawnableComponent))
class FATMANSIMULATOR_API UJohnnyVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJohnnyVoiceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
	TMap<EJohnnyLine, FJohnnyLineSet> LineSets;

	UPROPERTY(BlueprintAssignable) FOnVoiceLine OnLineSpoken;

	UFUNCTION(BlueprintCallable, Category = "Voice")
	void Say(EJohnnyLine Kind);

	void MaybeSayWhileSprinting(float DeltaSeconds);
	void MaybeSayWhileStarving(float DeltaSeconds);

protected:
	virtual void BeginPlay() override;
	void SeedDefaultLines();

	TMap<EJohnnyLine, float> LastSpokenTime;
	float SprintTalkAccumulator   = 0.f;
	float StarvingTalkAccumulator = 0.f;
};
