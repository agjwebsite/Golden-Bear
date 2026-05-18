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

	// ----- Locomotion / Hunger background chatter -----

	AddIfMissing(EJohnnyLine::SprintBelly, {
		TEXT("Ugh... this belly... bouncing like a sack of flour."),
		TEXT("Why did I eat that second burek..."),
		TEXT("Stomach, please. Just... stop jiggling."),
		TEXT("I hate you. I hate every kilo of you."),
		TEXT("Run, you useless slab of lard."),
		TEXT("My knees are filing for divorce."),
		TEXT("Each step is a small personal disaster."),
		TEXT("I should not be moving this fast. Nobody should."),
		TEXT("This is what dying feels like. I'm sure of it."),
		TEXT("Belly, listen — we had a DEAL. You behave, I feed you. This is not behaving."),
		TEXT("I'm an entire weather system. I have my own gravity."),
		TEXT("Bouncing... bouncing... oh god the bouncing..."),
		TEXT("This is cardio? People CHOOSE this?"),
		TEXT("My grandmother ran from the Ottomans. I run from a Tuesday meeting."),
		TEXT("One day I'll be thin. Today is not that day. Tomorrow either."),
		TEXT("Sweat. In places sweat should not exist."),
		TEXT("I can taste my own heartbeat. That's not normal."),
		TEXT("If I die, bury me sitting down. I'm tired."),
	}, 5.f);

	AddIfMissing(EJohnnyLine::FoodCraving, {
		TEXT("A nice kebapi right now... ten of them. Twelve."),
		TEXT("Gyro. Hot gyro. Drowning in tzatziki."),
		TEXT("One burger. Just one. A small one. Or eight."),
		TEXT("Baklava. Honey-soaked. I can almost smell it."),
		TEXT("I would sell my left lung for a tavče gravče."),
		TEXT("A burek. Spinach burek. A burek the size of my head."),
		TEXT("Ajvar. Just a jar. I'll eat it with a spoon. I don't need bread."),
		TEXT("Bean stew. With three kinds of sausage. Three."),
		TEXT("Slow-roasted lamb. The whole lamb. Skip the sides."),
		TEXT("Hot bread. Fresh out of the oven. Butter melting into the cracks."),
		TEXT("Šopska salata, with SO much sirenje you can't see the tomato."),
		TEXT("A cheese pita. Cheese so angry it bubbles."),
		TEXT("A rakija. To... settle the stomach. That's why. Medicinal."),
		TEXT("A whole chocolate. Family size. Not for the family."),
		TEXT("Goulash. The kind that takes three days to make. I have three days."),
		TEXT("Honey. Just a jar. I'll find a use for it."),
		TEXT("I'd commit a crime for a pljeskavica right now. A small crime."),
		TEXT("I miss food. I am literally walking around and I miss food."),
	}, 7.f);

	AddIfMissing(EJohnnyLine::Exhausted, {
		TEXT("Lungs... on fire... legs... gone..."),
		TEXT("That's it. I'm dying. Tell my mother I love her."),
		TEXT("Five seconds. Just give me five seconds."),
		TEXT("I can't feel my face. Is that bad?"),
		TEXT("My heart is... very, very angry with me."),
		TEXT("Oxygen is a SCAM."),
		TEXT("Body says no. Body says NO."),
		TEXT("Wheezing my last will and testament..."),
		TEXT("I will PAY anyone in this city to carry me."),
		TEXT("This was a mistake. The running. The whole running."),
		TEXT("Are these spots normal? Tell me these spots are normal."),
		TEXT("I just heard my own pulse. It sounds furious."),
	}, 4.f);

	AddIfMissing(EJohnnyLine::Starving, {
		TEXT("My stomach is eating itself. Cannibal."),
		TEXT("Need... food... anything..."),
		TEXT("Is that a sandwich on the ground? ...no. Just a leaf."),
		TEXT("I would chew my own arm. I might."),
		TEXT("Empty. EMPTY. The void inside me has a void."),
		TEXT("Acid. Just acid. I'm becoming acid."),
		TEXT("I can hear my organs negotiating who gets eaten first."),
		TEXT("Sticks look edible. STICKS. LOOK. EDIBLE."),
		TEXT("A pigeon. Could I catch a pigeon. Could I eat a raw pigeon. ...maybe."),
		TEXT("Even the rage is going. I can't even rage anymore."),
		TEXT("I keep trying to chew. There is nothing in my mouth. Nothing."),
		TEXT("I'd lick a kitchen wall right now and call it dinner."),
	}, 6.f);

	AddIfMissing(EJohnnyLine::Eating, {
		TEXT("Mmm. That's the stuff."),
		TEXT("Don't judge me. You'd do the same."),
		TEXT("Worth it. Every time."),
		TEXT("THANK you."),
		TEXT("Sweet, holy calories."),
		TEXT("This is the best moment of my week."),
		TEXT("I'm a new man. For four whole minutes."),
		TEXT("I would marry this. I would have its children."),
		TEXT("I'm crying. Don't look at me."),
		TEXT("Food. Food. Food. Foooooood."),
		TEXT("Forgive me, doctor. Forgive me, scale."),
		TEXT("This is why I get up in the morning. This. Right here."),
	}, 2.f);

	// Legacy — never triggered now, kept for save-game compatibility.
	AddIfMissing(EJohnnyLine::Dodge, {
		TEXT("Hup!"), TEXT("Move!"), TEXT("Ow my knee.")
	}, 0.5f);

	// ----- Combat -----

	AddIfMissing(EJohnnyLine::Parry, {
		TEXT("Hah!"),
		TEXT("Try me."),
		TEXT("Caught it."),
		TEXT("Nice try."),
		TEXT("WRONG day, friend."),
		TEXT("Sit DOWN."),
		TEXT("You see that? My reflexes work."),
		TEXT("Knew it."),
		TEXT("Read you like a recipe."),
		TEXT("Telegraphed harder than my mother-in-law's opinions."),
		TEXT("Tch. Amateur."),
	}, 0.4f);

	AddIfMissing(EJohnnyLine::ParryWhiff, {
		TEXT("Where did he—"),
		TEXT("Oh no."),
		TEXT("That's not... that's not where his hand is."),
		TEXT("I COMMITTED!"),
		TEXT("Damn it. Damn it."),
		TEXT("Pretend that didn't happen."),
		TEXT("I meant to do that. I meant to do that."),
		TEXT("Stupid stupid stupid—"),
	}, 1.2f);

	AddIfMissing(EJohnnyLine::Attack, {
		TEXT("Take this!"),
		TEXT("Eat fist!"),
		TEXT("This is for my belly!"),
		TEXT("Down, dog!"),
		TEXT("Hah!"),
		TEXT("For the kebapi!"),
		TEXT("Get OFF me!"),
		TEXT("Move!"),
		TEXT("I'm not running anymore!"),
		TEXT("Tax season, friend!"),
		TEXT("Christmas is CANCELED!"),
		TEXT("Don't make me— oh you DID make me."),
		TEXT("Ow. So many bones in my hand."),
		TEXT("This is harder than it looks!"),
		TEXT("Just go DOWN already!"),
		TEXT("I have a meeting! I have a MEETING!"),
		TEXT("My mother punches harder than this and she's NINETY!"),
	}, 0.4f);

	AddIfMissing(EJohnnyLine::Heavy, {
		TEXT("RAARGH!"),
		TEXT("Belly says HELLO!"),
		TEXT("Come HERE."),
		TEXT("HOO-RAA!"),
		TEXT("TAKE THIS WEIGHT!"),
		TEXT("Five hundred years of stew, IN ONE PUNCH!"),
		TEXT("INCOMING!"),
		TEXT("KEBAPI POWER!"),
		TEXT("Buckle up."),
		TEXT("This is going to hurt me too."),
		TEXT("Body — please don't break. BODY!"),
		TEXT("OPA!"),
	}, 0.8f);

	AddIfMissing(EJohnnyLine::ThrowFood, {
		TEXT("Take my burek! TAKE IT!"),
		TEXT("Catch."),
		TEXT("This hurts me more than you."),
		TEXT("I was SAVING that!"),
		TEXT("No no no NOT the baklava NOT THE BAKLAVA—"),
		TEXT("Fine. FINE. Have my dinner."),
		TEXT("I will mourn this."),
		TEXT("Eat well, monster."),
		TEXT("I hope you choke. I HOPE YOU CHOKE."),
		TEXT("This is for the lunch I could've had."),
		TEXT("Calorically speaking — you owe me."),
	}, 0.6f);

	AddIfMissing(EJohnnyLine::HotbarEmpty, {
		TEXT("...nothing? You ate the LAST one?!"),
		TEXT("I'm OUT. The horror."),
		TEXT("Wait, where's my— oh. I ate it. Of course I ate it."),
		TEXT("Empty pockets. Empty soul."),
		TEXT("Even my snacks have abandoned me."),
		TEXT("Past-me ATE my future-me's ammunition!"),
	}, 1.0f);

	AddIfMissing(EJohnnyLine::Finisher, {
		TEXT("Stay DOWN."),
		TEXT("That's enough out of you."),
		TEXT("Should've stayed home."),
		TEXT("Pavement, meet face. Face, meet pavement."),
		TEXT("Don't get up. DON'T."),
		TEXT("Goodnight, sweet prince."),
		TEXT("I'm too tired for mercy."),
		TEXT("Send the bill to your boss."),
		TEXT("That one was for the cardio."),
		TEXT("I am NEVER doing this again. Until the next one."),
	}, 0.8f);

	AddIfMissing(EJohnnyLine::Hurt, {
		TEXT("Aagh!"),
		TEXT("Not the face!"),
		TEXT("Why is this happening to me..."),
		TEXT("OW!"),
		TEXT("Right in the kidney!"),
		TEXT("I JUST got that kidney fixed!"),
		TEXT("That's going to bruise."),
		TEXT("Add it to the list."),
		TEXT("My mother would CRY."),
		TEXT("The pain. The PAIN."),
		TEXT("THIS is why I stay home!"),
		TEXT("I am a BUSINESSMAN, not a— OW!"),
		TEXT("(sob) ...why..."),
		TEXT("My liver! Not my liver! It's the only one I like!"),
	}, 0.6f);

	AddIfMissing(EJohnnyLine::LowHealth, {
		TEXT("I am... not okay. I am NOT okay."),
		TEXT("This much blood is normal? Right? Right?"),
		TEXT("One more hit and I'm a wall ornament."),
		TEXT("I should sit down. I should sit down forever."),
		TEXT("Walking it off. Walking it... oh god."),
		TEXT("A doctor would scream if they saw me."),
		TEXT("I'm held together by spite and grease."),
		TEXT("This is fine. This is FINE. THIS IS NOT FINE."),
		TEXT("Mama warned me about days like this. About all the days like this."),
	}, 8.f);

	AddIfMissing(EJohnnyLine::Death, {
		TEXT("Tell them... I tried..."),
		TEXT("I should've... had... one more burek..."),
		TEXT("Mama... I'm coming home..."),
		TEXT("This is... so embarrassing..."),
		TEXT("Of course. Of course this is how I go."),
		TEXT("Bury me... sitting down... please..."),
	}, 999.f);

	// ----- Story / NPC reactions -----

	AddIfMissing(EJohnnyLine::GangsterTaunt, {
		TEXT("I'll get the money! I SWEAR!"),
		TEXT("Just one more week!"),
		TEXT("Take my car! Wait. The car's already dead."),
		TEXT("I'll pay! I'll PAY! Stop running!"),
		TEXT("It's not personal — it's the slot machines! The MACHINES!"),
		TEXT("I have a wife! I have a wife AND a job interview! Have mercy!"),
		TEXT("Tomorrow! TOMORROW I pay! Why won't you take tomorrow?!"),
		TEXT("I am very, VERY good for it!"),
	}, 3.f);

	AddIfMissing(EJohnnyLine::BossFires, {
		TEXT("You're firing me? Over a meeting?! I RAN here!"),
		TEXT("I'm— I'm fired? I haven't even SAT DOWN yet."),
		TEXT("Sir. SIR. Look at me. I am sweating my SOUL out. And you're firing me."),
	}, 999.f);

	AddIfMissing(EJohnnyLine::Jackpot, {
		TEXT("A MILLION?! A MILLION DOLLARS?!"),
		TEXT("THE MACHINE LOVES ME! THE MACHINE FINALLY LOVES ME!"),
		TEXT("I take it BACK! I take back every BAD thing I said about gambling!"),
		TEXT("MAMA! MAMA, I WON! MAMAAAA!"),
	}, 999.f);

	AddIfMissing(EJohnnyLine::BrokeAgain, {
		TEXT("Where did it all go... oh. The kebapi. Right."),
		TEXT("A million dollars. Spent. In... three days. Three glorious days."),
		TEXT("Maybe... just one more spin..."),
		TEXT("I regret... nothing... well... most things..."),
		TEXT("Mama. Don't tell mama."),
	}, 999.f);

	// ----- Car opening sequence -----

	AddIfMissing(EJohnnyLine::CarSputter, {
		TEXT("Come on. Come on come on COME ON."),
		TEXT("Don't you dare. Don't you DARE."),
		TEXT("Not now. Anything but now."),
		TEXT("I will buy you new spark plugs, I swear on my mother."),
		TEXT("We have a DEAL, you and I."),
		TEXT("Look at me. We've been through worse. The flood. The mountain. That guy from Bitola."),
		TEXT("Easy. Easy. EASY!"),
		TEXT("This is a betrayal. A car betrayal. The worst kind."),
	}, 1.0f);

	AddIfMissing(EJohnnyLine::CarDying, {
		TEXT("Oh no. Oh no no no. That sound. THAT sound."),
		TEXT("You old piece of garbage..."),
		TEXT("That's the rattle. That's the death rattle."),
		TEXT("I know that noise. I have NIGHTMARES about that noise."),
	}, 999.f);

	AddIfMissing(EJohnnyLine::CarDeadResignation, {
		TEXT("Of course. Of course this happens to me."),
		TEXT("Well. I guess I'm walking."),
		TEXT("Veles. On foot. In these shoes. With these legs."),
		TEXT("Twenty kilometers. On feet. Like a peasant. Like my grandfather."),
		TEXT("I'm going to be FIRED. I am going to be FIRED."),
		TEXT("Goodbye, old friend. You did your best. Your best was bad."),
	}, 999.f);
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

void UJohnnyVoiceComponent::MaybeSayWhileLowHealth(float DeltaSeconds)
{
	LowHealthTalkAccumulator += DeltaSeconds;
	if (LowHealthTalkAccumulator >= 8.f)
	{
		LowHealthTalkAccumulator = 0.f;
		Say(EJohnnyLine::LowHealth);
	}
}
