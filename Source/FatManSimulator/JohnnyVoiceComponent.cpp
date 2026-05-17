#include "JohnnyVoiceComponent.h"
#include "Kismet/GameplayStatics.h"

UJohnnyVoiceComponent::UJohnnyVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJohnnyVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	SeedDefaultLines();
}

void UJohnnyVoiceComponent::SeedDefaultLines()
{
	// Defaults are only used if the designer hasn't filled in line sets in the BP.
	auto AddIfMissing = [&](EJohnnyLine Kind, TArray<FString> Lines, float MinInterval)
	{
		if (LineSets.Contains(Kind)) return;
		FJohnnyLineSet Set;
		Set.MinIntervalSeconds = MinInterval;
		for (const FString& L : Lines) Set.Subtitles.Add(FText::FromString(L));
		LineSets.Add(Kind, Set);
	};

	AddIfMissing(EJohnnyLine::SprintBelly, {
		TEXT("Ugh... this belly... bouncing like a sack of flour."),
		TEXT("Why did I eat that second burek..."),
		TEXT("Stomach, please. Just... stop jiggling."),
		TEXT("I hate you. I hate every kilo of you."),
		TEXT("Run, you useless slab of lard."),
	}, 5.f);

	AddIfMissing(EJohnnyLine::FoodCraving, {
		TEXT("A nice kebapi right now... ten of them. Twelve."),
		TEXT("Gyro. Hot gyro. Drowning in tzatziki."),
		TEXT("One burger. Just one. A small one. Or eight."),
		TEXT("Baklava. Honey-soaked. I can almost smell it."),
		TEXT("I would sell my left lung for a tavče gravče."),
	}, 7.f);

	AddIfMissing(EJohnnyLine::Exhausted, {
		TEXT("Lungs... on fire... legs... gone..."),
		TEXT("That's it. I'm dying. Tell my mother I love her."),
		TEXT("Five seconds. Just give me five seconds."),
	}, 4.f);

	AddIfMissing(EJohnnyLine::Starving, {
		TEXT("My stomach is eating itself. Cannibal."),
		TEXT("Need... food... anything..."),
		TEXT("Is that a sandwich on the ground? ...no. It's a leaf."),
	}, 6.f);

	AddIfMissing(EJohnnyLine::Eating, {
		TEXT("Mmm. That's the stuff."),
		TEXT("Don't judge me. You'd do the same."),
		TEXT("Worth it. Every time."),
	}, 2.f);

	AddIfMissing(EJohnnyLine::Dodge,        { TEXT("Hup!"), TEXT("Move!"), TEXT("Ow my knee."), }, 0.5f);
	AddIfMissing(EJohnnyLine::Attack,       { TEXT("Take this!"), TEXT("Eat fist!"), TEXT("This is for my belly!"), }, 0.4f);
	AddIfMissing(EJohnnyLine::Hurt,         { TEXT("Aagh!"), TEXT("Not the face!"), TEXT("Why is this happening to me..."), }, 0.6f);
	AddIfMissing(EJohnnyLine::Death,        { TEXT("Tell them... I tried..."), }, 999.f);
	AddIfMissing(EJohnnyLine::GangsterTaunt,{ TEXT("I'll get the money! I swear!"), TEXT("Just one more week!"), }, 3.f);
	AddIfMissing(EJohnnyLine::BossFires,    { TEXT("You're firing me? Over a meeting?! I RAN here!"), }, 999.f);
	AddIfMissing(EJohnnyLine::Jackpot,      { TEXT("A MILLION?! A MILLION DOLLARS?!"), }, 999.f);
	AddIfMissing(EJohnnyLine::BrokeAgain,   { TEXT("Where did it all go... oh. The kebapi. Right."), }, 999.f);
}

void UJohnnyVoiceComponent::Say(EJohnnyLine Kind)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	FJohnnyLineSet* Set = LineSets.Find(Kind);
	if (!Set || Set->Subtitles.Num() == 0) return;

	const float* LastTime = LastSpokenTime.Find(Kind);
	if (LastTime && (Now - *LastTime) < Set->MinIntervalSeconds) return;
	LastSpokenTime.Add(Kind, Now);

	const int32 Index = FMath::RandRange(0, Set->Subtitles.Num() - 1);
	const FText& Subtitle = Set->Subtitles[Index];

	if (Set->Sounds.IsValidIndex(Index) && Set->Sounds[Index])
	{
		UGameplayStatics::PlaySoundAtLocation(this, Set->Sounds[Index], GetOwner()->GetActorLocation());
	}

	OnLineSpoken.Broadcast(Kind, Subtitle);
}

void UJohnnyVoiceComponent::MaybeSayWhileSprinting(float DeltaSeconds)
{
	SprintTalkAccumulator += DeltaSeconds;
	if (SprintTalkAccumulator >= 5.f)
	{
		SprintTalkAccumulator = 0.f;
		Say(FMath::RandBool() ? EJohnnyLine::SprintBelly : EJohnnyLine::FoodCraving);
	}
}

void UJohnnyVoiceComponent::MaybeSayWhileStarving(float DeltaSeconds)
{
	StarvingTalkAccumulator += DeltaSeconds;
	if (StarvingTalkAccumulator >= 6.f)
	{
		StarvingTalkAccumulator = 0.f;
		Say(EJohnnyLine::Starving);
	}
}
